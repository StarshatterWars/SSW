#include "QuitView.h"
#include "Menu.h"
#include "MenuItem.h"
#include "Sim.h"

// If you want to trigger engine quit later, include your game singletons here.
// #include "Starshatter.h"
// #include "Game.h"

QuitView* QuitView::quit_view = nullptr;

// Action payloads stored in MenuItem::Data:
static constexpr uintptr_t QUIT_ACTION_RESUME = 1;
static constexpr uintptr_t QUIT_ACTION_QUIT = 2;

QuitView::QuitView(View* InParent)
    : View(InParent, 0, 0,
        InParent ? InParent->Width() : 1024,
        InParent ? InParent->Height() : 768)
{
    quit_view = this;

    // Size from our rect (NOT window):
    width = rect.w;
    height = rect.h;
    xcenter = width / 2;
    ycenter = height / 2;

    sim = Sim::GetSim();

    // Build menu (adjust Menu ctor signature if yours is FString-based):
    QuitMenu = new Menu("PAUSE MENU");

    QuitMenu->AddItem("RESUME", QUIT_ACTION_RESUME);
    QuitMenu->AddItem("QUIT", QUIT_ACTION_QUIT);

    bMenuShown = true;
}

QuitView::~QuitView()
{
    // If Menu owns items, let Menu delete them; otherwise, clean up here.
    // For now:
    delete QuitMenu;
    QuitMenu = nullptr;

    if (quit_view == this)
        quit_view = nullptr;
}

void QuitView::Initialize(View* Parent)
{
    if (!quit_view && Parent)
        new QuitView(Parent);
}

void QuitView::Close()
{
    if (quit_view) {
        delete quit_view;
        quit_view = nullptr;
    }
}

void QuitView::OnWindowMove()
{
    // When our rect changes, recompute layout:
    width = rect.w;
    height = rect.h;
    xcenter = width / 2;
    ycenter = height / 2;
}

void QuitView::ExecFrame()
{
    // Keep lightweight; you can add pause logic here later if needed.
}

bool QuitView::CanAccept()
{
    return bMenuShown && QuitMenu != nullptr;
}

bool QuitView::IsMenuShown() const
{
    return bMenuShown;
}

void QuitView::ShowMenu()
{
    bMenuShown = true;
    MouseIndex = -1;
}

void QuitView::CloseMenu()
{
    bMenuShown = false;
    MouseIndex = -1;
}

void QuitView::Refresh()
{
    if (!shown)
        return;

    if (bMenuShown)
        DrawMenu();
}

void QuitView::DrawMenu()
{
    if (!QuitMenu || !GetFont())
        return;

    Rect menuRect(xcenter - 120, ycenter - 60, 240, 18);

    // Title
    DrawTextRect(QuitMenu->GetTitle(), -1, menuRect, DT_CENTER);

    menuRect.y += 24;

    const TArray<MenuItem*>& Items = QuitMenu->GetItems();
    for (int32 i = 0; i < Items.Num(); ++i) {
        MenuItem* Item = Items[i];
        if (!Item) continue;

        Rect lineRect = menuRect;

        if (i == MouseIndex) {
            DrawTextRect(TEXT(">"), -1, lineRect, DT_LEFT);
        }

        lineRect.x += 16;
        DrawTextRect(Item->GetText(), -1, lineRect, DT_LEFT);

        menuRect.y += 16;
    }
}

int QuitView::HitTestItem(const Rect& menuRect, int x, int y) const
{
    if (!QuitMenu)
        return -1;

    const int itemsTop = menuRect.y + 24;
    if (y < itemsTop)
        return -1;

    const int relY = y - itemsTop;
    const int idx = relY / 16;

    const TArray<MenuItem*>& Items = QuitMenu->GetItems();
    if (!Items.IsValidIndex(idx))
        return -1;

    return idx;
}

bool QuitView::OnMouseMove(const FVector2D& ScreenPos)
{
    if (!CanAccept())
        return false;

    Rect menuRect(xcenter - 120, ycenter - 60, 240, 18);

    const int x = (int)ScreenPos.X;
    const int y = (int)ScreenPos.Y;

    MouseIndex = HitTestItem(menuRect, x, y);
    return MouseIndex >= 0;
}

bool QuitView::OnMouseButtonDown(int32 Button, const FVector2D& ScreenPos)
{
    if (!CanAccept())
        return false;

    mouse_latch = true;
    return OnMouseMove(ScreenPos);
}

bool QuitView::OnMouseButtonUp(int32 Button, const FVector2D& ScreenPos)
{
    if (!CanAccept())
        return false;

    mouse_latch = false;

    Rect menuRect(xcenter - 120, ycenter - 60, 240, 18);
    const int idx = HitTestItem(menuRect, (int)ScreenPos.X, (int)ScreenPos.Y);
    if (idx < 0)
        return false;

    const TArray<MenuItem*>& Items = QuitMenu->GetItems();
    MenuItem* chosen = Items[idx];
    if (!chosen)
        return false;

    ExecuteAction(chosen->GetData());
    return true;
}

bool QuitView::OnKeyDown(int32 Key, bool bRepeat)
{
    if (!CanAccept())
        return false;

    // Minimal: ESC closes menu / resumes
    // Replace Key codes with your input system mapping if needed.
    if (Key == 27 /* ESC */) {
        ExecuteAction(QUIT_ACTION_RESUME);
        return true;
    }

    return false;
}

void QuitView::ExecuteAction(uintptr_t Action)
{
    if (Action == QUIT_ACTION_RESUME) {
        CloseMenu();

        // Hook this to your actual pause/dialog stack:
        // e.g. sim->SetPaused(false); or Starshatter::GetInstance()->PopView(this);
    }
    else if (Action == QUIT_ACTION_QUIT) {
        // Hook this to your real quit path:
        // e.g. Game::Exit(); or Starshatter::GetInstance()->RequestExit();
    }
}
