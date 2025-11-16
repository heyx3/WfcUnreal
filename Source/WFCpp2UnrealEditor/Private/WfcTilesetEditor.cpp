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
#include "IStructureDetailsView.h"
#include "SEnumCombo.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcGenerator.h"
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

	//Lambdas for controlling the UI:
	auto showIfMatchingFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Tile:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Generation:
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Collapsed;
			
			case EWfcTilesetEditorMode::Matches:
				return EVisibility::Visible;
									
			default:
				check(false);
				return EVisibility::Hidden;
		}
	};
	auto showIfNotGeneratingOrOverrideFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Tile:
				return EVisibility::Visible;
			
			case EWfcTilesetEditorMode::Generation:
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Collapsed;

			default:
				check(false);
			return EVisibility::Hidden;
		}
	};
	auto showIfGeneratingNotOverrideFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Tile:
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Collapsed;
			
			case EWfcTilesetEditorMode::Generation:
				return EVisibility::Visible;

			default:
				check(false);
				return EVisibility::Hidden;
		}
	};
	auto showIfGeneratingOrOverrideFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Tile:
				return EVisibility::Collapsed;
			
			case EWfcTilesetEditorMode::Generation:
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Visible;

			default:
				check(false);
				return EVisibility::Hidden;
		}
	};

	auto showIfOverrideGeneratingFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Tile:
			case EWfcTilesetEditorMode::Generation:
				return EVisibility::Collapsed;
			
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Visible;

			default:
				check(false);
			return EVisibility::Hidden;
		}
	};
	auto showIfNotOverrideGeneratingFn = [&]() {
		switch (GetScene().Mode)
		{
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Tile:
			case EWfcTilesetEditorMode::Generation:
				return EVisibility::Visible;
			
			case EWfcTilesetEditorMode::OverrideGeneration:
				return EVisibility::Collapsed;

			default:
				check(false);
			return EVisibility::Hidden;
		}
	};
	auto showIfGeneratingAndNotDoneFn = [&]() {
		auto* generation = GetScene().GetGeneratorManager();
		if (!generation)
			return EVisibility::Collapsed;

		if (!generation->GetGenerator()->IsRunning())
			return EVisibility::Collapsed;

		return EVisibility::Visible;
	};
	auto showIfGeneratingWithHistoryFn = [&](bool keepLayout) { return [&]() {
		auto hideValue = (keepLayout ? EVisibility::Hidden : EVisibility::Collapsed);
		
		auto* generation = GetScene().GetGeneratorManager();
		if (!generation)
			return hideValue;

		if (generation->GetGenerator()->GetHistoryLength() <= 0)
			return hideValue;

		return EVisibility::Visible;
	}; };
	auto faceMatcherToggleWidget = [&](WFC_Directions3D face)
	{
		auto* scene = &GetScene();
		auto label = FText::FromString(UEnum::GetValueAsString(face).RightChop(18));
		return SNew(SHorizontalBox)
	      + SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).AutoWidth() [
		      SNew(STextBlock).Text(label)
	      ]
		  + SHorizontalBox::Slot().MinWidth(5).MaxWidth(5) [ SNullWidget::NullWidget ]
	      + SHorizontalBox::Slot().VAlign(EVerticalAlignment::VAlign_Center).AutoWidth() [
		      SNew(SCheckBox)
		        .IsChecked_Lambda([face, scene]() {
			        return scene->FacesToMatchAgainst.Contains(face) ?
		                ECheckBoxState::Checked :
		                ECheckBoxState::Unchecked;
		        })
		        .OnCheckStateChanged_Lambda([face, scene](ECheckBoxState newState) {
		        	if (newState == ECheckBoxState::Checked)
						scene->FacesToMatchAgainst.Add(face);
			        else if (newState == ECheckBoxState::Unchecked)
			        	scene->FacesToMatchAgainst.Remove(face);
		        })
		  ];
	};

	TArray<int32> viewModeEnumsSansOverrides;
	for (auto e : TEnumRange<EWfcTilesetEditorMode>{ })
		switch (e)
		{
			case EWfcTilesetEditorMode::Tile:
			case EWfcTilesetEditorMode::Permutations:
			case EWfcTilesetEditorMode::Matches:
			case EWfcTilesetEditorMode::Generation:
				viewModeEnumsSansOverrides.Add(static_cast<int32>(e));
			break;
			
			case EWfcTilesetEditorMode::OverrideGeneration:
			break;

			default: check(false);
		}

	//Pointers for lambdas to capture:
	auto* scenePtr = &GetScene();

	//Set up a property editor for the permutation to match tiles against.
	auto& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs matchingPermutationEditorArgs;
	matchingPermutationEditorArgs.bAllowSearch = false;
	matchingPermutationEditorArgs.bHideSelectionTip = true;
	matchingPermutationEditorArgs.bSearchInitialKeyFocus = true;
	matchingPermutationEditorArgs.bShowOptions = true;
	matchingPermutationEditorArgs.bLockable = false;
	matchingPermutationEditorArgs.bUpdatesFromSelection = false;
	matchingPermutationEditorArgs.bShowScrollBar = false;
	matchingPermutationEditorArgs.bShowModifiedPropertiesOption = false;
	FStructureDetailsViewArgs matchingPermutationEditorStructArgs;
	matchingPermutationEditorStructArgs.bShowObjects = true;
	matchingPermutationEditorStructArgs.bShowAssets = true;
	matchingPermutationEditorStructArgs.bShowClasses = true;
	matchingPermutationEditorStructArgs.bShowInterfaces = true;
	editorForPermutationToMatch = propertyEditorModule.CreateStructureDetailView(
		matchingPermutationEditorArgs, matchingPermutationEditorStructArgs, nullptr
	);
	auto* scopedMatchingPermutationStruct = new FStructOnScope(
		FWFC_Transform3D::StaticStruct(),
		reinterpret_cast<uint8_t*>(&GetScene().PermutationToMatchAgainst)
	);
	editorForPermutationToMatch->SetStructureData(MakeShareable(scopedMatchingPermutationStruct));

	//Set up a property editor for the generator settings.
	FDetailsViewArgs generatorSettingsEditorArgs;
	generatorSettingsEditorArgs.bAllowSearch = false;
	generatorSettingsEditorArgs.bHideSelectionTip = true;
	generatorSettingsEditorArgs.bSearchInitialKeyFocus = true;
	generatorSettingsEditorArgs.bShowOptions = true;
	generatorSettingsEditorArgs.bLockable = false;
	generatorSettingsEditorArgs.bUpdatesFromSelection = false;
	generatorSettingsEditorArgs.bShowScrollBar = false;
	generatorSettingsEditorArgs.bShowModifiedPropertiesOption = false;
	FStructureDetailsViewArgs generatorSettingsEditorStructArgs;
	generatorSettingsEditorStructArgs.bShowObjects = true;
	generatorSettingsEditorStructArgs.bShowAssets = true;
	generatorSettingsEditorStructArgs.bShowClasses = true;
	generatorSettingsEditorStructArgs.bShowInterfaces = true;
	editorForGeneratorSettings = propertyEditorModule.CreateStructureDetailView(
		generatorSettingsEditorArgs, generatorSettingsEditorStructArgs, nullptr
	);
	auto* scopedGeneratorSettingsStruct = new FStructOnScope(
		FEditorSceneObject_WfcGeneration_Settings::StaticStruct(),
		reinterpret_cast<uint8_t*>(&GetScene().GenerationSettings)
	);
	editorForGeneratorSettings->SetStructureData(MakeShareable(scopedGeneratorSettingsStruct));

	return SAssignNew(tileSelectorTab, SDockTab)
			.Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
			.Label(LOCTEXT("EditorSettingsTabLabel", "Editor Settings"))
			.TabColorScale(GetTabColorScale())
			[
				SNew(SScrollBox)
				
				//Checkbox: "Try generating from initial state asset"
				+ SScrollBox::Slot()
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([&]() {
							return GetScene().Mode == EWfcTilesetEditorMode::OverrideGeneration ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([&](ECheckBoxState newState) {
							if (newState == ECheckBoxState::Checked)
							{
								lastNonOverriddenMode = GetScene().Mode;
								GetScene().Mode = EWfcTilesetEditorMode::OverrideGeneration;
							}
							else if (newState == ECheckBoxState::Unchecked)
							{
								GetScene().Mode = lastNonOverriddenMode;
							}
						})
					[
						SNew(STextBlock)
							.Text(LOCTEXT("GenerateFromInitialSettingsAsset", "Generate from Initial Settings asset"))
					]
				]

				//Editor to select an Initial State asset for override-generation
				+ SScrollBox::Slot()
				[
					//For some reason the property window isn't displaying the asset,
					//    so we need to add a label that does the job.
					
					SNew(SHorizontalBox)
					  .Visibility_Lambda(showIfOverrideGeneratingFn)
					
					+ SHorizontalBox::Slot()
					  .FillWidth(1.0f)
					[
						SNew(SObjectPropertyEntryBox)
							.OnObjectChanged_Lambda([&](const FAssetData& newAsset) {
								if (newAsset.IsValid())
									initialGeneratorState = CastChecked<UWfcGeneratorInitialState>(newAsset.GetAsset());
								else
									initialGeneratorState = nullptr;
							})
							.AllowedClass(UWfcGeneratorInitialState::StaticClass())
							.DisplayUseSelected(false)
							.DisplayBrowse(true)
							.DisplayThumbnail(false)
							.AllowCreate(false)
							.AllowClear(true)
					]
					
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					[
						SNew(SSpacer)
							.Size(FVector2D{ 25, 1 })
					]
					
					+ SHorizontalBox::Slot()
					  .FillWidth(2.0f)
					[
						SNew(STextBlock)
							.Text_Lambda([&]() {
								if (IsValid(initialGeneratorState))
									return FText::FromString(initialGeneratorState->GetPathName());
								else
									return FText::GetEmpty();
							})
					]
				]

				//Tile selector dropdown
				+ SScrollBox::Slot()
				[
				    SAssignNew(tileSelector, STextComboBox)
						.Visibility_Lambda(showIfNotOverrideGeneratingFn)
				        .OptionsSource(&tilesetTileSelectorChoices)
				        .OnSelectionChanged(this, &FWfcTilesetEditor::OnTileSelected)
				]

				//View mode dropdown
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
						.Visibility_Lambda(showIfNotOverrideGeneratingFn)
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
						  .EnumValueSubset(viewModeEnumsSansOverrides)
						  .CurrentValue_Lambda([&]() {
						      return static_cast<int32>(GetScene().Mode);
						  })
						  .OnEnumSelectionChanged_Lambda([&](int32 v, ESelectInfo::Type) {
						      GetScene().Mode = static_cast<EWfcTilesetEditorMode>(v);
						  })
					]
				]

				//Other display settings
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
								case EWfcTilesetEditorMode::Generation:
								case EWfcTilesetEditorMode::OverrideGeneration:
									return EVisibility::Visible;
								
								default:
									check(false);
									return EVisibility::Hidden;
							}
						})

					//Tile separation:
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

					//Generator shows face constraints:
					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						  .Visibility_Lambda(showIfGeneratingOrOverrideFn)
						  .Size(FVector2D{ 20, 1 })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						  .Visibility_Lambda(showIfGeneratingOrOverrideFn)
						  .Text(LOCTEXT("GenShowFaceConstraintsLabel", "Show face constraints"))
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SCheckBox)
						  .Visibility_Lambda(showIfGeneratingOrOverrideFn)
						  .IsChecked_Lambda([&]() { return GetScene().DisplayFaceConstraintsInGeneration ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						  .OnCheckStateChanged_Lambda([&](ECheckBoxState newState) {
						      GetScene().DisplayFaceConstraintsInGeneration =
						      	(newState == ECheckBoxState::Checked);
						  })
					]
				]

				//Show Faces checkbox
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
						.Visibility_Lambda(showIfNotGeneratingOrOverrideFn)

					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ShowFaceDataLabel", "Show face data"))
					]
					+SHorizontalBox::Slot()
					[
						SNew(SCheckBox)
							.IsChecked_Lambda([&]() { return GetScene().ShowFaceData ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([&](ECheckBoxState newState) {
								GetScene().ShowFaceData = (newState == ECheckBoxState::Checked);
							})
					]
				]

				//Match Face selection grid
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
					+ SHorizontalBox::Slot()
					    .MinWidth(20).MaxWidth(20)
					    [ SNew(SSpacer).Size(FVector2D{ 1, 1 }) ]
					//Display a grid of checkboxes for each face:
					+ SHorizontalBox::Slot() [
						SNew(SGridPanel)
						  + SGridPanel::Slot(0, 0).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MinX) ]
						  + SGridPanel::Slot(1, 0).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MinY) ]
						  + SGridPanel::Slot(2, 0).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MinZ) ]
						  + SGridPanel::Slot(0, 1).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MaxX) ]
						  + SGridPanel::Slot(1, 1).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MaxY) ]
						  + SGridPanel::Slot(2, 1).Padding(7.0f, 3.5f) [ faceMatcherToggleWidget(WFC_Directions3D::MaxZ) ]
					]
				]

				//Match Permutation editor
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
					    .Visibility_Lambda(showIfMatchingFn)
					+ SHorizontalBox::Slot() [ editorForPermutationToMatch->GetWidget()->AsShared() ]
				]
				
				//Generator settings editor
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
						.Visibility_Lambda(showIfGeneratingNotOverrideFn)
					+ SHorizontalBox::Slot() [ editorForGeneratorSettings->GetWidget()->AsShared() ]
				]

				//Generator tick execution/rewind
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
						.Visibility_Lambda(showIfGeneratingOrOverrideFn)
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
							.Text(FText::FromString(TEXT("Ticks")))
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpinBox<int>)
							.Value_Lambda([&]() { return nTicksPerClick; })
						    .OnValueChanged_Lambda([&](int newVal) { nTicksPerClick = newVal; })
						    .MinValue(1)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SButton)
							.Text(FText::FromString(TEXT("Execute")))
							.Visibility_Lambda(showIfGeneratingAndNotDoneFn)
							.OnPressed_Lambda([&]() { GetScene().NGeneratorTicksToRun += nTicksPerClick; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
							.Size(FVector2D{ 15, 1 })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SButton)
						    .Text(FText::FromString(TEXT("Rewind once")))
						    .Visibility_Lambda(showIfGeneratingWithHistoryFn(true))
							.OnPressed_Lambda([&]() { GetScene().NRewindsToRun += 1; })
					]
					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
							.Text_Lambda([&]() {
								auto* generatorManager = GetScene().GetGeneratorManager();
								if (generatorManager && generatorManager->GetGenerator())
									return FText::FromString(FString::Printf(TEXT("(%i)"), generatorManager->GetGenerator()->GetHistoryLength()));
								else
									return FText::FromString(TEXT("!"));
							})
					]
				]
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
	
    tileSceneTabBody->GetScene()->Refresh(
    	tileset, tileToVisualize,
    	camPos,
    	initialGeneratorState,
    	viewportClient
    );
}


FName FWfcTilesetEditor::GetToolkitFName() const { return ToolkitName; }
FText FWfcTilesetEditor::GetBaseToolkitName() const { return LOCTEXT("AppLabel", "WFC Tileset Editor"); }
FText FWfcTilesetEditor::GetToolkitToolTipText() const { return LOCTEXT("ToolTip", "WFC Tileset Editor"); }
FString FWfcTilesetEditor::GetWorldCentricTabPrefix() const { return LOCTEXT("WorldCentricTabPrefix", "AnimationDatabase ").ToString(); }
FLinearColor FWfcTilesetEditor::GetWorldCentricTabColorScale() const { return FColor::Red; }

#undef LOCTEXT_NAMESPACE
