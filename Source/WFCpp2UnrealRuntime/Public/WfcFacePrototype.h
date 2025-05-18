#pragma once

#include "WfcDataReflection.h"

#include "WfcFacePrototype.generated.h"


//To be more user-friendly than the core WFC++ library,
//    we make opinionated choices on how corner/edge ID's are configured.
//
//Each face prototype is given up to four ID's to assign to its corners and edges.
//The first, default ID is an implicit 'null' which is not visualized,
//    making it a good candidate for "empty space".
//
//The other three identifiers are sequentially opt-in, added by the designer as needed.


UENUM(BlueprintType)
enum class EWfcPointID : uint8
{
	null = 0,
	p1 = 1,
	p2 = 2,
	p3 = 3
};
ENUM_RANGE_BY_COUNT(EWfcPointID, 4);

//Definitions for a set of corners or edges on one tile face.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcFacePointDefs
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(InlineEditConditionToggle))
	bool AddPoint1 = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="AddPoint1"))
	FString NameOfPoint1 = TEXT("p1");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="AddPoint1", EditConditionHides=true, InlineEditConditionToggle))
	bool AddPoint2 = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="AddPoint2"))
	FString NameOfPoint2 = TEXT("p2");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="AddPoint2", EditConditionHides=true, InlineEditConditionToggle))
	bool AddPoint3 = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="AddPoint3"))
	FString NameOfPoint3 = TEXT("p3");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EWfcPointID PointAA = EWfcPointID::null;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EWfcPointID PointAB = EWfcPointID::null;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EWfcPointID PointBA = EWfcPointID::null;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EWfcPointID PointBB = EWfcPointID::null;

	
	//Gets all defined points that are enabled for this face, not including the default 'null' point.
	TArray<FString, TInlineAllocator<3>> GatherPoints() const;

	//Gets the point ID at the given location.
	EWfcPointID& PointAt(WFC::Tiled3D::FacePoints point);
	EWfcPointID PointAt(WFC::Tiled3D::FacePoints point) const { return const_cast<FWfcFacePointDefs*>(this)->PointAt(point); }

	//Returns null for the 'null' point ID.
	const FString* GetName(EWfcPointID pointID) const;
	//Returns null for the 'null' point ID.
	const FString* GetName(WFC::Tiled3D::FacePoints point) const { return GetName(PointAt(point)); }

	void PostScriptConstruct();
	bool operator==(const FWfcFacePointDefs& d2) const
	{
		return GatherPoints() == d2.GatherPoints() &&
			   PointAA == d2.PointAA && PointAB == d2.PointAB &&
			   PointBA == d2.PointBA && PointBB == d2.PointBB;
	}
};
template<>
struct TStructOpsTypeTraits<FWfcFacePointDefs> : public TStructOpsTypeTraitsBase2<FWfcFacePointDefs>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithPostScriptConstruct = true
	};
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWfcFacePointDefs Corners;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWfcFacePointDefs Edges;
	
	WFC::Tiled3D::FaceIdentifiers Unwrap(int pointIdOffset) const;

	bool operator==(const FWfcFacePrototype& f2) const
	{
		return Nickname == f2.Nickname && Corners == f2.Corners && Edges == f2.Edges;
	}
};
template<>
struct TStructOpsTypeTraits<FWfcFacePrototype> : public TStructOpsTypeTraitsBase2<FWfcFacePrototype>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};

//Face prototypes have a unique ID in a tile-set.
using WfcFacePrototypeID = int32;
constexpr WfcFacePrototypeID INVALID_FACE_ID = 0,
			                 FIRST_VALID_FACE_ID = 1;