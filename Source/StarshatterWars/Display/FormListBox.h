/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    SUBSYSTEM:    Stars.exe (UE)
    FILE:         FormListBox.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UE port of legacy FormListBox control.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

#include "FormListBox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFormListSelectionChanged, int32, SelectedIndex);

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UFormListBox : public UWidget
{
    GENERATED_BODY()

public:
    UFormListBox(const FObjectInitializer& ObjectInitializer);

    /** Legacy-like API */
    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    void Clear();

    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    void AddItem(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    int32 GetNumItems() const { return ItemsText.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    int32 GetSelectedIndex() const { return SelectedIndex; }

    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    void SetSelectedIndex(int32 InIndex);

    UFUNCTION(BlueprintCallable, Category = "Form|ListBox")
    FText GetItemText(int32 InIndex) const;

public:
    UPROPERTY(BlueprintAssignable, Category = "Form|ListBox")
    FFormListSelectionChanged OnSelectionChanged;

protected:
    // UWidget
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    using FItemPtr = TSharedPtr<FString>;

    TSharedRef<ITableRow> GenerateRow(FItemPtr InItem, const TSharedRef<STableViewBase>& OwnerTable);
    void HandleSelectionChanged(FItemPtr InItem, ESelectInfo::Type SelectInfo);

    void RebuildItemsFromText();
    void RefreshList();

private:
    // Backing store:
    TArray<FItemPtr> Items;
    UPROPERTY()
    TArray<FText> ItemsText;

    // Slate:
    TSharedPtr<SListView<FItemPtr>> ListView;

    // State:
    int32 SelectedIndex = INDEX_NONE;

    // Optional style knobs (add later if you want Starshatter exact visuals):
    UPROPERTY(EditAnywhere, Category = "Form|ListBox")
    float RowPadding = 2.0f;
};
