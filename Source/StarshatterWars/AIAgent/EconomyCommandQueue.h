#pragma once
#include "CoreMinimal.h"

// Admission queue only; live credit/cargo changes belong to the saved worker commit.
enum class EEconomyRequestStatus : uint8 { Queued, Processing, Saved, Failed, Cancelled };
struct FEconomyRequestRow
{
    FGuid Receipt;
    FText Label,Reason;
    EEconomyRequestStatus Status=EEconomyRequestStatus::Queued;
};
class FEconomyCommandQueue
{
public:
    static constexpr int32 Capacity=16,HistoryLimit=32;
    bool Enqueue(FGuid Receipt,const FText& Label)
    {
        if(!Receipt.IsValid() || Count()>=Capacity || Rows.ContainsByPredicate([Receipt](const auto& R){return R.Receipt==Receipt;}))return false;
        Rows.Add({Receipt,Label,FText::GetEmpty(),EEconomyRequestStatus::Queued});return true;
    }
    FGuid Next() const
    {for(const auto& R:Rows)if(R.Status==EEconomyRequestStatus::Queued)return R.Receipt;return {};}
    bool Start(FGuid Receipt)
    {
        if(Receipt!=Next() || Rows.ContainsByPredicate([](const auto& R){return R.Status==EEconomyRequestStatus::Processing;}))return false;
        auto* R=Find(Receipt);if(!R)return false;R->Status=EEconomyRequestStatus::Processing;return true;
    }
    bool Finish(FGuid Receipt,bool Saved,const FText& Reason)
    {
        auto* R=Find(Receipt);if(!R || R->Status!=EEconomyRequestStatus::Processing)return false;
        R->Status=Saved?EEconomyRequestStatus::Saved:EEconomyRequestStatus::Failed;R->Reason=Reason;Trim();return true;
    }
    bool Cancel(FGuid Receipt,const FText& Reason)
    {
        auto* R=Find(Receipt);if(!R || R->Status!=EEconomyRequestStatus::Queued)return false;
        R->Status=EEconomyRequestStatus::Cancelled;R->Reason=Reason;Trim();return true;
    }
    int32 Count()const
    {int32 Total=0;for(const auto& R:Rows)if(R.Status==EEconomyRequestStatus::Queued || R.Status==EEconomyRequestStatus::Processing)++Total;return Total;}
    const TArray<FEconomyRequestRow>& Snapshot()const{return Rows;}
    void Reset(){Rows.Reset();}
private:
    FEconomyRequestRow* Find(FGuid Receipt){return Rows.FindByPredicate([Receipt](const auto& R){return R.Receipt==Receipt;});}
    void Trim()
    {
        while(Rows.Num()>HistoryLimit)
        {
            const int32 Index=Rows.IndexOfByPredicate([](const auto& R){return R.Status!=EEconomyRequestStatus::Queued && R.Status!=EEconomyRequestStatus::Processing;});
            if(Index==INDEX_NONE)break;Rows.RemoveAt(Index);
        }
    }
    TArray<FEconomyRequestRow> Rows;
};
