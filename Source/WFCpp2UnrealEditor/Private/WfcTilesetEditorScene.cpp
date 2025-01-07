#include "WfcTilesetEditorScene.h"

#include "GameFramework/WorldSettings.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcTileData.h"
#include "WfcTilesetEditorUtils.h"
#include "WfcTileVisualizer.h"


//Most visual settings are encapsulated in this namespace.
namespace
{
    const FLinearColor axisColors[6] = {
        //Note that this order should match the order of the enum WFC::Tiled3D::Directions3D.

        //Negative/Positive X:
        { 0.2f, 0.0f, 0.0f },
        { 0.05f, 0.0f, 0.0f },

        //Negative/Positive Y:
        { 0.0f, 0.2f, 0.0f },
        { 0.0f, 0.05f, 0.0f },

        //Negative/Positive Z:
        { 0.0f, 0.0f, 0.2f },
        { 0.0f, 0.0f, 0.05f },
    };
    const FLinearColor faceNameColor = { 0.0f, 0.0f, 0.0f };
    
    constexpr float initialTileLength = 1000.0f;
    const float planeExtent = 128;
    const FVector planeCenter = FVector::ZeroVector;

    float GetTileSize(const UWfcTileset* optionalTileset)
    {
        return (optionalTileset == nullptr) ?
                   1000.0f :
                   optionalTileset->TileLength;
    }

    float GetFaceThickness(float tileSize) { return tileSize / 50.0f; }
    float GetCornerSphereRadius(float tileSize) { return tileSize / 100.0f; }

    auto GetLabelColor(WFC::Tiled3D::Directions3D face)
    {
        return axisColors[face].ToFColor(true);
    }
    auto GetLabelThickness(float tileSize) { return 35.0f * (tileSize / 1000.0f); }

    auto ConvertVec(const WFC::Vector3i& v) { return FVector(v.x, v.y, v.z); }

    FString MakePointLabel(WFC::Tiled3D::FacePoints corner, const FString& symmetryName)
    {
        const auto* cornerText = TEXT("ERROR");
        switch (corner)
        {
            case WFC::Tiled3D::FacePoints::AA: cornerText = TEXT("AA"); break;
            case WFC::Tiled3D::FacePoints::AB: cornerText = TEXT("AB"); break;
            case WFC::Tiled3D::FacePoints::BA: cornerText = TEXT("BA"); break;
            case WFC::Tiled3D::FacePoints::BB: cornerText = TEXT("BB"); break;
            default: check(false);
        }
        return FString{ cornerText } + TEXT(": ") + symmetryName;
    }
}


void FWfcTilesetEditorScene::InitializeFacePointViz(WFC::Tiled3D::Directions3D face,
                                                    FacePointViz& inOutData)
{
    const auto& color = axisColors[face];
    
    auto* sphere = NewObject<USphereComponent>(GetTransientPackage());
    AddComponent(sphere, FTransform());
    inOutData.Shape = sphere;
    sphere->SetWorldLocation(inOutData.Pos);
    sphere->ShapeColor = color.ToFColor(false);

    //Put some text next to the sphere, indicating which corner this is.
    auto* text = NewObject<UTextRenderComponent>();
    AddComponent(text, FTransform());
    inOutData.Label = text;
    text->TextRenderColor = GetLabelColor(face);
    text->SetWorldLocation(inOutData.Pos);
    text->HorizontalAlignment = EHTA_Center;
    text->VerticalAlignment = EVRTA_TextBottom;
    text->SetText(FText::FromString(MakePointLabel(inOutData.CornerType, TEXT("[null]"))));
}
void FWfcTilesetEditorScene::InitializeFaceViz(FaceViz& inOutData)
{
    //Set up a plane with our transparent material.
    static const TCHAR* const MaterialPaths[] = {
        //Order must match ordering of WFC::Tiled3D::Directions3D
        TEXT("/WFCpp2/Editor/MI_FacePlane_MinX.MI_FacePlane_MinX"),
        TEXT("/WFCpp2/Editor/MI_FacePlane_MaxX.MI_FacePlane_MaxX"),
        TEXT("/WFCpp2/Editor/MI_FacePlane_MinY.MI_FacePlane_MinY"),
        TEXT("/WFCpp2/Editor/MI_FacePlane_MaxY.MI_FacePlane_MaxY"),
        TEXT("/WFCpp2/Editor/MI_FacePlane_MinZ.MI_FacePlane_MinZ"),
        TEXT("/WFCpp2/Editor/MI_FacePlane_MaxZ.MI_FacePlane_MaxZ")
    };
    inOutData.Component = NewObject<UStaticMeshComponent>(GetTransientPackage());
    inOutData.Component->SetStaticMesh(LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Engine/EditorMeshes/EditorPlane.EditorPlane"),
        nullptr, LOAD_EditorOnly, nullptr
    ));
    inOutData.Component->bSelectable = false;
    inOutData.Component->SetMaterial(0, LoadObject<UMaterialInterface>(
        nullptr,
        MaterialPaths[static_cast<int>(inOutData.Dir)],
        nullptr, LOAD_EditorOnly
    ));

    //Calculate the transform for the box.
    //Note that the plane faces -X before transformation.
    static const FRotator FaceRotations[] = {
        //Order must match ordering of WFC::Tiled3D::Directions3D
        FRotator::ZeroRotator,
        FRotator{ 0, 180, 0 },
        FRotator{ 0, 90, 0 },
        FRotator{ 0, 270, 0 },
        FRotator{ 90, 0, 0 },
        FRotator{ 270, 0, 0 }
    };
    auto mainAxis = WFC::Tiled3D::GetAxisIndex(inOutData.Dir);
    //   1. Size:
    float thickness = GetFaceThickness(initialTileLength);
    //   2. Pos:
    inOutData.Pos = ConvertVec(WFC::Tiled3D::GetFaceDirection(inOutData.Dir)) * (initialTileLength / 2.0f);
    inOutData.Pos[mainAxis] += GetFaceThickness(initialTileLength) * 0.5f * FMath::Sign(inOutData.Pos[mainAxis]);
    auto planePos = inOutData.Pos;
    planePos += (thickness / 2.0f) * ConvertVec(WFC::Tiled3D::GetFaceDirection(inOutData.Dir));
    //   3. Final:
    FTransform boxTr{
        FaceRotations[static_cast<int>(inOutData.Dir)],
        planePos - planeCenter,
        FVector{ (initialTileLength / 2.0f) / planeExtent }
    };
    AddComponent(inOutData.Component, boxTr);
}


FWfcTilesetEditorScene::FWfcTilesetEditorScene(ConstructionValues cvs)
    : FAdvancedPreviewScene(cvs)
{
    auto& world = *GetWorld();
    auto& worldSettings = *world.GetWorldSettings();

    //Start up the world.
    worldSettings.NotifyBeginPlay();
    worldSettings.NotifyMatchStarted();
    worldSettings.SetActorHiddenInGame(false);
    world.bBegunPlay = true;

    //Configure the scene.
    SetFloorVisibility(false);
    
    //Set up the visualization of each face.
    for (int i = 0; i < WFC::Tiled3D::N_DIRECTIONS_3D; ++i)
    {
        auto face = static_cast<WFC::Tiled3D::Directions3D>(i);
        faces[i].Dir = face;
        InitializeFaceViz(faces[i]);

        for (int j = 0; j < WFC::Tiled3D::N_FACE_POINTS; ++j)
        {
            auto& cornerField = faces[i].Points[j];
            cornerField.CornerType = static_cast<WFC::Tiled3D::FacePoints>(j);
            InitializeFacePointViz(face, cornerField);
        }
    }

    faceLabel = NewObject<UTextRenderComponent>();
    // ReSharper disable once CppVirtualFunctionCallInsideCtor
    AddComponent(faceLabel.Get(), FTransform());
    faceLabel->TextRenderColor = FColor::Black;
    faceLabel->HorizontalAlignment = EHTA_Center;
    faceLabel->VerticalAlignment = EVRTA_TextBottom;
    faceLabel->TextRenderColor = faceNameColor.ToFColor(true);
    //Hide the label for now; it's positioned and labeled on Refresh().
    faceLabel->SetWorldLocation({ 0, 0, 0});
    faceLabel->SetText(FText::FromString(TEXT("")));
}

void FWfcTilesetEditorScene::Refresh(const UWfcTileset* tileset, TOptional<WfcTileID> tile, const FVector& camPos,
                                     FWfcTilesetEditorViewportClient* owner)
{
    check(owner);
    
    //If the referenced tile is nonexistent, don't select any tile.
    if (tileset != nullptr && tile.IsSet() && !tileset->Tiles.Contains(*tile))
    {
        UE_LOG(LogWFCppEditor, Error,
               TEXT("Scene is trying to visualize nonexistent tile %i from tileset %s"),
               *tile, *tileset->GetFullName());
        tile.Reset();
    }
    
    //Calculate properties in case the tileset/tile is null.
    float tileLength = (tileset == nullptr) ?
                           1000.0f :
                           tileset->TileLength;
    auto* tileData = (tileset == nullptr || !tile.IsSet()) ?
                         nullptr :
                         tileset->Tiles[*tile].Data;
    const FWfcTile* assetPOD;
    if (tile.IsSet())
        assetPOD = &tileset->Tiles[*tile];
    else
        assetPOD = nullptr;

    //Update the faces and points.
    for (auto& face : faces)
    {
        uint_fast8_t mainAxis, planeAxis1, planeAxis2;
        WFC::Tiled3D::GetAxes(face.Dir, mainAxis, planeAxis1, planeAxis2);

        //Get the tile's data for this face.
        const FWfcTileFace* assetFace;
        if (assetPOD)
            assetFace = &assetPOD->GetFace(face.Dir);
        else
            assetFace = nullptr;

        //Get the prototype for this face.
        const FWfcFacePrototype* assetFacePrototype;
        if (assetFace == nullptr)
            assetFacePrototype = nullptr;
        else if (tileset->FacePrototypes.Contains(assetFace->PrototypeID))
            assetFacePrototype = &tileset->FacePrototypes[assetFace->PrototypeID];
        else
        {
            assetFacePrototype = nullptr;
            UE_LOG(LogWFCppEditor, Warning,
                   TEXT("Tile %i of tileset '%s' references a nonexistent face prototype %i"),
                   *tile, *tileset->GetFullName(), assetFace->PrototypeID)
        };
        
        //Update position.
        face.Pos = ConvertVec(WFC::Tiled3D::GetFaceDirection(face.Dir)) * (tileLength / 2.0f);
        face.Pos[mainAxis] += GetFaceThickness(tileLength) * 0.5f * FMath::Sign(face.Pos[mainAxis]);
        face.Component->SetWorldLocation(face.Pos - planeCenter);

        //Update size.
        FVector boxExtents(tileLength / 2.0f);
        // float thickness = GetFaceThickness(tileLength);
        // boxExtents[WFC::Tiled3D::GetAxisIndex(face.Dir)] = thickness / 2.0f;
        face.Component->SetRelativeScale3D(boxExtents / planeExtent);
        face.Component->MarkRenderTransformDirty();
        
        //Update individual corners of the face.
        for (auto& point : face.Points)
        {
            //Update position.
            FVector pointPos;
            point.Pos[mainAxis] = face.Pos[mainAxis] * 1.25f;
            point.Pos[planeAxis1] = (tileLength / 2.0f) *
                                    (WFC::Tiled3D::IsFirstMin(point.CornerType) ? -1 : 1);
            point.Pos[planeAxis2] = (tileLength / 2.0f) *
                                    (WFC::Tiled3D::IsSecondMin(point.CornerType) ? -1 : 1);
            point.Shape->SetWorldLocation(point.Pos);
            point.Label->SetWorldLocation(point.Pos);

            //Update size.
            point.Shape->SetSphereRadius(GetCornerSphereRadius(tileLength));

            //Update labels.
            point.Label->SetWorldSize(GetLabelThickness(tileLength));
            point.Label->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(point.Pos, camPos));
            if (tileset != nullptr && tile.IsSet() && assetFacePrototype != nullptr)
            {
                auto prototypeCornerType = assetFace->GetPrototypeCorner(point.CornerType);
                auto pointSymmetry = assetFacePrototype->GetPointSymmetry(prototypeCornerType);
                const auto& pointName = assetFacePrototype->PointNicknames[static_cast<size_t>(pointSymmetry)];
                FString text = pointName.IsEmpty() ? pointName : MakePointLabel(point.CornerType, pointName);
                
                point.Label->SetText(FText::FromString(text));
            }
            else
            {
                point.Label->SetText(FText::FromString(TEXT("")));
            }
        }
    }

    //Update the tile visualization.
    if (tileData != chosenTileData)
    {
        //Clean up the old visualization.
        if (tileVisualizer)
        {
            tileVisualizer.Get()->TearDownVisualization(
                *this, *owner, *tileset,
                chosenTileIdx, *assetPOD,  chosenTileData.Get(),
                tileVizActors, tileVizLoneComponents
            );
        }
        for (auto oldComponent : tileVizLoneComponents)
            if (oldComponent.IsValid())
                RemoveComponent(oldComponent.Get());
        tileVizLoneComponents.Empty();
        for (auto oldActor : tileVizActors)
            WfcTilesetEditorUtils::DestroyPreviewSceneActor(oldActor.Get());
        tileVizActors.Empty();

        //Generate the new visualization.
        chosenTileIdx = *tile;
        chosenTileData = tileData;
        if (IsValid(tileData))
            tileVisualizer = FModuleManager::LoadModuleChecked<IWFCpp2UnrealEditorModule>("WFCpp2UnrealEditor").GetTileDataVisualizer(tileData->GetClass());
        else
            tileVisualizer = nullptr;
        if (tileVisualizer)
        {
            tileVisualizer.Get()->SetUpVisualization(
                *this, *owner, *tileset,
                chosenTileIdx, *assetPOD,  chosenTileData.Get(),
                tileVizActors, tileVizLoneComponents
            );
        }
    }

    //Update the face labeling.
    if (tile.IsSet() && IsValid(tileset))
    {
        //Find the face we are closest to.
        auto camPosAxisMagnitudes = camPos.GetAbs();
        int faceAxis;
        if (camPosAxisMagnitudes.X > camPosAxisMagnitudes.Y && camPosAxisMagnitudes.X > camPosAxisMagnitudes.Z)
            faceAxis = 0;
        else if (camPosAxisMagnitudes.Y > camPosAxisMagnitudes.X && camPosAxisMagnitudes.Y > camPosAxisMagnitudes.Z)
            faceAxis = 1;
        else
            faceAxis = 2;
        int faceDir = FMath::Sign(camPos[faceAxis]);
        auto faceEnum = WFC::Tiled3D::MakeDirection3D(camPos[faceAxis] < 0, faceAxis);

        //Move the label to the top of that face.
        FVector labelPos = FVector::ZeroVector;
        labelPos[faceAxis] = faceDir * tileLength / 2;
        labelPos[(faceAxis == 2 ? 0 : 2)] = tileLength / 2 + (GetCornerSphereRadius(tileLength));
        faceLabel->SetWorldLocation(labelPos);

        //Rotate it to look at the camera.
        faceLabel->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(labelPos, camPos));
        faceLabel->SetWorldSize(GetLabelThickness(tileLength) * 1.5f);

        //Update the label's text to the face prototype.
        auto& tileRef = tileset->Tiles[*tile];
        auto& face = tileRef.GetFace(faceEnum);
        FString labelText;
        if (tileset->FacePrototypes.Contains(face.PrototypeID))
            labelText = tileset->FacePrototypes[face.PrototypeID].Nickname;
        else
            labelText = TEXT("[null]");
        faceLabel->SetText(FText::FromString(labelText));
    }
    else
    {
        faceLabel->SetText(FText::FromString(""));
    }
}
