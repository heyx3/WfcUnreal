#pragma once

#include "CoreMinimal.h"

#include <functional>

#include "WfcTileData.h"
#include "WfcTileset.h"


//The editor for a particular kind of UWfcTileGameData.
//You should not directly inherit from this, but from the child class TWfcTileVisualizer.
class WFCPP2UNREALEDITOR_API FWfcTileVisualizerBase
{
public:
    //Queries the kind of tile data this visualizer is meant for.
    TSubclassOf<UWfcTileGameData> GetTileDataSubclass() const { return GetSubclassImpl(); }
    
    virtual void SetUpVisualization(class FWfcTilesetEditorScene& editorScene,
                                    class FWfcTilesetEditorViewportClient& editorSceneManager,
                                    const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                    const UWfcTileGameData* untypedTileData,
                                    TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                    TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) = 0;
    virtual void TearDownVisualization(class FWfcTilesetEditorScene& editorScene,
                                       class FWfcTilesetEditorViewportClient& editorSceneManager,
                                       const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                       const UWfcTileGameData* untypedTileData,
                                       const TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                       const TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) = 0;
    
private:

    //The templated subclass is the only one allowed to directly inherit from us.
    template<typename T>
    friend class TWfcTileVisualizer;
    
    virtual ~FWfcTileVisualizerBase() { }
    virtual TSubclassOf<UWfcTileGameData> GetSubclassImpl() const = 0;
};

//The editor for a particular kind of UWfcTileGameData.
template<typename TWfcTileGameData>
class WFCPP2UNREALEDITOR_API TWfcTileVisualizer : public FWfcTileVisualizerBase
{
public:
    static_assert(std::is_base_of_v<UWfcTileGameData, TWfcTileGameData>,
                  "Template argument must be a child of UWfcTileGameData!");

    virtual void SetUpVisualization(class FWfcTilesetEditorScene& editorScene,
                                    class FWfcTilesetEditorViewportClient& editorSceneManager,
                                    const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                    const UWfcTileGameData* untypedTileData,
                                    TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                    TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) final override
    {
        const auto* typedTileData = CastChecked<TWfcTileGameData>(untypedTileData);
        SetUpVisualizationImpl(editorScene, editorSceneManager, tileset, tileIdx, tile, typedTileData, outNewActors, outNewLoneComponents);
    }
    virtual void TearDownVisualization(class FWfcTilesetEditorScene& editorScene,
                                       class FWfcTilesetEditorViewportClient& editorSceneManager,
                                       const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                       const UWfcTileGameData* untypedTileData,
                                       const TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                       const TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) final override
    {
        const auto* typedTileData = CastChecked<TWfcTileGameData>(untypedTileData);
        TearDownVisualizationImpl(editorScene, editorSceneManager, tileset, tileIdx, tile, typedTileData, outNewActors, outNewLoneComponents);
    }

protected:
    //Asks this instance to create a visualization of the given tile data instance, within the given editor scene.
    virtual void SetUpVisualizationImpl(class FWfcTilesetEditorScene& editorScene,
                                        class FWfcTilesetEditorViewportClient& editorSceneManager,
                                        const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                        const TWfcTileGameData* tileData,
                                        TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                        TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) = 0;
    //Notifies this instance that the given visualization is about to be destroyed.
    //Is NOT called when the preview world is being destroyed altogether.
    virtual void TearDownVisualizationImpl(class FWfcTilesetEditorScene& editorScene,
                                           class FWfcTilesetEditorViewportClient& editorSceneManager,
                                           const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                           const TWfcTileGameData* tileData,
                                           const TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                           const TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) { }

    
private:
    virtual TSubclassOf<UWfcTileGameData> GetSubclassImpl() const final override { return TWfcTileGameData::StaticClass(); }
};


//The editor for a Static-Mesh WfcTileGameData.
class WFCPP2UNREALEDITOR_API FWfcTileVisualizer_StaticMesh : public TWfcTileVisualizer<UWfcTileGameData_StaticMesh>
{
    virtual void SetUpVisualizationImpl(class FWfcTilesetEditorScene& editorScene,
                                        class FWfcTilesetEditorViewportClient& editorSceneManager,
                                        const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                        const UWfcTileGameData_StaticMesh* tileData,
                                        TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                        TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) override;
};
//The editor for an Actor WfcTileGameData.
class WFCPP2UNREALEDITOR_API FWfcTileVisualizer_Actor : public TWfcTileVisualizer<UWfcTileGameData_Actor>
{
    virtual void SetUpVisualizationImpl(class FWfcTilesetEditorScene& editorScene,
                                        class FWfcTilesetEditorViewportClient& editorSceneManager,
                                        const UWfcTileset& tileset, int32 tileIdx, const FWfcTile& tile,
                                        const UWfcTileGameData_Actor* tileData,
                                        TArray<TWeakObjectPtr<AActor>>& outNewActors,
                                        TArray<TWeakObjectPtr<UActorComponent>>& outNewLoneComponents) override;
};