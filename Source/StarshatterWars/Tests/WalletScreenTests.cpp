#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "TransactionDlg.h"
#include "StarshatterPlayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
void WalletTextPositions(const TSharedRef<SWidget>& Widget,TMap<FString,FVector2D>& Positions)
{
    if (Widget->GetType()==FName(TEXT("STextBlock")))
        Positions.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString(),Widget->GetCachedGeometry().GetAbsolutePosition());
    if (auto* Children=Widget->GetChildren())
        for (int32 Index=0;Index<Children->Num();++Index) WalletTextPositions(Children->GetChildAt(Index),Positions);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalletScreenTest,"Starshatter.UI.Wallet.ScreenAndBlueprint",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWalletScreenTest::RunTest(const FString&)
{
    auto* WalletClass=LoadClass<UTransactionDlg>(nullptr,TEXT("/Game/Screens/inGame/WBP_Transaction.WBP_Transaction_C"));
    if (!TestNotNull(TEXT("Registered wallet Blueprint derives from TransactionDlg"),WalletClass)) return false;
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
    TStrongObjectPtr<UStarshatterPlayerSubsystem> Players(NewObject<UStarshatterPlayerSubsystem>(Instance.Get()));
    auto& Pilot=Players->GetMutablePlayerInfo(); Pilot.Name=TEXT("Wallet Test Pilot"); Pilot.Callsign=TEXT("Test");
    auto& State=Pilot.Economy; State.bLegacyWalletMigrated=true;
    State.Wallets.Add(FEconomyLedger::Pilot(),250000);
    TestTrue(TEXT("Insurance receipt commits"),FEconomyLedger::Pay(State,FEconomyLedger::Pilot(),TEXT("Insurance fund"),2500,FGuid::NewGuid(),TEXT("Insurance premium")));
    TestTrue(TEXT("Manufacturing receipt commits"),FEconomyLedger::Pay(State,FEconomyLedger::Pilot(),TEXT("Janek Materials Works"),500,FGuid::NewGuid(),TEXT("Manufacturing installation")));
    const int32 Before=State.Journal.Num(); const int64 Balance=State.Wallets.FindRef(FEconomyLedger::Pilot());
    auto Widget=UTransactionDlg::BuildWalletContent(Players.Get()); Widget->SlatePrepass();
    TestEqual(TEXT("Opening wallet does not change balance"),State.Wallets.FindRef(FEconomyLedger::Pilot()),Balance);
    TestEqual(TEXT("Opening wallet does not create receipts"),State.Journal.Num(),Before);
    if (GDynamicRHI && GDynamicRHI->GetInterfaceType()!=ERHIInterfaceType::Null)
    {
        FWidgetRenderer Renderer(false,false);
        Renderer.DrawWidget(Widget,FVector2D(1280,760)); FlushRenderingCommands(); Widget->SlatePrepass();
        auto* Target=Renderer.DrawWidget(Widget,FVector2D(1280,760)); FlushRenderingCommands();
        TMap<FString,FVector2D> TextPositions; WalletTextPositions(Widget,TextPositions);
        const auto* First=TextPositions.Find(FText::AsNumber(-2500).ToString());
        const auto* Second=TextPositions.Find(FText::AsNumber(-500).ToString());
        if (TestNotNull(TEXT("First wallet receipt visible"),First) && TestNotNull(TEXT("Second wallet receipt visible"),Second))
            TestTrue(TEXT("Receipt rows do not overlap after refresh"),FMath::Abs(First->Y-Second->Y)>=30);
        TArray<FColor> Pixels; FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
        if (!Target || !Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))
        { AddError(TEXT("Wallet render capture failed")); return false; }
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("Automation/Wallet");
        IFileManager::Get().MakeDirectory(*Folder,true); TArray64<uint8> Png;
        FImageUtils::PNGCompressImageArray(1280,760,Pixels,Png);
        TestTrue(TEXT("Wallet visual capture saved"),FFileHelper::SaveArrayToFile(Png,*(Folder/TEXT("Wallet.png"))));
    }
    return true;
}
#endif
