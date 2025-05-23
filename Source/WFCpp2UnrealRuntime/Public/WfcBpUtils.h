﻿#pragma once

#include "CoreMinimal.h"

#include "WfcFacePrototype.h"
#include "WfcDataReflection.h"
#include "WfcTile.h"

#include "WfcBpUtils.generated.h"

UCLASS()
class WFCPP2UNREALRUNTIME_API UWfcUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="=="))
	static bool FaceEquals(const FWFC_Face& a, const FWFC_Face& b) { return a == b; }
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="!="))
	static bool FaceNotEquals(const FWFC_Face& a, const FWFC_Face& b) { return !FaceEquals(a, b); }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="=="))
	static bool CubeEquals(const FWFC_Cube& a, const FWFC_Cube& b) { return a == b; }
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="!="))
	static bool CubeNotEquals(const FWFC_Cube& a, const FWFC_Cube& b) { return !CubeEquals(a, b); }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="=="))
	static bool TransfEquals(const FWFC_Transform3D& a, const FWFC_Transform3D& b) { return a == b; }
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="!="))
	static bool TransfNotEquals(const FWFC_Transform3D& a, const FWFC_Transform3D& b) { return !TransfEquals(a, b); }

    UFUNCTION(BlueprintCallable, Category="Faces")
    static const FWfcTileFace& GetFace(const FWfcTile& tile, WFC_Directions3D dir) { return tile.GetFace(static_cast<WFC::Tiled3D::Directions3D>(dir)); }

    UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="Transform"))
    static FTransform WfcToFTransform(const FWFC_Transform3D& wfcTransform) { return wfcTransform.ToFTransform(); }
    UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="Rotator"))
	static FRotator WfcToFRotator(WFC_Directions3D face);

	//Can also be used for rotations (just make a transform with no inversion).
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FWFC_Transform3D CombineWfcTransforms(const FWFC_Transform3D& first, const FWFC_Transform3D& second)
	{
		return { first.Unwrap().Then(second.Unwrap()) };
	}

	//The special face prototype ID that represents 'null'.
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="NULL face ID"))
	static int FaceIDInvalid() { return (int)INVALID_FACE_ID; }
	//The first valid face prototype ID. All subsequent values are also valid ID's.
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(CompactNodeTitle="First face ID"))
	static int FaceIDFirstValid() { return (int)FIRST_VALID_FACE_ID; }
};
