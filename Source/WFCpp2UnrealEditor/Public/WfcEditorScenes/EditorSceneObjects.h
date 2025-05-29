#pragma once

#include "CoreMinimal.h"

#include "WfcDataReflection.h"
#include "EditorSceneComponents.h"
#include "WfcFacePrototype.h"
#include "WfcTilesetEditorViewportClient.h"
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


struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcFace_Settings
{
	float AlphaScale = 1;
	bool ColorByFace = true;
};
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile_Settings : public FEditorSceneObject_WfcFace_Settings
{
	bool IncludeDataVisualizer = true;
	FLinearColor BoundsColor = { 0, 0, 0, 1 };
};

struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcPermutations_Settings : public FEditorSceneObject_WfcTile_Settings
{
	FLinearColor PermutationLabelColor = { 0.1, 0.1, 0.1, 1 };
	FLinearColor LabelsTint = { 0.5, 0.5, 0.5, 1 };
};
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcMatches_Settings : public FEditorSceneObject_WfcTile_Settings
{
	FLinearColor LabelsTint = { 0.5, 0.5, 0.5, 1 };
};


//An editor object that displays the symmetry info for a WFC tile face.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcFace : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcFace(FPreviewScene* owner,
							   const FTransform& tileTransform, double cubeExtents,
							   WFC_Directions3D faceDir,
							   const FWfcFacePrototype& faceData,
							   WFC_Transforms2D facePermutation,
							   const FEditorSceneObject_WfcFace_Settings& settings);

	void SetTileTransform(const FTransform& tr)
	{
		tileTr = tr;
		RebuildTransform();
	}
	void SetAlphaScale(float newScale)
	{
		settings.AlphaScale = newScale;
		RebuildColors();
	}

	
private:

	FTransform tileTr;
	double cubeExtents;
	
	WFC_Directions3D faceSide;
	FWfcFacePrototype facePrototype;
	WFC_Transforms2D facePermutation;

	TOptional<FEditorWireSphereComponent> centerSphere;
	TOptional<FEditorTextComponent> fallbackLabel;
	TOptional<FEditorPlaneComponent> facePlane;
	std::array<TOptional<FEditorTextComponent>, 4> cornerLabels, edgeLabels;
	std::array<TOptional<FEditorArrowComponent>, 4> cornerArrows, edgeArrows;
	std::array<EWfcPointID, 4> cornerIDs, edgeIDs;

	FEditorSceneObject_WfcFace_Settings settings;
	

	void RebuildColors();
	void RebuildTransform();
};

//Displays one WFC tile, with its symmetry information.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTile(FWfcTilesetEditorScene& owner, class FWfcTilesetEditorViewportClient& viewportClient,
							   const FTransform& worldTr,
							   const class UWfcTileset* tileset, int32 tileID, const FWFC_Transform3D& permutation,
							   const FEditorSceneObject_WfcTile_Settings& settings);

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

	FEditorSceneObject_WfcTile_Settings settings;
};

//Displays all supported permutations of one WFC tile.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTileWithPermutations : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTileWithPermutations(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
										       const FTransform& tr, double spacingBetweenTiles,
											   const class UWfcTileset* tileset, int32 tileID,
											   const FEditorSceneObject_WfcPermutations_Settings& settings);

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

	FEditorSceneObject_WfcPermutations_Settings settings;
};

struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTileWithMatches : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTileWithMatches(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
										  const FTransform& tr, double spacingBetweenTiles,
										  const class UWfcTileset* tileset, int32 tileID,
										  const FWFC_Transform3D& permutation,
										  const TSet<WFC_Directions3D>& facesToMatchAfterPermutation,
										  const FEditorSceneObject_WfcMatches_Settings& settings);

private:

	TOptional<UWfcTileset::Unwrapped> libraryTilesetData;
	TOptional<FEditorSceneObject_WfcTile> sourceTile;

	TArray<FEditorTextComponent> faceLabels;

	FEditorSceneObject_WfcMatches_Settings settings;
	
	struct Match
	{
		int32 TileID;
		FWFC_Transform3D Permutation;
		
		WFC_Directions3D SrcFaceToMatch;
		
		FEditorSceneObject_WfcTile EditorObject;
		FEditorTextComponent Label;
	};
	TArray<Match> matches;
};