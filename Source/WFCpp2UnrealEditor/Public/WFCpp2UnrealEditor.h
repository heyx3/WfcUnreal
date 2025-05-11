#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"


class UWfcTileset;
class IWfcTilesetEditor;

extern const FName WfcTilesetEditorAppIdentifier;
WFCPP2UNREALEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogWFCppEditor, Log, All);


class IWFCpp2UnrealEditorModule : public IModuleInterface,
                                  public IHasMenuExtensibility,
                                  public IHasToolBarExtensibility
{
public:
    virtual TSharedRef<IWfcTilesetEditor> CreateCustomAssetEditor(EToolkitMode::Type mode,
                                                                  const TSharedPtr<IToolkitHost>& initToolkitHost,
                                                                  UWfcTileset* tileset) = 0;
};
