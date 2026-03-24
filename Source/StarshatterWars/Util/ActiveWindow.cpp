#include "ActiveWindow.h"
#include "Screen.h"

ActiveWindow::ActiveWindow(Screen* InScreen,
    int ax, int ay, int aw, int ah,
    uint32 InID,
    uint32 InStyle,
    ActiveWindow* InParent)
    // IMPORTANT: uses your View root ctor that takes Screen*
    : View(InScreen, ax, ay, aw, ah)
    , id(InID)
    , style(InStyle)
    , parent_aw(InParent)
{
    // If you want parent attachment behavior like legacy ActiveWindow,
    // do it explicitly here (View already supports child views):
    if (parent_aw)
    {
        parent_aw->AddView(this);
        form_aw = parent_aw->form_aw ? parent_aw->form_aw : parent_aw;
    }

    ClientEvent(EID_CREATE);
}

ActiveWindow::~ActiveWindow()
{
    ClientEvent(EID_DESTROY);
    clients.destroy();
}

void ActiveWindow::Show()
{
    View::Show();
}

void ActiveWindow::Hide()
{
    View::Hide();
}

void ActiveWindow::MoveTo(const Rect& r)
{
    View::MoveTo(r);
}

bool ActiveWindow::IsFormActive() const
{
    if (form_aw)
        return form_aw->IsVisible();
    return true;
}

// ---------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------
void ActiveWindow::DrawText(const char* txt, int count, Rect& txt_rect, DWORD flags)
{
    DrawTextRect(txt, count, txt_rect, flags);
}

// ---------------------------------------------------------------------
// Events (MapView calls these)
// ---------------------------------------------------------------------
int ActiveWindow::OnLButtonDown(int x, int y)
{
    ClientEvent(EID_LBUTTON_DOWN, x, y);
    return 0;
}

int ActiveWindow::OnLButtonUp(int x, int y)
{
    ClientEvent(EID_LBUTTON_UP, x, y);
    return 0;
}

int ActiveWindow::OnRButtonDown(int x, int y)
{
    ClientEvent(EID_RBUTTON_DOWN, x, y);
    return 0;
}

int ActiveWindow::OnRButtonUp(int x, int y)
{
    ClientEvent(EID_RBUTTON_UP, x, y);
    return 0;
}

int ActiveWindow::OnClick()
{
    ClientEvent(EID_CLICK);
    return 0;
}

int ActiveWindow::OnSelect()
{
    ClientEvent(EID_SELECT);
    return 0;
}

// ---------------------------------------------------------------------
// Client callback map
// ---------------------------------------------------------------------
void ActiveWindow::RegisterClient(int EID, ActiveWindow* client, PFVAWE callback)
{
    if (!client || !callback)
        return;

    // FIX: no debug operator new overload in your UE port
    AWMap* map = new AWMap(EID, client, callback);
    if (map)
        clients.append(map);
}

void ActiveWindow::UnregisterClient(int EID, ActiveWindow* client)
{
    AWMap test(EID, client, nullptr);
    int idx = clients.index(&test);

    if (idx >= 0)
        delete clients.removeIndex(idx);
}

void ActiveWindow::ClientEvent(int EID, int x, int y)
{
    event.window = this;
    event.eid = EID;
    event.x = x;
    event.y = y;

    ListIter<AWMap> it = clients;
    while (++it)
    {

        if (it->eid == EID && it->func)
            it->func(it->client, &event);
    }
}

void ActiveWindow::Draw()
{
    // Legacy semantics: refresh this window and its children.
    // Your View base already knows how to do this.
    Paint();
}
