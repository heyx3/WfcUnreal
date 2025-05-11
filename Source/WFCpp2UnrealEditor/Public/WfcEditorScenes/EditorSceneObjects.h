#pragma once

#include "CoreMinimal.h"

#include "WfcDataReflection.h"
#include "EditorSceneComponents.h"
#include "WfcFacePrototype.h"
#include "WfcTileVisualizer.h"


//A scoped owner of a group of components in a preview/editor scene,
//    not unlike an Actor in a game scene.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject
{
public:

	FPreviewScene* const Owner;
	
	FEditorSceneObject(FPreviewScene* owner) : Owner(owner) { }
	FEditorSceneObject(FEditorSceneObject&&) = default;

	virtual ~FEditorSceneObject() { }

	FEditorSceneObject& operator=(const FEditorSceneObject&) = delete;
	FEditorSceneObject& operator=(FEditorSceneObject&&) = delete;

	
	virtual void Tick(float deltaSeconds) { }
};

//An editor object that displays the symmetry info for a WFC tile face.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcFace : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcFace(FPreviewScene* owner,
							   const FTransform& tileTransform, double cubeExtents,
							   WFC_Directions3D faceDir,
							   const FWfcFacePrototype& faceData,
							   WFC_Transforms2D facePermutation);

	void SetTileTransform(const FTransform& tr)
	{
		tileTr = tr;
		RebuildTransform();
	}
	void SetAlphaScale(float newScale)
	{
		alphaScale = newScale;
		RebuildColors();
	}

	
private:

	FTransform tileTr;
	double cubeExtents;
	
	WFC_Directions3D faceSide;
	FWfcFacePrototype facePrototype;
	WFC_Transforms2D facePermutation;

	TOptional<FEditorWireSphereComponent> centerSphere;
	TOptional<FEditorPlaneComponent> facePlane;
	std::array<TOptional<FEditorTextComponent>, 4> cornerLabels, edgeLabels;
	std::array<TOptional<FEditorArrowComponent>, 4> cornerArrows, edgeArrows;
	std::array<EWfcPointID, 4> cornerIDs, edgeIDs;

	float alphaScale = 1.0f;

	void RebuildColors();
	void RebuildTransform();
};

//An editor object that visualizes the symmetry info for a WFC tile.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTile(FWfcTilesetEditorScene& owner, class FWfcTilesetEditorViewportClient& viewportClient,
							   const FTransform& tr,
							   const FLinearColor& boundsColor,
							   const class UWfcTileset* tileset, int32 tileID,
							   bool includeVisualizer = true);

	FTransform GetCurrentTransform() const { return currentTr; }
	void SetTransform(const FTransform& newTr);

	const auto& GetFaces() const { return faces; }
	FEditorSceneObject_WfcFace& GetFace(int i) { return faces[i]; }
	FEditorSceneObject_WfcFace& GetFace(WFC::Tiled3D::Directions3D face) { return faces[face]; }
	FEditorSceneObject_WfcFace& GetFace(WFC_Directions3D face) { return faces[static_cast<int>(face)]; }
	
private:

	FTransform currentTr;
	
 	TArray<FEditorSceneObject_WfcFace, TInlineAllocator<WFC::Tiled3D::N_DIRECTIONS_3D>> faces;
	FEditorWireBoxComponent tileBounds;
	TUniquePtr<WfcTileVisualizer> tileDataVisualizer;
};

//An editor object that visualizes all supported permutations of a WFC tile.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTileWithPermutations : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTileWithPermutations(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
										       const FTransform& tr, double spacingBetweenTiles,
											   const FLinearColor& boundsColor, const FLinearColor& labelColor,
											   const class UWfcTileset* tileset, int32 tileID,
											   bool visualizeTileData = true);

private:

	struct Permutation
	{
		FEditorSceneObject_WfcTile Tile;
		FEditorTextComponent Label;
		FWFC_Transform3D WfcTransform;

		Permutation(FEditorSceneObject_WfcTile&& tile, FEditorTextComponent&& label, const FWFC_Transform3D& tr)
			: Tile(MoveTemp(tile)), Label(MoveTemp(label)), WfcTransform(tr) { }
	};
	TArray<Permutation, TInlineAllocator<WFC::Tiled3D::N_TRANSFORMS>> permutations;
	TOptional<FEditorTextComponent> overallLabel;
};