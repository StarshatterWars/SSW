/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         UIButton.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Native UIButton implementation (nGenEx) with Unreal-compatible logging.
*/

#include "UIButton.h"

#include "Types.h"
#include "ActiveWindow.h"
#include "Bitmap.h"

// Unreal (minimal, for UE_LOG):
#include "CoreMinimal.h"

// +--------------------------------------------------------------------+
// Static state (sound routing is game-specific; keep behavior, log for now)
// +--------------------------------------------------------------------+

static int ui_button_volume = 100;

// +--------------------------------------------------------------------+

void UIButton::Initialize()
{
    // Legacy nGenEx would load UI sounds/resources here.
    UE_LOG(LogTemp, Log, TEXT("UIButton::Initialize (volume=%d)"), ui_button_volume);
}

void UIButton::Close()
{
    // Legacy nGenEx would release UI sounds/resources here.
    UE_LOG(LogTemp, Log, TEXT("UIButton::Close"));
}

void UIButton::PlaySound(int n)
{
    // Starshatter Wars will typically route this to AMusicController / audio manager.
    // Keep call site semantics; implement actual audio hookup in your UE bridge layer.
    UE_LOG(LogTemp, Verbose, TEXT("UIButton::PlaySound(%d)"), n);
}

void UIButton::SetVolume(int vol)
{
    ui_button_volume = vol;
    UE_LOG(LogTemp, Log, TEXT("UIButton::SetVolume(%d)"), ui_button_volume);
}

// +--------------------------------------------------------------------+
// Construction / Destruction
// +--------------------------------------------------------------------+

UIButton::UIButton(Screen* s, int ax, int ay, int aw, int ah, DWORD id)
    : ActiveWindow(s, ax, ay, aw, ah, id)
    , animated(false)
    , drop_shadow(false)
    , sticky(false)
    , border(true)
    , active_color(FColor(255, 255, 255, 255))
    , border_color(FColor(80, 80, 80, 255))
    , captured(0)
    , pre_state(0)
    , bevel_width(2)
    , button_state(0)
    , picture_loc(0)
    , picture()
    , standard_image(nullptr)
    , activated_image(nullptr)
    , transition_image(nullptr)
{
}

UIButton::UIButton(ActiveWindow* Parent, int ax, int ay, int aw, int ah, DWORD id)
    : ActiveWindow(Parent ? Parent->GetScreen() : 0, ax, ay, aw, ah, id, 0, Parent)
    , animated(false)
    , drop_shadow(false)
    , sticky(false)
    , border(true)
    , active_color(FColor(255, 255, 255, 255))
    , border_color(FColor(80, 80, 80, 255))
    , captured(0)
    , pre_state(0)
    , bevel_width(2)
    , button_state(0)
    , picture_loc(0)
    , picture()
    , standard_image(nullptr)
    , activated_image(nullptr)
    , transition_image(nullptr)
{
}


UIButton::~UIButton()
{
    // We do not own the Bitmap* state images; those are managed by the caller/resource system.
    standard_image = nullptr;
    activated_image = nullptr;
    transition_image = nullptr;
}

// +--------------------------------------------------------------------+
// Drawing
// +--------------------------------------------------------------------+

void UIButton::Draw()
{
    // In classic nGenEx this would draw into a backing store Bitmap owned by ActiveWindow.
    // Here, keep semantics and defer actual pixel rendering to your existing Bitmap pipeline.
    ActiveWindow::Draw();

    // Choose which image to draw (if any):
    Bitmap* img = standard_image;

    if (sticky) {
        if (button_state == 2 && transition_image)
            img = transition_image;
        else if (button_state == 1 && activated_image)
            img = activated_image;
        else
            img = standard_image;
    }
    else {
        // Non-sticky: pressed shows activated if present:
        if (button_state == 1 && activated_image)
            img = activated_image;
    }

    if (img) {
        const Rect irect = CalcPictureRect();
        DrawImage(img, irect);
    }

    // Picture overlay support (optional):
    // If picture contains something meaningful, draw it too.
    // (No-op if picture is empty; depends on your Bitmap implementation.)
    // DrawImage(&picture, CalcPictureRect());
}

// +--------------------------------------------------------------------+
// Event handling (classic behavior)
// +--------------------------------------------------------------------+

bool UIButton::OnMouseMove(int32 x, int32 y)
{
    if (captured) {
        // Optional: could update hover/pressed visuals while captured.
        return true;
    }

    return false;
}

int UIButton::OnLButtonDown(int x, int y)
{
    captured = 1;
    pre_state = button_state;

    // Pressed visual:
    if (!sticky) {
        button_state = 1;
    }
    else {
        // Sticky buttons typically show a transition state while pressed.
        button_state = transition_image ? 2 : button_state;
    }

    PlaySound(SND_CLICK);
    Draw();

    return 1;
}

int UIButton::OnLButtonUp(int x, int y)
{
    if (!captured)
        return 0;

    captured = 0;

    // If sticky, toggle between state 0 and 1 on release:
    if (sticky) {
        if (pre_state == 1)
            button_state = 0;
        else
            button_state = 1;
    }
    else {
        // Non-sticky returns to idle on release:
        button_state = 0;
    }

    Draw();
    return 1;
}

int UIButton::OnClick()
{
    // Classic nGenEx often plays an accept/confirm sound on click,
    // but many call sites already play SND_CLICK on down.
    PlaySound(SND_ACCEPT);
    return 1;
}

int UIButton::OnMouseEnter(int x, int y)
{
    // Hover feedback (sound/visual) is optional; keep minimal.
    PlaySound(SND_CHIRP);

    if (!captured && !sticky) {
        // Optional hover state could be implemented here.
    }

    return 1;
}

int UIButton::OnMouseExit(int x, int y)
{
    if (!captured && !sticky) {
        // Reset hover state if you implement one.
        Draw();
    }

    return 1;
}

// +--------------------------------------------------------------------+
// Property accessors
// +--------------------------------------------------------------------+

FColor UIButton::GetActiveColor()
{
    return active_color;
}

void UIButton::SetActiveColor(FColor c)
{
    active_color = c;
}

bool UIButton::GetAnimated()
{
    return animated;
}

void UIButton::SetAnimated(bool bNewValue)
{
    animated = bNewValue;
}

short UIButton::GetBevelWidth()
{
    return bevel_width;
}

void UIButton::SetBevelWidth(short nNewValue)
{
    bevel_width = nNewValue;
}

bool UIButton::GetBorder()
{
    return border;
}

void UIButton::SetBorder(bool bNewValue)
{
    border = bNewValue;
}

FColor UIButton::GetBorderColor()
{
    return border_color;
}

void UIButton::SetBorderColor(FColor c)
{
    border_color = c;
}

short UIButton::GetButtonState()
{
    return button_state;
}

void UIButton::SetButtonState(short nNewValue)
{
    button_state = nNewValue;
}

bool UIButton::GetDropShadow()
{
    return drop_shadow;
}

void UIButton::SetDropShadow(bool bNewValue)
{
    drop_shadow = bNewValue;
}

void UIButton::GetPicture(Bitmap& img)
{
    img = picture;
}

void UIButton::SetPicture(const Bitmap& img)
{
    picture = img;
}

short UIButton::GetPictureLocation()
{
    return picture_loc;
}

void UIButton::SetPictureLocation(short nNewValue)
{
    picture_loc = nNewValue;
}

bool UIButton::GetSticky()
{
    return sticky;
}

void UIButton::SetSticky(bool bNewValue)
{
    sticky = bNewValue;
}

void UIButton::SetStandardImage(Bitmap* img)
{
    standard_image = img;
}

void UIButton::SetActivatedImage(Bitmap* img)
{
    activated_image = img;
}

void UIButton::SetTransitionImage(Bitmap* img)
{
    transition_image = img;
}

// +--------------------------------------------------------------------+
// Layout helpers
// NOTE: Assumes Rect is the classic nGenEx Rect (x,y,w,h) from Types.h,
//       and that ActiveWindow exposes x,y,w,h as protected members.
//       If your Rect differs, adjust these helpers accordingly.
// +--------------------------------------------------------------------+

Rect UIButton::CalcLabelRect(int img_w, int img_h)
{
    Rect r;

    r.x = 0;
    r.y = 0;
    r.w = img_w;
    r.h = img_h;

    // Leave space for picture if present:
    if (img_w > 0 || img_h > 0) {
        // Simple heuristic: label takes remaining area.
        // (You can refine to match original nGenEx behavior.)
        if (picture_loc == 0) {           // left
            r.x = img_w + bevel_width;
            r.w = img_w - r.x;
        }
        else if (picture_loc == 1) {      // right
            r.w = img_w - (img_w + bevel_width);
        }
        else if (picture_loc == 2) {      // top
            r.y = img_h + bevel_width;
            r.h = img_h - r.y;
        }
        else if (picture_loc == 3) {      // bottom
            r.h = img_h - (img_h + bevel_width);
        }
    }

    return r;
}

Rect UIButton::CalcPictureRect()
{
    Rect r;

    const int Width = rect.w;
    const int Height = rect.h;

    // Default: centered picture rect inside bevel
    r.x = bevel_width;
    r.y = bevel_width;
    r.w = Width - 2 * bevel_width;
    r.h = Height - 2 * bevel_width;

    // picture_loc:
    // 0 = left, 1 = right, 2 = top, 3 = bottom, else = center
    if (picture_loc == 0) { // left
        r.w = FMath::Max(0, Width / 3);
    }
    else if (picture_loc == 1) { // right
        r.w = FMath::Max(0, Width / 3);
        r.x = Width - r.w - bevel_width;
    }
    else if (picture_loc == 2) { // top
        r.h = FMath::Max(0, Height / 3);
    }
    else if (picture_loc == 3) { // bottom
        r.h = FMath::Max(0, Height / 3);
        r.y = Height - r.h - bevel_width;
    }

    return r;
}

void UIButton::DrawImage(Bitmap* bmp, const Rect& irect)
{
    if (!bmp)
        return;

    // Classic nGenEx would blit bmp into this window's backing store.
    // The concrete API depends on your Bitmap/ActiveWindow implementation.
    //
    // Keep this method as the single integration point:
    // - If ActiveWindow exposes a backstore Bitmap, do the blit here.
    // - If you are bridging to Unreal textures, do the copy here.
    //
    // For now, do a log-only stub to keep Unreal compatibility.
    UE_LOG(LogTemp, Verbose, TEXT("UIButton::DrawImage (rect=%d,%d %dx%d)"), irect.x, irect.y, irect.w, irect.h);
}
