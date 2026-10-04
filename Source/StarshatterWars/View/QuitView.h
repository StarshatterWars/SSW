#pragma once
#include "View.h"
class APlayerController;
class SWidget;
class UQuitMissionMenu;

// Legacy Simulation Menu with an Unreal Slate presentation and host actions.
class QuitView : public View
{
public:
    enum EAction : uintptr_t { Accept = 1, Abort = 2, Resume = 3, Controls = 4 };
    explicit QuitView(View* Parent);
    virtual ~QuitView();
    static void Initialize(View* Parent);
    static void Close();
    static QuitView* GetInstance() { return quit_view; }
    void Configure(APlayerController* PC, TFunction<void(uintptr_t)> Handler, TSubclassOf<UQuitMissionMenu> WidgetClass = nullptr);
    virtual void Refresh() override {}
    virtual void OnWindowMove() override {}
    virtual void ExecFrame() override {}
    virtual bool CanAccept();
    virtual bool IsMenuShown() const { return bMenuShown; }
    virtual void ShowMenu();
    virtual void CloseMenu();
    virtual bool OnKeyDown(int32 Key, bool bRepeat) override;
    void ExecuteAction(uintptr_t Action);
private:
    static QuitView* quit_view;
    TWeakObjectPtr<APlayerController> Controller;
    TSharedPtr<SWidget> Overlay;
    TSubclassOf<UQuitMissionMenu> MenuWidgetClass;
    TWeakObjectPtr<UQuitMissionMenu> MenuWidget;
    TFunction<void(uintptr_t)> ActionHandler;
    FString Rejection;
    bool bMenuShown = false;
    bool bPreviousWorldPaused = false;
    bool bPreviousRuntimePaused = false;
    bool bPreviousCursor = false;
};
