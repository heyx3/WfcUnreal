#pragma once

#include "CoreMinimal.h"

#include "EditorSceneComponents.h"
#include "WfcTileData.h"
#include "WfcTileset.h"


struct FWfcTileVisualizerInputs
{
	class FWfcTilesetEditorScene& EditorScene;
	class FWfcTilesetEditorViewportClient& EditorSceneManager;
	const TWeakObjectPtr<const UWfcTileset> Tileset;

	const int32 TileIdx;
	const FWFC_Transform3D Permutation;
	const TOptional<FWfcTile> Tile;
	const UWfcTileGameData* GetTileGameData() const { return (Tile.IsSet() ? Tile->Data : nullptr); }

	//This transform is assumed to already incorporate the permutation (`Permutation.ToFTransform()`).
	FTransform TileTr;
};

#pragma region Base classes

using WfcTileDataPredicate = TFunction<bool (const FWfcTileVisualizerInputs&)>;
using WfcTileVisualizerFactory = TFunction<TUniquePtr<class WfcTileVisualizer> (const FWfcTileVisualizerInputs&)>;

#define REGISTER_VISUALIZER_IN_CPP_FILE(name, predicate, factory) \
    struct VizRegister##name { \
		VizRegister##name() { \
            WfcTileVisualizer::RegisterVisualizer(predicate, factory); \
        } \
    }; \
    VizRegister##name register##name


class WFCPP2UNREALEDITOR_API WfcTileVisualizer : protected FWfcTileVisualizerInputs
{
public:

	WfcTileVisualizer(const FWfcTileVisualizerInputs&);
	virtual ~WfcTileVisualizer() { }

	WfcTileVisualizer(const WfcTileVisualizer&) = delete;
	WfcTileVisualizer& operator=(const WfcTileVisualizer&) = delete;
	

	const FWfcTileVisualizerInputs& GetInputs() const { return *this; }
	void SetTileTransform(const FTransform& newTr)
	{
		auto oldTr = TileTr;
		
		TileTr = newTr;
		UpdateTransform(oldTr, newTr);
	}

	
	static void RegisterVisualizer(WfcTileDataPredicate isApplicable, WfcTileVisualizerFactory factory);

	//Returns null if no visualizer is applicable to the given tileset.
	static TUniquePtr<WfcTileVisualizer> MakeVisualizer(const FWfcTileVisualizerInputs& inputs);

protected:
	virtual void UpdateTransform(const FTransform& oldTileTr, const FTransform& newTileTr) { }
};

#pragma endregion


class WFCPP2UNREALEDITOR_API WfcTileVisualizer_StaticMesh : public WfcTileVisualizer
{
public:

	WfcTileVisualizer_StaticMesh(const FWfcTileVisualizerInputs& inputs);

	FEditorMeshComponent meshComponent;
};