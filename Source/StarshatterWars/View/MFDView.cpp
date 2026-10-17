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
#include "Instruction.h"
#include "Sensor.h"
#include "Sim.h"
#include "SimContact.h"
#include "Geometry.h"
#include "Camera.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

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


// UE native MFD primitives (FSlateDrawElement). Falls back to the legacy
// View/Bitmap path only when no HUD paint context is bound this frame.
static FLinearColor MFDToLinear(const FColor& C)
{
    return FLinearColor(C.R / 255.f, C.G / 255.f, C.B / 255.f, C.A / 255.f);
}
static bool MFDPaint(FPaintContext*& Ctx, const FGeometry*& Geo, int32& Layer)
{
    HUDView* Hud = HUDView::GetInstance();
    if (!Hud || !Hud->HasPaint()) return false;
    Ctx = Hud->GetPaintContext();
    Geo = Hud->GetPaintGeometry();
    Layer = Hud->GetPaintLayer();
    return Ctx && Geo;
}
static void MFDLine(View* V, Bitmap* Tex, int X1, int Y1, int X2, int Y2, const FColor& C)
{
    FPaintContext* Ctx = nullptr;
    const FGeometry* Geo = nullptr;
    int32 Layer = 0;
    if (MFDPaint(Ctx, Geo, Layer))
    {
        TArray<FVector2D> Pts;
        Pts.Add(FVector2D((float)X1, (float)Y1));
        Pts.Add(FVector2D((float)X2, (float)Y2));
        FSlateDrawElement::MakeLines(
            Ctx->OutDrawElements, Layer, Geo->ToPaintGeometry(),
            Pts, ESlateDrawEffect::None, MFDToLinear(C), true, 1.f);
        return;
    }
    if (Tex) Tex->DrawLine(X1, Y1, X2, Y2, C);
    else if (V) V->DrawLine(X1, Y1, X2, Y2, C);
}
static void MFDFill(View* V, Bitmap* Tex, int X1, int Y1, int X2, int Y2, const FColor& C)
{
    FPaintContext* Ctx = nullptr;
    const FGeometry* Geo = nullptr;
    int32 Layer = 0;
    if (MFDPaint(Ctx, Geo, Layer))
    {
        const FVector2D Pos((float)X1, (float)Y1);
        const FVector2D Size((float)(X2 - X1), (float)(Y2 - Y1));
        FSlateDrawElement::MakeBox(
            Ctx->OutDrawElements, Layer, Geo->ToPaintGeometry(Pos, Size),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None, MFDToLinear(C));
        return;
    }
    if (Tex) Tex->FillRect(X1, Y1, X2, Y2, C);
    else if (V) V->FillRect(X1, Y1, X2, Y2, C);
}
static void MFDEllipse(View* V, Bitmap* Tex, int X1, int Y1, int X2, int Y2, const FColor& C)
{
    FPaintContext* Ctx = nullptr;
    const FGeometry* Geo = nullptr;
    int32 Layer = 0;
    if (MFDPaint(Ctx, Geo, Layer))
    {
        const float Cx = (X1 + X2) * 0.5f;
        const float Cy = (Y1 + Y2) * 0.5f;
        const float Rx = FMath::Abs(X2 - X1) * 0.5f;
        const float Ry = FMath::Abs(Y2 - Y1) * 0.5f;
        constexpr int32 Segs = 32;
        TArray<FVector2D> Pts;
        Pts.Reserve(Segs + 1);
        for (int32 i = 0; i <= Segs; ++i)
        {
            const float T = (2.f * PI) * ((float)i / (float)Segs);
            Pts.Add(FVector2D(Cx + FMath::Cos(T) * Rx, Cy + FMath::Sin(T) * Ry));
        }
        FSlateDrawElement::MakeLines(
            Ctx->OutDrawElements, Layer, Geo->ToPaintGeometry(),
            Pts, ESlateDrawEffect::None, MFDToLinear(C), true, 1.f);
        return;
    }
    if (Tex) Tex->DrawEllipse(X1, Y1, X2, Y2, C);
    else if (V) V->DrawEllipse(X1, Y1, X2, Y2, C);
}
static void MFDCross(View* V, Bitmap* Tex, int X, int Y)
{
    const FColor White = FColor::White;
    MFDLine(V, Tex, X - 2, Y - 2, X + 2, Y + 2, White);
    MFDLine(V, Tex, X - 2, Y + 2, X + 2, Y - 2, White);
}
static void MFDThreat(View* V, Bitmap* Tex, int X, int Y, const FColor& C, bool bShip)
{
    if (bShip)
        MFDEllipse(V, Tex, X - 4, Y - 4, X + 3, Y + 3, C);
    else
    {
        MFDLine(V, Tex, X,     Y - 5, X + 5, Y,     C);
        MFDLine(V, Tex, X + 5, Y,     X,     Y + 5, C);
        MFDLine(V, Tex, X,     Y + 5, X - 5, Y,     C);
        MFDLine(V, Tex, X - 5, Y,     X,     Y - 5, C);
    }
}

void MFDView::DrawSensorMFD()
{
    if (!ShipPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    Sensor* SensorPtr = ShipPtr->GetSensor();
    if (!SensorPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    const int ScanR = RectPx.w;
    const int ScanX = CockpitHUDTexture ? (Index * 128) : RectPx.x;
    const int ScanY = CockpitHUDTexture ? 256           : RectPx.y;
    const int R     = ScanR / 2;
    const double XCtr = (ScanR / 2.0) - 0.5;
    const double YCtr = (ScanR / 2.0) + 0.5;
    const int Xc = (int)(ScanX + XCtr);
    const int Yc = (int)(ScanY + YCtr);

    double SweepScale = (double)R / (PI * 0.5);
    if (SensorPtr->GetBeamLimit() > 90.0 * DEGREES)
        SweepScale = (double)R / (90.0 * DEGREES);

    // Beam-limit ellipse (legacy FOV, mode < GM).
    if (SensorPtr->GetMode() < ESensorMode::GM)
    {
        const int Az = (int)(SensorPtr->GetBeamLimit() * SweepScale);
        const int El = Az;
        MFDEllipse(window, CockpitHUDTexture, Xc - Az, Yc - El, Xc + Az, Yc + El, HudColor);
    }

    // Next nav point: white cross in AZ/EL.
    Instruction* NavPt = ShipPtr->GetNextNavPoint();
    if (NavPt && NavPt->GetRegion() == ShipPtr->GetRegion())
    {
        const Camera& Cam = ShipPtr->GetCam();
        const FVector Pt = NavPt->GetLocation() - ShipPtr->GetLocation();
        const double Tx = FVector::DotProduct(Pt, Cam.vrt());
        const double Ty = FVector::DotProduct(Pt, Cam.vup());
        const double Tz = FVector::DotProduct(Pt, Cam.vpn());

        if (Tz > 1.0)
        {
            const double Rng = Pt.Size();
            if (Rng > 1.0)
            {
                double Az = FMath::Asin(FMath::Abs(Tx) / Rng);
                double El = FMath::Asin(FMath::Abs(Ty) / Rng);
                if (Tx < 0) Az = -Az;
                if (Ty < 0) El = -El;

                if (FMath::Abs(Az) < 90.0 * DEGREES)
                {
                    Az *= SweepScale;
                    El *= SweepScale;

                    const int X = (int)(R + Az);
                    const int Y = (int)(R - El);
                    if (X > 0 && X < ScanR && Y > 0 && Y < ScanR)
                        MFDCross(window, CockpitHUDTexture, ScanX + X, ScanY + Y);
                }
            }
        }
    }

    // Contacts and threat markers.
    ListIter<SimContact> It = ShipPtr->GetContactList();
    while (++It)
    {
        SimContact* Contact = It.value();
        if (!Contact) continue;
        Ship* CShip = Contact->GetShip();
        if (CShip == ShipPtr) continue;

        double Az = 0, El = 0, Rng = 0;
        Contact->GetBearing(ShipPtr, Az, El, Rng);

        bool bAft = false;
        if (FMath::Abs(Az) < 90.0 * DEGREES)
        {
            Az *= SweepScale;
            El *= SweepScale;
        }
        else
        {
            const double Len = FMath::Sqrt(Az * Az + El * El);
            if (Len > 1e-6)
            {
                Az = R * Az / Len;
                El = R * El / Len;
            }
            else
            {
                Az = -R;
                El = 0;
            }
            bAft = true;
        }

        const int X = (int)(R + Az);
        const int Y = (int)(R - El);
        if (X < 0 || X > ScanR || Y < 0 || Y > ScanR) continue;

        FColor Mark = HUDView::MarkerColor(Contact);
        if (bAft)
        {
            Mark.R = (uint8)(Mark.R * 0.75f);
            Mark.G = (uint8)(Mark.G * 0.75f);
            Mark.B = (uint8)(Mark.B * 0.75f);
        }

        int Size = 1;
        if (CShip && CShip == ShipPtr->GetTarget())
            Size = 2;

        const int MX = ScanX + X;
        const int MY = ScanY + Y;
        MFDFill(window, CockpitHUDTexture, MX - Size, MY - Size, MX + Size, MY + Size, Mark);

        if (Contact->Threat(ShipPtr))
            MFDThreat(window, CockpitHUDTexture, MX, MY, Mark, CShip != nullptr);
    }

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

    Sensor* SensorPtr = ShipPtr->GetSensor();
    if (!SensorPtr)
    {
        Rect r = RectPx;
        DrawMFDText(0, "INACTIVE", r, DT_CENTER);
        return;
    }

    const int ScanR = RectPx.w;
    const int ScanX = CockpitHUDTexture ? (Index * 128) : RectPx.x;
    const int ScanY = CockpitHUDTexture ? 256           : RectPx.y;
    const int R     = ScanR / 2 - 4;
    const double XCtr = (ScanR / 2.0) - 0.5;
    const double YCtr = (ScanR / 2.0) + 0.5;
    const int Xc = (int)XCtr + ScanX;
    const int Yc = (int)YCtr + ScanY;

    // Beam-limit sector lines (legacy HSD, mode < PST).
    if (SensorPtr->GetMode() < ESensorMode::PST)
    {
        const double S = FMath::Sin(SensorPtr->GetBeamLimit());
        const double C = FMath::Cos(SensorPtr->GetBeamLimit());
        const int X0 = (int)(0.1 * R * S);
        const int Y0 = (int)(0.1 * R * C);
        const int X1 = (int)(1.0 * R * S);
        const int Y1 = (int)(1.0 * R * C);
        MFDLine(window, CockpitHUDTexture, Xc - X0, Yc - Y0, Xc - X1, Yc - Y1, HudColor);
        MFDLine(window, CockpitHUDTexture, Xc + X0, Yc - Y0, Xc + X1, Yc - Y1, HudColor);
    }

    const double RScale = (double)R / SensorPtr->GetBeamRange();

    // Flatten camera to horizontal (legacy Y-up).
    Camera HsdCam = ShipPtr->GetCam();
    FVector Look = ShipPtr->GetLocation() + ShipPtr->GetHeading() * 1000.0;
    Look.Y = ShipPtr->GetLocation().Y;
    HsdCam.LookAt(Look);

    // Range-ring tick marks (N/E/S/W).
    for (int Dir = 0; Dir < 4; ++Dir)
    {
        FVector Tick = FVector::ZeroVector;
        switch (Dir)
        {
        case 0: Tick = FVector(0, 0, 1000); break;
        case 1: Tick = FVector(1000, 0, 0); break;
        case 2: Tick = FVector(0, 0, -1000); break;
        case 3: Tick = FVector(-1000, 0, 0); break;
        }
        const double Tx = FVector::DotProduct(Tick, HsdCam.vrt());
        const double Tz = FVector::DotProduct(Tick, HsdCam.vpn());
        double Az = FMath::Asin(FMath::Abs(Tx) / 1000.0);
        if (Tx < 0) Az = -Az;
        if (Tz < 0) Az = (Az < 0) ? (-PI - Az) : (PI - Az);

        for (double Range = 0.3; Range < 1.0; Range += 0.3)
        {
            const int X0 = (int)(FMath::Sin(Az) * R * Range);
            const int Y0 = (int)(FMath::Cos(Az) * R * Range);
            const int X1 = (int)(FMath::Sin(Az) * R * (Range + 0.1));
            const int Y1 = (int)(FMath::Cos(Az) * R * (Range + 0.1));
            MFDLine(window, CockpitHUDTexture, Xc + X0, Yc - Y0, Xc + X1, Yc - Y1, HudColor);
        }
    }

    // Next nav point: white cross on polar range/azimuth.
    Instruction* NavPt = ShipPtr->GetNextNavPoint();
    if (NavPt && NavPt->GetRegion() == ShipPtr->GetRegion())
    {
        const FVector Pt = NavPt->GetLocation() - ShipPtr->GetLocation();
        const double Tx = FVector::DotProduct(Pt, HsdCam.vrt());
        const double Tz = FVector::DotProduct(Pt, HsdCam.vpn());
        double Rng = Pt.Size();
        if (Rng > 1.0)
        {
            double Az = FMath::Asin(FMath::Abs(Tx) / Rng);
            if (Rng > SensorPtr->GetBeamRange())
                Rng = SensorPtr->GetBeamRange();
            if (Tx < 0) Az = -Az;
            if (Tz < 0) Az = (Az < 0) ? (-PI - Az) : (PI - Az);

            const int X = (int)(Xc + FMath::Sin(Az) * Rng * RScale);
            const int Y = (int)(Yc - FMath::Cos(Az) * Rng * RScale);
            MFDCross(window, CockpitHUDTexture, X, Y);
        }
    }

    // Contacts and threat markers.
    const double Limit = SensorPtr->GetBeamRange();
    ListIter<SimContact> It = ShipPtr->GetContactList();
    while (++It)
    {
        SimContact* Contact = It.value();
        if (!Contact) continue;
        Ship* CShip = Contact->GetShip();
        if (CShip == ShipPtr) continue;

        const FVector TargPt = Contact->Location() - HsdCam.Pos();
        const double Tx = FVector::DotProduct(TargPt, HsdCam.vrt());
        const double Rg = Contact->Range(ShipPtr, Limit);
        const double TrueRange = TargPt.Size();
        if (Rg > Limit || Rg <= 0 || TrueRange < 1.0) continue;

        double Az = FMath::Asin(FMath::Abs(Tx) / TrueRange);
        if (Tx < 0) Az = -Az;
        if (!Contact->InFront(ShipPtr))
            Az = (Az < 0) ? (-PI - Az) : (PI - Az);

        const int X = (int)(Xc + FMath::Sin(Az) * Rg * RScale);
        const int Y = (int)(Yc - FMath::Cos(Az) * Rg * RScale);
        if (X < ScanX || Y < ScanY) continue;

        int Size = 2;
        if (CShip && CShip == ShipPtr->GetTarget())
            Size = 3;

        const FColor Mark = HUDView::MarkerColor(Contact);
        MFDFill(window, CockpitHUDTexture, X - Size, Y - Size, X + Size, Y + Size, Mark);

        if (Contact->Threat(ShipPtr))
            MFDThreat(window, CockpitHUDTexture, X, Y, Mark, CShip != nullptr);
    }

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

    const int cx = RectPx.x + RectPx.w / 2;
    const int cy = RectPx.y + RectPx.h / 2;
    const int R  = FMath::Min(RectPx.w, RectPx.h) / 2 - 6;

    auto Plot = [&](int x, int y, const FColor& C)
    {
        if (CockpitHUDTexture) CockpitHUDTexture->FillRect(x, y, x + 1, y + 1, C);
        else if (window)       window->FillRect(x, y, x + 1, y + 1, C);
    };

    // Semi-transparent sphere: outline, equator, meridian.
    const FColor Sphere(40, 90, 140, 90);
    const FColor Axis(60, 120, 170, 70);
    for (int a = 0; a < 360; a += 3)
    {
        const float Rad = FMath::DegreesToRadians((float)a);
        Plot(cx + (int)(R * FMath::Cos(Rad)), cy + (int)(R * FMath::Sin(Rad)), Sphere);
        Plot(cx + (int)(R * FMath::Cos(Rad)), cy + (int)(R * 0.35f * FMath::Sin(Rad)), Axis);
        Plot(cx + (int)(R * 0.35f * FMath::Cos(Rad)), cy + (int)(R * FMath::Sin(Rad)), Axis);
    }

    // Ships in 3D relative to the player.
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Sensor* SensorPtr = ShipPtr->GetSensor();
    const double Range = SensorPtr ? SensorPtr->GetBeamRange() : 75000.0;
    const FVector Origin = ShipPtr->GetLocation();

    if (Region)
    {
        ListIter<Ship> It = Region->GetShips();
        while (++It)
        {
            Ship* S = It.value();
            if (!S || S == ShipPtr || S->IsDead()) continue;
            const FVector Rel = S->GetLocation() - Origin;
            const double Dist = Rel.Size();
            if (Dist < 1.0 || Dist > Range) continue;

            const double Scale = (double)R / Range;
            const int px = cx + (int)(Rel.Y * Scale);
            const int py = cy - (int)(Rel.Z * Scale);
            const uint8 Bright = (uint8)FMath::Clamp(255.0 - (Dist / Range) * 180.0, 60.0, 255.0);
            const FColor Mark = (S->GetIFF() != ShipPtr->GetIFF())
                ? FColor(255, 60, 60, Bright) : FColor(80, 200, 255, Bright);
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    Plot(px + dx, py + dy, Mark);
        }
    }

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
