#include "WFCpp2UnrealEditor.h"

#include "Modules/ModuleManager.h"
#include "Algo/AnyOf.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "PropertyEditorModule.h"
#include "IDetailChildrenBuilder.h"

#include "AssetTypeActions_WfcTileset.h"
#include "WFCpp2UnrealRuntime.h"
#include "WfcTileData.h"
#include "WfcTilesetEditor.h"
#include "WfcEditorScenes/WfcTileVisualizer.h"


#define LOCTEXT_NAMESPACE "FWFCpp2UnrealEditorModule"
DEFINE_LOG_CATEGORY(LogWFCppEditor);

const FName WfcTilesetEditorAppIdentifier = FName(TEXT("CustomAssetEditorApp"));


class FWfcpp2UnrealEditorModule : public IWFCpp2UnrealEditorModule
{
public:

	virtual void StartupModule() override
	{
		UE_LOG(LogWFCppEditor, Log, TEXT("StartupModule() WFC-editor"));
		
		menuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		toolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		auto& assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(assetTools, MakeShareable(new FAssetTypeActions_WfcTileset));

	    //Register a custom Properties widget for tilesets.
	    auto& propEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	    // propEditorModule.RegisterCustomClassLayout(UWfcTileset::StaticClass()->GetFName(),
	    //                                            FOnGetDetailCustomizationInstance::CreateLambda([]() { return MakeShared(FTilesetDetailsCustomization()); }));
	    //TODO: Re-enable this
	}
	virtual void ShutdownModule() override
	{
		UE_LOG(LogWFCppEditor, Log, TEXT("ShutdownModule() WFC-editor"));

		menuExtensibilityManager.Reset();
		toolBarExtensibilityManager.Reset();

		//Unregister our custom assets from the AssetTools module.
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			auto& assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (const auto& action : createdAssetTypeActions)
				assetTools.UnregisterAssetTypeActions(action.ToSharedRef());
		}
		createdAssetTypeActions.Empty();

	    auto& propEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	    propEditorModule.UnregisterCustomClassLayout(UWfcTileset::StaticClass()->GetFName());
	}

	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override { return menuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override { return toolBarExtensibilityManager; }

	virtual TSharedRef<IWfcTilesetEditor> CreateCustomAssetEditor(EToolkitMode::Type mode,
																  const TSharedPtr<IToolkitHost>& initToolkitHost,
																  UWfcTileset* tileset) override
	{
	    auto* editor = new FWfcTilesetEditor;
	    TSharedRef<IWfcTilesetEditor> editorRef(editor);
	    
		editor->InitWfcTilesetEditorEditor(mode, initToolkitHost, tileset);
		return editorRef;
	}


private:
	TSharedPtr<FExtensibilityManager> menuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> toolBarExtensibilityManager;
	TArray<TSharedPtr<IAssetTypeActions>> createdAssetTypeActions;

	void RegisterAssetTypeAction(IAssetTools& assetTools, TSharedRef<IAssetTypeActions> action)
	{
		assetTools.RegisterAssetTypeActions(action);
		createdAssetTypeActions.Add(action);
	}
};

IMPLEMENT_GAME_MODULE(FWfcpp2UnrealEditorModule, Wfcpp2UnrealEditor);


#undef LOCTEXT_NAMESPACE
    