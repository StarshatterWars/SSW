/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         UIButton.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Button class (renamed to UIButton to avoid Unreal Engine name collisions)
*/

#pragma once

#include "Types.h"
#include "ActiveWindow.h"
#include "Bitmap.h"

// Unreal (minimal):
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // FMath

// +--------------------------------------------------------------------+

class Screen;

class UIButton : public ActiveWindow
{
public:
    enum SOUNDS {
        SND_BUTTON,
        SND_CLICK,
        SND_SWISH,
        SND_CHIRP,
        SND_ACCEPT,
        SND_REJECT,
        SND_CONFIRM,
        SND_LIST_SELECT,
        SND_LIST_SCROLL,
        SND_LIST_DROP,
        SND_COMBO_OPEN,
        SND_COMBO_CLOSE,
        SND_COMBO_HILITE,
        SND_COMBO_SELECT,
        SND_MENU_OPEN,
        SND_MENU_CLOSE,
        SND_MENU_SELECT,
        SND_MENU_HILITE
    };

    UIButton(Screen* s, int ax, int ay, int aw, int ah, DWORD id = 0);
    UIButton(ActiveWindow* p, int ax, int ay, int aw, int ah, DWORD id = 0);
    virtual ~UIButton();

    static void       Initialize();
    static void       Close();
    static void       PlaySound(int n = 0);
    static void       SetVolume(int vol);

    // Operations:
    virtual void      Draw();     // refresh backing store

    // Event Target Interface:
    virtual bool      OnMouseMove(int32 x, int32 y);
    virtual int       OnLButtonDown(int x, int y);
    virtual int       OnLButtonUp(int x, int y);
    virtual int       OnClick();
    virtual int       OnMouseEnter(int x, int y);
    virtual int       OnMouseExit(int x, int y);

    // Property accessors:
    FColor GetActiveColor();
    void   SetActiveColor(FColor c);
    bool   GetAnimated();
    void   SetAnimated(bool bNewValue);
    short  GetBevelWidth();
    void   SetBevelWidth(short nNewValue);
    bool   GetBorder();
    void   SetBorder(bool bNewValue);
    FColor GetBorderColor();
    void   SetBorderColor(FColor c);
    short  GetButtonState();
    void   SetButtonState(short nNewValue);
    bool   GetDropShadow();
    void   SetDropShadow(bool bNewValue);
    void   GetPicture(Bitmap& img);
    void   SetPicture(const Bitmap& img);
    short  GetPictureLocation();
    void   SetPictureLocation(short nNewValue);
    bool   GetSticky();
    void   SetSticky(bool bNewValue);

    void   SetStandardImage(Bitmap* img);
    void   SetActivatedImage(Bitmap* img);
    void   SetTransitionImage(Bitmap* img);

protected:
    Rect   CalcLabelRect(int img_w, int img_h);
    Rect   CalcPictureRect();
    void   DrawImage(Bitmap* bmp, const Rect& irect);

    bool           animated;
    bool           drop_shadow;
    bool           sticky;
    bool           border;

    FColor         active_color;
    FColor         border_color;

    int            captured;
    int            pre_state;
    short          bevel_width;
    short          button_state;

    short          picture_loc;
    Bitmap         picture;

    Bitmap* standard_image;   // state = 0
    Bitmap* activated_image;  // state = 1 (if sticky)
    Bitmap* transition_image; // state = 2 (if sticky)
};

