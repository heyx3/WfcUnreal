﻿#pragma once

#include "CoreMinimal.h"

#include "WfcDataReflection.h"
#include "EditorSceneComponents.h"
#include "WfcFacePrototype.h"


//A scoped owner of a group of components in a preview/editor scene,
//    not unlike an Actor in a game scene.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject
{
public:

	FPreviewScene* const Owner;
	FEditorSceneObject(FPreviewScene* owner) : Owner(owner) { }

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

	float alphaScale = 1.0f;

	void RebuildColors();
	void RebuildTransform();
};

//An editor object that visualizes the symmetry info for a WFC tile.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTile(FPreviewScene* owner,
							   const FTransform& tr, double extentsPreScaling,
							   const FLinearColor& boundsColor,
							   const class UWfcTileset* tileset, int32 tileID);

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
};