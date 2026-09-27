/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Foundation
    FILE:         ActiveWindow.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ActiveWindow (Unreal port)
    - Compatibility façade for legacy UI code paths (MapView, dialogs, etc.)
    - Does NOT implement old Layout/Poly/VertexSet/Material system
    - Delegates drawing + visibility + input to View
    - Keeps the old "client event" registration pattern
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "View.h"

// ---------------------------------------------------------------------
// Legacy-style event IDs (keep same numeric meanings as old code)
// ---------------------------------------------------------------------
enum {
    EID_CREATE,
    EID_DESTROY,
    EID_MOUSE_MOVE,
    EID_CLICK,
    EID_SELECT,
    EID_LBUTTON_DOWN,
    EID_LBUTTON_UP,
    EID_RBUTTON_DOWN,
    EID_RBUTTON_UP,
    EID_KEY_DOWN,
    EID_SET_FOCUS,
    EID_KILL_FOCUS,
    EID_MOUSE_ENTER,
    EID_MOUSE_EXIT,
    EID_MOUSE_WHEEL,
    EID_DRAG_START,
    EID_DRAG_DROP,

    EID_USER_1,
    EID_USER_2,
    EID_USER_3,
    EID_USER_4,

    EID_NUM_EVENTS
};

class ActiveWindow;

struct AWEvent
{
    static const char* TYPENAME() { return "AWEvent"; }

    AWEvent() : window(nullptr), eid(0), x(0), y(0) {}
    AWEvent(ActiveWindow* w, int e, int ax = 0, int ay = 0) : window(w), eid(e), x(ax), y(ay) {}

    int operator==(const AWEvent& e) const
    {
        return window == e.window && eid == e.eid && x == e.x && y == e.y;
    }

    ActiveWindow* window;
    int           eid;
    int           x;
    int           y;
};

typedef void (*PFVAWE)(ActiveWindow*, AWEvent*);

struct AWMap
{
    static const char* TYPENAME() { return "AWMap"; }

    AWMap() : eid(0), client(nullptr), func(nullptr) {}
    AWMap(int e, ActiveWindow* w, PFVAWE f) : eid(e), client(w), func(f) {}

    int operator==(const AWMap& m) const
    {
        return eid == m.eid && client == m.client;
    }

    int           eid;
    ActiveWindow* client;
    PFVAWE        func;
};

// ---------------------------------------------------------------------
// ActiveWindow
// ---------------------------------------------------------------------
class ActiveWindow : public View
{
public:
    static const char* TYPENAME() { return "ActiveWindow"; }

    ActiveWindow(class Screen* InScreen,
        int ax, int ay, int aw, int ah,
        uint32 InID = 0,
        uint32 InStyle = 0,
        ActiveWindow* InParent = nullptr);

    virtual ~ActiveWindow();

    // Identity compare (legacy idiom):
    int operator==(const ActiveWindow& w) const { return id == w.id; }

    // -----------------------------------------------------------------
    // Compatibility surface expected by MapView and legacy UI
    // -----------------------------------------------------------------
    virtual void   Show() override;
    virtual void   Hide() override;
    virtual void   MoveTo(const Rect& r) override;

    // Legacy names:
    bool           IsVisible() const { return IsShown(); }
    virtual bool   IsFormActive() const;          // legacy "form context"
    virtual Rect   TargetRect() const { return GetRect(); }

    // Drawing:
    void           DrawText(const char* txt, int count, Rect& txt_rect, DWORD flags);

    // Event interface expected by old callers:
    virtual int    OnLButtonDown(int x, int y);
    virtual int    OnLButtonUp(int x, int y);
    virtual int    OnRButtonDown(int x, int y);
    virtual int    OnRButtonUp(int x, int y);
    virtual int    OnClick();
    virtual int    OnSelect();

    virtual void Draw();
    
    void SetEnabled(bool bInEnabled = true) { enabled = bInEnabled; }
    bool IsEnabled() const { return enabled; }

    // Callback registration:
    virtual void   RegisterClient(int EID, ActiveWindow* client, PFVAWE callback);
    virtual void   UnregisterClient(int EID, ActiveWindow* client);
    virtual void   ClientEvent(int EID, int x = 0, int y = 0);

    // Properties:
    DWORD          GetID() const { return id; }
    DWORD          GetStyle() const { return style; }
    void           SetStyle(DWORD s) { style = s; }

    void           SetForeColor(const FColor& c) { ForeColor = c; }
    FColor         GetForeColor() const { return ForeColor; }

    void           SetBackColor(const FColor& c) { BackColorLocal = c; }
    FColor         GetBackColor() const { return BackColorLocal; }

private:
    DWORD          id = 0;
    DWORD          style = 0;

    ActiveWindow* parent_aw = nullptr;
    ActiveWindow* form_aw = nullptr;   // legacy "form" concept, optional

    // Minimal color state used by some views:
    FColor         ForeColor = FColor::White;
    FColor         BackColorLocal = FColor::Black;

    // Client callbacks:
    List<AWMap>    clients;
    AWEvent        event;

    bool enabled = true;
};
