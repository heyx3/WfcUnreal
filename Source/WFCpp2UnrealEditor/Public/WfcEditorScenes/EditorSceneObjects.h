#pragma once

#include "CoreMinimal.h"

#include "WfcDataReflection.h"
#include "EditorSceneComponents.h"
#include "WfcFacePrototype.h"
#include "WfcTilesetEditorViewportClient.h"
#include "WfcTileVisualizer.h"
#include "WfcConstraintHistory.h"
#include "WfcGenerator.h"

#include "EditorSceneObjects.generated.h"


//A scoped owner of a group of components in a preview/editor scene,
//    not unlike an Actor in a game scene.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject
{
public:

	FPreviewScene* const Owner;
	
	FEditorSceneObject(FPreviewScene* owner) : Owner(owner) { }
	FEditorSceneObject(FEditorSceneObject&&) = default;

	virtual ~FEditorSceneObject() { }

	FEditorSceneObject& operator=(const FEditorSceneObject&) = delete;
	FEditorSceneObject& operator=(FEditorSceneObject&&) = delete;
};


struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcFace_Settings
{
	float AlphaScale = 1;
	bool ColorByFace = true;
};
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile_Settings : public FEditorSceneObject_WfcFace_Settings
{
	bool IncludeDataVisualizer = true;
	FLinearColor BoundsColor = { 0, 0, 0, 1 };
	bool ShowFaces = true;
};

struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcPermutations_Settings : public FEditorSceneObject_WfcTile_Settings
{
	FLinearColor PermutationLabelColor = { 0.1, 0.1, 0.1, 1 };
	FLinearColor LabelsTint = { 0.5, 0.5, 0.5, 1 };
};
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcMatches_Settings : public FEditorSceneObject_WfcTile_Settings
{
	FLinearColor LabelsTint = { 0.5, 0.5, 0.5, 1 };
};

USTRUCT()
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcGeneration_Settings
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere)
	FIntVector Resolution = { 5, 5, 3 };
	UPROPERTY(EditAnywhere)
	int Seed = 12345;

	//If > 0, immediately runs the generator to this step or until it finishes/fails.
	UPROPERTY(EditAnywhere)
	int ImmediatelyRunIterations = 0;

	//From 0 to 1, how fast the clear region increases around tiles that have already been cleared a lot.
	//Use a low value for tilesets that produce many small errors requiring limited clearing.
	UPROPERTY(EditAnywhere, meta=(UIMin=0, UIMax=1))
	float TemperatureClearGrowthRateT = 0.1f;
	//The amount of randomness in which cells get set first.
	//If set to 0, the algorithm always picks (randomly) from the cells with the fewest number of options.
	UPROPERTY(EditAnywhere)
	float Fuzziness = 0.1f;

	UPROPERTY(EditAnywhere, meta=(ClampMin=0))
	int MaxUnwinding = 0;

	UPROPERTY(EditAnywhere, AdvancedDisplay)
	bool PeriodicX = false;
	UPROPERTY(EditAnywhere, AdvancedDisplay)
	bool PeriodicY = false;
	UPROPERTY(EditAnywhere, AdvancedDisplay)
	bool PeriodicZ = false;

	UPROPERTY(EditAnywhere, AdvancedDisplay)
	TArray<TInstancedStruct<FWfcConstraintEntry>> InitialConstraints;

	
	//Returns true if the given settings represents the same generator as this one
	//    but potentially with a different immediate-tick-count.
	inline bool IsSameGeneratorAs(const FEditorSceneObject_WfcGeneration_Settings& other) const
	{
		return Resolution == other.Resolution && Seed == other.Seed &&
			   TemperatureClearGrowthRateT == other.TemperatureClearGrowthRateT &&
			   Fuzziness == other.Fuzziness &&
			   MaxUnwinding == other.MaxUnwinding &&
			   PeriodicX == other.PeriodicX &&
			   PeriodicY == other.PeriodicY &&
			   PeriodicZ == other.PeriodicZ &&
			   InitialConstraints == other.InitialConstraints;
	}
};
inline bool operator==(const FEditorSceneObject_WfcGeneration_Settings& a, const FEditorSceneObject_WfcGeneration_Settings& b)
{
	return a.IsSameGeneratorAs(b) &&
		   a.ImmediatelyRunIterations == b.ImmediatelyRunIterations;
}
template<> struct TStructOpsTypeTraits<FEditorSceneObject_WfcGeneration_Settings> : public TStructOpsTypeTraitsBase2<FEditorSceneObject_WfcGeneration_Settings>
{
	enum
	{
		WithZeroConstructor = true,
		WithNoDestructor = true,
		WithIdenticalViaEquality = true
	};
};

USTRUCT()
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcGeneration_Display
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere)
	float ExtraSpacing = 0;

	UPROPERTY(EditAnywhere)
	FTransform Transform;

	UPROPERTY(EditAnywhere)
	bool ShowFaceConstraints = false;
	UPROPERTY(EditAnywhere)
	bool ShowHotSpots = false;
	UPROPERTY(EditAnywhere)
	bool ShowUnsolvable = true;
	UPROPERTY(EditAnywhere)
	bool ShowBoring = false;

	bool operator==(const FEditorSceneObject_WfcGeneration_Display& other) const
	{
		return ExtraSpacing == other.ExtraSpacing &&
			   Transform.Equals(other.Transform) &&
			   ShowFaceConstraints == other.ShowFaceConstraints &&
			   ShowHotSpots == other.ShowHotSpots &&
			   ShowUnsolvable == other.ShowUnsolvable &&
			   ShowBoring == other.ShowBoring;
	}
};
template<> struct TStructOpsTypeTraits<FEditorSceneObject_WfcGeneration_Display> : public TStructOpsTypeTraitsBase2<FEditorSceneObject_WfcGeneration_Display>
{
	using TrTT = TStructOpsTypeTraits<FTransform>;
	enum
	{
		WithZeroConstructor = TrTT::WithZeroConstructor,
		WithNoDestructor = TrTT::WithNoDestructor,
		WithIdenticalViaEquality = true
	};
};


//An editor object that displays the symmetry info for a WFC tile face.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcFace : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcFace(FPreviewScene* owner,
							   const FTransform& tileTransform, double cubeExtents,
							   WFC_Directions3D faceDir,
							   const FWfcFacePrototype& faceData,
							   WFC_Transforms2D facePermutation,
							   const FEditorSceneObject_WfcFace_Settings& settings);

	void SetTileTransform(const FTransform& tr)
	{
		tileTr = tr;
		RebuildTransform();
	}
	void SetAlphaScale(float newScale)
	{
		settings.AlphaScale = newScale;
		RebuildColors();
	}

	
private:

	FTransform tileTr;
	double cubeExtents;
	
	WFC_Directions3D faceSide;
	FWfcFacePrototype facePrototype;
	WFC_Transforms2D facePermutation;

	TOptional<FEditorWireSphereComponent> centerSphere;
	TOptional<FEditorTextComponent> fallbackLabel;
	TOptional<FEditorPlaneComponent> facePlane;
	std::array<TOptional<FEditorTextComponent>, 4> cornerLabels, edgeLabels;
	std::array<TOptional<FEditorArrowComponent>, 4> cornerArrows, edgeArrows;
	std::array<EWfcPointID, 4> cornerIDs, edgeIDs;

	FEditorSceneObject_WfcFace_Settings settings;
	

	void RebuildColors();
	void RebuildTransform();
};

//Displays one WFC tile, with its symmetry information.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTile : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTile(FWfcTilesetEditorScene& owner, class FWfcTilesetEditorViewportClient& viewportClient,
							   const FTransform& worldTr,
							   const class UWfcTileset* tileset, int32 tileID, const FWFC_Transform3D& permutation,
							   const FEditorSceneObject_WfcTile_Settings& settings);

	FTransform GetCurrentTransform() const { return currentTr; }
	void SetTransform(const FTransform& newTr);

	const auto& GetFaces() const { return faces; }
	FEditorSceneObject_WfcFace& GetFace(int i) { return faces[i]; }
	FEditorSceneObject_WfcFace& GetFace(WFC::Tiled3D::Directions3D face) { return faces[face]; }
	FEditorSceneObject_WfcFace& GetFace(WFC_Directions3D face) { return faces[static_cast<int>(face)]; }
	
private:

	FTransform currentTr;
	
 	TArray<FEditorSceneObject_WfcFace, TInlineAllocator<WFC::Tiled3D::N_DIRECTIONS_3D>> faces;
	FEditorWireBoxComponent tileBounds;
	TUniquePtr<WfcTileVisualizer> tileDataVisualizer;

	FEditorSceneObject_WfcTile_Settings settings;
};

//Displays all supported permutations of one WFC tile.
struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTileWithPermutations : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTileWithPermutations(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
										       const FTransform& tr, double spacingBetweenTiles,
											   const class UWfcTileset* tileset, int32 tileID,
											   const FEditorSceneObject_WfcPermutations_Settings& settings);

private:

	struct Permutation
	{
		FEditorSceneObject_WfcTile Tile;
		FEditorTextComponent Label;
		FWFC_Transform3D WfcTransform;

		Permutation(FEditorSceneObject_WfcTile&& tile, FEditorTextComponent&& label, const FWFC_Transform3D& tr)
			: Tile(MoveTemp(tile)), Label(MoveTemp(label)), WfcTransform(tr) { }
	};
	TArray<Permutation, TInlineAllocator<WFC::Tiled3D::N_TRANSFORMS>> permutations;
	TOptional<FEditorTextComponent> overallLabel;

	FEditorSceneObject_WfcPermutations_Settings settings;
};

struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcTileWithMatches : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcTileWithMatches(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
										  const FTransform& tr, double spacingBetweenTiles,
										  const class UWfcTileset* tileset, int32 tileID,
										  const FWFC_Transform3D& permutation,
										  const TSet<WFC_Directions3D>& facesToMatchAfterPermutation,
										  const FEditorSceneObject_WfcMatches_Settings& settings);

private:

	TOptional<UWfcTileset::Unwrapped> libraryTilesetData;
	TOptional<FEditorSceneObject_WfcTile> sourceTile;

	TArray<FEditorTextComponent> faceLabels;

	FEditorSceneObject_WfcMatches_Settings settings;
	
	struct Match
	{
		int32 TileID;
		FWFC_Transform3D Permutation;
		
		WFC_Directions3D SrcFaceToMatch;
		
		FEditorSceneObject_WfcTile EditorObject;
		FEditorTextComponent Label;
	};
	TArray<Match> matches;
};

struct WFCPP2UNREALEDITOR_API FEditorSceneObject_WfcGeneration : public FEditorSceneObject
{
public:

	FEditorSceneObject_WfcGeneration(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
									 const FEditorSceneObject_WfcGeneration_Display& display,
									 const UWfcTileset* tileset,
									 const FEditorSceneObject_WfcGeneration_Settings& settings);
	FEditorSceneObject_WfcGeneration(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
									 const FEditorSceneObject_WfcGeneration_Display& display,
									 const class UWfcGeneratorInitialState* initialState,
									 int immediatelyRunNIterations = 0);

	//Remakes this instance to match the given settings.
	//Attempts to optimize the operation if the change is small
	//    (e.x. if you only increased ImmediatelyRunIterations we'll just run Tick that many times).
	void RefreshSettings(const FEditorSceneObject_WfcGeneration_Settings& newSettings);
	//Runs N updates of the generator (or until completion), then updates visualizations accordingly.
	//For convenience, does nothing when n < 1.
	void Tick(int n = 1);
	//Undoes the previous N-ticks the user executed.
	//You must first check the generator to see if any history exists.
	void Rewind();

	//Updates the display settings for this generator sim, without having to restart the generator.
	//Always skips redrawing this object if nothing actually changed.
	void ChangeSpace(const FEditorSceneObject_WfcGeneration_Display& newDisplay,
					 bool immediateRedraw = true);
	
	
	const class UWfcGenerator* GetGenerator() const { return generator; }
	class UWfcGenerator* GetGenerator() { return generator; }
	const auto& GetCurrentSettings() const { return currentSettings; }

private:

	FWfcTilesetEditorViewportClient* viewportClient;
	TWeakObjectPtr<const UWfcTileset> tileset;
	
	TObjectPtr<class UWfcGenerator> generator;
	TMap<FIntVector, int> currentUnsolvableCellCounts;
	TArray<TMap<FIntVector, int>> generatorHistoryOfUnsolvableCellCounts;

	FEditorSceneObject_WfcGeneration_Settings currentSettings;
	FEditorSceneObject_WfcGeneration_Display currentDisplay;
	double tileSeparation;

	int nIterations = 0;
	
	struct FSetCell
	{
		WfcTileID TileID;
		TUniquePtr<WfcTileVisualizer> Viz;
		TArray<FEditorWireBoxComponent> ClearedViz;
	};
	TMap<FIntVector3, FSetCell> setCells;

	//A face within an unsolvable cell.
	struct FUnsolvableCellFace
	{
		//The number of permuted faces that could satisfy this cell
		//  (including redundant ones, e.g. flipping a face with mirror symmetry).
		//
		//If zero, then this face has no possibilities --
		//    usually because the externally-set constraints are impossible to satisfy.
		int NPossibilities = -1;
		
		//If this face has exactly one possibility, this is its prototype (by ID).
		int ForcedPrototypeID = -1;
		//If this face has exactly one possibility, this is its prototype's permutation.
		WFC_Transforms2D ForcedFaceTransform = WFC_Transforms2D::None;
	};
	struct FUnsolvableCell
	{
		TOptional<FEditorTextComponent> MarkerViz;
		TStaticArray<UWfcGenerator::ForcedCellFaceSources, WFC::Tiled3D::N_DIRECTIONS_3D> FaceData; 
		TStaticArray<TOptional<FEditorSceneObject_WfcFace>, WFC::Tiled3D::N_DIRECTIONS_3D> FaceConstraintsViz;
	};
	TMap<FIntVector3, FUnsolvableCell> unsolvableCells;
	
	struct FUnsetCell
	{
		float Temperature;
		TOptional<FEditorMeshComponent> TemperatureViz;
		TOptional<FEditorTextComponent> EntropyViz;
		TOptional<FEditorWireBoxComponent> BoringViz;
		TArray<FEditorWireBoxComponent> ClearedViz;
		TStaticArray<TOptional<FEditorSceneObject_WfcFace>, WFC::Tiled3D::N_DIRECTIONS_3D> FaceConstraintsViz;
	};
	TMap<FIntVector3, FUnsetCell> unsetCells;

	TOptional<FEditorWireBoxComponent> areaBox;

	void RefreshViz();
};