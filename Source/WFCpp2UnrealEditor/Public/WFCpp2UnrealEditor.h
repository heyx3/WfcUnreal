#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"

#include <numeric>


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

//Utility Functions:
namespace WfcppUnrealEditor
{
    template<typename... Trs>
    inline auto ComposeTransforms(const Trs&... transforms)
    {
        //Fold expressions are far easier with operators than function calls,
        //    so I looked into UKismetMathLibrary::ComposeTransform()'s source
        //    and saw that transform multiplication order is 'A * B'.
        return (... * transforms);
    }

    template<typename F>
    inline UE::Math::TTransform<F> WithoutScale(const UE::Math::TTransform<F>& withScale) { return { withScale.GetRotation(), withScale.GetLocation() }; }
}