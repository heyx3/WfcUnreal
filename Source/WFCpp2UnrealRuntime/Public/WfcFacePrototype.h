#pragma once

#include "WfcDataReflection.h"

#include "WfcFacePrototype.generated.h"


//ID's for points on a tile face, to identify symmetries and match with other faces.
UENUM(BlueprintType)
enum class PointSymmetry : uint8
{
	a = 0,
	b = 1,
	c = 2,
	d = 3
};


//A specific face that tiles can have.
//Defines symmetries and the ability to match with other copies of this face.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcFacePrototype
{
	GENERATED_BODY()
public:

	static const TCHAR* NameOfNull() { return TEXT("NULL"); }

	//A display name for human readability.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Nickname = "New Face";

	//To make things more user-friendly than the underlying WFC++ data structures,
	//    we make more opinionated choices.
	//The first (and default) corner/edge identifier represents Null/unimportant.
	//These will not be explicitly drawn in the tileset editor,
	//      and make a good signifier for 'empty space'.

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString NameOfB = TEXT("b");
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString NameOfC = TEXT("c");
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString NameOfD = TEXT("D");
	

	//The identifier for the corner on the 'min' side of both face axes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry CornerAA = PointSymmetry::a;
	//The identifier for the corner on the 'min' side of the first axis (X or Y),
	//    and the 'max' side of the second axis (Y or Z).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry CornerAB = PointSymmetry::a;
	//The identifier for the corner on the 'max' side of the first axis (X or Y),
	//    and the 'min' side of the second axis (Y or Z).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry CornerBA = PointSymmetry::a;
	//The identifier for the corner on the 'max' side of both face axes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry CornerBB = PointSymmetry::a;
	
	//The identifier for the 'min' edge parallel to the first face axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry EdgeAA = PointSymmetry::a;
	//The identifier for the 'max' edge parallel to the first face axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry EdgeAB = PointSymmetry::a;
	//The identifier for the 'min' edge parallel to the second face axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry EdgeBA = PointSymmetry::a;
	//The identifier for the 'max' edge parallel to the second face axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	PointSymmetry EdgeBB = PointSymmetry::a;

	PointSymmetry& GetCornerSymmetry(WFC::Tiled3D::FacePoints corner)
	{
		switch (corner)
		{
			case WFC::Tiled3D::FacePoints::AA: return CornerAA;
			case WFC::Tiled3D::FacePoints::AB: return CornerAB;
			case WFC::Tiled3D::FacePoints::BA: return CornerBA;
			case WFC::Tiled3D::FacePoints::BB: return CornerBB;
			default: check(false); return CornerAA;
		}
	}
	PointSymmetry GetCornerSymmetry(WFC::Tiled3D::FacePoints corner) const { return const_cast<FWfcFacePrototype*>(this)->GetCornerSymmetry(corner); }
	PointSymmetry& GetEdgeSymmetry(WFC::Tiled3D::FacePoints edge)
	{
		switch (edge)
		{
			case WFC::Tiled3D::FacePoints::AA: return EdgeAA;
			case WFC::Tiled3D::FacePoints::AB: return EdgeAB;
			case WFC::Tiled3D::FacePoints::BA: return EdgeBA;
			case WFC::Tiled3D::FacePoints::BB: return EdgeBB;
			default: check(false); return EdgeAA;
		}
	}
	PointSymmetry GetEdgeSymmetry(WFC::Tiled3D::FacePoints edge) const { return const_cast<FWfcFacePrototype*>(this)->GetEdgeSymmetry(edge); }

	WFC::Tiled3D::FaceIdentifiers Unwrap(int pointIdOffset) const;
	//Returns nullptr for point A (because it represents a null/invisible identifier).
	const FString* GetName(PointSymmetry point) const;
};

//Face prototypes have a unique ID in a tile-set.
using WfcFacePrototypeID = int32;
constexpr WfcFacePrototypeID INVALID_FACE_ID = 0,
			                 FIRST_VALID_FACE_ID = 1;