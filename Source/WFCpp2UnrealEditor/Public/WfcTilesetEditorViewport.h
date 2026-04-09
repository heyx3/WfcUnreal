#pragma once

#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"

class UWfcTileset;
class FWfcTilesetEditorScene;

//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//A slate widget acting as the outermost handler for the 3D tile visualization scene.
class SWfcTilesetEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
    //Slate stuff:
public:
    SLATE_BEGIN_ARGS(SWfcTilesetEditorViewport) { }
    SLATE_END_ARGS()
    void Construct(const FArguments& args);

    //Interface:
public:
    TSharedPtr<FWfcTilesetEditorScene> GetWfcScene() const { return scene; }
    
    //Toolbar interface:
public:
    virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
    virtual TSharedPtr<FExtender> GetExtenders() const override;
    virtual void OnFloatingButtonClicked() override;

    //SEditorViewport interface:
protected:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    #if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 6)
        #define WFCPP_UNREAL_NEW_VIEWPORT_TOOLBAR 1
        virtual TSharedPtr<SWidget> BuildViewportToolbar() override;
    #else
        #define WFCPP_UNREAL_NEW_VIEWPORT_TOOLBAR 0
        virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
    #endif

    //Fields:
private:
    TSharedPtr<FEditorViewportClient> viewportClient;
    TSharedPtr<FWfcTilesetEditorScene> scene;

    //Functions:
private:
};