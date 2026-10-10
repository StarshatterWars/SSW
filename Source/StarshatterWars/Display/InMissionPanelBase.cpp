#include "InMissionPanelBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/NativeWidgetHost.h"
#include "Engine/Texture2D.h"
#include "Widgets/SNullWidget.h"
#include "InputCoreTypes.h"
#include "Sim.h"
#include "Ship.h"

TSharedRef<SWidget> UInMissionPanelBase::RebuildWidget()
{
    if(WidgetTree && !WidgetTree->RootWidget) {
        auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();
        WidgetTree->RootWidget=Root;
        auto* Frame=WidgetTree->ConstructWidget<UBorder>();
        auto* FrameSlot=Root->AddChildToCanvas(Frame);
        FrameSlot->SetAnchors(FAnchors(0,0,1,1));FrameSlot->SetOffsets(FMargin(0));
        if(auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Frame3c.Frame3c")))Frame->SetBrushFromTexture(Texture);
        Frame->SetBrushColor(FLinearColor(1,1,1,0.4f));
        auto* Interior=WidgetTree->ConstructWidget<UCanvasPanel>();Frame->SetContent(Interior);
        RuntimeHost=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("RuntimeHost"));Interior->AddChildToCanvas(RuntimeHost);
        PanelTitle=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("PanelTitle"));
        auto* TitleSlot=Interior->AddChildToCanvas(PanelTitle);
        TitleSlot->SetAnchors(FAnchors(0,0,1,0));TitleSlot->SetOffsets(FMargin(24,48,24,32));
        PanelTitle->SetJustification(ETextJustify::Center);
    }
    return Super::RebuildWidget();
}
void UInMissionPanelBase::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    if(!RuntimeHost)RuntimeHost=Cast<USizeBox>(GetWidgetFromName(TEXT("RuntimeHost")));
    if(!PanelTitle)PanelTitle=Cast<UTextBlock>(GetWidgetFromName(TEXT("PanelTitle")));
    if(PanelTitle)PanelTitle->SetText(GetPanelCaption());
    if(!RuntimeHost || !WidgetTree){UE_LOG(LogTemp,Error,TEXT("%s requires SizeBox RuntimeHost"),*GetName());return;}
    if (UCanvasPanelSlot* HostCanvasSlot = Cast<UCanvasPanelSlot>(RuntimeHost->Slot))
    {
        HostCanvasSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        HostCanvasSlot->SetAlignment(FVector2D::ZeroVector);
        HostCanvasSlot->SetOffsets(ContentInsets);
        HostCanvasSlot->SetAutoSize(false);
    }
    RuntimeHost->SetVisibility(ESlateVisibility::Visible);
    RuntimeHost->SetClipping(EWidgetClipping::ClipToBounds);
    if(!GeneratedContent)GeneratedContent=WidgetTree->ConstructWidget<UNativeWidgetHost>();
    GeneratedContent->SetContent(CreatePanelContent());
    RuntimeHost->SetContent(GeneratedContent);
}
void UInMissionPanelBase::NativeDestruct()
{
    if(GeneratedContent)GeneratedContent->SetContent(SNullWidget::NullWidget);
    OnPanelClosed.Unbind();
    Super::NativeDestruct();
}
void UInMissionPanelBase::RequestPanelClose(){if(OnPanelClosed.IsBound())OnPanelClosed.Execute();else RemoveFromParent();}
FReply UInMissionPanelBase::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){
    if(E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::Delete){RequestPanelClose();return FReply::Handled();}
    return Super::NativeOnKeyDown(G,E);
}
TSharedRef<SWidget> UInMissionPanelBase::CreatePanelContent(){return SNullWidget::NullWidget;}
FText UInMissionPanelBase::GetPanelCaption() const{return FText::FromString(TEXT("Mission Panel"));}
Ship* UInMissionPanelBase::ResolvePanelShip() const {
    auto* S=Sim::GetSim();auto* P=S && S->GetMission()?S->GetPlayerShip():nullptr;
    return P && !P->IsDead()?P:nullptr;
}
