#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"


class UWfcTileset;
class IWfcTilesetEditor;

extern const FName WfcTilesetEditorAppIdentifier;
DECLARE_LOG_CATEGORY_EXTERN(LogWFCppEditor, Log, All);


class IWFCpp2UnrealEditorModule : public IModuleInterface,
                                  public IHasMenuExtensibility,
                                  public IHasToolBarExtensibility
{
public:
    virtual TSharedRef<IWfcTilesetEditor> CreateCustomAssetEditor(EToolkitMode::Type mode,
                                                                  const TSharedPtr<IToolkitHost>& initToolkitHost,
                                                                  UWfcTileset* tileset) = 0;

    //Sets up a visualizer for the given kind of tile data.
    //Throws an error if you already registered a visualizer for the same type (children and parents are OK).
    virtual void RegisterTileDataVisualizer(TSubclassOf<class UWfcTileGameData>, TSharedRef<class FWfcTileVisualizerBase>) = 0;
    //Gets the most specific visualizer for the given kind of tile data.
    //Returns null if no visualizer is registered.
    virtual TSharedPtr<class FWfcTileVisualizerBase> GetTileDataVisualizer(TSubclassOf<class UWfcTileGameData>) const = 0;
};
