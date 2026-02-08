#pragma once

#include "CoreMinimal.h"

#include "WfcTileDebugActor.h"

#include "WfcTileData.generated.h"


//Managed data associated with a tile, for example the static mesh or Actor it represents.
//This data is represented in editor by a corresponding FWfcTileVisualizerBase.
UCLASS(BlueprintType, Blueprintable, Abstract)
class WFCPP2UNREALRUNTIME_API UWfcTileGameData : public UDataAsset
{
    GENERATED_BODY()
public:

    UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent)
    FString GetEditorDescription() const;
};
inline FString UWfcTileGameData::GetEditorDescription_Implementation() const { return GetName(); }


//Associates a WFC tile with a static mesh asset.
UCLASS()
class WFCPP2UNREALRUNTIME_API UWfcTileGameData_StaticMesh : public UWfcTileGameData
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UStaticMesh* Mesh = nullptr;

    virtual FString GetEditorDescription_Implementation() const override { return IsValid(Mesh) ? Mesh->GetName() : FString(TEXT("[null]")); }
};

//Associates a WFC tile with an actor.
//Consider having that actor check whether it's in an editor preview scene before running any logic!
UCLASS()
class WFCPP2UNREALRUNTIME_API UWfcTileGameData_Actor : public UWfcTileGameData
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AActor> ActorType = nullptr;

    //Guaranteed to be non-null, by defaulting to AActor.
    TSubclassOf<AActor> SanitizedActorType() const { return IsValid(ActorType) ? ActorType : TSubclassOf<AActor>{ AActor::StaticClass() }; }

    virtual FString GetEditorDescription_Implementation() const override { return SanitizedActorType()->GetName(); }
};

UCLASS()
class WFCPP2UNREALRUNTIME_API UWfcTileGameData_Debug : public UWfcTileGameData
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MinX;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MinY;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MinZ;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MaxX;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MaxY;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor MaxZ;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FLinearColor Default;
            
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText DebugText = FText::GetEmpty();
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AWfcTileDebugActor> DebugActor;
    
    TSubclassOf<AActor> SanitizedActorType() const { return IsValid(DebugActor) ? DebugActor : TSubclassOf<AActor>{ AActor::StaticClass() }; }
    
    FString GetEditorDescription_Implementation() const override { return SanitizedActorType()->GetName(); }
};
