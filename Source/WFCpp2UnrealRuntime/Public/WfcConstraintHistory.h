#pragma once

#include "WFCpp2.h"
#include "WfcTileset.h"

#include "WfcConstraintHistory.generated.h"


//Base struct type for various kinds of WFC constraints that can be added to a grid.
USTRUCT(BlueprintType)
struct FWfcConstraintEntry
{
    GENERATED_BODY()
};

//Setting of a WFC grid face to be (or not be) a particular value.
USTRUCT(BlueprintType)
struct FWfcConstraintEntry_Face : public FWfcConstraintEntry
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FIntVector Cell = { 0, 0, 0 };
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    WFC_Directions3D Side = WFC_Directions3D::MinX;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int FacePrototypeId = 0;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    WFC_Transforms2D FacePrototypeTransform = WFC_Transforms2D::None;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsForbidding = false;

    
    //Converts the affected face from Unreal data to core WFCpp data.
    //TODO: Pull the core parts into a WFC utility function
    static WFC::Tiled3D::FaceIdentifiers UnwrapFacePermutation(WFC_Directions3D side,
                                                               WFC_Transforms2D faceTransform,
                                                               const FWfcFacePrototype& facePrototype,
                                                               int firstWfcIDForFace)
    {
        auto rawPoints = facePrototype.Unwrap(firstWfcIDForFace),
             permutedPoints = rawPoints;

        for (auto srcPoint : WFC::Tiled3D::ALL_FACE_POINTS)
        {
            auto destCornerPoint = WFC::Tiled3D::TransformFaceCorner(
                srcPoint,
                static_cast<WFC::Tiled3D::Directions3D>(side),
                static_cast<WFC::Transformations>(faceTransform)
            );
            permutedPoints.Corners[destCornerPoint] = rawPoints.Corners[srcPoint];

            auto destEdgePoint = WFC::Tiled3D::TransformFaceEdge(
                srcPoint,
                static_cast<WFC::Tiled3D::Directions3D>(side),
                static_cast<WFC::Transformations>(faceTransform)
            );
            permutedPoints.Edges[destEdgePoint] = rawPoints.Edges[srcPoint];
        }

        return permutedPoints;
    }
};

//Setting of a WFC grid tile to be (or not be) a particular cell.
USTRUCT(BlueprintType)
struct FWfcConstraintEntry_Cell : public FWfcConstraintEntry
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FIntVector Cell = { 0, 0, 0 };

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int TileId = 0;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FWFC_Transform3D TilePermutation = { };
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsForbidding = false;
};

