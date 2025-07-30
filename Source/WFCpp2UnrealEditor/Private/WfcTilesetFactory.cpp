#include "WfcTilesetFactory.h"

#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "WfcAssetFactory"


DECLARE_DELEGATE_OneParam(FOnGeneratorSelection, TSubclassOf<UWfcTilesetGenerator>);

class SWfcTilesetConfigureWindow : public SWindow
{
public:

	SLATE_BEGIN_ARGS(SWfcTilesetConfigureWindow)
		{}
		SLATE_ARGUMENT(TSubclassOf<UWfcTilesetGenerator>, GeneratorType)
		SLATE_ARGUMENT(FOnGeneratorSelection, OnGeneratorSelected)
		SLATE_ARGUMENT(FSimpleDelegate, OnCanceled)
	SLATE_END_ARGS()

	TSubclassOf<UWfcTilesetGenerator> GeneratorType;
	FOnGeneratorSelection OnGeneratorSelected;
	FSimpleDelegate OnCanceled;

	void Construct(const FArguments& args)
	{
		GeneratorType = args._GeneratorType;
		OnGeneratorSelected = args._OnGeneratorSelected;
		OnCanceled = args._OnCanceled;

		SWindow::Construct(SWindow::FArguments()
			.Title(LOCTEXT("WfcNewTilesetConfigureTitle", "Create WFC Tileset"))
			.SizingRule(ESizingRule::UserSized)
			.ClientSize(FVector2D(500, 600))
			.SupportsMinimize(false)
			.SupportsMaximize(false)
		[
			SNew(SBorder)
			   .BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SNew(SVerticalBox)
				
				+SVerticalBox::Slot()
				    .FillHeight(0.6)
				[
					SNullWidget::NullWidget //TODO: Subclass selector
				]

				//TODO: Editor for the generator instance's ExposeOnSpawn parameters

				+SVerticalBox::Slot()
				    .FillHeight(0.4)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
							.Text(LOCTEXT("Accept", "Accept"))
							.HAlign(HAlign_Center)
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
							.OnClicked_Raw(this, &SWfcTilesetConfigureWindow::OnAccept)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						    .Text(LOCTEXT("Cancel", "Cancel"))
							.HAlign(HAlign_Center)
						    .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						    .OnClicked_Raw(this, &SWfcTilesetConfigureWindow::OnCancel)
					]
				]
			]
		]);
	}
	FReply OnAccept()
	{
		OnGeneratorSelected.Execute(GeneratorType);
		RequestDestroyWindow();
		return FReply::Handled();
	}
	FReply OnCancel()
	{
		OnCanceled.Execute();
		RequestDestroyWindow();
		return FReply::Handled();
	}
};

bool UWfcTilesetFactory::ConfigureProperties()
{
	bool didCancel = false;
	auto window = SNew(SWfcTilesetConfigureWindow)
		.GeneratorType(Generator)
		.OnGeneratorSelected(FOnGeneratorSelection::CreateUObject(this, &UWfcTilesetFactory::OnGeneratorSelectedInWindow))
		.OnCanceled(FSimpleDelegate::CreateLambda([&]() { didCancel = true; }))
	;
	
	GEditor->EditorAddModalWindow(window);
	return !didCancel;
}

UObject* UWfcTilesetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent,
											  FName InName, EObjectFlags Flags,
											  UObject* Context, FFeedbackContext* Warn)
{
	auto* tileset = NewObject<UWfcTileset>(InParent, InClass, InName, Flags);
	if (Generator.Get())
		CastChecked<UWfcTilesetGenerator>(Generator->ClassDefaultObject)->GenerateToExistingInstance(tileset);
	return tileset;
}
