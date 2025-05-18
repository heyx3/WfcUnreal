#include "WfcTilesetEditor.h"

#include "Modules/ModuleManager.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"

#include "Widgets/Docking/SDockTab.h"
//#include "Widgets/Docking/SDockTabStack.h"
#include "CameraController.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "SAssetDropTarget.h"
#include "EditorStyleSet.h"
#include "SEnumCombo.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcEditorScenes/WfcTilesetEditorScene.h"
#include "WfcTilesetEditorSceneViewTab.h"
#include "WfcTilesetEditorViewport.h"
#include "WfcEditorScenes/WfcTilesetEditorViewportClient.h"
#include "WfcTilesetTabBody.h"


#define LOCTEXT_NAMESPACE "WfcTilesetEditor"

const FName ToolkitName(TEXT("WfcTilesetEditor"));

const FName WfcTileset_TabID_Properties     (TEXT("WfcTilesetEditor_Properties"));
const FName WfcTileset_TabID_EditorSettings (TEXT("WfcTilesetEditor_EditorSettings"));

namespace
{
    FString EventToString(FPropertyChangedEvent& ev)
    {
        return FString::Printf(TEXT("'%s' from '%s', object %i of %i, array idx %i"),
                               *ev.GetPropertyName().ToString(),
                               *ev.MemberProperty->GetName(),
                               ev.ObjectIteratorIndex, ev.GetNumObjectsBeingEdited(),
                               ev.GetArrayIndex(ev.GetPropertyName().ToString()));
    }
    FString EventToString(FPropertyChangedChainEvent& ev)
    {
        FString baseStr = EventToString(static_cast<FPropertyChangedEvent&>(ev));
        baseStr += " [";

        for (auto* chain : ev.PropertyChain)
            baseStr += FString::Printf(TEXT("\n\tFrom %s"), *chain->GetFullName());

        baseStr += "\n]";

        return baseStr;
    }
}

void FWfcTilesetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& tabManager)
{
	WorkspaceMenuCategory = tabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_WfcTilesetEditor", "WFC Tileset Editor"));

	//Call a helper function which initializes the UI layout for our new tab spawners.
	FAssetEditorToolkit::RegisterTabSpawners(tabManager);

	//Register the tabs/panes of this editor.
	tabManager->RegisterTabSpawner(WfcTileset_TabID_Properties, FOnSpawnTab::CreateSP(this, &FWfcTilesetEditor::GeneratePropertiesTab))
		.SetDisplayName(LOCTEXT("PropertiesTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
	tabManager->RegisterTabSpawner(WfcTileset_TabID_EditorSettings, FOnSpawnTab::CreateSP(this, &FWfcTilesetEditor::GenerateEditorSettingsTab))
		.SetDisplayName(LOCTEXT("EditorSettingsTab", "Editor Settings"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
	    .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
    
    //The scene view tab has its own special factory.
    //Probably wouldn't hurt to refactor the other tabs similarly (apart from Properties).
    if (!tileSceneTabFactory.IsValid())
        tileSceneTabFactory = MakeShareable(new FWfcTilesetEditorSceneViewTab(SharedThis(this)));
    tileSceneTabFactory->RegisterTabSpawner(tabManager, nullptr)
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());
}
void FWfcTilesetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& tabManager)
{
	//Invoke base-class logic.
	FAssetEditorToolkit::UnregisterTabSpawners(tabManager);
	
	//Also remove the button to spawn our custom editor.
	tabManager->UnregisterTabSpawner(WfcTileset_TabID_Properties);
	tabManager->UnregisterTabSpawner(WfcTileset_TabID_EditorSettings);
	tabManager->UnregisterTabSpawner(tileSceneTabFactory->GetIdentifier());
}

TSharedRef<SDockTab> FWfcTilesetEditor::GeneratePropertiesTab(const FSpawnTabArgs& args)
{
	check(args.GetTabId() == WfcTileset_TabID_Properties);

	return SAssignNew(propertiesTab, SDockTab)
		 .Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
		 .Label(LOCTEXT("GenericDetailsTitle", "Details"))
		 .TabColorScale(GetTabColorScale())
		 [
			 SNew(SVerticalBox)
			 +SVerticalBox::Slot() .HAlign(HAlign_Left) [
		         detailsView.ToSharedRef()
		     ]
		 ];
}
TSharedRef<SDockTab> FWfcTilesetEditor::GenerateEditorSettingsTab(const FSpawnTabArgs& args)
{
	check(args.GetTabId() == WfcTileset_TabID_EditorSettings);

	auto showIfMatchingFn = [&]() {
		switch (GetScene().Mode)
		{
		case EWfcTilesetEditorMode::Tile:
		case EWfcTilesetEditorMode::Permutations:
			return EVisibility::Collapsed;
		case EWfcTilesetEditorMode::Matches:
			return EVisibility::Visible;
								
		default:
			check(false);
			return EVisibility::Hidden;
		}
	};
	possibleFaceNamesToMatch.Empty();
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MinX"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
						    (MakeTuple(WFC_Directions3D::MinX, FText::FromString(TEXT("MinX")))));
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MinY"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
						    (MakeTuple(WFC_Directions3D::MinY, FText::FromString(TEXT("MinY")))));
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MinZ"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
							(MakeTuple(WFC_Directions3D::MinZ, FText::FromString(TEXT("MinZ")))));
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MaxX"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
					        (MakeTuple(WFC_Directions3D::MaxX, FText::FromString(TEXT("MaxX")))));
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MaxY"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
						    (MakeTuple(WFC_Directions3D::MaxY, FText::FromString(TEXT("MaxY")))));
	possibleFaceNamesToMatch.Add(TEXT("WfcDir3D_MaxZ"));
		faceNamesLookup.Add(possibleFaceNamesToMatch.Last(),
							(MakeTuple(WFC_Directions3D::MaxZ, FText::FromString(TEXT("MaxZ")))));

	//Pointers for lambdas to caapture:
	auto* scenePtr = &GetScene();
	auto* faceNamesLookupPtr = &faceNamesLookup;
	
	return SAssignNew(tileSelectorTab, SDockTab)
			.Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
			.Label(LOCTEXT("EditorSettingsTabLabel", "Editor Settings"))
			.TabColorScale(GetTabColorScale())
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
				    SAssignNew(tileSelector, STextComboBox)
				        .OptionsSource(&tilesetTileSelectorChoices)
				        .OnSelectionChanged(this, &FWfcTilesetEditor::OnTileSelected)
				]
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
					[
						SNew(STextBlock)
						  .Text(LOCTEXT("ViewModeDropdownLabel", "Tile View Mode"))
						  .Justification(ETextJustify::Type::Left)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SEnumComboBox, StaticEnum<EWfcTilesetEditorMode>())
						  .CurrentValue_Lambda([&]() {
						      return static_cast<int32>(GetScene().Mode);
						  })
						  .OnEnumSelectionChanged_Lambda([&](int32 v, ESelectInfo::Type) {
						      GetScene().Mode = static_cast<EWfcTilesetEditorMode>(v);
						  })
					]
				]
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
						.Visibility_Lambda([&]() {
							switch (GetScene().Mode)
							{
								case EWfcTilesetEditorMode::Tile:
									return EVisibility::Collapsed;
								case EWfcTilesetEditorMode::Matches:
								case EWfcTilesetEditorMode::Permutations:
									return EVisibility::Visible;
								
								default:
									check(false);
									return EVisibility::Hidden;
							}
						})
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						  .Text(LOCTEXT("TileSeparationLabel", "Tile Visual Separation"))
						  .Justification(ETextJustify::Type::Left)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SNumericEntryBox<float>)
						  .Value_Lambda([&]() { return GetScene().SpacingBetweenTiles; })
						  .OnValueCommitted_Lambda([&](float f, ETextCommit::Type) { GetScene().SpacingBetweenTiles = f; })
					]
				]
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					  .Visibility_Lambda(showIfMatchingFn)
					  .Text(LOCTEXT("MatchFacesLabel", "Faces to match with:"))
					  .Justification(ETextJustify::Type::Left)
				]
				+ SScrollBox::Slot()
				[
					//Tab in:
					SNew(SHorizontalBox)
					   .Visibility_Lambda(showIfMatchingFn)
					+ SHorizontalBox::Slot()[ SNew(SSpacer).Size(FVector2D{ 20, 1 }) ]
					//Display a grid of checkboxes for each face:
					+ SHorizontalBox::Slot() [
						SNew(STileView<FName>)
						  .ListItemsSource(&possibleFaceNamesToMatch)
						  .OnGenerateTile_Lambda([scenePtr, faceNamesLookupPtr](FName dirName, const TSharedRef<STableViewBase>& owner) {
						  	  const auto& [dir, label] = (*faceNamesLookupPtr)[dirName];
						      return SNew(STableRow<TSharedPtr<SWidget>>, owner)
						      	      .Padding(FMargin{ 10.0f, 5.0f })
						      [
							      SNew(SHorizontalBox)
						      	  + SHorizontalBox::Slot() [ SNew(STextBlock).Text(label) ]
							      + SHorizontalBox::Slot() [
								      SNew(SCheckBox)
								        .IsChecked_Lambda([scenePtr, dir]() {
									        return scenePtr->FacesToMatchAgainst.Contains(dir) ?
									        	ECheckBoxState::Checked :
								        		ECheckBoxState::Unchecked;
								        })
										.OnCheckStateChanged_Lambda([scenePtr, dir](ECheckBoxState newState) {
											if (newState == ECheckBoxState::Checked)
												scenePtr->FacesToMatchAgainst.Add(dir);
											else if (newState == ECheckBoxState::Unchecked)
												scenePtr->FacesToMatchAgainst.Remove(dir);
										})
							      ]
							  ];
						  })
						  .Orientation(Orient_Horizontal)
						  .SelectionMode(ESelectionMode::None)
					]
				]
				//TODO: widget for the permutation to use for matching
			];
}

void FWfcTilesetEditor::RefreshTileChoices()
{
    //Update the two "tile selector" arrays.
    tilesetTileSelectorChoices.Empty();
    tilesetTileSelectorChoiceIDs.Empty();
    for (const auto& tileByID : tileset->Tiles)
    {
        auto displayName = FString::FromInt(tileByID.Key) + TEXT(": ") + tileByID.Value.GetDisplayName();
        tilesetTileSelectorChoices.Add(MakeShareable<FString>(new FString(displayName)));
        tilesetTileSelectorChoiceIDs.Add(tileByID.Key);
    }

    //Update the selector widget.
    if (tileSelector.IsValid())
        tileSelector->RefreshOptions();
}
void FWfcTilesetEditor::OnTileSelected(TSharedPtr<FString> name, ESelectInfo::Type)
{
    //For some reason, "null" gets selected sometimes when using the widget.
    if (!name.IsValid())
        return;
    
    //Get which tile this is.
    //Unfortunately, the iterator returned by std::find() doesn't provide the index, so we have to do it manually...
    int foundI;
    for (foundI = 0; foundI < tilesetTileSelectorChoices.Num(); ++foundI)
        if (*tilesetTileSelectorChoices[foundI] == *name)
            break;
    check(foundI < tilesetTileSelectorChoices.Num());
    int tileID = tilesetTileSelectorChoiceIDs[foundI];

    //Update the tile 3D visualization tab.
    tileToVisualize = tileID;    
    tileSceneTabBody->GetViewportClient()->Invalidate();
}

FWfcTilesetEditor::FWfcTilesetEditor()
{

}
void FWfcTilesetEditor::InitWfcTilesetEditorEditor(const EToolkitMode::Type mode,
                                                   const TSharedPtr<IToolkitHost>& initToolkitHost,
                                                   UWfcTileset* newTileset)
{
    if (!tileSceneTabFactory.IsValid())
        tileSceneTabFactory = MakeShareable(new FWfcTilesetEditorSceneViewTab(SharedThis(this)));
    
	SetAsset(newTileset);
	
	//Retrieve the property editor module and assign properties to the DetailsView.
	auto& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	detailsView = propertyEditorModule.CreateDetailView(FDetailsViewArgs(
		false, true, true,
		FDetailsViewArgs::ObjectsUseNameArea,
		false
	));
    detailsView->OnFinishedChangingProperties().AddRaw(this, &FWfcTilesetEditor::OnTilesetEdited);

	//Create our editor's layout.
	auto standaloneDefaultLayout = FTabManager::NewLayout("Standalone_WfcTilesetEditor_Layout_v2")
    ->AddArea(
        FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            // ->Split(
            //     FTabManager::NewStack()
            //         ->SetSizeCoefficient(0.1f)
            //         ->SetHideTabWell(true)
            //         ->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
            // )
            ->Split(
                FTabManager::NewSplitter()
                    ->SetOrientation(Orient_Horizontal)
                    ->Split(
                        FTabManager::NewStack()
                        ->AddTab(WfcTileset_TabID_SceneView, ETabState::OpenedTab)
                    )
                    ->Split(
                        FTabManager::NewSplitter()
                            ->SetOrientation(Orient_Vertical)
                            ->Split(
                                FTabManager::NewStack()
                                ->AddTab(WfcTileset_TabID_EditorSettings, ETabState::OpenedTab)
                            )
                            ->Split(
                                FTabManager::NewStack()
                                ->AddTab(WfcTileset_TabID_Properties, ETabState::OpenedTab)
                            )
                    )
            )
    );

	//Start up the editor.
	InitAssetEditor(
		mode, initToolkitHost, WfcTilesetEditorAppIdentifier,
		standaloneDefaultLayout, true, true,
		tileset
	);
	if (detailsView.IsValid())
		detailsView->SetObject(tileset);
}

FWfcTilesetEditor::~FWfcTilesetEditor()
{

}

void FWfcTilesetEditor::SetAsset(UWfcTileset* asset)
{
	tileset = asset;
    RefreshTileChoices();
}

TSharedRef<SWidget> FWfcTilesetEditor::SpawnSceneView()
{
    auto ref = SAssignNew(tileSceneTabBody, SWfcTilesetTabBody);
    tileSceneTabBody->GetViewportClient()->OnTick.AddRaw(this, &FWfcTilesetEditor::OnSceneTick);
    return ref;
}

FWfcTilesetEditorScene& FWfcTilesetEditor::GetScene() const
{
	check(tileSceneTabBody.IsValid());
	check(tileSceneTabBody->GetScene());
	return *tileSceneTabBody->GetScene();
}

void FWfcTilesetEditor::OnTilesetEdited(const FPropertyChangedEvent& data)
{
    RefreshTileChoices();
    tileSceneTabBody->GetViewportClient()->Invalidate();
}
// ReSharper disable once CppMemberFunctionMayBeConst
void FWfcTilesetEditor::OnSceneTick(float deltaSeconds)
{
	auto* viewportClient = tileSceneTabBody->GetViewportClient().Get();
    const auto& camPos = viewportClient->GetViewLocation();
    tileSceneTabBody->GetScene()->Refresh(tileset, tileToVisualize, camPos, viewportClient);
}


FName FWfcTilesetEditor::GetToolkitFName() const { return ToolkitName; }
FText FWfcTilesetEditor::GetBaseToolkitName() const { return LOCTEXT("AppLabel", "WFC Tileset Editor"); }
FText FWfcTilesetEditor::GetToolkitToolTipText() const { return LOCTEXT("ToolTip", "WFC Tileset Editor"); }
FString FWfcTilesetEditor::GetWorldCentricTabPrefix() const { return LOCTEXT("WorldCentricTabPrefix", "AnimationDatabase ").ToString(); }
FLinearColor FWfcTilesetEditor::GetWorldCentricTabColorScale() const { return FColor::Red; }

#undef LOCTEXT_NAMESPACE
