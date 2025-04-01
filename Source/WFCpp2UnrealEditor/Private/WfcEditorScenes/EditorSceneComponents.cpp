#include "WfcEditorScenes/EditorSceneComponents.h"

#include <WFC++/include/Tiled3D/Transform3D.h>

#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"


FEditorSceneComponent::FEditorSceneComponent(FPreviewScene* _owner, FTransform transform, TSubclassOf<UActorComponent> type)
	: owner(_owner)
{
	check(owner);

	//For some reason, we have to remove the scale from the transform when spawning,
	//    *then* set the scale afterwards.
	auto scale = transform.GetScale3D();
	transform.SetScale3D(FVector::OneVector);
	
	componentUntyped = NewObject<UActorComponent>(GetTransientPackage(), type);
	owner->AddComponent(componentUntyped.Get(), transform);

	if (auto* sceneComponent = Cast<USceneComponent>(componentUntyped))
		sceneComponent->SetRelativeScale3D(scale);

	//Not sure if this is really needed...
	componentUntyped->MarkRenderTransformDirty();
	componentUntyped->MarkRenderStateDirty();
}
FEditorSceneComponent::~FEditorSceneComponent()
{
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
	
	component->MarkRenderStateDirty();
}
FEditorArrowComponent::FEditorArrowComponent(FPreviewScene* owner,
											 const FTransform& transform,
											 const FColor& color)
	: TEditorSceneComponent<UArrowComponent>(owner, transform)
{
	auto* component = GetComponent();
	
	component->SetArrowColor(color);
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

	component->MarkRenderStateDirty();
}

FTransform FEditorArrowComponent::GetTransform(const FVector& base, const FVector& head,
											   double thickness)
{
	return {
		UKismetMathLibrary::ComposeRotators(
			UKismetMathLibrary::MakeRotFromX({ 1, 0, 0 }).GetInverse(),
			UKismetMathLibrary::MakeRotFromX(head - base)
		),
		base,
		FVector{ (head - base).Length(), thickness, thickness }
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
		rot.RotateVector({ 1, sizeRelative.X, sizeRelative.Y }).GetAbs()
	};
}
