#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "WfcTile.h"

#include "WfcTileset.generated.h"


using WfcTileID = int32;


UCLASS(BlueprintType)
class WFCPP2UNREALRUNTIME_API UWfcTileset : public UObject
{
	GENERATED_BODY()
public:

	//The faces that tiles can have.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Nickname))
	TMap<int32, FWfcFacePrototype> FacePrototypes;

	//All tiles in this set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Data))
	TMap<int32, FWfcTile> Tiles;

    //The width/height/depth of each tile.
    //Used when visualizing the tile in the editor, but you can also use it when placing tiles in the world.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TileLength = 1000.0f;


	//Grabs the ID of the first tile containing the given data reference.
	UFUNCTION(BlueprintCallable)
	int GetTileIDForData(UWfcTileGameData* targetData, bool& foundTile) const;
	//Grabs the ID of the first tile containing the given data reference.
	TOptional<int> GetTileIDForData(UWfcTileGameData* targetData) const;

	//Grabs the ID of the face prototype containing the given nickname.
	UFUNCTION(BlueprintCallable)
	int GetFacePrototype(const FString& nickname, bool& foundFace) const;
	TOptional<int> GetFacePrototype(const FString& nickname) const;

	
	//Provide callbacks to the editor for when this asset changes.
	#if WITH_EDITOR
	
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTilesetEdited, UWfcTileset*, FPropertyChangedEvent&);
    FOnTilesetEdited OnEdited;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& eventData) override
    {
        OnEdited.Broadcast(this, eventData);
    }
    
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTilesetChainEdited, UWfcTileset*, FPropertyChangedChainEvent&);
    FOnTilesetChainEdited OnChainEdited;
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& eventData) override
    {
        OnChainEdited.Broadcast(this, eventData);
    }
	
	#endif
};