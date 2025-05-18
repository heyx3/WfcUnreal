#pragma once

#include "WFCpp2.h"

#include "WfcDataReflection.generated.h"


//Unfortunately, for structs and enums to show up properly in Unreal,
//    they need the reflection macros attached to them.
//So I have to redefine some WFC stuff here, using Unreal's reflection macros.

UENUM(Category=WFC, BlueprintType)
enum class WFC_Directions3D : uint8
{
	MinX = 0, // WFC::Tiled3D::Directions3D::MinX
	MaxX = WFC::Tiled3D::Directions3D::MaxX,
	MinY = WFC::Tiled3D::Directions3D::MinY,
	MaxY = WFC::Tiled3D::Directions3D::MaxY,
	MinZ = WFC::Tiled3D::Directions3D::MinZ,
	MaxZ = WFC::Tiled3D::Directions3D::MaxZ
};
static_assert(static_cast<int>(WFC_Directions3D::MinX) == WFC::Tiled3D::Directions3D::MinX,
			  "Needed to satisfy the compiler");

UENUM(Category=WFC, BlueprintType)
enum class WFC_Transforms2D : uint8
{
	None = 0, //WFC::Transformations::None

	Rotate90CW = WFC::Transformations::Rotate90CW,
	Rotate180 = WFC::Transformations::Rotate180,
	Rotate270CW = WFC::Transformations::Rotate270CW,

	FlipX = WFC::Transformations::FlipX,
	FlipY = WFC::Transformations::FlipY,

	//Mirror along the primary diagonal, going from the min corner to the max corner.
	FlipDiag1 = WFC::Transformations::FlipDiag1,
	//Mirror along the secondary diagonal.
	FlipDiag2 = WFC::Transformations::FlipDiag2
};
static_assert(static_cast<int>(WFC_Transforms2D::None) == WFC::Transformations::None,
			  "Needed to satisfy the compiler");


UENUM(Category=WFC, BlueprintType)
enum class WFC_Rotations3D : uint8
{
	//0- or 360-degree rotation.
	None = 0, //WFC::Tiled3D::Rotations3D::None

	//Rotation along an axis, clockwise when looking at the positive face.
	AxisX_90 = WFC::Tiled3D::Rotations3D::AxisX_90,
	AxisX_180 = WFC::Tiled3D::Rotations3D::AxisX_180,
	AxisX_270 = WFC::Tiled3D::Rotations3D::AxisX_270,
	AxisY_90 = WFC::Tiled3D::Rotations3D::AxisY_90,
	AxisY_180 = WFC::Tiled3D::Rotations3D::AxisY_180,
	AxisY_270 = WFC::Tiled3D::Rotations3D::AxisY_270,
	AxisZ_90 = WFC::Tiled3D::Rotations3D::AxisZ_90,
	AxisZ_180 = WFC::Tiled3D::Rotations3D::AxisZ_180,
	AxisZ_270 = WFC::Tiled3D::Rotations3D::AxisZ_270,

	//Rotation by grabbing two opposite edges and rotating 180 degrees.
	//Notated with the face that the edges are parallel to, and "a" or "b"
	//    for "major diagonal" (i.e. one of the edges is an axis)
	//    or "minor diagonal" respectively.
	EdgesXa = WFC::Tiled3D::Rotations3D::EdgesXa,
	EdgesXb = WFC::Tiled3D::Rotations3D::EdgesXb,
	EdgesYa = WFC::Tiled3D::Rotations3D::EdgesYa,
	EdgesYb = WFC::Tiled3D::Rotations3D::EdgesYb,
	EdgesZa = WFC::Tiled3D::Rotations3D::EdgesZa,
	EdgesZb = WFC::Tiled3D::Rotations3D::EdgesZb,

	//Rotation by grabbing opposite corners and rotating 120 or 240 degrees.
	//Notated with one corner, and rotation amount
	//    (clockwise while staring at the notated corner).
	//The corner is notated as "CornerXYZ", where X, Y, and Z are either
	//    "A" for the min side, or "B" for the max side.
	CornerAAA_120 = WFC::Tiled3D::Rotations3D::CornerAAA_120,
	CornerAAA_240 = WFC::Tiled3D::Rotations3D::CornerAAA_240,
	CornerABA_120 = WFC::Tiled3D::Rotations3D::CornerABA_120,
	CornerABA_240 = WFC::Tiled3D::Rotations3D::CornerABA_240,
	CornerBAA_120 = WFC::Tiled3D::Rotations3D::CornerBAA_120,
	CornerBAA_240 = WFC::Tiled3D::Rotations3D::CornerBAA_240,
	CornerBBA_120 = WFC::Tiled3D::Rotations3D::CornerBBA_120,
	CornerBBA_240 = WFC::Tiled3D::Rotations3D::CornerBBA_240
};

//A unique identifier for a specific cube face, using identifiers organized in world space.
//
//For consistency the axes are always ordered X->Y->Z (e.g. on the Z face, axis 1 is X and axis 2 is Y).
//
//This face's symmetries are implicit in the use of duplicate identifiers;
//    for example if all corners and faces have the same ID
//    then all permutations of this face are equivalent.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWFC_Face
{
	GENERATED_BODY()
public:

	//The corner on the 'min' side of both face axes.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int CornerAA = 0;
	//The corner on the 'min' side of the first face axis (X or Y),
	//    and the 'max' side of the second face axis (Y or Z).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0))
	int CornerAB = 0;
	//The corner on the 'max' side of the first face axis (X or Y),
	//    and the 'min' side of the second face axis (Y or Z).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0))
	int CornerBA = 0;
	//The corner on the 'max' side of both face axes.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int CornerBB = 0;

	//The 'min' edge parallel to the first face axis (X or Y).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int EdgeAA = 0;
	//The 'max' edge parallel to the first face axis (X or Y).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int EdgeAB = 0;
	//The 'min' edge parallel to the second face axis (Y or Z).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int EdgeBA = 0;
	//The 'max' edge parallel to the second face axis (Y or Z).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0))
	int EdgeBB = 0;

	WFC::Tiled3D::FaceIdentifiers Unwrap() const
	{
		WFC::Tiled3D::FaceIdentifiers fi;
		fi.Corners[WFC::Tiled3D::AA] = CornerAA;
		fi.Corners[WFC::Tiled3D::AB] = CornerAB;
		fi.Corners[WFC::Tiled3D::BA] = CornerBA;
		fi.Corners[WFC::Tiled3D::BB] = CornerBB;
		fi.Edges[WFC::Tiled3D::AA] = EdgeAA;
		fi.Edges[WFC::Tiled3D::AB] = EdgeAB;
		fi.Edges[WFC::Tiled3D::BA] = EdgeBA;
		fi.Edges[WFC::Tiled3D::BB] = EdgeBB;
		return fi;
	}
	auto GetFields() const { return MakeTuple(CornerAA, CornerAB, CornerBA, CornerBB,
											  EdgeAA, EdgeAB, EdgeBA, EdgeBB); }
	bool operator==(const FWFC_Face& f) const
	{
		return GetFields() == f.GetFields();
	}
};
inline uint32 GetTypeHash(const FWFC_Face& face) { return GetTypeHash(face.GetFields()); }
template<>
struct TStructOpsTypeTraits<FWFC_Face> : public TStructOpsTypeTraitsBase2<FWFC_Face>
{
	enum
	{
		WithZeroConstructor = true,
		WithNoDestructor = true,
		WithIdenticalViaEquality = true
	};
};


//Symmetry/face-matching data for all 6 faces of a tile.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWFC_Cube
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MinX = { 1, 2, 3, 4 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MaxX = { 5, 6, 7, 8 };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MinY = { 9, 10, 11, 12 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MaxY = { 13, 14, 15, 16 };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MinZ = { 17, 18, 19, 20 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWFC_Face MaxZ = { 21, 22, 23, 24 };

    
	bool operator==(const FWFC_Cube& c) const
	{
		return MinX == c.MinX && MinY == c.MinY && MinZ == c.MinZ &&
			   MaxX == c.MaxX && MaxY == c.MaxY && MaxZ == c.MaxZ;
	}
};
template<>
struct TStructOpsTypeTraits<FWFC_Cube> : public TStructOpsTypeTraitsBase2<FWFC_Cube>
{
	enum
	{
		WithZeroConstructor = TStructOpsTypeTraits<FWFC_Face>::WithZeroConstructor,
		WithNoDestructor = TStructOpsTypeTraits<FWFC_Face>::WithNoDestructor,
		WithIdenticalViaEquality = true
	};
};
inline uint32 GetTypeHash(const FWFC_Cube& cube) { return GetTypeHash(MakeTuple(cube.MinX, cube.MinY, cube.MinZ, cube.MaxX, cube.MaxY, cube.MaxZ)); }


//A transformation that can be done to a cube (while keeping it axis-aligned).
//Defines hashing and equality.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWFC_Transform3D
{
	GENERATED_BODY()
public:
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
	WFC_Rotations3D Rot = WFC_Rotations3D::None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Invert = false;

	bool operator==(const FWFC_Transform3D& t) const
	{
		return Rot == t.Rot && Invert == t.Invert;
	}

    FWFC_Transform3D() = default;
    FWFC_Transform3D(WFC_Rotations3D rot, bool inverted) :  Rot(rot), Invert(inverted) { }
    FWFC_Transform3D(WFC_Rotations3D rot) : FWFC_Transform3D(rot, false) { }
	FWFC_Transform3D(const WFC::Tiled3D::Transform3D& tr) : Rot(static_cast<WFC_Rotations3D>(tr.Rot)), Invert(tr.Invert) { }

    WFC::Tiled3D::Transform3D Unwrap() const { return { Invert, static_cast<WFC::Tiled3D::Rotations3D>(Rot) }; }
    FTransform ToFTransform() const;
	
	FString ToString() const
	{
		return FString::Printf(
			TEXT("%sRot%s"),
			Invert ? TEXT("Invert->") : TEXT(""),
			*UEnum::GetValueAsString(Rot).RightChop(13) //Remove the 'Rotations3D::'
		);
	}
};
inline uint32 GetTypeHash(const FWFC_Transform3D& t)
{
	return GetTypeHash(MakeTuple(t.Rot, t.Invert));
}
template<>
struct TStructOpsTypeTraits<FWFC_Transform3D> : public TStructOpsTypeTraitsBase2<FWFC_Transform3D>
{
	enum
	{
		WithZeroConstructor = true,
		WithNoDestructor = true,
		WithIdenticalViaEquality = true
	};
};

//An intuitive set of tile permutations that come from taking a set of initial permutations
//    and applying groups of transforms to them.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcImplicitTransformSet
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<FWFC_Transform3D> InitialPermutations = { { } };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool AllowInversions = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool AllowAllRotations = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations"))
	bool AllowAxisRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations"))
	bool AllowCornerRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations"))
	bool AllowEdgeRotations = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowAxisRotations"))
	bool AllowAxisXRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowAxisRotations"))
	bool AllowAxisYRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowAxisRotations"))
	bool AllowAxisZRotations = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowEdgeRotations"))
	bool AllowEdgeXRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowEdgeRotations"))
	bool AllowEdgeYRotations = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditConditionHides, EditCondition="!AllowAllRotations && !AllowEdgeRotations"))
	bool AllowEdgeZRotations = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<FWFC_Transform3D> SpecificAllowedTransforms = { };
	

	WFC::Tiled3D::ImplicitTransformSet Unwrap() const
	{
		WFC::Tiled3D::ImplicitTransformSet set;
		
		set.InitialTransforms.Clear();
		for (const auto& tr : InitialPermutations)
			set.InitialTransforms.Add(tr.Unwrap());

		set.AllowInversion = AllowInversions;
		set.AllowAllRotations =  AllowAllRotations;

		set.AllowAxisRots = AllowAxisRotations;
		set.AllowCornerRots = AllowCornerRotations;
		set.AllowEdgeRots = AllowEdgeRotations;

		set.AllowAxisXRots = AllowAxisXRotations;
		set.AllowAxisYRots = AllowAxisYRotations;
		set.AllowAxisZRots = AllowAxisZRotations;

		set.AllowEdgeXRots = AllowEdgeXRotations;
		set.AllowEdgeYRots = AllowEdgeYRotations;
		set.AllowEdgeZRots = AllowEdgeZRotations;

		for (const auto& tr : SpecificAllowedTransforms)
			set.SpecificAllowedTransforms.Add(tr.Unwrap());

		return set;
	}

	bool operator==(const FWfcImplicitTransformSet& s2) const
	{
		return Unwrap().GetExplicit() == s2.Unwrap().GetExplicit();
	}
};
inline uint32_t GetTypeHash(const FWfcImplicitTransformSet& set)
{
	return static_cast<uint32_t>(set.Unwrap().GetExplicit().Bits());
}
template<>
struct TStructOpsTypeTraits<FWfcImplicitTransformSet> : public TStructOpsTypeTraitsBase2<FWfcImplicitTransformSet>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};