#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"

#include "WfcFacePrototype.h"
#include "WfcDataReflection.h"
#include "WfcTileData.h"

#include "WfcTile.generated.h"


//A specific face of a specific tile.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcTileFace
{
	GENERATED_BODY()
public:
	
	//The ID of the 'WfcFace' this face takes after.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PrototypeID;

    //Maps the symmetry points of this face to the original prototype.
    //For example, "Rot90Clockwise" means to rotate this tile face's corners 90 degrees clockwise
    //    to get the corresponding corners on the prototype.
    //The rotation is in the space defined by the corner points -- AA, AB, BA, BB --
    //    assuming "BA" is {1, 0} and "AB" is {0, 1}.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	WFC_Transforms2D PrototypeOrientation;
	
    //Transforms a point on this face's corners into its corresponding point in the face prototype,
    //    based on "PrototypeOrientation".
	WFC::Tiled3D::FacePoints GetPrototypeCorner(WFC::Tiled3D::FacePoints thisCorner) const;
	//Transforms a point on this face's edges into its corresponding point in the face prototype,
	//    based on "PrototypeOrientation".
	WFC::Tiled3D::FacePoints GetPrototypeEdge(WFC::Tiled3D::FacePoints thisEdge) const;
};
template<>
struct TStructOpsTypeTraits<FWfcTileFace> : public TStructOpsTypeTraitsBase2<FWfcTileFace>
{
	enum
	{
		WithZeroConstructor = true,
		WithNoDestructor = true
	};
};


//A specific tile.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcTile
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MinX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MinY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MinZ;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MaxX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MaxY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faces")
	FWfcTileFace MaxZ;

	//Higher values make this tile more popular during generation.
    //The value will be casted into uint32.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0, ClampMax=4294967295))
	int WeightU32 = 100;

	//An asset associated with this tile.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWfcTileGameData* Data = nullptr;
	//If not empty, this name overrides the automatically-computed nickname.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString NicknameOverride;
	
	//Specific transformations to support on this tile.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms")
	TArray<FWFC_Transform3D> PrecisePermutations;

	//NOTE: Supporting the more flexible system below requires knowing
	//    what rotation comes from combining any other two rotations.
	//      This is a pain in the ass to implement so for now I'm not.
	/*
	//Base supported permutations of this tile.
	//Each of these can then be transformed by the 'supported transforms'
	//    to build the full set of legal permutations.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms")
	TArray<FWFC_Transform3D> BasePermutations = {
		FWFC_Transform3D{ WFC_Rotations3D::None, false },
		FWFC_Transform3D{ WFC_Rotations3D::None, true }
	};
 
    //Whether to include all rotations of the 'base permutations' for this tile.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms")
    bool SupportAllRotations = false;
    
    //Whether to include all rotations of this tile by 90 degrees around the X axis.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations"))
	bool UseXAxisRotations = false;
	//Whether to include all 180-degree rotations of this tile around the X axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations && !UseXAxisRotations"))
	bool UseXAxisRotation180 = false;
	//Whether to include all rotations of this tile by 90 degrees around the Y axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations"))
	bool UseYAxisRotations = false;
	//Whether to include all 180-degree rotations of this tile around the Y axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations && !UseXAxisRotations"))
	bool UseYAxisRotation180 = false;
	//Whether to include all rotations of this tile by 90 degrees around the Z axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations"))
	bool UseZAxisRotations = true;
	//Whether to include all 180-degree rotations of this tile around the Z axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations && !UseXAxisRotations"))
	bool UseZAxisRotation180 = false;
    //Whether to include all rotations of this tile around a pair of opposite edges.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations"))
	bool UseEdgeRotations = false;
    //Whether to include all rotations of this tile around a pair of opposite corners.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms", meta=(EditConditionHides, EditCondition="!SupportAllRotations"))
	bool UseCornerRotations = false;

	*/

	
    FString GetDisplayName() const
    {
    	if (!NicknameOverride.IsEmpty())
    		return NicknameOverride;
    	else if (IsValid(Data))
    		return Data->GetEditorDescription();
    	else
    		return TEXT("[null]");
    }

    const FWfcTileFace& GetFace(WFC::Tiled3D::Directions3D dir) const
    {
        switch (dir)
        {
            case WFC::Tiled3D::Directions3D::MinX: return MinX;
            case WFC::Tiled3D::Directions3D::MaxX: return MaxX;
            case WFC::Tiled3D::Directions3D::MinY: return MinY;
            case WFC::Tiled3D::Directions3D::MaxY: return MaxY;
            case WFC::Tiled3D::Directions3D::MinZ: return MinZ;
            case WFC::Tiled3D::Directions3D::MaxZ: return MaxZ;
            default: check(false); return MinX;
        }
    }
    FWfcTileFace& GetFace(WFC::Tiled3D::Directions3D dir) { return const_cast<FWfcTileFace&>(const_cast<const FWfcTile*>(this)->GetFace(dir)); }

	//Overwrites the given Set to contain all transforms that can be done on this tile.
    void GetSupportedTransforms(TSet<FWFC_Transform3D>& output) const;
};