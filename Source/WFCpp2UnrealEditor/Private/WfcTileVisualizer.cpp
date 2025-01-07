#include "WfcTileVisualizer.h"

#include "WfcTilesetEditorScene.h"
#include "Algo/AnyOf.h"


void FWfcTileVisualizer_StaticMesh::SetUpVisualizationImpl(FWfcTilesetEditorScene& editorScene,
                                                           FWfcTilesetEditorViewportClient& editorSceneManager,
                                                           const UWfcTileset& tileset,
                                                           int32 tileIdx,
                                                           const FWfcTile& tile,
                                                           const UWfcTileGameData_StaticMesh* tileData,
                                                           TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                                           TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents)
{
    auto* c = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
    c->SetStaticMesh(tileData->Mesh);
    editorScene.AddComponent(c, FTransform{ });
    outNewLoneComponents.Add(c);
}
void FWfcTileVisualizer_Actor::SetUpVisualizationImpl(FWfcTilesetEditorScene& editorScene,
                                                      FWfcTilesetEditorViewportClient& editorSceneManager,
                                                      const UWfcTileset& tileset,
                                                      int32 tileIdx,
                                                      const FWfcTile& tile,
                                                      const UWfcTileGameData_Actor* tileData,
                                                      TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                                      TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents)
{
    //TODO: Implement.
    check(false);
    
    //TODO: Try intercepting 'UChildActorComponent' and throw an error on discovering it, because it could crash the editor.
}
