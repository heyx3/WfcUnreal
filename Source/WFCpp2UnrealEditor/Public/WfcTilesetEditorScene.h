﻿#pragma once

#include <array>

#include "WFC++/include/Tiled3D/Transform3D.h"
#include "WfcTileset.h"

#include "AdvancedPreviewScene.h"


class UBoxComponent;
class USphereComponent;
class UTextRenderComponent;

//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//Stores the actors/components in the 3D tile visualization scene.
//The runtime logic for the scene is handled by the "viewport client", FWfcTilesetEditorViewportClient.
//Displays a main tile visualization component,
//    plus another tile oriented so that it can line up with a specific face on the main tile.
class FWfcTilesetEditorScene : public FAdvancedPreviewScene
{
public:
    FWfcTilesetEditorScene(ConstructionValues cvs = ConstructionValues());

    //Call continuously so that this scene can respond to changes in tile data, camera, etc.
    void Refresh(const UWfcTileset* tileset, TOptional<WfcTileID> tileID, const FVector& camPos,
                 class FWfcTilesetEditorViewportClient* owner);

    //TODO: Configurable origin point
    
private:
    
    struct FacePointViz
    {
        FVector Pos;
        WFC::Tiled3D::FacePoints CornerType;
        
        USphereComponent* Shape;
        UTextRenderComponent* Label;
    };
    void InitializeFacePointViz(WFC::Tiled3D::Directions3D face, FacePointViz& inOutData);
    
    struct FaceViz
    {
        FVector Pos;
        WFC::Tiled3D::Directions3D Dir;
        
        UStaticMeshComponent* Component;
        TStaticArray<FacePointViz, WFC::Tiled3D::N_FACE_POINTS> Points;
    };
    void InitializeFaceViz(FaceViz& inOutData);

    
    TStaticArray<FaceViz, WFC::Tiled3D::N_DIRECTIONS_3D> faces;
    TWeakObjectPtr<UTextRenderComponent> faceLabel;

    int32 chosenTileIdx;
    TWeakObjectPtr<class UWfcTileGameData> chosenTileData;
    TSharedPtr<class FWfcTileVisualizerBase> tileVisualizer;
    
    TArray<TWeakObjectPtr<AActor>> tileVizActors;
    TArray<TWeakObjectPtr<UActorComponent>> tileVizLoneComponents;
};