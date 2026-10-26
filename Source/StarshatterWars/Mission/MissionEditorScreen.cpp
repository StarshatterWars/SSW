#include "MissionEditorScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "MissionUIStyle.h"

UMissionEditorScreen::UMissionEditorScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UMissionEditorScreen::BindFormWidgets()
{
    // The FORM bridge must not replace the pointers assigned by BuildNativeLayout.
    // The base class continues to own validation, button delegates, parsing, and Save.
}

TSharedRef<SWidget> UMissionEditorScreen::RebuildWidget()
{
    // UUserWidget turns WidgetTree into Slate before NativeConstruct runs.
    // Build the native controls first, or an empty Blueprint canvas is rendered.
    BuildNativeLayout();
    return Super::RebuildWidget();
}

void UMissionEditorScreen::NativeConstruct()
{
    Super::NativeConstruct();

    // Native controls are created before Slate reconstruction. Re-enable and
    // explicitly make text inputs editable after the base editor initializes.
    for (UEditableTextBox* Input : { TxtName, TxtDescription, TxtSituation, TxtObjective })
    {
        if (Input)
        {
            Input->SetIsEnabled(true);
            Input->SetIsReadOnly(false);
        }
    }
    for (UComboBoxString* Choice : { CmbType, CmbSystem, CmbRegion })
    {
        if (Choice) Choice->SetIsEnabled(true);
    }
}


void UMissionEditorScreen::BuildNativeLayout()
{
    if (!WidgetTree || WidgetTree->FindWidget(TEXT("SSW_MissionEditorRoot"))) return;

    const FLinearColor Back(0.012f, 0.025f, 0.045f, 1.f);
    const FLinearColor Panel(0.025f, 0.055f, 0.083f, 1.f);
    const FLinearColor TitleBack(0.105f, 0.113f, 0.125f, 1.f);
    const FLinearColor Ink(0.93f, 0.96f, 0.99f, 1.f);
    const FLinearColor Accent(0.16f, 0.77f, 0.91f, 1.f);

    auto Label = [&](const FString& Value, int32 Size, bool bTitle = false) -> UTextBlock*
    {
        UTextBlock* W = WidgetTree->ConstructWidget<UTextBlock>();
        W->SetText(FText::FromString(Value));
        W->SetVisibility(ESlateVisibility::HitTestInvisible);
        W->SetColorAndOpacity(FSlateColor(bTitle ? Ink : Accent));
        W->SetFont(bTitle ? MissionUIStyle::GetHeaderFont(Size) : MissionUIStyle::GetInfoLabelFont(Size));
        return W;
    };
    auto Border = [&](const FLinearColor& Color) -> UBorder*
    {
        UBorder* W = WidgetTree->ConstructWidget<UBorder>();
        W->SetBrushColor(Color);
        W->SetPadding(FMargin(14.f));
        return W;
    };
    auto Button = [&](const FString& Text, const FName Name, UButton*& Output) -> UButton*
    {
        Output = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        UTextBlock* Caption = Label(Text, 15);
        Caption->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Output->AddChild(Caption)))
        {
            ContentSlot->SetPadding(FMargin(14.f, 8.f));
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
        }
        return Output;
    };
    auto Field = [&](UVerticalBox* Parent, const TCHAR* Name, UEditableTextBox*& Output, bool bMultiline = false)
    {
        Parent->AddChildToVerticalBox(Label(Name, 14))->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
        Output = WidgetTree->ConstructWidget<UEditableTextBox>();
        Output->SetHintText(FText::FromString(Name));
        Output->SetIsReadOnly(false);
        Output->SetIsEnabled(true);
        Output->SetSelectAllTextWhenFocused(false);
        Output->SetClearKeyboardFocusOnCommit(false);
        Output->SetMinDesiredWidth(180.f);
        // Keep UEditableTextBox: the legacy editor reads this exact control type.
        Parent->AddChildToVerticalBox(Output)->SetPadding(FMargin(0.f, 0.f, 0.f, bMultiline ? 10.f : 3.f));
    };
    auto Combo = [&](UVerticalBox* Parent, const TCHAR* Name, UComboBoxString*& Output)
    {
        Parent->AddChildToVerticalBox(Label(Name, 14))->SetPadding(FMargin(0.f, 4.f, 0.f, 3.f));
        Output = WidgetTree->ConstructWidget<UComboBoxString>();
        Output->SetIsEnabled(true);
        // Taller compact selectors with a more generous drop-down list.
        Output->SetMaxListHeight(360.f);
        USizeBox* ComboHeight = WidgetTree->ConstructWidget<USizeBox>();
        ComboHeight->SetMinDesiredHeight(48.f);
        ComboHeight->SetContent(Output);
        Parent->AddChildToVerticalBox(ComboHeight)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
    };

    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (Root) Root->ClearChildren();
    else
    {
        Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SSW_MissionEditorRoot"));
        WidgetTree->RootWidget = Root;
    }
    Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UBorder* Background = Border(Back);
    if (UCanvasPanelSlot* RootSlot = Root->AddChildToCanvas(Background))
    {
        RootSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        RootSlot->SetOffsets(FMargin(0.f));
    }
    UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
    Background->SetContent(Main);
    Background->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UBorder* Heading = Border(TitleBack);
    Heading->SetContent(Label(TEXT("MISSION EDITOR"), 34, true));
    Main->AddChildToVerticalBox(Heading)->SetPadding(FMargin(12.f, 12.f, 12.f, 12.f));

    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(12.f, 0.f, 12.f, 10.f));
    auto Tab = [&](const TCHAR* TabCaption, const TCHAR* Id, UButton*& Target)
    {
        Tabs->AddChildToHorizontalBox(Button(TabCaption, FName(Id), Target))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    };
    Tab(TEXT("SIT"), TEXT("BtnSit"), BtnSit);
    Tab(TEXT("PKG"), TEXT("BtnPkg"), BtnPkg);
    Tab(TEXT("MAP"), TEXT("BtnMap"), BtnMap);

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Main->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Body);

    UBorder* DetailsPanel = Border(Panel);
    Body->AddChildToVerticalBox(DetailsPanel)->SetPadding(FMargin(12.f, 0.f, 12.f, 12.f));
    UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>();
    DetailsPanel->SetContent(Details);
    DetailsPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    // Two-column properties save the vertical space used by four stacked dropdowns.
    UHorizontalBox* PropertyTop = WidgetTree->ConstructWidget<UHorizontalBox>();
    Details->AddChildToVerticalBox(PropertyTop);
    UVerticalBox* NameColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBox* TypeColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    PropertyTop->AddChildToHorizontalBox(NameColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill))->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    PropertyTop->AddChildToHorizontalBox(TypeColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Field(NameColumn, TEXT("Mission name"), TxtName);
    Combo(TypeColumn, TEXT("Mission type"), CmbType);
    // The legacy editor reads mission type as an index, not as a string value.
    // Insert choices in their original enum order.
    if (CmbType)
    {
        CmbType->ClearOptions();
        CmbType->AddOption(TEXT("Patrol"));
        CmbType->AddOption(TEXT("Sweep"));
        CmbType->AddOption(TEXT("Intercept"));
        CmbType->AddOption(TEXT("Airborne Patrol"));
        CmbType->AddOption(TEXT("Airborne Sweep"));
        CmbType->AddOption(TEXT("Airborne Intercept"));
        CmbType->AddOption(TEXT("Strike"));
        CmbType->AddOption(TEXT("Assault"));
        CmbType->AddOption(TEXT("Defend"));
        CmbType->AddOption(TEXT("Escort"));
        CmbType->AddOption(TEXT("Freight Escort"));
        CmbType->AddOption(TEXT("Shuttle Escort"));
        CmbType->AddOption(TEXT("Strike Escort"));
        CmbType->AddOption(TEXT("Intel"));
        CmbType->AddOption(TEXT("Scout"));
        CmbType->AddOption(TEXT("Recon"));
        CmbType->AddOption(TEXT("Blockade"));
        CmbType->AddOption(TEXT("Fleet"));
        CmbType->AddOption(TEXT("Attack"));
        CmbType->AddOption(TEXT("Flight Ops"));
        CmbType->AddOption(TEXT("Transport"));
        CmbType->AddOption(TEXT("Cargo"));
        CmbType->AddOption(TEXT("Training"));
        CmbType->AddOption(TEXT("Misc"));
        CmbType->SetSelectedIndex(0);
    }
    UHorizontalBox* LocationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Details->AddChildToVerticalBox(LocationRow);
    UVerticalBox* SystemColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBox* RegionColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    LocationRow->AddChildToHorizontalBox(SystemColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill))->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    LocationRow->AddChildToHorizontalBox(RegionColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Combo(SystemColumn, TEXT("System"), CmbSystem);
    Combo(RegionColumn, TEXT("Region"), CmbRegion);

    UBorder* SitPanel = Border(Panel);
    Body->AddChildToVerticalBox(SitPanel)->SetPadding(FMargin(12.f, 0.f, 12.f, 12.f));
    UVerticalBox* Sit = WidgetTree->ConstructWidget<UVerticalBox>();
    SitPanel->SetContent(Sit);
    SitPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Field(Sit, TEXT("Description"), TxtDescription, true);
    Field(Sit, TEXT("Situation"), TxtSituation, true);
    Field(Sit, TEXT("Objective"), TxtObjective, true);

    UBorder* PkgPanel = Border(Panel);
    Body->AddChildToVerticalBox(PkgPanel)->SetPadding(FMargin(12.f, 0.f, 12.f, 12.f));
    UVerticalBox* Pkg = WidgetTree->ConstructWidget<UVerticalBox>();
    PkgPanel->SetContent(Pkg);
    PkgPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Pkg->AddChildToVerticalBox(Label(TEXT("ELEMENTS"), 18));
    LstElem = WidgetTree->ConstructWidget<UListView>();
    Pkg->AddChildToVerticalBox(LstElem);
    UHorizontalBox* ElementActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Pkg->AddChildToVerticalBox(ElementActions);
    auto Action = [&](UHorizontalBox* Row, const TCHAR* Caption, const TCHAR* Id, UButton*& Target)
    {
        Row->AddChildToHorizontalBox(Button(Caption, FName(Id), Target))->SetPadding(FMargin(0.f, 5.f, 7.f, 6.f));
    };
    Action(ElementActions, TEXT("ADD"), TEXT("BtnElemAdd"), BtnElemAdd);
    Action(ElementActions, TEXT("EDIT"), TEXT("BtnElemEdit"), BtnElemEdit);
    Action(ElementActions, TEXT("DELETE"), TEXT("BtnElemDel"), BtnElemDel);
    Action(ElementActions, TEXT("UP"), TEXT("BtnElemInc"), BtnElemInc);
    Action(ElementActions, TEXT("DOWN"), TEXT("BtnElemDec"), BtnElemDec);
    Pkg->AddChildToVerticalBox(Label(TEXT("EVENTS"), 18));
    LstEvent = WidgetTree->ConstructWidget<UListView>();
    Pkg->AddChildToVerticalBox(LstEvent);
    UHorizontalBox* EventActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Pkg->AddChildToVerticalBox(EventActions);
    Action(EventActions, TEXT("ADD"), TEXT("BtnEventAdd"), BtnEventAdd);
    Action(EventActions, TEXT("EDIT"), TEXT("BtnEventEdit"), BtnEventEdit);
    Action(EventActions, TEXT("DELETE"), TEXT("BtnEventDel"), BtnEventDel);
    Action(EventActions, TEXT("UP"), TEXT("BtnEventInc"), BtnEventInc);
    Action(EventActions, TEXT("DOWN"), TEXT("BtnEventDec"), BtnEventDec);

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Footer)->SetPadding(FMargin(12.f));
    auto FooterButton = [&](const TCHAR* Caption, const TCHAR* WidgetName, UButton*& Target)
    {
        USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>();
        WidthBox->SetWidthOverride(210.f);
        WidthBox->SetHeightOverride(46.f);
        WidthBox->SetContent(Button(Caption, FName(WidgetName), Target));
        if (UHorizontalBoxSlot* FooterSlot = Footer->AddChildToHorizontalBox(WidthBox))
        {
            FooterSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
        }
    };
    FooterButton(TEXT("CANCEL"), TEXT("BtnCancel"), BtnCancel);
    FooterButton(TEXT("SAVE MISSION"), TEXT("BtnAccept"), BtnAccept);
}
