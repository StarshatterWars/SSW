#pragma once

#include "Types.h"
#include "View.h"
#include "Text.h"

#include "Math/Vector.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"

#include "GameStructs.h"

class Menu;
class MenuItem;

class MenuView : public View
{
public:
    MenuView(View* InParent, int ax = 0, int ay = 0, int aw = 0, int ah = 0);
    virtual ~MenuView();

    // Operations:
    virtual void      Refresh();
    virtual void      OnWindowMove();
    virtual void      DoMouseFrame();
    virtual void      DrawMenu();
    virtual void      DrawMenu(int x, int y, Menu* menu);
    virtual int       ProcessMenuItem();
    virtual void      ClearMenuSelection(Menu* menu);

    virtual bool      IsShown() { return show_menu != 0; }
    virtual int       GetAction() { return action; }
    virtual Menu* GetMenu() { return menu; }
    virtual void      SetMenu(Menu* m) { menu = m; }
    virtual MenuItem* GetMenuItem() { return menu_item; }

protected:
    int         width = 0;
    int         height = 0;

    int         shift_down = 0;
    int         mouse_down = 0;
    int         right_down = 0;
    int         show_menu = 0;

    FVector     right_start = FVector::ZeroVector;
    FVector     offset = FVector::ZeroVector;

    int         action = 0;
    Menu* menu = nullptr;
    MenuItem* menu_item = nullptr;
    MenuItem* selected = nullptr;
};
