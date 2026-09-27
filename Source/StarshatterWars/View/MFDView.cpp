// MFDView.cpp

#include "MFDView.h"

#include "View.h"
#include "Mouse.h"
#include "Bitmap.h"

#include "FontManager.h"     // You said this only resolves reliably when included in .h; move if needed.
#include "Ship.h"

// Optional: if you still rely on these HUD helpers (PrepareBitmap/ColorizeBitmap/MarkerColor)
#include "HUDView.h"

// If you have Game::GetText in UE, include your wrapper; otherwise replace calls with literal strings.
#include "Game.h"

bool   MFDView::bInitialized = false;
Bitmap MFDView::SensorFOV;
Bitmap MFDView::SensorFWD;
Bitmap MFDView::SensorHSD;
Bitmap MFDView::Sensor3D;

uint8* MFDView::SensorFOVShade = nullptr;
uint8* MFDView::SensorFWDShade = nullptr;
uint8* MFDView::SensorHSDShade = nullptr;
uint8* MFDView::Sensor3DShade = nullptr;

void MFDView::Initialize()
{
    if (bInitialized)
        return;

    // If you kept HUDView::PrepareBitmap + shade tables, keep this.
    // Otherwise: load into Bitmap without shade.
    HUDView::PrepareBitmap("sensor_fov.pcx", SensorFOV, SensorFOVShade);
    HUDView::PrepareBitmap("sensor_fwd.pcx", SensorFWD, SensorFWDShade);
    HUDView::PrepareBitmap("sensor_hsd.pcx", SensorHSD, SensorHSDShade);
    HUDView::PrepareBitmap("sensor_3d.pcx", Sensor3D, Sensor3DShade);

    SensorFOV.SetType(Bitmap::BMP_TRANSLUCENT);
    SensorFWD.SetType(Bitmap::BMP_TRANSLUCENT);
    SensorHSD.SetType(Bitmap::BMP_TRANSLUCENT);
    Sensor3D.SetType(Bitmap::BMP_TRANSLUCENT);

    bInitialized = true;
}

void MFDView::Close()
{
    SensorFOV.ClearImage();
    SensorFWD.ClearImage();
    SensorHSD.ClearImage();
    Sensor3D.ClearImage();

    delete[] SensorFOVShade; SensorFOVShade = nullptr;
    delete[] SensorFWDShade; SensorFWDShade = nullptr;
    delete[] SensorHSDShade; SensorHSDShade = nullptr;
    delete[] Sensor3DShade;  Sensor3DShade = nullptr;

    bInitialized = false;
}

MFDView::MFDView(View* InWindow, int32 InIndex)
    : View(InWindow, 0, 0, 0, 0)
    , RectPx(0, 0, 0, 0)
    , Index(InIndex)
    , Mode(EMFDMode::OFF)
{
    Initialize();

    HudFont = FontManager::Find("HUD");

    for (int32 i = 0; i < TXT_LAST; ++i)
    {
        TextSlots[i].Font = HudFont;
        TextSlots[i].Color = FColor::White;
        TextSlots[i].bHidden = true;
    }
}

MFDView::~MFDView()
{
    // No sprite object in this UE port; bitmaps are static.
}

void MFDView::UseCameraView(CameraView* InView)
{
    if (InView && !CamView)
        CamView = InView;
}

void MFDView::SetHUDColor(const FColor& InHudColor)
{
    this->HudColor = InHudColor;

    // Legacy colorizes the bitmaps via shade tables:
    if (SensorFOVShade) HUDView::ColorizeBitmap(SensorFOV, SensorFOVShade, InHudColor);
    if (SensorFWDShade) HUDView::ColorizeBitmap(SensorFWD, SensorFWDShade, InHudColor);
    if (SensorHSDShade) HUDView::ColorizeBitmap(SensorHSD, SensorHSDShade, InHudColor);
    if (Sensor3DShade)  HUDView::ColorizeBitmap(Sensor3D, Sensor3DShade, InHudColor);
}

void MFDView::SetColor(const FColor& InHudColor, const FColor& InTextColor)
{
    HudColor = InHudColor;
    TextColor = InTextColor;

    // Legacy colorizes the bitmaps via shade tables:
    if (SensorFOVShade) HUDView::ColorizeBitmap(SensorFOV, SensorFOVShade, InHudColor);
    if (SensorFWDShade) HUDView::ColorizeBitmap(SensorFWD, SensorFWDShade, InHudColor);
    if (SensorHSDShade) HUDView::ColorizeBitmap(SensorHSD, SensorHSDShade, InHudColor);
    if (Sensor3DShade)  HUDView::ColorizeBitmap(Sensor3D, Sensor3DShade, InHudColor);
}

void MFDView::SetText3DColor(const FColor& InColor)
{
    for (int32 i = 0; i < TXT_LAST; ++i)
        TextSlots[i].Color = InColor;
}

void MFDView::Show()
{
    bHidden = false;
}

void MFDView::Hide()
{
    for (int32 i = 0; i < TXT_LAST; ++i)
        HideMFDText(i);

    bHidden = true;
}

void MFDView::SetRect(const Rect& InRect)
{
    RectPx = InRect;
    SetRectPx(InRect);   
}

void MFDView::SetMode(EMFDMode InMode)
{
    Mode = InMode;

    for (int32 i = 0; i < TXT_LAST; ++i)
        HideMFDText(i);

    switch (Mode)
    {
    case EMFDMode::GAME:
    case EMFDMode::SHIP:
        Lines = 0;
        break;
    default:
        break;
    }
}

bool MFDView::IsMouseLatched() const
{
    return bMouseIn;
}

void MFDView::Draw()
{
    bMouseIn = false;

    if (Mouse::LButton() == 0)
        MouseLatch = 0;

    if (RectPx.Contains(Mouse::X(), Mouse::Y()))
        bMouseIn = true;

    // Legacy: click to turn on / cycle when off (only for non-sensor modes)
    if ((int32)Mode < (int32)EMFDMode::FOV && Mouse::LButton() && !MouseLatch)
    {
        MouseLatch = 1;
        if (bMouseIn)
        {
            if (HUDView* Hud = HUDView::GetInstance())
                Hud->CycleMFDMode(Index);
        }
    }

    for (int32 i = 0; i < TXT_LAST; ++i)
        HideMFDText(i);

    if (bHidden || (int32)Mode < (int32)EMFDMode::FOV)
    {
        // Legacy cockpit texture path: clear the 128x128 block.
        if (CockpitHUDTexture)
        {
            const int32 x1 = Index * 128;
            const int32 y1 = 256;
            CockpitHUDTexture->FillRect(x1, y1, x1 + 128, y1 + 128, FColor::Black);
        }

        if (bHidden)
            return;
    }

    // Draw sensor background bitmap (legacy sprite frame) for sensor modes.
    if ((int32)Mode >= (int32)EMFDMode::FOV)
    {
        Bitmap* Frame = nullptr;

        switch (Mode)
        {
        case EMFDMode::FOV: Frame = &SensorFOV; break;
        case EMFDMode::HSD: Frame = &SensorHSD; break;
        case EMFDMode::RADAR3D: Frame = &Sensor3D; break;
        default: break;
        }

        if (Frame && Frame->Width() > 0 && Frame->Height() > 0)
        {
            if (CockpitHUDTexture)
            {
                const int32 x1 = Index * 128;
                const int32 y1 = 256;
                CockpitHUDTexture->BitBlt(x1, y1, *Frame, 0, 0, Frame->Width(), Frame->Height());
            }
            else if (window)
            {
                const int32 cx = RectPx.x + RectPx.w / 2;
                const int32 cy = RectPx.y + RectPx.h / 2;
                const int32 w2 = Frame->Width() / 2;
                const int32 h2 = Frame->Height() / 2;

                DrawBitmap(cx - w2, cy - h2, cx + w2, cy + h2, Frame /*, blend if your API supports */);
            }
        }
    }

    switch (Mode)
    {
    default:
    case EMFDMode::OFF:                        break;
    case EMFDMode::GAME:     DrawGameMFD();    break;
    case EMFDMode::SHIP:     DrawStatusMFD();  break;

    case EMFDMode::FOV:      DrawSensorMFD();  break;
    case EMFDMode::HSD:      DrawHSD();        break;
    case EMFDMode::RADAR3D:  Draw3D();         break;
    }
}

FColor MFDView::ResolveStatusColor(int32 Status) const
{
    // Mirror legacy mapping; replace constants with your System status enum values.
    // 0 nominal, 1 degraded, 2 critical, 3 destroyed is a common pattern.
    switch (Status)
    {
    default:
    case 0:  return TextColor;
    case 1:  return FColor(255, 255, 0);
    case 2:  return FColor(255, 0, 0);
    case 3:  return FColor(0, 0, 0);
    }
}

void MFDView::ToWideUpper(const char* In, wchar_t* Out, int32 OutChars)
{
    if (!Out || OutChars <= 0)
        return;

    Out[0] = 0;
    if (!In)
        return;

    // Uppercase like legacy (and clamp)
    char tmp[256];
    int32 n = (int32)strlen(In);
    if (n > 250) n = 250;

    for (int32 i = 0; i < n; ++i)
    {
        const unsigned char c = (unsigned char)In[i];
        tmp[i] = (char)(FChar::IsLower(c) ? FChar::ToUpper(c) : c);
    }
    tmp[n] = 0;

#if defined(_MSC_VER)
    mbstowcs_s(nullptr, Out, OutChars, tmp, _TRUNCATE);
#else
    mbstowcs(Out, tmp, (size_t)(OutChars - 1));
    Out[OutChars - 1] = 0;
#endif
}

void MFDView::DrawMFDText(int32 Slot, const char* Txt, Rect& TxtRect, uint32 AlignFlags, int32 Status)
{
    if (Slot < 0 || Slot >= TXT_LAST)
        return;

    FMFDText& T = TextSlots[Slot];
    if (!T.Font)
        return;

    FColor C = (Status >= 0) ? ResolveStatusColor(Status) : T.Color;

    // Legacy: highlight white on hover (only when not using cockpit texture)
    if (!CockpitHUDTexture && TxtRect.Contains(Mouse::X(), Mouse::Y()))
        C = FColor::White;

    T.Font->SetColor(C);

    // cockpit texture path: translate to 128x128 atlas region (legacy)
    Rect DrawRectPx = TxtRect;
    if (CockpitHUDTexture)
    {
        DrawRectPx.x = TxtRect.x + Index * 128 - RectPx.x;
        DrawRectPx.y = TxtRect.y + 256 - RectPx.y;
    }

    wchar_t W[256];
    ToWideUpper(Txt ? Txt : "", W, UE_ARRAY_COUNT(W));

    // Use your SystemFont draw; adjust to match your signature.
    // If your SystemFont wants: DrawTextW(text, len, x, y, clipRect, targetBitmap)
    //T.Font->DrawTextW(W, -1, DrawRectPx.x, DrawRectPx.y, DrawRectPx, CockpitHUDTexture);
    T.Font->DrawTextW(W, -1, DrawRectPx.x, DrawRectPx.y, DrawRectPx);

    T.RectPx = TxtRect;
    T.bHidden = false;
}

void MFDView::HideMFDText(int32 Slot)
{
    if (Slot < 0 || Slot >= TXT_LAST)
        return;

    TextSlots[Slot].bHidden = true;
}

// --------------------------------------------------------------------
// The big draw routines: keep structure identical, port internals next.
// --------------------------------------------------------------------

void MFDView::DrawSensorLabels(const char* MfdModeLabel)
{
    if (!ShipPtr)
        return;

    // NOTE: This function depends on Sensor + your localization wrapper.
    // Porting line-by-line is straightforward once your Sensor API is in UE.

    // For now, keep the clickable rect layout matching legacy:
    const int32 scan_r = RectPx.w;
    const int32 scan_x = CockpitHUDTexture ? (Index * 128) : RectPx.x;
    const int32 scan_y = CockpitHUDTexture ? 256 : RectPx.y;

    Rect mode_rect(scan_x + 2, scan_y + 2, 40, 12);
    Rect range_rect(scan_x + 2, scan_y + scan_r - 12, 40, 12);
    Rect disp_rect(scan_x + scan_r - 41, scan_y + 2, 40, 12);
    Rect probe_rect(scan_x + scan_r - 41, scan_y + scan_r - 12, 40, 12);

    // Replace these strings with your Game::GetText equivalents when available.
    Rect r0 = mode_rect;
    DrawMFDText(0, "STD", r0, DT_LEFT);

    Rect r1 = range_rect;
    DrawMFDText(1, "-100+", r1, DT_LEFT);

    Rect r2 = disp_rect;
    DrawMFDText(2, (MfdModeLabel ? MfdModeLabel : "MFD"), r2, DT_RIGHT);

    // probe label depends on your ProbeLauncher; keep hidden by default:
    HideMFDText(3);

    // Click handling exactly like legacy:
    if (Mouse::LButton() && !MouseLatch)
    {
        MouseLatch = 1;

        if (disp_rect.Contains(Mouse::X(), Mouse::Y()))
        {
            if (HUDView* Hud = HUDView::GetInstance())
                Hud->CycleMFDMode(Index);
        }

        // TODO: add sensor mode/range/probe clicks once Sensor + ship hooks are UE-ready.
    }
}

void MFDView::DrawSensorMFD()
{
    // This is the big AZ/EL scanner. Port line-by-line once Sensor/Contact math is UE-side.
    // For now, keep the skeleton with the right label call and safe guards.

    if (!ShipPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    // TODO: draw ellipses, nav point, contacts, threat markers using your Window/Bitmap API.
    DrawSensorLabels("FOV");
}

void MFDView::DrawHSD()
{
    if (!ShipPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    // TODO: port legacy HSD camera flattening + ring ticks + nav + contacts.
    DrawSensorLabels("HSD");
}

void MFDView::Draw3D()
{
    if (!ShipPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    // TODO: port elite-style 3D radar; keep same marker rules as legacy.
    DrawSensorLabels("3D");
}

void MFDView::DrawMap()
{
    Rect top(RectPx.x, RectPx.y, RectPx.w, 12);
    DrawMFDText(0, "GROUND", top, DT_CENTER);
}

void MFDView::DrawGauge(int32 X, int32 Y, int32 Percent)
{
    // Match legacy: 53x8 outline + half-scale fill
    if (Percent < 3) return;
    if (Percent > 100) Percent = 100;

    int32 px = X;
    int32 py = Y;

    if (CockpitHUDTexture)
    {
        px += Index * 128 - RectPx.x;
        py += 256 - RectPx.y;
        CockpitHUDTexture->DrawRect(px, py, px + 53, py + 8, FColor(64, 64, 64));
    }
    else if (window)
    {
        DrawRect(px, py, px + 53, py + 8, FColor(64, 64, 64));
    }

    Percent /= 2;

    if (CockpitHUDTexture)
        CockpitHUDTexture->FillRect(px + 2, py + 2, px + 2 + Percent, py + 7, FColor(128, 128, 128));
    else if (window)
        FillRect(px + 2, py + 2, px + 2 + Percent, py + 7, FColor(128, 128, 128));
}

void MFDView::DrawGameMFD()
{
    if (Lines < 10) Lines++;

    Rect txt_rect(RectPx.x, RectPx.y, RectPx.w, 12);

    int32 t = 0;

    // Replace these with your Game::FrameRate / ShowFPS / etc when ported:
    DrawMFDText(t++, "MFD GAME", txt_rect, DT_LEFT);
    txt_rect.y += 10;

    if (Lines <= 2) return;

    if (ShipPtr)
    {
        DrawMFDText(t++, "SHIP", txt_rect, DT_LEFT);
        txt_rect.y += 10;
    }
}

void MFDView::DrawStatusMFD()
{
    if (Lines < 10) Lines++;

    Rect status_rect(RectPx.x, RectPx.y, RectPx.w, 12);
    int32 row = 0;

    if (!ShipPtr)
        return;

    // Port these once Drive/Power/Shield/Weapon hooks are available in UE:
    DrawMFDText(row++, "THRUST", status_rect, DT_LEFT);
    DrawGauge(status_rect.x + 70, status_rect.y, 50);
    status_rect.y += 10;
}
