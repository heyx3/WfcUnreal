#pragma once

#include "CoreMinimal.h"
#include "WfcDataReflection.h"

#include "WfcTileData.generated.h"


//By default, to avoid excessive heap usage,
//    a tile's GameData does not generate a Description in shipping builds.
//You may change this at the plugin level.
#ifndef WFCPP2_TILE_DATA_GENERATE_DESCRIPTION
    #if UE_BUILD_SHIPPING || defined(__INTELLISENSE__) || defined(__RESHARPER__)
        #define WFCPP2_TILE_DATA_GENERATE_DESCRIPTION 1
    #else
        #define WFCPP2_TILE_DATA_GENERATE_DESCRIPTION 0
    #endif
#endif



//An Instanced Struct; the base type for data associated with a WFC tile.
//This base type can represent "null".
//
//For more info on instanced structs,
//    see https://github.com/mattyman174/GenericItemization?tab=readme-ov-file#instanced-structs
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcGameData
{
    GENERATED_BODY()
public:

    //A short, human-readable description of this data.
    //Automatically regenerated after editing, by calling the virtual function 'GenerateDescription()'.
    //
    //**IMPORTANT NOTE**: By default, descriptions are not generated in release builds!
    //This can be reconfigured in this plugin's C# build file.
    //
    //'GenerateDescription()' can only be implemented in C++, so Blueprint child structs can't control it.
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
    FString Description;
    
    virtual ~FWfcGameData() { }
    virtual FString GenerateDescription() const { return TEXT("[no description]"); }

    #if WFCPP2_TILE_DATA_GENERATE_DESCRIPTION
        void PostSerialize(const FArchive& ar)
        {
            if (ar.IsLoading())
                Description = GenerateDescription();
        }
        void PostScriptConstruct()
        {
            Description = GenerateDescription();
        }
    #endif
    
    
    //Assume the other instance is the same child type as this one.
    bool operator==(const FWfcGameData& other) const { return Compare(other); }
    virtual uint32 Hash() const { return 1; }
    bool Identical(const FWfcGameData* other, uint32 flags) const { return Compare(*other); }

protected:
    //Assume the other instance is the same child type as this one.
    virtual bool Compare(const FWfcGameData& other) const    PURE_VIRTUAL(FWfcGameData::Compare, return false; )

private:
};
inline uint32 GetTypeHash(const FWfcGameData& d) { return d.Hash(); }
template<>
struct TStructOpsTypeTraits<FWfcGameData> : public TStructOpsTypeTraitsBase2<FWfcGameData>
{
    enum
    {
        WithPostSerialize = WFCPP2_TILE_DATA_GENERATE_DESCRIPTION,
        WithPostScriptConstruct = WFCPP2_TILE_DATA_GENERATE_DESCRIPTION,
        WithIdentical = true //NOTE: if using IdenticalViaEquality, then Unreal's polymorphic wrapper can't compare them :(
    };
};
#define WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS_INNER
#define WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS(className) \
    template<> struct TStructOpsTypeTraits<className> : public TStructOpsTypeTraits<className::Super> { \
		WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS_INNER \
	}


//Associates a WFC tile with a static mesh asset.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcGameData_StaticMesh : public FWfcGameData
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UStaticMesh* Mesh = nullptr;

    virtual FString GenerateDescription() const override { return IsValid(Mesh) ? Mesh->GetName() : Super::GenerateDescription(); }
    virtual bool Compare(const FWfcGameData& other) const override { return Mesh == reinterpret_cast<const FWfcGameData_StaticMesh&>(other).Mesh; }
    virtual uint32 Hash() const override { return GetTypeHash(Mesh); }
};
WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS(FWfcGameData_StaticMesh);


//Associates a WFC tile with an actor.
//Consider having that actor check whether it's in an editor preview scene before running any logic!
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcGameData_Actor : public FWfcGameData
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AActor> ActorType = nullptr;
    //Guaranteed to be non-null, by defaulting to AActor.
    TSubclassOf<AActor> SanitizedActorType() const { return IsValid(ActorType) ? ActorType : TSubclassOf<AActor>{ AActor::StaticClass() }; }

    virtual FString GenerateDescription() const override { return SanitizedActorType()->GetName(); }
    virtual bool Compare(const FWfcGameData& other) const override { return ActorType == reinterpret_cast<const FWfcGameData_Actor&>(other).ActorType; }
    virtual uint32 Hash() const override { return GetTypeHash(ActorType); }
};
WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS(FWfcGameData_Actor);


USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcGameData_MeshList_Element
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UStaticMesh* Mesh = nullptr;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FWFC_Transform3D Permutation = { };

    bool operator==(const FWfcGameData_MeshList_Element& other) const
    {
        return (Mesh == other.Mesh) &&
               (Permutation == other.Permutation);
    }
    bool Serialize(FArchive& ar)
    {
        ar << Mesh;
        ar << Permutation;
        return true;
    }
};
inline FArchive& operator<<(FArchive& ar, FWfcGameData_MeshList_Element& e)
{
    e.Serialize(ar);
    return ar;
}
inline uint32 GetTypeHash(const FWfcGameData_MeshList_Element& e)
{
    return GetTypeHash(MakeTuple(e.Mesh, e.Permutation.Unwrap().GetID()));
}
template<>
struct TStructOpsTypeTraits<FWfcGameData_MeshList_Element> : public TStructOpsTypeTraitsBase2<FWfcGameData_MeshList_Element>
{
    enum
    {
        WithIdenticalViaEquality = true,
        WithSerializer = true,
        WithZeroConstructor = true,
        WithNoDestructor = true
    };
};
//Associates a WFC tile with a list of static mesh assets,
//    each with a cube-aligned transform.
USTRUCT(BlueprintType)
struct WFCPP2UNREALRUNTIME_API FWfcGameData_MeshList : public FWfcGameData
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FWfcGameData_MeshList_Element> Elements;

    virtual FString GenerateDescription() const override { return FString::Printf(TEXT("%i Meshes"), Elements.Num()); }
    virtual bool Compare(const FWfcGameData& other) const override { return Elements == reinterpret_cast<const FWfcGameData_MeshList&>(other).Elements; }
    virtual uint32 Hash() const override { return GetTypeHash(Elements); }
};
WFCPP_UNREAL_TILE_GAME_DATA_TYPE_TRAITS(FWfcGameData_MeshList);