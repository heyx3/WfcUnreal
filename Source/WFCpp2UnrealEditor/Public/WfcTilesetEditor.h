#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

#include "WfcTileset.h"

class IWfcTilesetEditor : public FAssetEditorToolkit
{
public:
	virtual UWfcTileset* GetAsset() const = 0;
	virtual void SetAsset(UWfcTileset* asset) = 0;
};


extern WFCPP2UNREALEDITOR_API const FName WfcTileset_TabID_Properties,
										  WfcTileset_TabID_EditorSettings;

//The top-level class representing our custom editor for a WFC tileset asset. 
class WFCPP2UNREALEDITOR_API FWfcTilesetEditor : public IWfcTilesetEditor
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& tabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& tabManager) override;

	void InitWfcTilesetEditorEditor(const EToolkitMode::Type mode,
								    const TSharedPtr<class IToolkitHost>& initToolkitHost,
								    UWfcTileset* tileset);

    FWfcTilesetEditor();
	virtual ~FWfcTilesetEditor() override;

	//IToolkit interface:
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual bool IsPrimaryEditor() const override { return true; }

	//IWfcTilesetEditor interface:
	virtual UWfcTileset* GetAsset() const override { return tileset; }
	virtual void SetAsset(UWfcTileset* asset) override;

    //Meant to be usd by WfcTilesetEditorSceneViewTab:
    TSharedRef<SWidget> SpawnSceneView();

	class FWfcTilesetEditorScene& GetScene() const;
    
private:

	TSharedRef<SDockTab> GeneratePropertiesTab(const FSpawnTabArgs& args);
    TSharedRef<SDockTab> GenerateEditorSettingsTab(const FSpawnTabArgs& args);

    void RefreshTileChoices();
    void OnTileSelected(TSharedPtr<FString> name, ESelectInfo::Type);

    void OnTilesetEdited(const FPropertyChangedEvent&);
    void OnSceneTick(float deltaSeconds);

    UWfcTileset* tileset = nullptr;
    TOptional<WfcTileID> tileToVisualize;
    TArray<TSharedPtr<FString>> tilesetTileSelectorChoices;
    TArray<int> tilesetTileSelectorChoiceIDs;

	TSharedPtr<SDockTab> propertiesTab, tileSelectorTab, tileSceneTab;
	TSharedPtr<IDetailsView> detailsView;
    TSharedPtr<STextComboBox> tileSelector;
	TSharedPtr<IStructureDetailsView> editorForPermutationToMatch;
    
    TSharedPtr<struct FWfcTilesetEditorSceneViewTab> tileSceneTabFactory;
    TSharedPtr<class SWfcTilesetTabBody> tileSceneTabBody;
};