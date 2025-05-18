#pragma once

#include "EditorViewportClient.h"

#include "WfcTilesetEditorScene.h"


//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//Controls the actors/components in the 3D tile visualization scene.
//The scene's setup and data is handled by the "preview scene", FWfcTilesetEditorScene.
class FWfcTilesetEditorViewportClient : public FEditorViewportClient
{
public:
    FOnGameViewportTick OnTick;
    
    FWfcTilesetEditorViewportClient(class FWfcTilesetEditorScene& scene);
    
    virtual void Draw(FViewport* viewport, FCanvas* canvas) override;
    virtual void Tick(float deltaSeconds) override;

    virtual bool ShouldOrbitCamera() const override { return true; }

private:

    bool firstTick = true;
};