#pragma once
#include "CoreMinimal.h"
#include "ShipSystemIcon.h"
#include "Widgets/SCompoundWidget.h"
#include "SimDataEvents.h"

struct FRecordDetailField { FText Label; FText Value; FText Source; };
struct FRecordDetailSection { FText Heading; FText Body; };
struct FRecordDetails
{
    FText Title, Subtitle, Status, Description;
    EShipSystemIcon Icon = EShipSystemIcon::Cargo;
    TArray<FRecordDetailField> Fields;
    TArray<FRecordDetailSection> Sections;
    FRecordDetails& Fact(const FText& Label,const FText& Value,const FText& Source=FText()) { Fields.Add({Label,Value,Source}); return *this; }
    FRecordDetails& Section(const FText& Heading, const FText& Body) { if(!Body.IsEmpty()) Sections.Add({Heading,Body}); return *this; }
};
// Native Slate only: no temporary UImage/brush owners to be collected.
class SRecordDetails : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecordDetails):_EventDriven(false),_SelectionRevision(0) {} SLATE_ATTRIBUTE(FRecordDetails, Record) SLATE_ARGUMENT(bool,EventDriven) SLATE_ATTRIBUTE(uint32,SelectionRevision) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    ~SRecordDetails();
    void Tick(const FGeometry&, double CurrentTime, float DeltaTime) override;
private:
    void Refresh(int32 Columns);
    TAttribute<FRecordDetails> Record;
    TSharedPtr<class SVerticalBox> Content;
    FString Signature;
    int32 LastColumns = 0;
    double NextRefresh = 0;
    bool bEventDriven=false,bDirty=true;
    TAttribute<uint32> SelectionRevision;
    uint32 LastSelection=0;
    FDelegateHandle ChangeHandle;
};
