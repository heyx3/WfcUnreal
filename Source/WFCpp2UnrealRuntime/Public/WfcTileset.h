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

	struct Unwrapped
	{
		std::vector<WFC::Tiled3D::Tile> Tiles;
		TArray<WfcTileID> WfcTileIDs;
		TMap<WfcTileID, WFC::Tiled3D::TileIdx> WfcTileIDByUnrealID;

		//Each face prototype is given four unique point ID's (re-used by corners and edges),
		//    even if it doesn't use all four.
		TMap<WfcFacePrototypeID, WFC::Tiled3D::PointID> WfcFacePrototypeFirstIDs;

		//Internal buffer; not part of the unwrapped data.
		TSet<FWFC_Transform3D> _supportedTransforms;
		//Internal buffer; not part of the unwrapped data.
		TSet<int32> _sortedUnrealIDs;
	};
	//Converts this tileset into a plain WFC library tileset.
	//Guaranteed to produce the same thing every time it's called (same tile/point ID's).
	Unwrapped Unwrap() const { Unwrapped u; Unwrap(u); return u; }
	//Converts this tileset into a plain WFC library tileset.
	//Guaranteed to produce the same thing every time it's called (same tile/point ID's).
	void Unwrap(Unwrapped& output) const;

	
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