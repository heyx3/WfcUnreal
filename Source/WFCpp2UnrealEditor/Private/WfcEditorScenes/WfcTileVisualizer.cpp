#include "WfcEditorScenes/WfcTileVisualizer.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcEditorScenes/WfcTilesetEditorScene.h"


namespace
{
	FCriticalSection VizFactoryLocker;
	TArray<TTuple<WfcTileDataPredicate, WfcTileVisualizerFactory>> VizFactories;
}

WfcTileVisualizer::WfcTileVisualizer(const FWfcTileVisualizerInputs& inputs, bool normalViz): FWfcTileVisualizerInputs(inputs)
{
	if (normalViz)
	{
		tileBaseViz.Emplace(&inputs.EditorScene, inputs.TileTr, inputs.Tileset->TileLength,
							FLinearColor{ 0, 0, 0, 1 }, inputs.Tileset.Get(), inputs.TileIdx);
	}
}

void WfcTileVisualizer::RegisterVisualizer(WfcTileDataPredicate isApplicable,
                                           WfcTileVisualizerFactory factory)
{
	FScopeLock lock{ &VizFactoryLocker };
	VizFactories.Emplace(isApplicable, factory);
}
TUniquePtr<WfcTileVisualizer> WfcTileVisualizer::MakeVisualizer(const FWfcTileVisualizerInputs& inputs)
{
	FScopeLock lock{ &VizFactoryLocker };
	for (int i = VizFactories.Num() - 1; i >= 0; --i)
	{
		auto& [predicate, factory] = VizFactories[i];
		if (predicate(inputs))
			return factory(inputs);
	}

	return nullptr;
}


namespace WfcTileVisualizerStaticMesh
{
	static bool IsApplicable(const FWfcTileVisualizerInputs& inputs)
	{
		auto* data = inputs.GetTileGameData();
		return (IsValid(data) && data->IsA<UWfcTileGameData_StaticMesh>());
	}
	static TUniquePtr<WfcTileVisualizer> MakeViz(const FWfcTileVisualizerInputs& inputs)
	{
		return MakeUnique<WfcTileVisualizer_StaticMesh>(inputs);
	}
	
	REGISTER_VISUALIZER_IN_CPP_FILE(WfcTileVisualizer_StaticMesh, IsApplicable, MakeViz);
}
WfcTileVisualizer_StaticMesh::WfcTileVisualizer_StaticMesh(const FWfcTileVisualizerInputs& inputs)
	: WfcTileVisualizer(inputs),
      meshComponent(&inputs.EditorScene,
				    CastChecked<UWfcTileGameData_StaticMesh>(inputs.GetTileGameData())->Mesh,
				    inputs.TileTr)
{
	
}
