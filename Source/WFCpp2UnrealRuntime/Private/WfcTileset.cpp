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

void UWfcTileset::Unwrap(Unwrapped& output) const
{
	output.Tiles.clear();
	output.WfcTileIDs.Empty();
	output.WfcTileIDByUnrealID.Empty();
	output.WfcFacePrototypeFirstIDs.Empty();
	output._supportedTransforms.Empty();
	output._sortedUnrealIDs.Empty();
	
	//Assign unique point ID's based on the face prototypes.
	//Edges and corners do not interchange, so they can reuse the same ID values.
    WFC::Tiled3D::PointID nextPointID = 1;
    for (const auto& facePrototype : FacePrototypes)
    {
        output.WfcFacePrototypeFirstIDs.Add(facePrototype.Get<0>(), nextPointID);
        nextPointID += 4;
    }
    //In case a nonexistent face prototype is referenced, keep a special hidden null face around.
    auto nullFaceFirstID = nextPointID;

    //Convert each serialized Unreal tile into a WFC library tile.
	Tiles.GetKeys(output._sortedUnrealIDs);
	for (auto tileID : output._sortedUnrealIDs)
    {
    	auto& tileData = Tiles[tileID];
        output.WfcTileIDs.Add(tileID);
    	output.WfcTileIDByUnrealID.Add(tileID, output.WfcTileIDs.Num() - 1);
    	
        auto& wfcTile = output.Tiles.emplace_back();
        wfcTile.Weight = static_cast<uint32_t>(tileData.WeightU32); wfcTile.Permutations.Clear(); //TODO: Move to the next line

        //Convert the Unreal-serialized permutation set into a WFC library permutation set.
        output._supportedTransforms.Empty();
        tileData.GetSupportedTransforms(output._supportedTransforms);
        for (auto transform : output._supportedTransforms)
            wfcTile.Permutations.Add(transform.Unwrap());

        //Convert the Unreal-serialized face data into WFC library face data.
        for (int faceI = 0; faceI < WFC::Tiled3D::N_DIRECTIONS_3D; ++faceI)
        {
            auto dir = static_cast<WFC::Tiled3D::Directions3D>(faceI);
            wfcTile.Data.Faces[faceI].Side = dir;

        	//If this face is right-handed, swap the tile face's PrototypeOrientation to compensate.
            auto assetFace = tileData.GetFace(dir);
        	if (!WFC::Tiled3D::IsFaceLeftHanded(dir))
        		assetFace.PrototypeOrientation = static_cast<WFC_Transforms2D>(WFC::Invert(static_cast<WFC::Transformations>(assetFace.PrototypeOrientation)));
        	
            //Generate the point ID's for this face.
            const auto* prototype = FacePrototypes.Find(assetFace.PrototypeID);
            auto* tryFirstFaceID = output.WfcFacePrototypeFirstIDs.Find(assetFace.PrototypeID);
            auto firstFaceID = tryFirstFaceID ? *tryFirstFaceID : nullFaceFirstID;
            for (int pointI = 0; pointI < WFC::Tiled3D::N_FACE_POINTS; ++pointI)
            {
                auto localPoint = static_cast<WFC::Tiled3D::FacePoints>(pointI);
                auto prototypeCorner = assetFace.GetPrototypeCorner(localPoint),
            		 prototypeEdge = assetFace.GetPrototypeEdge(localPoint);
                auto cornerID = prototype ? prototype->Corners.PointAt(prototypeCorner) : static_cast<EWfcPointID>(nullFaceFirstID),
            		 edgeID = prototype ? prototype->Edges.PointAt(prototypeEdge) : static_cast<EWfcPointID>(nullFaceFirstID);

                //Convert the 0-3 symmetry value stored in the asset,
                //    into a unique index across all tile faces.
                auto cornerUniqueID = firstFaceID + static_cast<WFC::Tiled3D::PointID>(cornerID),
            		 edgeUniqueID = firstFaceID + static_cast<WFC::Tiled3D::PointID>(edgeID);
                wfcTile.Data.Faces[faceI].Points.Corners[pointI] = cornerUniqueID;
            	wfcTile.Data.Faces[faceI].Points.Edges[pointI] = edgeUniqueID;
            }
        }
    }
}
