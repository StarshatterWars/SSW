/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpCompleteDlg.cpp
    AUTHOR:       Carlos Bott
*/

#include "CmpCompleteDlg.h"

#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

// If you have these, include them. Otherwise remove and wire Campaign via Manager.
//#include "CampaignSubsystem.h"
#include "Campaign.h"
#include "CmpnScreen.h"

UCmpCompleteDlg::UCmpCompleteDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpCompleteDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Bind Close clicked -> legacy OnClose:
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UCmpCompleteDlg::HandleCloseClicked);
    }

    ShowTime = 0.0f;

    // Optional: if you have a campaign subsystem, resolve it here:
    // if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
    // {
    //     CampaignSubsystem = GI->GetSubsystem<UCampaignSubsystem>();
    //     Campaign = CampaignSubsystem ? CampaignSubsystem->GetCampaign() : nullptr;
    // }
}

void UCmpCompleteDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ShowTime += InDeltaTime;
    ExecFrame(InDeltaTime);
}

void UCmpCompleteDlg::BindFormWidgets()
{
    // Legacy IDs:
    BindImage(100, TitleImage);
    BindLabel(101, InfoLabel);
    BindButton(1, CloseButton);

    // Optional background IDs if your UMG exposes them as images:
    BindImage(300, BgTop);
    BindImage(400, BgBottom);
}

FString UCmpCompleteDlg::GetLegacyFormText() const
{
    // Embedded legacy .frm text exactly as provided:
    return TEXT(R"FORM(
form: {
   back_color: (  0,   0,   0)
   fore_color: (255, 255, 255)
   font:       Limerick12,

   layout: {
      x_mins:     (0)
      x_weights:  (1)

      y_mins:     (0, 0, 0)
      y_weights:  (1, 6, 2)
   }

   defctrl: {
      fore_color:       (0,0,0)
      cell_insets:      (0,0,0,0)
   }

   ctrl: {
      id:            300
      type:          background
      texture:       LoadDlg1
      cells:         (0,0,1,1)
      margins:       (248,2,2,32)
      hide_partial:  false
   }

   ctrl: {
      id:            100
      type:          image
      cells:         (0,1,1,1)
      hide_partial:  false
   }

   ctrl: {
      id:            400
      type:          background
      texture:       LoadDlg2
      cells:         (0,2,1,1)
      margins:       (2,248,48,2)
      hide_partial:  false

      layout: {
         x_mins:     (20, 100, 100, 20)
         x_weights:  ( 0,   1,   0,  0)

         y_mins:     (20, 20, 30)
         y_weights:  ( 1,  0,  0)
      }
   }

   ctrl: {
      id:               1
      pid:              400
      type:             button
      text:             Close

      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      fixed_height:     19

      cells:            (2,1,1,1)
   }
}
)FORM");
}

void UCmpCompleteDlg::Show()
{
    // Resolve Campaign if not already set:
    // If you do not have CampaignSubsystem/Campaign UObject types, route through Manager.
    Campaign = Campaign->GetCampaign();

    if (!TitleImage)
        return;

    // If you have no campaign object at this layer, you can still display a default image or do nothing.
    if (!Campaign)
        return;

    // PSEUDOCODE you will replace with your campaign API:
    CombatEvent* event = Campaign->GetLastEvent();
    if (!event) return;
    //FString ImageName = UTF8_TO_TCHAR(event->ImageFile());

    // Since we cannot assume your exact UObject campaign API here, keep it as a clearly isolated step:
    FString ImageName;       // set from Campaign->GetLastEvent()->ImageFile()
    FString CampaignPath;    // set from Campaign->Path()

    if (ImageName.IsEmpty() || CampaignPath.IsEmpty())
        return;

    // Ensure .pcx extension (legacy behavior):
    if (!ImageName.EndsWith(TEXT(".pcx"), ESearchCase::IgnoreCase))
    {
        ImageName += TEXT(".pcx");
    }

    BannerTexture = LoadCampaignTexture(CampaignPath, ImageName);
    if (BannerTexture)
    {
        TitleImage->SetBrushFromTexture(BannerTexture, /*bMatchSize*/ false);
    }
}

void UCmpCompleteDlg::ExecFrame(float DeltaTime)
{
    (void)DeltaTime;
    // Legacy ExecFrame() was empty.
}

void UCmpCompleteDlg::HandleCloseClicked()
{
    // Legacy:
    // if (manager) manager->ShowCmdDlg();

    if (Manager)
    {
        // If your manager is a UObject with a ShowCmdDlg() UFUNCTION, call it here.
        // Manager->ShowCmdDlg();
        OnRequestShowCmdDlg(); // keep BP event available even with manager
        return;
    }

    // No manager wired: raise BP event so the owning screen can transition.
    OnRequestShowCmdDlg();
}

UTexture2D* UCmpCompleteDlg::LoadCampaignTexture(const FString& CampaignPath, const FString& ImageFile) const
{
    // Intentionally returns nullptr here.
    // Override this in your project where your Bitmap/DataLoader bridge exists:
    //
    // - loader->SetDataPath(campaignPath)
    // - loader->LoadBitmap(pcx, bannerBitmap)
    // - convert bannerBitmap -> UTexture2D
    //
    (void)CampaignPath;
    (void)ImageFile;
    return nullptr;
}
