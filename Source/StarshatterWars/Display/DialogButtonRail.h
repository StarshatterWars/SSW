/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         DialogButtonRail.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Reusable left-side button rail panel.

    Provides a consistent vertical button layout for dialog screens,
    including:
    - dynamic top button list
    - optional bottom Accept / Cancel buttons
    - shared styling and spacing
    - background panel support via UBorder

    Used by:
    - Operations Screen
    - Mission Briefing Dialog
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogButtonRail.generated.h"

class UBorder;
class UVerticalBox;
class USpacer;
class UMenuButton;

USTRUCT()
struct FDialogButtonSpec
{
    GENERATED_BODY()

    UPROPERTY()
    FName Id;

    UPROPERTY()
    FString Label;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogRailButtonClicked, FName);

UCLASS()
class STARSHATTERWARS_API UDialogButtonRail : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void SetMenuButtonClass(TSubclassOf<UMenuButton> InClass);
    void SetTopButtons(const TArray<FDialogButtonSpec>& InButtons);
    void SetShowBottomButtons(bool bInShowAccept, bool bInShowCancel);
    void SetButtonSize(float InWidth, float InHeight);

    void RebuildRail();

    FOnDialogRailButtonClicked OnButtonClicked;

    // -----------------------------------------------------------------
    // Style
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FSlateBrush PanelBrush;

protected:
    void BuildRoot();
    void ClearRail();

    UMenuButton* CreateButton(const FDialogButtonSpec& Spec);
    void AddTopButton(UMenuButton* Button);
    void AddBottomButton(UMenuButton* Button);
    void BindButton(UMenuButton* Button, FName Id);

    UFUNCTION()
    void HandleButtonSelected(UMenuButton* SelectedButton);

protected:
    UPROPERTY()
    UBorder* RootBorder = nullptr;

    UPROPERTY()
    UVerticalBox* ButtonContainer = nullptr;

    UPROPERTY()
    USpacer* FillSpacer = nullptr;

    UPROPERTY()
    TSubclassOf<UMenuButton> MenuButtonClass;

    UPROPERTY()
    TArray<FDialogButtonSpec> TopButtons;

    UPROPERTY()
    TMap<TObjectPtr<UMenuButton>, FName> ButtonMap;

    UPROPERTY()
    UMenuButton* AcceptButton = nullptr;

    UPROPERTY()
    UMenuButton* CancelButton = nullptr;

    bool bShowAccept = true;
    bool bShowCancel = true;

    float ButtonSpacing = 8.f;
    float ButtonWidth = 256.f;
    float ButtonHeight = 32.f;
};