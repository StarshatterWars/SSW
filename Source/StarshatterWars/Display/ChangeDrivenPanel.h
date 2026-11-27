#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "SimDataEvents.h"
#include "Sim.h"
// Events only mark dirty. Slate coalesces them once while the widget is visible;
// detached/closed panels do not query the simulation or rebuild their rows.
class SChangeDrivenPanel : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SChangeDrivenPanel):_Domains(ESimDataDomain::All),_Inventory(NAME_None){} SLATE_ARGUMENT(ESimDataDomain,Domains) SLATE_ATTRIBUTE(FName,Inventory) SLATE_EVENT(FSimpleDelegate,OnChanged) SLATE_DEFAULT_SLOT(FArguments,Content) SLATE_END_ARGS()
 void Construct(const FArguments& A){Interest=A._Domains;Inventory=A._Inventory;Refresh=A._OnChanged;ChildSlot[A._Content.Widget];Handle=FSimDataEvents::DetailedChanged().AddSP(this,&SChangeDrivenPanel::Changed);}
 ~SChangeDrivenPanel(){FSimDataEvents::DetailedChanged().Remove(Handle);}
 void Tick(const FGeometry& G,double T,float D)override{SCompoundWidget::Tick(G,T,D);if(bDirty && T>=NextDispatch){bDirty=false;NextDispatch=T+1;Refresh.ExecuteIfBound();}}
private:
 void Changed(const FEconomyState& S,const FSimDataChange& Change){if(Change.Affects(Interest,Inventory.Get()) && Sim::GetSim() && &Sim::GetSim()->ReadEconomy()==&S)bDirty=true;}
 ESimDataDomain Interest=ESimDataDomain::All;TAttribute<FName> Inventory;
 FSimpleDelegate Refresh;FDelegateHandle Handle;bool bDirty=false;double NextDispatch=0;
};
class SChangeDrivenText : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SChangeDrivenText):_SelectionRevision(0),_Countdown(false){} SLATE_ATTRIBUTE(FText,Text) SLATE_ATTRIBUTE(uint32,SelectionRevision) SLATE_ARGUMENT(bool,Countdown) SLATE_ARGUMENT(FSlateFontInfo,Font) SLATE_END_ARGS()
 void Construct(const FArguments& A){Source=A._Text;Selection=A._SelectionRevision;Countdown=A._Countdown;ChildSlot[SAssignNew(Label,STextBlock).AutoWrapText(true).Font(A._Font).Text(Source.Get())];LastSelection=Selection.Get();Handle=FSimDataEvents::Changed().AddSP(this,&SChangeDrivenText::Changed);}
 ~SChangeDrivenText(){FSimDataEvents::Changed().Remove(Handle);}
 void Tick(const FGeometry& G,double T,float D)override{SCompoundWidget::Tick(G,T,D);const uint32 Id=Selection.Get();if((bDirty && T>=NextDispatch) || Id!=LastSelection || (Countdown && T>=NextSecond)){bDirty=false;NextDispatch=T+.25;LastSelection=Id;NextSecond=T+1;Label->SetText(Source.Get());}}
private:
 void Changed(const FEconomyState& S){if(const auto* Simulation=Sim::GetSim();Simulation && &Simulation->ReadEconomy()==&S)bDirty=true;}
 TAttribute<FText> Source;TAttribute<uint32> Selection;TSharedPtr<STextBlock> Label;FDelegateHandle Handle;uint32 LastSelection=0;double NextSecond=0,NextDispatch=0;bool Countdown=false,bDirty=false;
};
