#include "CmpLoadDlg.h"

// UMG:
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"

// Engine:
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Styling/SlateBrush.h"

// Starshatter:
#include "Campaign.h"
#include "Starshatter.h"
#include "Game.h"

// If your port still uses a Bitmap wrapper for campaign images:
#include "Bitmap.h"

UCmpLoadDlg::UCmpLoadDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpLoadDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

void UCmpLoadDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Legacy behavior: update continuously while visible
    ExecFrame();
}

void UCmpLoadDlg::BindFormWidgets()
{
    // IDs from CmpLoadDlg.frm:
    BindImage(300, BgTop);
    BindImage(100, ImgTitle);
    BindImage(400, BgPanel);

    BindLabel(101, LblActivity);
    BindSlider(102, SldProgress);

    // Parity with original C++ (FindControl(200))
    BindLabel(200, LblTitle);
}

void UCmpLoadDlg::Show()
{
    // Equivalent intent to FormWindow::Show()
    SetVisibility(ESlateVisibility::Visible);

    ApplyCampaignTitleCard();
    ShowTimeMs = GetRealTimeMs();
}

bool UCmpLoadDlg::IsDone() const
{
    const uint32 Now = GetRealTimeMs();
    if (Now - ShowTimeMs < 5000u)
        return false;

    return true;
}

void UCmpLoadDlg::ExecFrame()
{
    Starshatter* Stars = Starshatter::GetInstance();
    if (!Stars)
        return;

    if (LblActivity)
    {
        // Assuming const char* from legacy:
        LblActivity->SetText(FText::FromString(UTF8_TO_TCHAR(Stars->GetLoadActivity())));
    }

    if (SldProgress)
    {
        // Assuming progress is 0..1:
        SldProgress->SetValue((float)Stars->GetLoadProgress());
    }
}

uint32 UCmpLoadDlg::GetRealTimeMs() const
{
    // Prefer your ported Game::RealTime() if present:
    // return (uint32)Game::RealTime();

    const double Sec = FPlatformTime::Seconds();
    return (uint32)(Sec * 1000.0);
}

void UCmpLoadDlg::ApplyCampaignTitleCard()
{
    Campaign* CampaignObj = Campaign::GetCampaign();
    if (!CampaignObj)
        return;

    // Campaign name -> title label (legacy ctrl 200)
    if (LblTitle)
    {
        LblTitle->SetText(FText::FromString(UTF8_TO_TCHAR(CampaignObj->Name())));
    }

    // Campaign image -> ImgTitle (legacy image index 3)
    if (ImgTitle)
    {
        Bitmap* Bmp = CampaignObj->GetImage(3);
        if (Bmp)
        {
            // Assumes your Bitmap wrapper exposes UTexture2D* GetTexture()
            UTexture2D* Tex = Bmp->GetTexture();
            if (Tex)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(Tex);
                Brush.ImageSize = FVector2D((float)Tex->GetSizeX(), (float)Tex->GetSizeY());
                ImgTitle->SetBrush(Brush);
            }
        }
    }
}

FString UCmpLoadDlg::GetLegacyFormText() const
{
    // Embedded CmpLoadDlg.frm
    return TEXT(R"FORM(
form: {
   back_color: (  0,   0,   0)
   fore_color: (255, 255, 255)
   font:       Limerick12,

   layout: {
      x_mins:     (0)
      x_weights:  (1)

      y_mins:     (0, 0, 0)
      y_weights:  (2, 1, 3)
   },

   // background images:

   defctrl: {
      fore_color:       (0,0,0)
      cell_insets:      (0,0,0,0)
   },

   ctrl: {
      id:            300
      type:          background
      texture:       LoadDlg1
      cells:         (0,0,1,1)
      margins:       (248,2,2,32)
      hide_partial:  false
   },

   ctrl: {
      id:            100
      type:          image
      cells:         (0,1,1,1)
      hide_partial:  false
   },

   ctrl: {
      id:            400
      type:          background
      texture:       LoadDlg2
      cells:         (0,2,1,1)
      margins:       (2,248,48,2)
      hide_partial:  false

      layout: {
         x_mins:     (30, 150, 30)
         x_weights:  ( 1,   1,  1)

         y_mins:     (20, 20, 20, 20)
         y_weights:  ( 1,  1,  1,  3)
      }
   },


   ctrl: {
      id:            101
      pid:           400
      type:          label,
      cells:         (1,1,1,1)
      text:          "",
      font:          Verdana
      align:         center
      transparent:   true
   },

   ctrl: {
      id:            102
      pid:           400
      type:          slider
      cells:         (1,2,1,1)

      active_color:  (255, 255, 160)
      back_color:    ( 21,  21,  21)
      border:        true
      transparent:   false
      fixed_height:  10
   },
}
)FORM");
}
