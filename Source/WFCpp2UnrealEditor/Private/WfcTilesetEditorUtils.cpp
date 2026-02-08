#include "WfcTilesetEditorUtils.h"

#include "DataprepCoreUtils.h"


//Learned how to do the below from SDataprepEditorViewport.
TWeakObjectPtr<AActor> WfcTilesetEditorUtils::CreatePreviewSceneActor(UWorld* world,
                                                                      TSubclassOf<AActor> type,
                                                                      USceneComponent* attachParent)
{
    if (!IsValid(world))
        return nullptr;
    
    UClass* spawnClass = IsValid(type.Get()) ? type.Get() : AActor::StaticClass();
    auto* actor = world->SpawnActor<AActor>(spawnClass);
    if (!IsValid(actor))
        return nullptr;
    actor->SetFlags(RF_Transient);
    if (actor->GetRootComponent() == nullptr)
    {
        USceneComponent* RootComponent = NewObject<USceneComponent>(
            actor, USceneComponent::StaticClass(),
            TEXT("RootOfCustomActor"), RF_NoFlags
        );
        actor->SetRootComponent(RootComponent);
    }

    if (attachParent)
        actor->GetRootComponent()->AttachToComponent(attachParent, FAttachmentTransformRules::KeepRelativeTransform);
    actor->RegisterAllComponents();

    return actor;
}
void WfcTilesetEditorUtils::DestroyPreviewSceneActor(AActor* actor, EDetachmentRule rule)
{
    auto* world = actor->GetWorld();
    if (!IsValid(world))
        return;

    FDetachmentTransformRules detachment{ rule, true };

    TArray<UActorComponent*> toDelete;
    actor->GetComponents(toDelete);

    actor->UnregisterAllComponents();
    FDataprepCoreUtils::PurgeObjects(TArray<UObject*>{ toDelete });
    actor->RegisterAllComponents();

    //TODO: Call PurgeObjects again for the actor itself?
}
