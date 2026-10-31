#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "PlayerDlg.h"
#include "TacRefDlg.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModernReferenceScreensTest,"Starshatter.Menu.LogbookAndTacticalReference",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FModernReferenceScreensTest::RunTest(const FString& Parameters)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(NewObject<UGameInstance>());
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto* PlayerClass=LoadClass<UPlayerDlg>(nullptr,TEXT("/Game/Screens/Player/WBP_PlayerLogbook.WBP_PlayerLogbook_C"));
    auto* ReferenceClass=LoadClass<UTacRefDlg>(nullptr,TEXT("/Game/Screens/TacRef/WBP_TacRefScreen.WBP_TacRefScreen_C"));
    if(!TestNotNull(TEXT("Original logbook Blueprint loads"),PlayerClass) || !TestNotNull(TEXT("Original reference Blueprint loads"),ReferenceClass)) return false;
    auto* Player=NewObject<UPlayerDlg>(World,PlayerClass);Player->Initialize();
    auto PlayerSlate=Player->TakeWidget();
    TestNotNull(TEXT("Logbook modern shell built"),Player->ModernShell.Get());
    TestTrue(TEXT("Modern root retained by widget tree"),Player->WidgetTree->RootWidget==Player->ModernShell);
    TestNotNull(TEXT("Profile name editor retained"),Player->edt_name.Get());
    TestNotNull(TEXT("Callsign editor retained"),Player->edt_callsign.Get());
    TestEqual(TEXT("All ten chat macro editors retained"),Player->MacroEdits.Num(),10);
    TestEqual(TEXT("Sixteen medal slots retained"),Player->MedalImages.Num(),16);
    auto* PlayerRoot=Player->ModernShell.Get();Player->BuildModernShell();
    TestTrue(TEXT("Repeated layout build does not duplicate logbook"),Player->ModernShell==PlayerRoot);
    Player->edt_name->SetText(FText::FromString(TEXT("TEST PILOT")));
    Player->edt_callsign->SetText(FText::FromString(TEXT("VANGUARD")));
    Player->txt_missions->SetText(FText::FromString(TEXT("12")));

    auto* Reference=NewObject<UTacRefDlg>(World,ReferenceClass);Reference->Initialize();
    auto* Combo=Reference->ShipCombo;auto* Table=Reference->ShipDesignTable.Get();auto* Fighter=Reference->FighterButton;
    auto ReferenceSlate=Reference->TakeWidget();
    TestNotNull(TEXT("Reference modern shell built"),Reference->ModernShell.Get());
    TestTrue(TEXT("Original selection combo reused"),Reference->ShipCombo==Combo);
    TestTrue(TEXT("Authored ship table retained"),Reference->ShipDesignTable==Table);
    TestTrue(TEXT("Original fighter button and delegates retained"),Reference->FighterButton==Fighter && Fighter->OnClicked.IsBound());
    TestNotNull(TEXT("Weapon artwork host available"),Reference->WeaponBeauty);
    Reference->ActiveCategory=EShipCategory::CapitalShip;
    Reference->bPopulatingReference=true;Reference->PopulateShipDropdown();Reference->bPopulatingReference=false;
    TestEqual(TEXT("Filtered row IDs match dropdown options"),Reference->ShipRowNames.Num(),Reference->ShipCombo->GetOptionCount());
    TestTrue(TEXT("Public capital ship rows available"),Reference->ShipRowNames.Num()>0);
    if(Reference->ShipRowNames.Num()>0)
        if(const auto* Row=Table->FindRow<FShipDesign>(Reference->ShipRowNames[0],TEXT("ModernReference")))
        {
            FString Caption,Stats,Description;Reference->BuildShipTexts(*Row,Caption,Stats,Description);
            Reference->TxtDisplayName->SetText(FText::FromString(Caption));Reference->TxtCaption->SetText(FText::FromString(Caption));
            Reference->TxtStats->SetText(FText::FromString(Stats));Reference->TxtDescription->SetText(FText::FromString(Description));
        }
    auto* ReferenceRoot=Reference->ModernShell.Get();Reference->BuildModernShell();TestTrue(TEXT("Reference layout rebuild is idempotent"),Reference->ModernShell==ReferenceRoot);

    FString CaptureDir;
    if(!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("ReferenceCaptureDir="),CaptureDir))
    {
        IFileManager::Get().MakeDirectory(*CaptureDir,true);FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
        FWidgetRenderer Renderer(true,false);
        auto Capture=[&](TSharedRef<SWidget> Widget,const TCHAR* Name)
        {
            Widget->SlatePrepass();Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();
            if(GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();FlushRenderingCommands();
            auto* Target=Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();
            TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags);
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
            TestTrue(TEXT("Screen image saved"),FFileHelper::SaveArrayToFile(PNG,*FPaths::Combine(CaptureDir,FString(Name)+TEXT(".png"))));
        };
        Capture(PlayerSlate,TEXT("PlayerLogbook"));Capture(ReferenceSlate,TEXT("TacticalReference"));
    }
    Reference->HandleWeaponModeClicked();TestEqual(TEXT("Weapon mode still selects weapon browser"),Reference->Mode,1);
    TestFalse(TEXT("Empire filter disabled for weapons"),Reference->EmpireCombo->GetIsEnabled());
    return true;
}
#endif
