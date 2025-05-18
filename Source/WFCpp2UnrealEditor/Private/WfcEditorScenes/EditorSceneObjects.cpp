#include "WfcEditorScenes/EditorSceneObjects.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Algo/Count.h"

#include "WfcTileset.h"
#include "WfcBpUtils.h"
#include "WFCpp2UnrealEditor.h"
#include "WfcEditorScenes/WfcTileVisualizer.h"
#include "WfcEditorScenes/WfcTilesetEditorScene.h"
#include "WfcEditorScenes/WfcTilesetEditorViewportClient.h"


namespace
{
	UMaterialInterface* LoadFaceEditorMaterial(WFC_Directions3D face)
	{
		static const TCHAR* const MaterialPaths[] = {
			//Order must match ordering of WFC_Directions3D
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinX.MI_FacePlane_MinX"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxX.MI_FacePlane_MaxX"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinY.MI_FacePlane_MinY"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxY.MI_FacePlane_MaxY"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinZ.MI_FacePlane_MinZ"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxZ.MI_FacePlane_MaxZ")
		};
		return LoadObject<UMaterialInterface>(
			nullptr,
			MaterialPaths[static_cast<int>(face)],
			nullptr, LOAD_EditorOnly
		);
	}
}

FEditorSceneObject_WfcFace::FEditorSceneObject_WfcFace(FPreviewScene* owner,
                                                       const FTransform& _tileTr, double _cubeExtents,
                                                       WFC_Directions3D _face, const FWfcFacePrototype& _facePoints,
                                                       WFC_Transforms2D _facePermutation)
    : FEditorSceneObject(owner),
	  cubeExtents(_cubeExtents),
      faceSide(_face), facePrototype(_facePoints), facePermutation(_facePermutation)
{
	centerSphere.Emplace(Owner, FTransform{ }, FColor{ 1, 1, 1 });
	facePlane.Emplace(Owner, FTransform{ }, LoadFaceEditorMaterial(faceSide));
	
	//Generate components for the face corners.
	auto doCorner = [&](WFC::Tiled3D::FacePoints pointLocationTile)
	{
		auto pointLocationPrototype = WFC::Tiled3D::TransformFaceCorner(
			pointLocationTile,
			static_cast<WFC::Tiled3D::Directions3D>(faceSide),
			WFC::Invert(static_cast<WFC::Transformations>(facePermutation))
		);
		
		cornerIDs[pointLocationTile] = facePrototype.Corners.PointAt(pointLocationPrototype);
		
		const auto* pointLabel = facePrototype.Corners.GetName(pointLocationPrototype);
		if (pointLabel != nullptr)
		{
			cornerArrows[pointLocationTile].Emplace(Owner, FTransform{ }, FColor{ });
			cornerLabels[pointLocationTile].Emplace(Owner, FTransform{ }, *pointLabel, FColor{ });

			cornerArrows[pointLocationTile]->GetComponent()->bSelectable = false;
			cornerLabels[pointLocationTile]->GetComponent()->bSelectable = false;
		}
	};
	doCorner(WFC::Tiled3D::FacePoints::AA);
	doCorner(WFC::Tiled3D::FacePoints::AB);
	doCorner(WFC::Tiled3D::FacePoints::BA);
	doCorner(WFC::Tiled3D::FacePoints::BB);

	//Generate components for the face edges.
	auto doEdge = [&](WFC::Tiled3D::FacePoints pointLocationTile)
	{
		auto pointLocationPrototype = WFC::Tiled3D::TransformFaceEdge(
			pointLocationTile,
			static_cast<WFC::Tiled3D::Directions3D>(faceSide),
			WFC::Invert(static_cast<WFC::Transformations>(facePermutation))
		);

		edgeIDs[pointLocationTile] = facePrototype.Edges.PointAt(pointLocationPrototype);
		
		const auto* pointLabel = facePrototype.Edges.GetName(pointLocationPrototype);
		if (pointLabel != nullptr)
		{
			edgeArrows[pointLocationTile].Emplace(Owner, FTransform{ }, FColor{ });
			edgeLabels[pointLocationTile].Emplace(Owner, FTransform{ }, *pointLabel, FColor{ });

			edgeArrows[pointLocationTile]->GetComponent()->bSelectable = false;
			edgeLabels[pointLocationTile]->GetComponent()->bSelectable = false;
		}
	};
	doEdge(WFC::Tiled3D::FacePoints::AA);
	doEdge(WFC::Tiled3D::FacePoints::AB);
	doEdge(WFC::Tiled3D::FacePoints::BA);
	doEdge(WFC::Tiled3D::FacePoints::BB);

	//Initialize visual state.
	SetTileTransform(_tileTr);
	SetAlphaScale(1.0f);
}
void FEditorSceneObject_WfcFace::RebuildTransform()
{
	FTransform faceTr;
	std::array<FTransform, 4> cornerTrs, edgeTrs;

	auto unwrappedFace = static_cast<WFC::Tiled3D::Directions3D>(faceSide);
	uint_fast8_t faceIdx, faceAxis1, faceAxis2;
	WFC::Tiled3D::GetAxes(unwrappedFace, faceIdx, faceAxis1, faceAxis2);

	FVector faceOffset{ 0, 0, 0 };
	faceOffset[faceIdx] = WFC::Tiled3D::IsMin(unwrappedFace) ? -cubeExtents : cubeExtents;
	faceOffset = tileTr.TransformVector(faceOffset);
	faceTr = {
		UKismetMathLibrary::ComposeRotators(
			UKismetMathLibrary::ComposeRotators(
				FRotator{ 0, 180, 0 },
				UWfcUtils::WfcToFRotator(faceSide)
			),
			tileTr.Rotator()
		),
		tileTr.GetLocation() + faceOffset,
		tileTr.GetScale3D()
	};
	if (centerSphere.IsSet())
		centerSphere->GetComponent()->SetWorldTransform(FTransform{ faceTr.Rotator(), faceTr.GetLocation(), FVector{ 10 }});
	if (facePlane.IsSet())
		facePlane->GetComponent()->SetWorldTransform(FEditorPlaneComponent::GetTransform(
			faceTr.GetLocation(),
			FVector2D{ cubeExtents },
			faceTr.Rotator().Vector()
		));

	//Calculate Corner transforms.
	for (int faceAxis1Dir = 0; faceAxis1Dir < 2; ++faceAxis1Dir)
		for (int faceAxis2Dir = 0; faceAxis2Dir < 2; ++faceAxis2Dir)
		{
			auto facePointTile = WFC::Tiled3D::MakeCornerFacePoint(faceAxis1Dir == 0, faceAxis2Dir == 0);
			
			FVector cornerOffset{ 0, 0, 0 };
			cornerOffset[faceAxis1] = (faceAxis1Dir == 0) ? -cubeExtents : cubeExtents;
			cornerOffset[faceAxis2] = (faceAxis2Dir == 0) ? -cubeExtents : cubeExtents;
			cornerOffset = tileTr.TransformVector(cornerOffset);

			cornerTrs[facePointTile] = {
				faceTr.Rotator(),
				faceTr.GetLocation() + cornerOffset,
				faceTr.GetScale3D()
			};
			if (cornerLabels[facePointTile].IsSet())
			{
				//Give each face a different vertical offset so the labels don't intersect each other.
				float verticalOffset = std::to_array({-40, 0, 40})[faceIdx];
				
				auto tr = cornerTrs[facePointTile];
				tr.SetLocation(tr.GetLocation() +
								 (tr.GetRotation().GetForwardVector() * (cubeExtents * 0.1)) +
								 (tr.GetRotation().GetUpVector() * (verticalOffset)));
				cornerLabels[facePointTile]->GetComponent()->SetWorldTransform(tr);
			}
			if (cornerArrows[facePointTile].IsSet())
			{
				auto* arrow = cornerArrows[facePointTile]->GetComponent();
				
				auto arrowTr = FEditorArrowComponent::GetTransform(
					faceTr.GetLocation(),
					cornerTrs[facePointTile].GetLocation(),
					10.0
				);
				arrow->SetWorldTransform(FTransform{ arrowTr.GetRotation(), arrowTr.GetLocation() });
				arrow->SetArrowSize(arrowTr.GetScale3D().Y);
				arrow->SetArrowLength(arrowTr.GetScale3D().X);
			}
		}
	
	//Calculate edge transforms.
	for (int edgeParallelAxis = 0; edgeParallelAxis < 2; ++edgeParallelAxis)
		for (int edgeSide = 0; edgeSide < 2; ++edgeSide)
		{
			auto facePoint = WFC::Tiled3D::MakeEdgeFacePoint(edgeParallelAxis == 0, edgeSide == 0);
			
			int faceEdgeParallelAxis = std::to_array({ faceAxis1, faceAxis2 })[edgeParallelAxis],
				faceEdgePerpendicularAxis = std::to_array({ faceAxis1, faceAxis2 })[(edgeParallelAxis + 1) % 2];
		
			FVector edgeOffset{ 0, 0, 0 };
			edgeOffset[faceEdgePerpendicularAxis] = (edgeSide == 0) ? -cubeExtents : cubeExtents;
			edgeOffset = tileTr.TransformVector(edgeOffset);

			edgeTrs[facePoint] = {
				faceTr.Rotator(),
				faceTr.GetLocation() + edgeOffset,
				faceTr.GetScale3D()
			};
			if (edgeLabels[facePoint].IsSet())
			{
				auto tr = cornerTrs[facePoint];
				tr.SetLocation(tr.GetLocation() + tr.GetRotation().GetForwardVector() *
							   (cubeExtents * 0.1));
				edgeLabels[facePoint]->GetComponent()->SetWorldTransform(tr);
			}
			if (edgeArrows[facePoint].IsSet())
			{
				auto* arrow = edgeArrows[facePoint]->GetComponent();
				
				auto arrowTr = FEditorArrowComponent::GetTransform(
					faceTr.GetLocation(),
					edgeTrs[facePoint].GetLocation(),
					10.0
				);
				arrow->SetWorldTransform(FTransform{ arrowTr.GetRotation(), arrowTr.GetLocation() });
				arrow->SetArrowSize(arrowTr.GetScale3D().Y);
				arrow->SetArrowLength(arrowTr.GetScale3D().X);
			}
		}
}
void FEditorSceneObject_WfcFace::RebuildColors()
{
	auto rawFace = static_cast<WFC::Tiled3D::Directions3D>(faceSide);
	int faceIdx = WFC::Tiled3D::GetAxisIndex(rawFace);
	
	auto faceColor = std::to_array({
		FLinearColor{ 1, 0, 0 },
		FLinearColor{ 0, 1, 0 },
		FLinearColor{ 0, 0, 1 }
	})[faceIdx];
	auto pointIDTints = std::to_array({
		FLinearColor{ 0, 0, 0, 1 },
		FLinearColor{ 1, 0.5, 0.5, 1 },
		FLinearColor{ 0.5, 1, 0.5, 1 },
		FLinearColor{ 0.5, 0.5, 1, 1 }
	});
	FLinearColor cornerTint{ 0.85, 0.85, 0.85, 1 },
				 edgeTint{ 0.5, 0.5, 0.5, 1 };

	auto outputColor = [&](const FLinearColor& col) {
		return (col * FLinearColor{ 1, 1, 1, alphaScale }).ToFColorSRGB();
	};
	
	if (centerSphere.IsSet())
		centerSphere->GetComponent()->ShapeColor = outputColor(faceColor);
	//Alpha of the face plane is handled by its Material, based on viewing angle.
	for (auto& label : cornerLabels)
		if (label.IsSet())
			label->GetComponent()->SetTextRenderColor(outputColor({ 0, 0, 0, 0.35f}));
	for (auto& label : edgeLabels)
		if (label.IsSet())
			label->GetComponent()->SetTextRenderColor(outputColor({ 1, 1, 1, 0.5f}));
	for (int arrowI = 0; arrowI < 4; ++arrowI)
	{
		if (cornerArrows[arrowI].IsSet())
			cornerArrows[arrowI]->GetComponent()->SetArrowColor(outputColor(
				faceColor * cornerTint * pointIDTints[static_cast<int>(cornerIDs[arrowI])]
			));
		if (edgeArrows[arrowI].IsSet())
			edgeArrows[arrowI]->GetComponent()->SetArrowColor((outputColor(
				faceColor * edgeTint * pointIDTints[static_cast<int>(edgeIDs[arrowI])]
			)));
	}
}

FEditorSceneObject_WfcTile::FEditorSceneObject_WfcTile(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
													   const FTransform& tr,
													   const FLinearColor& boundsColor,
													   const UWfcTileset* tileset, int32 tileID,
													   bool includeVisualizer)
    : FEditorSceneObject(&owner),
      tileBounds(Owner,
      			 FBox{ tr.GetLocation() - ((tileset->TileLength / 2.0) * tr.GetScale3D()),
      			 	   tr.GetLocation() + ((tileset->TileLength / 2.0) * tr.GetScale3D()) },
      			 tr.GetRotation().Rotator(),
      			 boundsColor.ToFColorSRGB())
{
	check(tileset);
	check(tileset->Tiles.Contains(tileID));
	const auto& tile = tileset->Tiles[tileID];
	
	for (int i = 0; i < WFC::Tiled3D::N_DIRECTIONS_3D; ++i)
	{
		auto dir = static_cast<WFC::Tiled3D::Directions3D>(i);
		const auto& faceData = tile.GetFace(dir);
		const auto* facePrototype = tileset->FacePrototypes.Find(faceData.PrototypeID);
		if (facePrototype == nullptr)
		{
			//TODO: Display 3D indication of "bad face-prototype index"
			continue;
		}
		
		faces.Emplace(Owner, tr, tileset->TileLength / 2.0,
			          static_cast<WFC_Directions3D>(dir),
			          *facePrototype, faceData.PrototypeOrientation);
	}

	if (includeVisualizer)
		tileDataVisualizer = WfcTileVisualizer::MakeVisualizer({
			owner, viewportClient,
			{ tileset }, tileID, tileset->Tiles[tileID], tr
		});
}
void FEditorSceneObject_WfcTile::SetTransform(const FTransform& newTr)
{
	currentTr = newTr;
	
	for (auto& face : faces)
		face.SetTileTransform(newTr);
	tileBounds.GetComponent()->SetWorldTransform(currentTr);
}

FEditorSceneObject_WfcTileWithPermutations::FEditorSceneObject_WfcTileWithPermutations(
			FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
			const FTransform& rootTr, double spacingBetweenTiles,
			const FLinearColor& boundsColor, const FLinearColor& labelColor,
			const UWfcTileset* tileset, int32 tileID,
			bool visualizeTileData
		)
	: FEditorSceneObject(&owner)
{
	if (!IsValid(tileset) || !tileset->Tiles.Contains(tileID))
		return;
	const auto& tileData = tileset->Tiles[tileID];

	//Each permutation is positioned on a grid centered at the origin
	//    and scaled by the tile size.
	double gridSpacing = tileset->TileLength + spacingBetweenTiles;
	//Arrange the grid using a horizontal spiral outward from the origin.
	UE_LOG(LogWFCppEditor, Log,
		   TEXT("Positioning %i permutations of tile %i"),
		   tileData.GetSupportedTransforms().Size(), tileID);
	int spiralLayer = 0,
	    spiralEdgeAxis = 0,
		spiralEdgeDir = -1,
		spiralAlongEdge = 0;
	for (const auto& wfcTr : tileData.GetSupportedTransforms())
	{
		//Compute which grid element we're at.
		WFC::Vector3i gridIdx;
		gridIdx[spiralEdgeAxis] = spiralEdgeDir * spiralLayer;
		gridIdx[(spiralEdgeAxis + 1) % 2] = spiralAlongEdge;
		gridIdx.z = 0;
		UE_LOG(LogWFCppEditor, Log,
			   TEXT("\tPermutation %s : {%i, %i, %i} / %i|%i:%i|%i"),
		       *FString::Printf(
					TEXT("%s%s"),
					(wfcTr.Invert ? TEXT("Invert->\n") : TEXT("")),
					*UEnum::GetValueAsString(static_cast<WFC_Rotations3D>(wfcTr.Rot))
			   ),
			   gridIdx.x, gridIdx.y, gridIdx.z,
			   spiralLayer, spiralEdgeAxis, spiralEdgeDir, spiralAlongEdge);
		
		int cornerOffset = spiralEdgeAxis; //0 for first axis, 1 for second; helps avoid duplicate corner usage when going along second axis
		//Advance the grid spiral.
		spiralAlongEdge += 1;
		if (spiralAlongEdge > spiralLayer - cornerOffset)
		{
			//Innermost iteration is through the side (min vs max)
			if (spiralLayer > 0 && spiralEdgeDir == -1)
				spiralEdgeDir = 1;
			//Secondmost inner iteration is through the axis (0=X vs 1=Y).
			else if (spiralLayer > 0 && spiralEdgeAxis < 1)
			{
				spiralEdgeAxis += 1;
				spiralEdgeDir = -1;
			}
			//Outermost iteration is through the layer (where 0 is the grid origin).
			else
			{
				spiralLayer += 1;
				spiralEdgeDir = -1;
				spiralEdgeAxis = 0;
			}
			cornerOffset = spiralEdgeAxis; //Refresh this value
			
			spiralAlongEdge = -spiralLayer + cornerOffset;
		}

		//Compose the different transforms together.
		FTransform permutationTr{ FVector(gridIdx.x, gridIdx.y, gridIdx.z) * gridSpacing };
		FTransform worldTr = UKismetMathLibrary::ComposeTransforms(
			UKismetMathLibrary::ComposeTransforms(
				UWfcUtils::WfcToFTransform({ wfcTr }),
				permutationTr
			),
			rootTr
		);

		//Set up label data.
		FTransform labelTr{
			rootTr.GetRotation(),
			worldTr.GetLocation() +
				(rootTr.GetRotation().GetUpVector() * ((tileset->TileLength / 2) + 50)),
			rootTr.GetScale3D()
		};
		FString labelStr = FString::Printf(
			TEXT("%sRot%s"),
			(wfcTr.Invert ? TEXT("Invert->\n") : TEXT("")),
			*UEnum::GetValueAsString(static_cast<WFC_Rotations3D>(wfcTr.Rot)).RightChop(17) //Chop out 'WFC_Rotations3D::'
		);

		//Create the visualizer.
		permutations.Emplace(
			FEditorSceneObject_WfcTile{
				owner, viewportClient, worldTr,
				boundsColor, tileset, tileID, visualizeTileData
			},
			FEditorTextComponent{
				Owner, labelTr, labelStr, labelColor.ToFColorSRGB(),
				EHTA_Center, EVRTA_TextBottom
			},
			wfcTr
		);
	}

	//Set up an "overall label" describing the permutation set.
	overallLabel.Emplace(
		Owner,
		FTransform{
			rootTr.GetRotation(),
			rootTr.GetLocation() +
				(-rootTr.GetRotation().GetUpVector() *
				    ((tileset->TileLength / 2.0) + 100.0f)),
			rootTr.GetScale3D()
		},
		FString::Printf(
			TEXT("%i permutations\n%i inverted permutations"),
			permutations.Num(),
			static_cast<int>(Algo::CountIf(permutations, [&](const auto& perm) { return perm.WfcTransform.Invert; }))
		),
		labelColor.ToFColorSRGB(),
		EHTA_Center, EVRTA_TextTop
	);
}

FEditorSceneObject_WfcTileWithMatches::FEditorSceneObject_WfcTileWithMatches(
			FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
			const FTransform& rootTr, double spacingBetweenTiles,
			const FLinearColor& boundsColor, const FLinearColor& labelColor,
			const UWfcTileset* tileset, int32 tileID, const FWFC_Transform3D& permutation,
			const TSet<WFC_Directions3D>& facesToMatchAfterPermutation,
			bool visualizeTileData
		)
	: FEditorSceneObject(&owner)
{
	if (!IsValid(tileset) || !tileset->Tiles.Contains(tileID))
		return;
	
	libraryTilesetData.Emplace();
	tileset->Unwrap(*libraryTilesetData);

	const auto& tileData = tileset->Tiles[tileID];
	sourceTile.Emplace(owner, viewportClient,
				       UKismetMathLibrary::ComposeTransforms(
				       		rootTr,
				       		permutation.ToFTransform()
				       ),
				       boundsColor,
					   tileset, tileID, visualizeTileData);

	for (auto _srcFace : facesToMatchAfterPermutation)
	{
		auto srcFace = static_cast<WFC::Tiled3D::Directions3D>(_srcFace),
			 destFace = WFC::Tiled3D::GetOpposite(srcFace);

		const auto& libraryTile = libraryTilesetData->Tiles[libraryTilesetData->WfcTileIDByUnrealID[tileID]];
		auto facePoints = WFC::Tiled3D::GetFace(libraryTile.Data, permutation.Unwrap(), srcFace).Points;

		int matchI1 = 1;
		for (const auto& [matchTileID, matchTileData] : tileset->Tiles)
		{
			for (const auto& matchTilePermutation : matchTileData.GetSupportedTransforms())
			{
				auto matchLibraryTileID = libraryTilesetData->WfcTileIDByUnrealID[matchTileID];
				const auto& matchLibraryTile = libraryTilesetData->Tiles[matchLibraryTileID];
				auto matchFacePoints = WFC::Tiled3D::GetFace(
					matchLibraryTile.Data,
					matchTilePermutation,
					destFace
				).Points;

				if (facePoints == matchFacePoints)
				{
					//Position this tile along the face it matches with.
					WFC::Vector3i offsetMultiple = WFC::Tiled3D::GetFaceDirection(srcFace) * matchI1;
					FVector offsetMultipleF(offsetMultiple.x, offsetMultiple.y, offsetMultiple.z);
					auto pos = (tileset->TileLength + spacingBetweenTiles) * offsetMultipleF;
					
					FTransform matchedTileTr = UKismetMathLibrary::ComposeTransforms(
						FWFC_Transform3D{ matchTilePermutation }.ToFTransform(),
						FTransform{ pos }
					);
					
					//Flip the label to face the origin horizontally.
					//By default it'll face +X.
					float labelYaw;
					switch (srcFace)
					{
						case WFC::Tiled3D::MinX:
						case WFC::Tiled3D::MinZ:
						case WFC::Tiled3D::MaxZ:
							labelYaw = 0;
						break;

						case WFC::Tiled3D::MaxX:
							labelYaw = 180;
						break;
						case WFC::Tiled3D::MinY:
							labelYaw = 90;
						break;
						case WFC::Tiled3D::MaxY:
							labelYaw = 270;
						break;
						
						default: check(false); return;
					}

					auto labelTr = UKismetMathLibrary::ComposeTransforms(
						FTransform{
							FRotator{ 0, labelYaw, 0 },
							pos
						},
						UKismetMathLibrary::ComposeTransforms(
							FTransform{
								FVector{ 0, 0, (tileset->TileLength / 2.0) + 50.0 } //Above the tile's center
							},
							rootTr
						)
					);

					matches.Emplace(
						matchTileID, matchTilePermutation,
						static_cast<WFC_Directions3D>(srcFace),
						FEditorSceneObject_WfcTile{
							owner, viewportClient,
							UKismetMathLibrary::ComposeTransforms(
								matchedTileTr,
								rootTr
							),
							boundsColor, tileset, matchTileID
						},
						FEditorTextComponent{
							&owner,
							labelTr,
							FString::Printf(
								TEXT("%i/%s\n%s"),
								matchTileID,
								*FWFC_Transform3D{ matchTilePermutation }.ToString(),
								IsValid(matchTileData.Data) ?
								    *matchTileData.Data->GetEditorDescription() :
								    TEXT("[null]")
							),
							labelColor.ToFColorSRGB(),
							EHTA_Center, EVRTA_TextBottom
						}
					);
				
					matchI1 += 1;
				}
			}
		}
		
		//Add an informative label on top of the source tile's face.
		WFC::Vector3i faceDir = WFC::Tiled3D::GetFaceDirection(srcFace);
		auto faceLabelPos = FVector(faceDir.x, faceDir.y, faceDir.z) * ((tileset->TileLength / 2.0) - 20);
		faceLabelPos.Z += 20 * (srcFace == WFC::Tiled3D::Directions3D::MinZ ? -1 : 1);
		faceLabels.Emplace(
			&owner,
			UKismetMathLibrary::ComposeTransforms(
				FTransform{ faceLabelPos },
				rootTr
			),
			FString::Printf(TEXT("%i matches"), matchI1-1),
			labelColor.ToFColorSRGB(),
			EHTA_Center,
			(srcFace == WFC::Tiled3D::Directions3D::MinZ ? EVRTA_TextTop : EVRTA_TextBottom)
		);
	}
}