#include "AssetTypeActions_WfcTileset.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcTileset.h"

#define LOCTEXT_NAMESPACE "FAssetTypeActions_WfcTileset"


FText FAssetTypeActions_WfcTileset::GetName() const { return NSLOCTEXT("AssetTypeActions_WfcTileset", "AssetTypeActions_WfcTileset", "WaveFunctionCollapse Tileset"); }
FColor FAssetTypeActions_WfcTileset::GetTypeColor() const { return FLinearColor{ 0.8f, 0.3f, 1.0f }.ToFColorSRGB(); }
UClass* FAssetTypeActions_WfcTileset::GetSupportedClass() const { return UWfcTileset::StaticClass(); }
uint32 FAssetTypeActions_WfcTileset::GetCategories() { return EAssetTypeCategories::Misc; }

void FAssetTypeActions_WfcTileset::OpenAssetEditor(const TArray<UObject*>& objects,
                                                   TSharedPtr<IToolkitHost> editWithinLevelEditor)
{
    auto mode = editWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

    //Create one editor instance for each selected tileset asset.
    for (auto it = objects.CreateConstIterator(); it; ++it)
    {
        auto tileset = Cast<UWfcTileset>(*it);
        if (IsValid(tileset))
        {
            auto* thisModule = &FModuleManager::LoadModuleChecked<IWFCpp2UnrealEditorModule>("WFCpp2UnrealEditor");
            thisModule->CreateCustomAssetEditor(mode, editWithinLevelEditor, tileset);
        }
    }
}


#undef LOCTEXT_NAMESPACE
