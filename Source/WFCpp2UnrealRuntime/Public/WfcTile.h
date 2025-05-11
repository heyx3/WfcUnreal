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

	bool operator==(const FWfcTileFace& f2) const { return PrototypeID == f2.PrototypeID && PrototypeOrientation == f2.PrototypeOrientation; }
};
template<>
struct TStructOpsTypeTraits<FWfcTileFace> : public TStructOpsTypeTraitsBase2<FWfcTileFace>
{
	enum
	{
		WithZeroConstructor = true,
		WithNoDestructor = true,
		WithIdenticalViaEquality = true
	};
};
inline uint32 GetTypeHash(const FWfcTileFace& f) { return GetTypeHash(MakeTuple(f.PrototypeID, f.PrototypeOrientation)); }


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

	//A useful but less precise way of listing supported permutations on this tile.
	//Combines with 'PrecisePermutations'.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms")
	FWfcImplicitTransformSet ImplicitPermutations;
	//Specific transformations to support on this tile,
	//    along the 'ImplicitPermutations'.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transforms")
	TArray<FWFC_Transform3D> PrecisePermutations;

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

	//Creates a POD tuple of this struct's trivially-copyable fields, for hashing and equality.
	auto GetPODFields() const { return MakeTuple(
		MinX, MaxX, MinY, MaxY, MinZ, MaxZ, WeightU32, Data, ImplicitPermutations.Unwrap().GetExplicit().Bits()
	); }
	//Creates a tuple of pointers to this struct's non-trivially-copyable fields, for hashing and equality.
	auto GetSpecialFields() const { return MakeTuple(
		&PrecisePermutations
	); }
	bool operator==(const FWfcTile& t2) const
	{
		return GetPODFields() == t2.GetPODFields() &&
			   PrecisePermutations == t2.PrecisePermutations;
	}
};
template<>
struct TStructOpsTypeTraits<FWfcTile> : public TStructOpsTypeTraitsBase2<FWfcTile>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};

inline uint32 GetTypeHash(const FWfcTile& t)
{
	auto u = GetTypeHash(t.GetPODFields());
	auto f = t.GetSpecialFields();
	
	const TArray<FWFC_Transform3D>& precisePermutations = *f.Get<0>();
	u = GetTypeHash(MakeTuple(u, precisePermutations.Num()));
	for (auto p : precisePermutations)
		u = GetTypeHash(MakeTuple(u, p));

	static_assert(TTupleArity<decltype(f)>::Value == 1,
				  "Some 'special' fields were added or removed!");

	return u;
}