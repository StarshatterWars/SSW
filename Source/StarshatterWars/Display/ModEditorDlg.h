#pragma once
#include "BaseScreen.h"
#include "ModEditorDlg.generated.h"
class Sim;
class Mission;

/** Docked single-player authoring workspace; never edits the running Mission. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API UModEditorDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    static bool CanUseDockedEditor();
    static bool IsEditableMissionPath(const FString& Path);
    virtual void HandleCancel() override;
    bool BeginEditingSession();
    void EndEditingSession();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry&,float) override;
private:
    Sim* PausedSimulation=nullptr;
    Mission* PausedMission=nullptr;
    bool bOwnsPause=false;
};
