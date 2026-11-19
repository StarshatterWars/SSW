#include "RecordDetails.h"
#include "MissionUIStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
namespace {
const FLinearColor Accent(.31f,.77f,.89f), Muted(.50f,.63f,.69f), White(.88f,.94f,.97f);
const FSlateBrush& Card() { static const FSlateRoundedBoxBrush B(FLinearColor(.035f,.065f,.082f,1),4.f,FLinearColor(.11f,.21f,.26f,1),1.f); return B; }
const FSlateBrush& Badge() { static const FSlateRoundedBoxBrush B(FLinearColor(.045f,.14f,.17f,1),4.f); return B; }
FLinearColor StatusColor(const FText& Status) {
    const FString S=Status.ToString().ToUpper();
    if(S.Contains(TEXT("ILLICIT")) || S.Contains(TEXT("FAILED")) || S.Contains(TEXT("EXPIRED"))) return FLinearColor(1,.51f,.40f);
    if(S.Contains(TEXT("LOCKED")) || S.Contains(TEXT("AWAITING")) || S.Contains(TEXT("PRODUCING"))) return FLinearColor(.95f,.76f,.36f);
    if(S.Contains(TEXT("READY")) || S.Contains(TEXT("COMPLETE")) || S.Contains(TEXT("AVAILABLE"))) return FLinearColor(.47f,.86f,.68f);
    return Accent;
}
}
void SRecordDetails::Construct(const FArguments& Args) {
    Record=Args._Record;
    ChildSlot[SAssignNew(Content,SVerticalBox)];
    Refresh(2);
}
void SRecordDetails::Tick(const FGeometry& Geometry,double Time,float Delta) {
    SCompoundWidget::Tick(Geometry,Time,Delta);
    if(Time<NextRefresh) return;
    NextRefresh=Time+.25;
    const float Width=Geometry.GetLocalSize().X;
    Refresh(Width>=1100?4:Width>=760?3:Width>=430?2:1);
}
void SRecordDetails::Refresh(int32 Columns) {
    const auto Data=Record.Get();
    FString Key=Data.Title.ToString()+TEXT("\x1f")+Data.Subtitle.ToString()+TEXT("\x1f")+Data.Status.ToString()+TEXT("\x1f")+Data.Description.ToString()+FString::FromInt(int32(Data.Icon));
    for(const auto& Field:Data.Fields) Key+=TEXT("\x1f")+Field.Label.ToString()+TEXT("\x1e")+Field.Value.ToString();
    for(const auto& Section:Data.Sections) Key+=TEXT("\x1f")+Section.Heading.ToString()+TEXT("\x1e")+Section.Body.ToString();
    if(Key==Signature && Columns==LastColumns) return;
    Signature=MoveTemp(Key);LastColumns=Columns;Content->ClearChildren();
    auto Heading=SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Data.Title.IsEmpty()?INVTEXT("SELECT A RECORD"):Data.Title.ToUpper()).Font(MissionUIStyle::GetHeaderFont(16)).ColorAndOpacity(White).AutoWrapText(true)];
    if(!Data.Subtitle.IsEmpty()) Heading->AddSlot().AutoHeight().Padding(0,3,0,0)[SNew(STextBlock).Text(Data.Subtitle.ToUpper()).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)];
    auto Header=SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(SBox).WidthOverride(32).HeightOverride(32)[SNew(SShipSystemIcon).Icon(Data.Icon)]]
        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Heading];
    if(!Data.Status.IsEmpty()) Header->AddSlot().AutoWidth().VAlign(VAlign_Top).Padding(12,0,0,0)
        [SNew(SBorder).BorderImage(&Badge()).Padding(FMargin(10,5))[SNew(STextBlock).Text(Data.Status.ToUpper()).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(StatusColor(Data.Status))]];
    Content->AddSlot().AutoHeight().Padding(0,0,0,10)[Header];
    if(!Data.Description.IsEmpty()) Content->AddSlot().AutoHeight().Padding(0,0,0,12)
        [SNew(STextBlock).Text(Data.Description).Font(MissionUIStyle::GetRowFont(12)).ColorAndOpacity(Muted).AutoWrapText(true).LineHeightPercentage(1.18f)];
    if(!Data.Fields.IsEmpty()) {
        auto Grid=SNew(SGridPanel);
        for(int32 C=0;C<Columns;++C) Grid->SetColumnFill(C,1.f);
        for(int32 I=0;I<Data.Fields.Num();++I) {
            const auto& Field=Data.Fields[I];
            Grid->AddSlot(I%Columns,I/Columns).Padding(FMargin(0,0,I%Columns<Columns-1?8:0,8))
                [SNew(SBorder).BorderImage(&Card()).Padding(FMargin(10,8))
                    [SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Field.Label.ToUpper()).Font(MissionUIStyle::GetRowFont(10)).ColorAndOpacity(Muted)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,3,0,0)[SNew(STextBlock).Text(Field.Value.IsEmpty()?INVTEXT("—"):Field.Value.ToUpper()).Font(MissionUIStyle::GetRowFont(12)).ColorAndOpacity(White).AutoWrapText(true).WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)]]];
        }
        Content->AddSlot().AutoHeight()[Grid];
    }
    for(const auto& Section:Data.Sections) Content->AddSlot().AutoHeight().Padding(0,6,0,6)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)[SNew(STextBlock).Text(Section.Heading.ToUpper()).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(Accent)]
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Section.Body).Font(MissionUIStyle::GetRowFont(12)).ColorAndOpacity(White).AutoWrapText(true).LineHeightPercentage(1.18f)]];
}
