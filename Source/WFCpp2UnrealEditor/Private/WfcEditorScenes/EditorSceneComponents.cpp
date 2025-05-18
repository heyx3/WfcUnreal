#include "WfcEditorScenes/EditorSceneComponents.h"

#include <WFC++/include/Tiled3D/Transform3D.h>

#include "WFCpp2UnrealEditor.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"


FEditorSceneComponent::FEditorSceneComponent(FPreviewScene* _owner, const FTransform& transform, TSubclassOf<UActorComponent> type)
	: owner(_owner)
{
	check(owner);

	componentUntyped = NewObject<UActorComponent>(GetTransientPackage(), type);
	if (auto* sceneComponent = Cast<USceneComponent>(componentUntyped))
	{
		sceneComponent->SetVisibility(true);
		sceneComponent->SetHiddenInGame(false);
		sceneComponent->SetMobility(EComponentMobility::Type::Movable);
		sceneComponent->MarkRenderStateDirty();
	}

	owner->AddComponent(componentUntyped.Get(), transform);
}
FEditorSceneComponent::~FEditorSceneComponent()
{
	//NOTE: the usual DestroyComponent() can lead to crashes when closing the editor scene.
	if (componentUntyped.IsValid())
		owner->RemoveComponent(componentUntyped.Get());
	
	//In debug builds, null out the component pointer.
	#if DO_CHECK
		componentUntyped = nullptr;
	#endif
}

FEditorSceneComponent::FEditorSceneComponent(FEditorSceneComponent&& src)
	: owner(src.owner), componentUntyped(src.componentUntyped)
{
	check(&src != this);
	src.componentUntyped = nullptr;
}
FEditorSceneComponent& FEditorSceneComponent::operator=(FEditorSceneComponent&& src)
{
	if (&src != this)
		new (this) FEditorSceneComponent(MoveTemp(src));
	return *this;
}

FEditorMeshComponent::FEditorMeshComponent(FPreviewScene* owner,
									       UStaticMesh* mesh, const FTransform& transform,
									       UMaterialInterface* material)
	: TEditorSceneComponent<UStaticMeshComponent>(owner, transform)
{
	auto* component = GetComponent();
	
	component->SetStaticMesh(mesh);
	if (IsValid(material))
		component->SetMaterial(0, material);
	component->MarkRenderStateDirty();
}
FEditorWireSphereComponent::FEditorWireSphereComponent(FPreviewScene* owner,
												       const FTransform& transform,
											           const FColor& color)
    : TEditorSceneComponent<USphereComponent>(owner, transform)
{
	auto* component = GetComponent();

	component->ShapeColor = color;
	component->SetSphereRadius(1.0f);
}
FEditorWireBoxComponent::FEditorWireBoxComponent(FPreviewScene* owner,
										         const FTransform& transform,
										         const FColor& color)
	: TEditorSceneComponent<UBoxComponent>(owner, transform)
{
	auto* component = GetComponent();

	component->ShapeColor = color;
	component->SetBoxExtent(FVector::OneVector);
}
FEditorPlaneComponent::FEditorPlaneComponent(FPreviewScene* owner, const FTransform& transform,
											 UMaterialInterface* material)
	: TEditorSceneComponent<UStaticMeshComponent>(owner, transform)
{
	auto* component = GetComponent();

	component->SetStaticMesh(LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/EditorMeshes/EditorPlane.EditorPlane"),
		nullptr, LOAD_EditorOnly, nullptr
	));
	if (IsValid(material))
		component->SetMaterial(0, material);
}
FEditorArrowComponent::FEditorArrowComponent(FPreviewScene* owner,
											 const FTransform& transform,
											 const FColor& color,
											 UMaterialInterface* material)
	: TEditorSceneComponent<UArrowComponent>(owner,
											 //The transform's scaleX is interpreted as arrow length;
											 //   scaleY/Z are interpreted as arrow size.
											 FTransform{ transform.GetRotation(), transform.GetLocation(),
														 FVector::OneVector })
{
	auto* component = GetComponent();
	
	float desiredLength = transform.GetScale3D().X;
	float desiredThickness = FMath::Sqrt(FMath::Abs(transform.GetScale3D().Y * transform.GetScale3D().Z));
	
	component->SetArrowColor(color);
	if (IsValid(material))
		component->SetMaterial(0, material);
	component->SetArrowSize(desiredThickness);
	component->SetArrowLength(desiredLength);
	component->SetVisibility(true);
	component->bTreatAsASprite = false;
	component->bHiddenInGame = false;
	component->bIsScreenSizeScaled = false;
}
FEditorTextComponent::FEditorTextComponent(FPreviewScene* owner, const FTransform& transform,
										   const FString& contents, const FColor& color,
										   EHorizTextAligment alignHorz, EVerticalTextAligment alignVert)
    : TEditorSceneComponent<UTextRenderComponent>(owner, transform)
{
	auto* component = GetComponent();

	component->TextRenderColor = color;
	component->HorizontalAlignment = alignHorz;
	component->VerticalAlignment = alignVert;
	component->SetText(FText::FromString(contents));
	component->SetWorldSize(100.0f);
}

FTransform FEditorArrowComponent::GetTransform(const FVector& base, const FVector& head,
											   double thickness)
{
	double dist = (head - base).Length();
	return {
		UKismetMathLibrary::ComposeRotators(
			UKismetMathLibrary::MakeRotFromX({ 1, 0, 0 }).GetInverse(),
			UKismetMathLibrary::MakeRotFromX(head - base)
		),
		base,
		FVector{ dist / thickness, thickness, thickness }
	};
}
FTransform FEditorPlaneComponent::GetTransform(const FVector& origin, const FVector2D& planeSize, const FVector& normal)
{
	FRotator rot = UKismetMathLibrary::ComposeRotators(
		UKismetMathLibrary::MakeRotFromX({ -1, 0, 0 }).GetInverse(),
		UKismetMathLibrary::MakeRotFromX(normal)
	);
	auto sizeRelative = planeSize / 128.0;
	return FTransform{
		rot, origin,
		FVector{ -1, sizeRelative.X, sizeRelative.Y }.GetAbs()
	};
}
