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
	UMaterialInterface* LoadFaceEditorMaterial(TOptional<WFC_Directions3D> face)
	{
		static const TCHAR* const MaterialPathsByColor[] = {
			//Order must match ordering of WFC_Directions3D
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinX.MI_FacePlane_MinX"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxX.MI_FacePlane_MaxX"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinY.MI_FacePlane_MinY"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxY.MI_FacePlane_MaxY"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MinZ.MI_FacePlane_MinZ"),
			TEXT("/WFCpp2/Editor/MI_FacePlane_MaxZ.MI_FacePlane_MaxZ")
		};
		static const TCHAR* const MaterialPathGreyscale = {
			TEXT("/WFCpp2/Editor/MI_FacePlane_Grey.MI_FacePlane_Grey")
		};

		return LoadObject<UMaterialInterface>(
			nullptr,
			face.IsSet() ? MaterialPathsByColor[static_cast<int>(*face)] : MaterialPathGreyscale,
			nullptr, LOAD_EditorOnly
		);
	}
}

FEditorSceneObject_WfcFace::FEditorSceneObject_WfcFace(FPreviewScene* owner,
                                                       const FTransform& _tileTr, double _cubeExtents,
                                                       WFC_Directions3D _face, const FWfcFacePrototype& _facePoints,
                                                       WFC_Transforms2D _facePermutation,
                                                       const FEditorSceneObject_WfcFace_Settings& _settings)
    : FEditorSceneObject(owner),
	  cubeExtents(_cubeExtents),
      faceSide(_face), facePrototype(_facePoints), facePermutation(_facePermutation),
	  settings(_settings)
{
	centerSphere.Emplace(Owner, FTransform{ }, FColor{ 1, 1, 1 });
	facePlane.Emplace(Owner, FTransform{ }, LoadFaceEditorMaterial(settings.ColorByFace ? TOptional{ faceSide } : NullOpt));

	//Display the face's nickname at its center.
	fallbackLabel.Emplace(Owner, FTransform{ }, facePrototype.Nickname, FColor{ 1, 1, 1 });

	bool flipFaceCoords = !WFC::Tiled3D::IsFaceLeftHanded(static_cast<WFC::Tiled3D::Directions3D>(faceSide));
	
	//Generate components for the face corners.
	auto doCorner = [&](WFC::Tiled3D::FacePoints pointLocationTile)
	{
		auto pointLocationPrototype = WFC::Tiled3D::TransformFaceCorner(
			pointLocationTile,
			static_cast<WFC::Tiled3D::Directions3D>(faceSide),
			WFC::Invert(static_cast<WFC::Transformations>(facePermutation))
		);
		bool minAxis1 = WFC::Tiled3D::IsCornerFirstAxisMin(pointLocationTile),
			 minAxis2 = WFC::Tiled3D::IsCornerSecondAxisMin(pointLocationTile);
		
		cornerIDs[pointLocationTile] = facePrototype.Corners.PointAt(pointLocationPrototype);
		
		const auto* pointLabel = facePrototype.Corners.GetName(pointLocationPrototype);
		if (pointLabel != nullptr)
		{
			cornerArrows[pointLocationTile].Emplace(Owner, FTransform{ }, FColor{ });
			cornerLabels[pointLocationTile].Emplace(Owner, FTransform{ }, *pointLabel, FColor{ },
													((minAxis1 != flipFaceCoords) ? EHTA_Right : EHTA_Left),
													(minAxis2 ? EVRTA_TextTop : EVRTA_TextBottom));

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
		bool parallelAxis1 = WFC::Tiled3D::IsEdgeParallelToFirstAxis(pointLocationTile),
		     minEdge = WFC::Tiled3D::IsEdgeOnMinSide(pointLocationTile);

		edgeIDs[pointLocationTile] = facePrototype.Edges.PointAt(pointLocationPrototype);
		
		const auto* pointLabel = facePrototype.Edges.GetName(pointLocationPrototype);
		if (pointLabel != nullptr)
		{
			edgeArrows[pointLocationTile].Emplace(Owner, FTransform{ }, FColor{ });
			edgeLabels[pointLocationTile].Emplace(Owner, FTransform{ }, *pointLabel, FColor{ },
											      (parallelAxis1 ? EHTA_Center :
											       	  ((minEdge != flipFaceCoords) ? EHTA_Right : EHTA_Left)),
											      (!parallelAxis1 ? EVRTA_TextCenter :
											      	  ((minEdge != flipFaceCoords) ? EVRTA_TextTop : EVRTA_TextBottom)));

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
	auto unwrappedFace = static_cast<WFC::Tiled3D::Directions3D>(faceSide);
	uint_fast8_t faceIdx, faceAxis1, faceAxis2;
	WFC::Tiled3D::GetAxes(unwrappedFace, faceIdx, faceAxis1, faceAxis2);
	int faceSign = WFC::Tiled3D::IsMin(unwrappedFace) ? -1 : 1;

	//Compute this face's rotation.
	FVector faceOutward = FVector::ZeroVector,
			faceTangent1 = FVector::ZeroVector,
		    faceTangent2 = FVector::ZeroVector;
	faceOutward[faceIdx] = faceSign;
	faceTangent1[faceAxis1] = 1;
	faceTangent2[faceAxis2] = 1;
	auto faceLocalRot = UKismetMathLibrary::MakeRotFromXZ(faceOutward, faceTangent2);

	//Compute this face's center.
	FVector faceLocalCenter{ 0, 0, 0 };
	faceLocalCenter[faceIdx] = faceSign * cubeExtents;

	//Compute a transform for the face's center, looking outward.
	FTransform faceCenterLocalTr{
		UKismetMathLibrary::MakeRotFromXZ(faceOutward, faceTangent2),
		faceLocalCenter
	};

	//Some transform data should grow with the tile size.
	double arrowThickness = 0.5 * FMath::Lerp(2.5, 10.0, FMath::GetRangePct(100.0, 1000.0, cubeExtents)),
		   labelScale = FMath::Lerp(0.25, 1.0, FMath::GetRangePct(100.0, 1000.0, cubeExtents)),
		   sphereSize = FMath::Lerp(5.0, 25.0, FMath::GetRangePct(100.0, 1000.0, cubeExtents)),
		   edgeLabelOffset = FMath::Pow(cubeExtents / 10, 0.5);
	
	if (centerSphere.IsSet())
		centerSphere->SetWorldTransformFromSequence(
			FTransform{ FQuat::Identity, FVector::ZeroVector, FVector{ sphereSize } },
			faceCenterLocalTr,
			tileTr
		);
	
	if (facePlane.IsSet())
		facePlane->SetWorldTransformFromSequence(
			FEditorPlaneComponent::GetTransform(
				faceCenterLocalTr.GetLocation(),
				FVector2D{ cubeExtents },
				faceCenterLocalTr.GetRotation().GetForwardVector()
			),
			tileTr
		);

	if (fallbackLabel.IsSet())
		fallbackLabel->SetWorldTransformFromSequence(
			FTransform{
				FQuat::Identity,
				FVector{ cubeExtents * 0.1, 0, 0 },
				FVector::OneVector * labelScale
			},
			faceCenterLocalTr,
			tileTr
		);

	//Calculate Corner transforms.
	for (int faceAxis1Dir = 0; faceAxis1Dir < 2; ++faceAxis1Dir)
		for (int faceAxis2Dir = 0; faceAxis2Dir < 2; ++faceAxis2Dir)
		{
			auto facePointTile = WFC::Tiled3D::MakeCornerFacePoint(faceAxis1Dir == 0, faceAxis2Dir == 0);

			//Corner offsets must be in tile-local space, not face space,
			//    otherwise they are affected by the face's rotation
			//    causing details on opposite faces to not line up.
			FVector tileLocalCornerOffset;
			tileLocalCornerOffset[faceIdx] = cubeExtents * faceSign;
			tileLocalCornerOffset[faceAxis1] = cubeExtents * (faceAxis1Dir == 0 ? -1 : 1);
			tileLocalCornerOffset[faceAxis2] = cubeExtents * (faceAxis2Dir == 0 ? -1 : 1);
			FVector tileLocalCornerDir = tileLocalCornerOffset.GetUnsafeNormal();

			if (cornerLabels[facePointTile].IsSet())
			{
				//Give each face a different vertical offset so the labels don't intersect each other.
				double verticalOffset = cubeExtents * 0.15 * std::to_array({-1, 1, 0})[faceIdx];

				FVector tileLocalLabelOffset = tileLocalCornerOffset;
				tileLocalLabelOffset[faceIdx] *= 1.15;
				tileLocalLabelOffset.Z += verticalOffset;
				
				cornerLabels[facePointTile]->SetWorldTransformFromSequence(
					FTransform {
						faceCenterLocalTr.GetRotation(),
						tileLocalLabelOffset,
						FVector::OneVector * labelScale
					},
					tileTr
				);
			}
			if (cornerArrows[facePointTile].IsSet())
			{
				auto faceWorldTr = WfcppUnrealEditor::ComposeTransforms(
					faceCenterLocalTr,
					tileTr
				);
				auto arrowTr = FEditorArrowComponent::GetTransform(
					faceWorldTr.GetLocation(),
					tileTr.TransformPosition(tileLocalCornerOffset),
					arrowThickness
				);
				
				auto& arrow = cornerArrows[facePointTile];
				arrow->SetWorldTransformFromSequence(WfcppUnrealEditor::WithoutScale(arrowTr));
				arrow->GetComponent()->SetArrowSize(arrowTr.GetScale3D().Y);
				arrow->GetComponent()->SetArrowLength(arrowTr.GetScale3D().X);
			}
		}
	
	//Calculate edge transforms.
	for (int edgeParallelAxis = 0; edgeParallelAxis < 2; ++edgeParallelAxis)
		for (int edgeSide = 0; edgeSide < 2; ++edgeSide)
		{
			auto facePoint = WFC::Tiled3D::MakeEdgeFacePoint(edgeParallelAxis == 0, edgeSide == 0);
			
			int faceEdgeParallelAxis = std::to_array({ faceAxis1, faceAxis2 })[edgeParallelAxis],
				faceEdgePerpendicularAxis = std::to_array({ faceAxis1, faceAxis2 })[(edgeParallelAxis + 1) % 2];
			
			//Edge offsets must be in tile-local space, not face space,
			//    otherwise they are affected by the face's rotation
			//    causing details on opposite faces to not line up.
			FVector tileLocalEdgeOffset;
			tileLocalEdgeOffset[faceIdx] = cubeExtents * faceSign;
			tileLocalEdgeOffset[faceEdgeParallelAxis] = 0;
			tileLocalEdgeOffset[faceEdgePerpendicularAxis] = cubeExtents * (edgeSide == 0 ? -1 : 1);
			FVector tileLocalEdgeDir = tileLocalEdgeOffset.GetUnsafeNormal();
			
			if (edgeLabels[facePoint].IsSet())
			{
				FVector tileLocalLabelOffset = tileLocalEdgeOffset;
				tileLocalLabelOffset[faceEdgePerpendicularAxis] += edgeLabelOffset * (edgeSide == 0 ? -1 : 1);
				tileLocalLabelOffset[faceIdx] *= 1.15;
				
				edgeLabels[facePoint]->SetWorldTransformFromSequence(
					FTransform{
						FQuat::Identity,
						FVector::ZeroVector,
						FVector::OneVector * labelScale
					},
					faceCenterLocalTr,
					FTransform{
						FQuat::Identity,
						tileLocalLabelOffset - faceCenterLocalTr.GetLocation(),
						FVector::OneVector
					},
					tileTr
				);
			}
			if (edgeArrows[facePoint].IsSet())
			{
				auto faceWorldTr = WfcppUnrealEditor::ComposeTransforms(
					faceCenterLocalTr,
					tileTr
				);
				auto arrowTr = FEditorArrowComponent::GetTransform(
					faceWorldTr.GetLocation(),
					tileTr.TransformPosition(tileLocalEdgeOffset),
					arrowThickness
				);
				
				auto& arrow = edgeArrows[facePoint];
				arrow->SetWorldTransformFromSequence(WfcppUnrealEditor::WithoutScale(arrowTr));
				arrow->GetComponent()->SetArrowSize(arrowTr.GetScale3D().Y);
				arrow->GetComponent()->SetArrowLength(arrowTr.GetScale3D().X);
			}
		}
}
void FEditorSceneObject_WfcFace::RebuildColors()
{
	auto rawFace = static_cast<WFC::Tiled3D::Directions3D>(faceSide);
	int faceIdx = WFC::Tiled3D::GetAxisIndex(rawFace);
	
	auto faceColor = settings.ColorByFace ?
					   std::to_array({
					       FLinearColor{ 1, 0, 0 },
						   FLinearColor{ 0, 1, 0 },
						   FLinearColor{ 0, 0, 1 }
					   })[faceIdx] :
					   FLinearColor{ 0.2, 0.2, 0.2 };
	auto pointIDTints = std::to_array({
		FLinearColor{ 0, 0, 0, 1 },
		FLinearColor{ 1, 0.5, 0.5, 1 },
		FLinearColor{ 0.5, 1, 0.5, 1 },
		FLinearColor{ 0.5, 0.5, 1, 1 }
	});
	FLinearColor cornerTint{ 0.85, 0.85, 0.85, 1 },
				 edgeTint{ 0.5, 0.5, 0.5, 1 },
				 fallbackTint{ 0.8, 0.8, 0.8, 0.3 };

	auto outputColor = [&](const FLinearColor& col) {
		return (col * FLinearColor{ 1, 1, 1, settings.AlphaScale }).ToFColorSRGB();
	};

	if (fallbackLabel.IsSet())
		fallbackLabel->GetComponent()->SetTextRenderColor(outputColor(faceColor * fallbackTint));
	
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
													   const UWfcTileset* tileset, int32 tileID,
													   const FWFC_Transform3D& permutation,
													   const FEditorSceneObject_WfcTile_Settings& _settings)
    : FEditorSceneObject(&owner),
      tileBounds(Owner,
      			 FBox{ tr.GetLocation() - ((tileset->TileLength / 2.0) * tr.GetScale3D()),
      			 	   tr.GetLocation() + ((tileset->TileLength / 2.0) * tr.GetScale3D()) },
      			 tr.GetRotation().Rotator(),
      			 _settings.BoundsColor.ToFColorSRGB()),
      settings(_settings)
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
			          *facePrototype, faceData.PrototypeOrientation,
			          settings);
	}

	if (settings.IncludeDataVisualizer)
		tileDataVisualizer = WfcTileVisualizer::MakeVisualizer({
			owner, viewportClient,
			{ tileset }, tileID, permutation,
			tileset->Tiles[tileID], tr
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
			const UWfcTileset* tileset, int32 tileID,
			const FEditorSceneObject_WfcPermutations_Settings& _settings
		)
	: FEditorSceneObject(&owner), settings(_settings)
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
		auto permutationSettings = static_cast<FEditorSceneObject_WfcTile_Settings>(settings);
		permutationSettings.ColorByFace = wfcTr.IsIdentity();
		permutations.Emplace(
			FEditorSceneObject_WfcTile{
				owner, viewportClient, worldTr,
				tileset, tileID, wfcTr,
				permutationSettings
			},
			FEditorTextComponent{
				Owner, labelTr, labelStr,
				(settings.LabelsTint * settings.PermutationLabelColor).ToFColorSRGB(),
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
		settings.LabelsTint.ToFColorSRGB(),
		EHTA_Center, EVRTA_TextTop
	);
}

FEditorSceneObject_WfcTileWithMatches::FEditorSceneObject_WfcTileWithMatches(
			FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
			const FTransform& rootTr, double spacingBetweenTiles,
			const UWfcTileset* tileset, int32 tileID, const FWFC_Transform3D& permutation,
			const TSet<WFC_Directions3D>& facesToMatchAfterPermutation,
			const FEditorSceneObject_WfcMatches_Settings& _settings
		)
	: FEditorSceneObject(&owner), settings(_settings)
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
					   tileset, tileID, permutation,
					   settings);

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

					auto matchSettings = static_cast<FEditorSceneObject_WfcTile_Settings>(settings);
					matchSettings.ColorByFace = false;
					matches.Emplace(
						matchTileID, matchTilePermutation,
						static_cast<WFC_Directions3D>(srcFace),
						FEditorSceneObject_WfcTile{
							owner, viewportClient,
							UKismetMathLibrary::ComposeTransforms(
								matchedTileTr,
								rootTr
							),
							tileset, matchTileID, matchTilePermutation,
							matchSettings
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
							settings.LabelsTint.ToFColorSRGB(),
							EHTA_Center, EVRTA_TextBottom
						}
					);
				
					matchI1 += 1;
				}
			}
		}
		
		//Add an informative label on top of the source tile's face.
		FLinearColor faceLabelColor = settings.LabelsTint;
		if (settings.ColorByFace)
			faceLabelColor *= std::to_array({
				FLinearColor{ 1.0, 0.5, 0.5 },
				FLinearColor{ 0.5, 1.0, 0.5 },
				FLinearColor{ 0.5, 0.5, 1.0 }
			})[WFC::Tiled3D::GetAxisIndex(srcFace)];
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
			faceLabelColor.ToFColorSRGB(),
			EHTA_Center,
			(srcFace == WFC::Tiled3D::Directions3D::MinZ ? EVRTA_TextTop : EVRTA_TextBottom)
		);
	}
}