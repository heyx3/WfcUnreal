#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/WorldSettings.h"


#pragma region Base classes

//A scoped owner of a component in a preview/editor scene.
//It does not have any owning actor.
struct WFCPP2UNREALEDITOR_API FEditorSceneComponent
{
public:
	
	FEditorSceneComponent(FPreviewScene* owner, const FTransform& transform,
						  TSubclassOf<UActorComponent> type);
	virtual ~FEditorSceneComponent();

	FEditorSceneComponent(const FEditorSceneComponent& cpy) = delete;
	FEditorSceneComponent& operator=(const FEditorSceneComponent& cpy) = delete;
	
	FEditorSceneComponent(FEditorSceneComponent&& src);
	FEditorSceneComponent& operator=(FEditorSceneComponent&& src);


	FPreviewScene* GetOwner() { return owner; }
	const FPreviewScene* GetOwner() const { return owner; }

	operator UActorComponent*() { return componentUntyped.Get(); }
	operator const UActorComponent*() const { return componentUntyped.Get(); }

protected:

	FPreviewScene* owner;
	TWeakObjectPtr<UActorComponent> componentUntyped;
};


template<typename TComponent>
struct TEditorSceneComponent : public FEditorSceneComponent
{
public:

	TEditorSceneComponent(FPreviewScene* owner, const FTransform& transform,
						  TSubclassOf<TComponent> type = TComponent::StaticClass())
		: FEditorSceneComponent(owner, transform, type)
	{
		
	}

	TComponent* GetComponent() const { return CastChecked<TComponent>(componentUntyped, ECastCheckedType::NullAllowed); }
	operator TComponent*() { return GetComponent(); }
	operator const TComponent*() const { return GetComponent(); }
};

#pragma endregion

struct WFCPP2UNREALEDITOR_API FEditorMeshComponent : public TEditorSceneComponent<UStaticMeshComponent>
{
	FEditorMeshComponent(FPreviewScene* owner,
						 UStaticMesh* mesh, const FTransform& transform,
						 UMaterialInterface* material = nullptr);
};
struct WFCPP2UNREALEDITOR_API FEditorWireSphereComponent : public TEditorSceneComponent<class USphereComponent>
{
	FEditorWireSphereComponent(FPreviewScene* owner, const FTransform& transform,
				   		       const FColor& color);
};
//The box's unrotated extents are equal to its local scale.
struct WFCPP2UNREALEDITOR_API FEditorWireBoxComponent : public TEditorSceneComponent<class UBoxComponent>
{
	FEditorWireBoxComponent(FPreviewScene* owner, const FTransform& transform,
				  		    const FColor& color);
	FEditorWireBoxComponent(FPreviewScene* owner,
					        const FBox& unrotatedArea, const FRotator& rot,
					        const FColor& color)
		: FEditorWireBoxComponent(owner, GetTransform(unrotatedArea, rot), color)
	{
		
	}

	static FTransform GetTransform(const FBox& unrotatedArea, const FRotator& rot)
	{
		return {
			rot,
			unrotatedArea.GetCenter(),
			unrotatedArea.GetExtent()
		};
	}
};
//The plane's horizontal extents are equal to its local scale.
struct WFCPP2UNREALEDITOR_API FEditorPlaneComponent : public TEditorSceneComponent<class UStaticMeshComponent>
{
	FEditorPlaneComponent(FPreviewScene* owner, const FTransform& transform,
						  UMaterialInterface* material = nullptr);
	FEditorPlaneComponent(FPreviewScene* owner,
						  const FVector& origin, const FVector2D& planeSize, const FVector& normal,
						  UMaterialInterface* material = nullptr)
		: FEditorPlaneComponent(owner, GetTransform(origin, planeSize, normal), material)
	{
		
	}

	static FTransform GetTransform(const FVector& origin, const FVector2D& planeSize, const FVector& normal);
};

struct WFCPP2UNREALEDITOR_API FEditorArrowComponent : public TEditorSceneComponent<class UArrowComponent>
{
	FEditorArrowComponent(FPreviewScene* owner, const FTransform& transform,
					      const FColor& color);
	
	FEditorArrowComponent(FPreviewScene* owner,
						  const FVector& base, const FVector& head,
						  double thickness, const FColor& color)
		: FEditorArrowComponent(owner, GetTransform(base, head, thickness), color)
	{
		
	}
	
	static FTransform GetTransform(const FVector& base, const FVector& head, double thickness);
};

struct WFCPP2UNREALEDITOR_API FEditorTextComponent : public TEditorSceneComponent<UTextRenderComponent>
{
	FEditorTextComponent(FPreviewScene* owner, const FTransform& transform,
						 const FString& contents, const FColor& color,
						 EHorizTextAligment alignHorz = EHTA_Center,
						 EVerticalTextAligment alignVert = EVRTA_TextCenter);
};