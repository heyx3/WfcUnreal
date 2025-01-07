#include "WfcTileset.h"


int UWfcTileset::GetTileIDForData(UWfcTileGameData* targetData, bool& foundTile) const
{
    for (const auto& kvp : Tiles)
    {
        if (kvp.Value.Data == targetData)
        {
            foundTile = true;
            return kvp.Key;
        }
    }

    foundTile = false;
    return 0; //Technically this value is UB anyway, but in practice I've been using 0 as the null ID
              //    so this is a good default value.
}
TOptional<int> UWfcTileset::GetTileIDForData(UWfcTileGameData* targetData) const
{
    bool found;
    int id = GetTileIDForData(targetData, found);
		
    if (found)
        return id;
    else
        return NullOpt;
}

int UWfcTileset::GetFacePrototype(const FString& nickname, bool& foundFace) const
{
    for (const auto& kvp : FacePrototypes)
        if (kvp.Value.Nickname == nickname)
        {
            foundFace = true;
            return kvp.Key;
        }

    foundFace = false;
    return 0; //Technically this value is UB anyway, but in practice I've been using 0 as the null ID
              //    so this is a good default value.
}
TOptional<int> UWfcTileset::GetFacePrototype(const FString& nickname) const
{
    bool found;
    int id = GetFacePrototype(nickname, found);

    if (found)
        return id;
    else
        return NullOpt;
}
