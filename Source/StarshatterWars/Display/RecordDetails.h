#pragma once
#include "CoreMinimal.h"
#include "ShipSystemIcon.h"
#include "Widgets/SCompoundWidget.h"

struct FRecordDetailField { FText Label; FText Value; };
struct FRecordDetailSection { FText Heading; FText Body; };
struct FRecordDetails
{
    FText Title, Subtitle, Status, Description;
    EShipSystemIcon Icon = EShipSystemIcon::Cargo;
    TArray<FRecordDetailField> Fields;
    TArray<FRecordDetailSection> Sections;
    FRecordDetails& Fact(const FText& Label, const FText& Value) { Fields.Add({Label,Value}); return *this; }
    FRecordDetails& Section(const FText& Heading, const FText& Body) { if(!Body.IsEmpty()) Sections.Add({Heading,Body}); return *this; }
};
// Native Slate only: no temporary UImage/brush owners to be collected.
class SRecordDetails : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecordDetails) {} SLATE_ATTRIBUTE(FRecordDetails, Record) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void Tick(const FGeometry&, double CurrentTime, float DeltaTime) override;
private:
    void Refresh(int32 Columns);
    TAttribute<FRecordDetails> Record;
    TSharedPtr<class SVerticalBox> Content;
    FString Signature;
    int32 LastColumns = 0;
    double NextRefresh = 0;
};
