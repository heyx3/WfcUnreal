#include "WfcEditorScenes/WfcTileVisualizer.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcEditorScenes/WfcTilesetEditorScene.h"


namespace
{
	FCriticalSection VizFactoryLocker;
	TArray<TTuple<WfcTileDataPredicate, WfcTileVisualizerFactory>> VizFactories;
}

WfcTileVisualizer::WfcTileVisualizer(const FWfcTileVisualizerInputs& inputs)
    : FWfcTileVisualizerInputs(inputs)
{

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
		return (data && data->GetScriptStruct() == FWfcGameData_StaticMesh::StaticStruct());
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
      				reinterpret_cast<const FWfcGameData_StaticMesh&>(inputs.GetTileGameData()->Get()).Mesh,
				    inputs.TileTr)
{
	
}


namespace WfcTileVisualizerMeshList
{
	static bool IsApplicable(const FWfcTileVisualizerInputs& inputs)
	{
		auto* data = inputs.GetTileGameData();
		return (data && data->GetScriptStruct() == FWfcGameData_MeshList::StaticStruct());
	}
	static TUniquePtr<WfcTileVisualizer> MakeViz(const FWfcTileVisualizerInputs& inputs)
	{
		return MakeUnique<WfcTileVisualizer_MeshList>(inputs);
	}
	
	REGISTER_VISUALIZER_IN_CPP_FILE(WfcTileVisualizer_MeshList, IsApplicable, MakeViz);
}
WfcTileVisualizer_MeshList::WfcTileVisualizer_MeshList(const FWfcTileVisualizerInputs& inputs)
	: WfcTileVisualizer(inputs)
{
	const auto& meshList = inputs.GetTileGameData()->Get<FWfcGameData_MeshList>();
	for (const auto& mesh : meshList.Elements)
	{
		meshComponents.Emplace(
			&inputs.EditorScene,
			mesh.Mesh,
			WfcppUnrealEditor::ComposeTransforms(
				mesh.Permutation.ToFTransform(),
				inputs.TileTr
			)
		);
	}
}
