#include "WFCpp2UnrealEditor.h"

#include "Modules/ModuleManager.h"
#include "Algo/AnyOf.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "PropertyEditorModule.h"
#include "IDetailChildrenBuilder.h"

#include "AssetTypeActions_WfcTileset.h"
#include "WfcTileData.h"
#include "WfcTilesetEditor.h"
#include "WfcTileVisualizer.h"


#define LOCTEXT_NAMESPACE "FWFCpp2UnrealEditorModule"
DEFINE_LOG_CATEGORY(LogWFCppEditor);

const FName WfcTilesetEditorAppIdentifier = FName(TEXT("CustomAssetEditorApp"));


class FWfcppEditorModule : public IWFCpp2UnrealEditorModule
{
public:

	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Warning, TEXT("StartupModule() WFC-editor"));
		
		menuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		toolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

		auto& assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(assetTools, MakeShareable(new FAssetTypeActions_WfcTileset));

	    //Register a custom Properties widget for tilesets.
	    auto& propEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	    // propEditorModule.RegisterCustomClassLayout(UWfcTileset::StaticClass()->GetFName(),
	    //                                            FOnGetDetailCustomizationInstance::CreateLambda([]() { return MakeShared(FTilesetDetailsCustomization()); }));
	    //TODO: Re-enable this

		//Register our built-in tile data visualizers.
		RegisterTileDataVisualizer(UWfcTileGameData_StaticMesh::StaticClass(), MakeShared<FWfcTileVisualizer_StaticMesh>());
		RegisterTileDataVisualizer(UWfcTileGameData_Actor::StaticClass(), MakeShared<FWfcTileVisualizer_Actor>());
	}
	virtual void ShutdownModule() override
	{
		UE_LOG(LogTemp, Warning, TEXT("ShutdownModule() WFC-editor"));

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

		//Unregister all tile data visualizers.
		VisualizerLookup.Empty();
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
	
	TArray<TTuple<TWeakObjectPtr<UClass>, TSharedPtr<FWfcTileVisualizerBase>>> VisualizerLookup;
	virtual void RegisterTileDataVisualizer(TSubclassOf<UWfcTileGameData> tileDataType,
				  							TSharedRef<FWfcTileVisualizerBase> visualizer) override
	{
		if (!Algo::AnyOf(VisualizerLookup, [&](const auto& tuple) { return tuple.Key == tileDataType.Get(); }))
			VisualizerLookup.Emplace(tileDataType.Get(), visualizer);
		else
			checkf(false, TEXT("Registered more than one WfcTileVisualizer for %s"), *tileDataType->GetName());
	}
	virtual TSharedPtr<FWfcTileVisualizerBase> GetTileDataVisualizer(TSubclassOf<UWfcTileGameData> tileDataType) const override
	{
		if (!IsValid(tileDataType))
			return nullptr;
    
		using Element_t = std::remove_reference_t<decltype(VisualizerLookup[0])>;
		const Element_t* bestOption = nullptr;
		for (const auto& visualizerLookupData : VisualizerLookup)
		{
			const auto lookupClass = visualizerLookupData.Key;
        
			//Is the registered type still valid?
			if (lookupClass.IsValid())
				//Is the registered type compatible with this type?
					if ((tileDataType->IsChildOf(lookupClass.Get()) || lookupClass->IsChildOf(tileDataType)))
						//Is this the most *specific* registered type found so far?
							if (bestOption == nullptr || lookupClass->IsChildOf(bestOption->Key.Get()))
								bestOption = &visualizerLookupData;
		}

		return (bestOption == nullptr) ? nullptr : bestOption->Value;
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

IMPLEMENT_GAME_MODULE(FWfcppEditorModule, WfcppEditor);


#undef LOCTEXT_NAMESPACE
    