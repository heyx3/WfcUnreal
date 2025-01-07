#pragma once

#include "CoreMinimal.h"


namespace WfcTilesetEditorUtils
{
    extern WFCPP2UNREALEDITOR_API TWeakObjectPtr<AActor> CreatePreviewSceneActor(
        UWorld*,
        TSubclassOf<AActor> type = AActor::StaticClass(),
        USceneComponent* attachParent = nullptr
    );
    extern WFCPP2UNREALEDITOR_API void DestroyPreviewSceneActor(
        AActor*,
        EDetachmentRule rule = EDetachmentRule::KeepWorld
    );
}