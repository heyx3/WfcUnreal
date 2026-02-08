#include "WfcEditorScenes/WfcTileVisualizer.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcEditorScenes/WfcTilesetEditorScene.h"
#include "WfcTilesetEditorUtils.h"
#include "GameFramework/Actor.h"


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
		return (IsValid(data) && data->IsA<UWfcTileGameData_StaticMesh>());
	}
	static TUniquePtr<WfcTileVisualizer> MakeViz(const FWfcTileVisualizerInputs& inputs)
	{
		return MakeUnique<WfcTileVisualizer_StaticMesh>(inputs);
	}
	
	REGISTER_VISUALIZER_IN_CPP_FILE(WfcTileVisualizer_StaticMesh, IsApplicable, MakeViz);
}
namespace WfcTileVisualizerActor
{
	static bool IsApplicable(const FWfcTileVisualizerInputs& inputs)
	{
		auto* data = inputs.GetTileGameData();
		return (IsValid(data) && data->IsA<UWfcTileGameData_Actor>());
	}
	static TUniquePtr<WfcTileVisualizer> MakeViz(const FWfcTileVisualizerInputs& inputs)
	{
		return MakeUnique<WfcTileVisualizer_Actor>(inputs);
	}

	REGISTER_VISUALIZER_IN_CPP_FILE(WfcTileVisualizer_Actor, IsApplicable, MakeViz);
}
namespace WfcTileVisualizerDebug
{
	static bool IsApplicable(const FWfcTileVisualizerInputs& inputs)
	{
		auto* data = inputs.GetTileGameData();
		return (IsValid(data) && data->IsA<UWfcTileGameData_Debug>());
	}
	static TUniquePtr<WfcTileVisualizer> MakeViz(const FWfcTileVisualizerInputs& inputs)
	{
		return MakeUnique<WfcTileVisualizer_Debug>(inputs);
	}

	REGISTER_VISUALIZER_IN_CPP_FILE(WfcTileVisualizer_Debug, IsApplicable, MakeViz);
}
WfcTileVisualizer_StaticMesh::WfcTileVisualizer_StaticMesh(const FWfcTileVisualizerInputs& inputs)
	: WfcTileVisualizer(inputs),
      meshComponent(&inputs.EditorScene,
				    CastChecked<UWfcTileGameData_StaticMesh>(inputs.GetTileGameData())->Mesh,
				    inputs.TileTr)
{
	
}

WfcTileVisualizer_Actor::WfcTileVisualizer_Actor(const FWfcTileVisualizerInputs& inputs)
	: WfcTileVisualizer(inputs)
{
	auto* data = Cast<UWfcTileGameData_Actor>(inputs.GetTileGameData());
	if (!IsValid(data))
		return;

	auto* world = inputs.EditorScene.GetWorld();
	if (!IsValid(world))
		return;

	previewActor = WfcTilesetEditorUtils::CreatePreviewSceneActor(world, data->SanitizedActorType());
	if (previewActor.IsValid())
		previewActor->SetActorTransform(inputs.TileTr);
}
WfcTileVisualizer_Actor::~WfcTileVisualizer_Actor()
{
	if (previewActor.IsValid())
		WfcTilesetEditorUtils::DestroyPreviewSceneActor(previewActor.Get());
}
void WfcTileVisualizer_Actor::UpdateTransform(const FTransform& oldTileTr, const FTransform& newTileTr)
{
	if (previewActor.IsValid())
		previewActor->SetActorTransform(newTileTr);
}

WfcTileVisualizer_Debug::WfcTileVisualizer_Debug(const FWfcTileVisualizerInputs& inputs)
	: WfcTileVisualizer(inputs)
{
	const UWfcTileGameData_Debug* data = Cast<UWfcTileGameData_Debug>(inputs.GetTileGameData());
	if (!IsValid(data))
		return;

	auto* world = inputs.EditorScene.GetWorld();
	if (!IsValid(world))
		return;

	previewActor = Cast<AWfcTileDebugActor>(WfcTilesetEditorUtils::CreatePreviewSceneActor(world, data->SanitizedActorType()));
	if (previewActor.IsValid())
	{
		previewActor->OnConstructPreview(data);
		previewActor->SetActorTransform(inputs.TileTr);
	}

}

WfcTileVisualizer_Debug::~WfcTileVisualizer_Debug()
{
	if (previewActor.IsValid())
		WfcTilesetEditorUtils::DestroyPreviewSceneActor(previewActor.Get());
}

void WfcTileVisualizer_Debug::UpdateTransform(const FTransform& oldTileTr, const FTransform& newTileTr)
{
	if (previewActor.IsValid())
		previewActor->SetActorTransform(newTileTr);
}
