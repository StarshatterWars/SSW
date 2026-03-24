// MFDView.h
#pragma once

#include "CoreMinimal.h"

#include "Types.h"        // Rect
#include "View.h"         // View base (your fixed View)
#include "SystemFont.h"   // SystemFont
#include "GameStructs.h"     

class Window;
class Ship;
class CameraView;
class Bitmap;
class Contact;

class MFDView : public View
{
public:
    static const char* TYPENAME() { return "MFDView"; }

    // Static bitmap lifecycle (legacy Initialize/Close)
    static void Initialize();
    static void Close();

public:
    MFDView(View* InWindow, int32 InIndex);
    virtual ~MFDView();

    void UseCameraView(CameraView* InView);

    void SetHUDColor(const FColor& InHudColor);
    void SetColor(const FColor& InHudColor, const FColor& InTextColor);
    void SetText3DColor(const FColor& InColor);

    void Show();
    void Hide();

    void SetRect(const Rect& InRect);
    const Rect& GetRect() const { return RectPx; }

    void SetMode(EMFDMode InMode);
    EMFDMode GetMode() const { return Mode; }

    void SetShip(Ship* InShip) { ShipPtr = InShip; }
    Ship* GetShip() const { return ShipPtr; }

    void SetCockpitHUDTexture(Bitmap* InBmp) { CockpitHUDTexture = InBmp; }

    bool IsMouseLatched() const;

    // Main draw
    virtual void Draw();

private:
    struct FMFDText
    {
        SystemFont* Font = nullptr;
        Rect        RectPx;
        FColor      Color = FColor::White;
        bool        bHidden = true;
    };

private:
    // Text API (legacy DrawMFDText/HideMFDText)
    void DrawMFDText(int32 Slot, const char* Txt, Rect& TxtRect, uint32 AlignFlags, int32 Status = -1);
    void HideMFDText(int32 Slot);

    // Per-mode entry points (legacy)
    void DrawGameMFD();
    void DrawStatusMFD();
    void DrawSensorMFD();
    void DrawHSD();
    void Draw3D();
    void DrawMap();

    void DrawSensorLabels(const char* MfdModeLabel);
    void DrawGauge(int32 X, int32 Y, int32 Percent);

private:
    // narrow->wide for SystemFont::DrawTextW
    static void ToWideUpper(const char* In, wchar_t* Out, int32 OutChars);

    // Status color mapping (legacy System::NOMINAL/DEGRADED/CRITICAL/DESTROYED)
    FColor ResolveStatusColor(int32 Status) const;

private:
    // ---- Static legacy sensor art ----
    static bool   bInitialized;
    static Bitmap SensorFOV;
    static Bitmap SensorFWD;
    static Bitmap SensorHSD;
    static Bitmap Sensor3D;

    // If you still use shade tables, keep pointers; otherwise leave null.
    static uint8* SensorFOVShade;
    static uint8* SensorFWDShade;
    static uint8* SensorHSDShade;
    static uint8* Sensor3DShade;

private:
    Rect      RectPx;
    int32     Index = 0;
    EMFDMode  Mode = EMFDMode::OFF;

    bool      bHidden = true;

    Ship* ShipPtr = nullptr;
    CameraView* CamView = nullptr;

    int32 MouseLatch = 0;
    bool  bMouseIn = false;

    Bitmap* CockpitHUDTexture = nullptr;

    int32 Lines = 0;

    // Text slots
    static constexpr int32 TXT_LAST = 20;
    FMFDText TextSlots[TXT_LAST];

    // Fonts
    SystemFont* HudFont = nullptr;
};
