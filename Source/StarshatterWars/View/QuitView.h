#pragma once

#include "View.h"

class Menu;
class MenuItem;
class Sim;

class QuitView : public View
{
public:
    // NEW: no Window* — Window is “part of views” now
    QuitView(View* InParent);
    virtual ~QuitView();

    // Operations:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;
    virtual void ExecFrame() override;

    virtual bool CanAccept();
    virtual bool IsMenuShown() const;
    virtual void ShowMenu();
    virtual void CloseMenu();

    static void Initialize(View* Parent);
    static void Close();
    static QuitView* GetInstance() { return quit_view; }

    // Input (MATCHES View.h signatures):
    virtual bool OnMouseButtonDown(int32 Button, const FVector2D& ScreenPos) override;
    virtual bool OnMouseButtonUp(int32 Button, const FVector2D& ScreenPos) override;
    virtual bool OnMouseMove(const FVector2D& ScreenPos) override;
    virtual bool OnKeyDown(int32 Key, bool bRepeat) override;

protected:
    void DrawMenu();
    int  HitTestItem(const Rect& menuRect, int x, int y) const;

    // Action handler you can wire to your real pause/quit flow:
    void ExecuteAction(uintptr_t Action);

protected:
    int   width = 0;
    int   height = 0;
    int   xcenter = 0;
    int   ycenter = 0;

    bool  mouse_latch = false;
    bool  bMenuShown = false;

    int   MouseIndex = -1;

    Sim* sim = nullptr;

    Menu* QuitMenu = nullptr;

    static QuitView* quit_view;
};