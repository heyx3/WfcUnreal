#include "WfcEditorScenes/EditorSceneObjects.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include <WFC++/include/Helpers/WFCMath.h>
#include "WfcBpUtils.h"
#include "WfcTileset.h"


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
	faceOffset = tileTr.Rotator().RotateVector(faceOffset);
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
			cornerOffset = tileTr.Rotator().RotateVector(cornerOffset);

			cornerTrs[facePointTile] = {
				faceTr.Rotator(),
				faceTr.GetLocation() + cornerOffset,
				faceTr.GetScale3D()
			};
			if (cornerLabels[facePointTile].IsSet())
				cornerLabels[facePointTile]->GetComponent()->SetWorldTransform(cornerTrs[facePointTile]);
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
			edgeOffset = tileTr.Rotator().RotateVector(edgeOffset);

			edgeTrs[facePoint] = {
				faceTr.Rotator(),
				faceTr.GetLocation() + edgeOffset,
				faceTr.GetScale3D()
			};
			if (edgeLabels[facePoint].IsSet())
				edgeLabels[facePoint]->GetComponent()->SetWorldTransform(edgeTrs[facePoint]);
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

FEditorSceneObject_WfcTile::FEditorSceneObject_WfcTile(FPreviewScene* owner,
													   const FTransform& tr, double extentsPreScaling,
													   const FLinearColor& boundsColor,
													   const UWfcTileset* tileset, int32 tileID)
    : FEditorSceneObject(owner),
      tileBounds(Owner,
      			 FBox{ tr.GetLocation() - (extentsPreScaling * tr.GetScale3D()),
      			 	   tr.GetLocation() + (extentsPreScaling * tr.GetScale3D()) },
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
		
		faces.Emplace(Owner, tr, extentsPreScaling,
			          static_cast<WFC_Directions3D>(dir),
			          *facePrototype, faceData.PrototypeOrientation);
	}
}
void FEditorSceneObject_WfcTile::SetTransform(const FTransform& newTr)
{
	currentTr = newTr;
	
	for (auto& face : faces)
		face.SetTileTransform(newTr);
	tileBounds.GetComponent()->SetWorldTransform(currentTr);
}