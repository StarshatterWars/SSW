#include "MiningDroneAI.h"
namespace
{
bool Authorized(const FMiningDroneFrame& Frame,FText& Reason)
{if(Frame.bAuthority && Frame.bOwnerAlive)return true;Reason=INVTEXT("Mining drone commands require a live authoritative ship.");return false;}
bool Deployed(const FMiningDroneState& Drone){return Drone.State!=EMiningDroneState::Docked && Drone.State!=EMiningDroneState::Lost;}
void Move(FMiningDroneState& Drone,FVector Target,double Speed,double Seconds)
{const FVector Delta=Target-Drone.Position;const double Distance=Delta.Size();if(Distance>0)Drone.Position+=Delta/Distance*FMath::Min(Distance,Speed*Seconds);}
}
bool MiningDroneAI::Eligible(bool Authority,bool Playing,bool Cinematic,bool Exporting,bool Paused)
{return Authority && Playing && !Cinematic && !Exporting && !Paused;}
bool MiningDroneAI::Launch(FMiningDroneFleetState& Fleet,FGuid Id,const FMiningDroneBayDefinition& Bay,const FMiningDroneFrame& Frame,FText& Reason)
{
    if(!Authorized(Frame,Reason))return false;auto* Drone=Fleet.Drones.FindByPredicate([Id](const auto& Item){return Item.Id==Id;});
    int32 Used=0;for(const auto& Item:Fleet.Drones)if(Deployed(Item))Used+=Bay.DroneBandwidth;
    if(!Drone || Drone->State!=EMiningDroneState::Docked || Bay.DroneBandwidth<=0 || Used>Bay.Bandwidth-Bay.DroneBandwidth)
    {Reason=INVTEXT("No docked drone or insufficient control bandwidth.");return false;}
    Drone->Position=Frame.OwnerPosition+FVector(0,100,50);Drone->bRepeatMining=false;Drone->State=EMiningDroneState::Holding;Reason=INVTEXT("Drone launched.");return true;
}
bool MiningDroneAI::Mine(FMiningDroneState& Drone,const FMiningDroneBayDefinition& Bay,const FMiningDroneFrame& Frame,FText& Reason,bool bRepeat)
{
    if(!Authorized(Frame,Reason))return false;
    if(!Deployed(Drone) || !Frame.bTargetValid || !Frame.bTargetAssayed || Frame.TargetContact<0 || Frame.TargetOre.IsNone() ||
        !FMath::IsFinite(Frame.RemainingTonnes) || Frame.RemainingTonnes<=0 || FVector::Dist(Frame.OwnerPosition,Frame.TargetPosition)>Bay.ControlRange ||
        (Drone.OreTonnes>0 && Drone.Ore!=Frame.TargetOre))
    {Reason=INVTEXT("Select an assayed asteroid within drone control range. Unload a different ore before mining it.");return false;}
    if(bRepeat && (!FMath::IsFinite(Frame.OwnerFreeTonnes) || Frame.OwnerFreeTonnes<=UE_SMALL_NUMBER))
    {Reason=INVTEXT("Repeat mining requires free space in the owner hold.");return false;}
    Drone.TargetContact=Frame.TargetContact;Drone.bRepeatMining=bRepeat;Drone.State=EMiningDroneState::Approaching;
    Reason=bRepeat?INVTEXT("Drone will repeat mining this deposit until it is depleted or the owner hold is full."):INVTEXT("Drone approaching the selected deposit.");return true;
}
bool MiningDroneAI::Stop(FMiningDroneState& Drone,const FMiningDroneFrame& Frame,FText& Reason)
{if(!Authorized(Frame,Reason) || !Deployed(Drone))return false;Drone.State=EMiningDroneState::Holding;Drone.TargetContact=INDEX_NONE;Drone.bRepeatMining=false;Reason=INVTEXT("Drone holding position.");return true;}
bool MiningDroneAI::Return(FMiningDroneState& Drone,const FMiningDroneFrame& Frame,FText& Reason)
{if(!Authorized(Frame,Reason) || !Deployed(Drone))return false;Drone.State=EMiningDroneState::Returning;Drone.TargetContact=INDEX_NONE;Drone.bRepeatMining=false;Reason=INVTEXT("Drone returning to its owner.");return true;}
bool MiningDroneAI::Dock(FMiningDroneState& Drone,const FMiningDroneFrame& Frame,TFunctionRef<double(FName,double)> Unload,FText& Reason)
{
    if(!Authorized(Frame,Reason) || !Deployed(Drone) || FVector::Dist(Drone.Position,Frame.OwnerPosition)>30)
    {Reason=INVTEXT("Return the drone to within 30 m of its owner before docking.");return false;}
    Drone.bRepeatMining=false;
    if(Drone.OreTonnes>0)
    {
        const double Amount=Unload(Drone.Ore,Drone.OreTonnes);
        if(!FMath::IsFinite(Amount) || Amount<0 || Amount>Drone.OreTonnes)return false;
        Drone.OreTonnes-=Amount;
    }
    if(Drone.OreTonnes>UE_SMALL_NUMBER){Reason=INVTEXT("The owner hold is full. Ore remains aboard the waiting drone.");return false;}
    Drone.OreTonnes=0;Drone.Ore=NAME_None;Drone.TargetContact=INDEX_NONE;Drone.State=EMiningDroneState::Docked;Reason=INVTEXT("Drone docked and its ore unloaded.");return true;
}
void MiningDroneAI::Lose(FMiningDroneState& Drone)
{Drone.State=EMiningDroneState::Lost;Drone.OreTonnes=0;Drone.Ore=NAME_None;Drone.TargetContact=INDEX_NONE;Drone.bRepeatMining=false;}
bool MiningDroneAI::Tick(FMiningDroneState& Drone,const FMiningDroneBayDefinition& Bay,const FMiningDroneFrame& Frame,double Seconds,double Rate,
    TFunctionRef<double(double)> Extract,TFunctionRef<double(FName,double)> Unload)
{
    if(!Frame.bAuthority || !FMath::IsFinite(Seconds) || Seconds<=0 || !Deployed(Drone))return false;
    if(!Frame.bOwnerAlive){Lose(Drone);return true;}
    const auto Previous=Drone.State;const double BeforeOre=Drone.OreTonnes;const bool WasRepeating=Drone.bRepeatMining;
    bool WorkPerformed=false;double RemainingAfterExtraction=Frame.RemainingTonnes;
    if(FVector::Dist(Drone.Position,Frame.OwnerPosition)>Bay.ControlRange){Drone.State=EMiningDroneState::Returning;Drone.bRepeatMining=false;}
    if(Drone.State==EMiningDroneState::Approaching || Drone.State==EMiningDroneState::Mining)
    {
        if(!Frame.bTargetValid || !Frame.bTargetAssayed || Frame.TargetContact!=Drone.TargetContact || !FMath::IsFinite(Frame.RemainingTonnes) || Frame.RemainingTonnes<=0 ||
            Frame.TargetOre.IsNone() || FVector::Dist(Frame.OwnerPosition,Frame.TargetPosition)>Bay.ControlRange || (Drone.OreTonnes>0 && Drone.Ore!=Frame.TargetOre))
        {Drone.State=Drone.OreTonnes>0?EMiningDroneState::Returning:EMiningDroneState::Holding;Drone.bRepeatMining=false;}
        else if(Drone.bRepeatMining && (!FMath::IsFinite(Frame.OwnerFreeTonnes) || Frame.OwnerFreeTonnes<=UE_SMALL_NUMBER))
        {Drone.State=Drone.OreTonnes>0?EMiningDroneState::Returning:EMiningDroneState::Holding;Drone.bRepeatMining=false;}
        else
        {
            FVector Away=(Drone.Position-Frame.TargetPosition).GetSafeNormal();if(Away.IsNearlyZero())Away=FVector::UpVector;
            const FVector WorkPoint=Frame.TargetPosition+Away*(FMath::Max(0.0,Frame.TargetRadius)+FMath::Min(50.0,Bay.MiningRange));
            Move(Drone,WorkPoint,Bay.Speed,Seconds);
            if(FVector::Dist(Drone.Position,Frame.TargetPosition)<=Frame.TargetRadius+Bay.MiningRange)
            {
                Drone.State=EMiningDroneState::Mining;
                const double Request=FMath::Min3(FMath::Max(0.0,Bay.CapacityTonnes-Drone.OreTonnes),Frame.RemainingTonnes,FMath::Max(0.0,Rate)*Seconds);
                if(Request>0 && FMath::IsFinite(Request))
                {
                    const double Amount=Extract(Request);
                    if(FMath::IsFinite(Amount) && Amount>=0 && Amount<=Request)
                    {Drone.Ore=Frame.TargetOre;Drone.OreTonnes+=Amount;RemainingAfterExtraction-=Amount;WorkPerformed|=Amount>0;}
                }
                if(Drone.OreTonnes>=Bay.CapacityTonnes-UE_SMALL_NUMBER)Drone.State=EMiningDroneState::Returning;
            }
        }
    }
    if(Drone.State==EMiningDroneState::Returning)
    {
        Move(Drone,Frame.OwnerPosition,Bay.Speed,Seconds);
        if(FVector::Dist(Drone.Position,Frame.OwnerPosition)<=30)
        {
            const bool Repeat=Drone.bRepeatMining;const int32 Deposit=Drone.TargetContact;
            const double BeforeUnload=Drone.OreTonnes;FText Ignored;
            if(Dock(Drone,Frame,Unload,Ignored) && Repeat)
            {
                // Cargo is credited only by Dock. Revalidate after this haul
                // using the same deposit and the remaining owner capacity.
                FMiningDroneFrame Next=Frame;Next.OwnerFreeTonnes=FMath::Max(0.0,Frame.OwnerFreeTonnes-BeforeUnload);
                Next.RemainingTonnes=RemainingAfterExtraction;
                if(Frame.TargetContact==Deposit)
                {
                    Drone.State=EMiningDroneState::Holding;
                    if(!Mine(Drone,Bay,Next,Ignored,true))Drone.State=EMiningDroneState::Docked;
                }
            }
        }
    }
    return WorkPerformed || Previous!=Drone.State || BeforeOre!=Drone.OreTonnes || WasRepeating!=Drone.bRepeatMining;
}
