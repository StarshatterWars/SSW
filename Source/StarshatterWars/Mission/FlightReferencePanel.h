#pragma once

#include "ShipDesignRegistry.h"
#include "TacRefDlg.h"
#include "WeaponDesignRegistry.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SFlightReferencePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFlightReferencePanel) {}
        SLATE_ARGUMENT(TSharedPtr<SWidget>, PreviewWidget)
        SLATE_ARGUMENT(TSharedPtr<SWidget>, GazetteerMap)
        SLATE_ARGUMENT(UGameInstance*, GameInstance)
        SLATE_ARGUMENT(TFunction<void(const FString&)>, UpdateGazetteerMap)
        SLATE_ARGUMENT(TFunction<void(const FShipDesign*)>, UpdatePreview)
    SLATE_END_ARGS()

    void SetReferenceActive(bool Active)
    {
        if (UpdatePreview) UpdatePreview(Active && Selected && !Selected->DesignKey.IsNone() ? ShipDesignRegistry::Find(Selected->DesignKey) : nullptr);
    }
    void Construct(const FArguments& Args)
    {
        UpdatePreview = Args._UpdatePreview;
        UpdateGazetteerMap = Args._UpdateGazetteerMap;
        for (const auto& [Key, Design] : ShipDesignRegistry::GetAll())
        {
            if (Design.Secret || Design.Hidden) continue;
            FString Name, Stats, Description;
            UTacRefDlg::BuildShipTexts(Design, Name, Stats, Description);
            if (Name.TrimStartAndEnd().IsEmpty()) Name = Key.ToString();
            for (const TCHAR* MarkupTag : {TEXT("<HUD_Section>"), TEXT("<HUD_Label>"), TEXT("<HUD_Value>"),
                TEXT("<HUD_Indent>"), TEXT("<HUD_Muted>"), TEXT("<Default>"), TEXT("</>")})
            {
                Stats.ReplaceInline(MarkupTag, TEXT(""));
                Description.ReplaceInline(MarkupTag, TEXT(""));
            }
            EShipSystemIcon Icon;
            switch (Design.Category)
            {
            case EShipCategory::CapitalShip: Icon = EShipSystemIcon::Ship; break;
            case EShipCategory::Transport: Icon = EShipSystemIcon::Transport; break;
            case EShipCategory::Shuttle: Icon = EShipSystemIcon::Transport; break;
            case EShipCategory::Station: Icon = EShipSystemIcon::Station; break;
            case EShipCategory::Platform: Icon = EShipSystemIcon::Station; break;
            case EShipCategory::Building: Icon = EShipSystemIcon::Building; break;
            case EShipCategory::Fighter: Icon = EShipSystemIcon::Fighter; break;
            default: continue;
            }
            Catalog.Add(MakeShared<FEntry>(
                Icon, Name,
                MoveTemp(Stats), MoveTemp(Description), Key, int32(Design.Category)));
        }
        for (const auto& [Key, Design] : WeaponDesignRegistry::GetAll())
        {
            if (Design.bSecret) continue;
            FString Name = Design.Name.IsEmpty() ? Key.ToString() : Design.Name;
            FString Stats;
            FString Description = Design.Description;
            if (const auto* Legacy = WeaponDesignRegistry::FindLegacy(TCHAR_TO_UTF8(*Key.ToString())))
                UTacRefDlg::BuildWeaponTexts(*Legacy, Name, Stats, Description);
            if (Stats.IsEmpty())
                Stats = FString::Printf(TEXT("Range  %.0f m\nProjectile speed  %.0f m/s"), Design.MaxRange, Design.Speed);
            Catalog.Add(MakeShared<FEntry>(EShipSystemIcon::Weapon, Name,
                MoveTemp(Stats), MoveTemp(Description)));
        }
        TArray<FString> LoreTitles;
        TMap<FString, FString> Lore;
        UTacRefDlg::LoadGazetteer(LoreTitles, Lore);
        for (const FString& Title : LoreTitles)
        {
            FString Body = UTacRefDlg::BuildGazetteerDetails(Title, Args._GameInstance) + Lore.FindChecked(Title);
            for (const TCHAR* Markup : {TEXT("<HUD_Section>"), TEXT("<HUD_Label>"), TEXT("<HUD_Value>"),
                TEXT("<HUD_Indent>"), TEXT("<HUD_Muted>"), TEXT("<Default>"), TEXT("</>")})
                Body.ReplaceInline(Markup, TEXT(""));
            Catalog.Add(MakeShared<FEntry>(EShipSystemIcon::Gazetteer, Title, FString(), MoveTemp(Body)));
        }
        Catalog.Sort([](const auto& A, const auto& B) { return A->Name < B->Name; });
        SelectCategory(EShipSystemIcon::Ship);

        auto Filters = SNew(SHorizontalBox);
        const auto AddFilter = [&](EShipSystemIcon Icon, const FText& Label)
        {
            Filters->AddSlot().FillWidth(1).Padding(0, 0, 6, 0)
            [
                SNew(SButton).ContentPadding(FMargin(8, 5)).HAlign(HAlign_Center).ToolTip(MissionUIStyle::MakeToolTip(Label))
                .ButtonColorAndOpacity_Lambda([this, Icon]()
                {
                    return ActiveCategory == Icon ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG;
                })
                .OnClicked_Lambda([this, Icon]()
                {
                    if (ActiveCategory != Icon) SelectCategory(Icon);
                    return FReply::Handled();
                })
                [ SNew(SShipSystemIcon).Icon(Icon) ]
            ];
        };
        AddFilter(EShipSystemIcon::Ship, INVTEXT("Ships"));
        AddFilter(EShipSystemIcon::Transport, INVTEXT("Transports and shuttles"));
        AddFilter(EShipSystemIcon::Fighter, INVTEXT("Fighters"));
        AddFilter(EShipSystemIcon::Station, INVTEXT("Stations and platforms"));
        AddFilter(EShipSystemIcon::Building, INVTEXT("Buildings"));
        AddFilter(EShipSystemIcon::Weapon, INVTEXT("Weapons"));
        AddFilter(EShipSystemIcon::Gazetteer, INVTEXT("Gazetteer"));

        using ESection = UTacRefDlg::EGazetteerSection;
        auto Subfilters=SNew(SHorizontalBox);
        const auto AddSubtype=[&](const TCHAR* Label,int32 Type,EShipSystemIcon Category)
        {
            Subfilters->AddSlot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ContentPadding(FMargin(10,5)).ToolTip(MissionUIStyle::MakeToolTip(FText::FromString(Label)))
                .Visibility_Lambda([this,Category](){return ActiveCategory==Category?EVisibility::Visible:EVisibility::Collapsed;})
                .ButtonColorAndOpacity_Lambda([this,Type](){return Subcategory==Type?MissionUIStyle::RowSelectedBG:MissionUIStyle::RowBG;})
                .OnClicked_Lambda([this,Type](){SelectSubcategory(Type);return FReply::Handled();})
                [SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FLinearColor::White).Font(MissionUIStyle::GetRowFont(12))]];
        };
        AddSubtype(TEXT("ALL"),-1,EShipSystemIcon::Station);
        AddSubtype(TEXT("STATIONS"),int32(EShipCategory::Station),EShipSystemIcon::Station);
        AddSubtype(TEXT("PLATFORMS"),int32(EShipCategory::Platform),EShipSystemIcon::Station);
        AddSubtype(TEXT("ALL"),-1,EShipSystemIcon::Transport);
        AddSubtype(TEXT("TRANSPORTS"),int32(EShipCategory::Transport),EShipSystemIcon::Transport);
        AddSubtype(TEXT("SHUTTLES"),int32(EShipCategory::Shuttle),EShipSystemIcon::Transport);
        auto Sections = SNew(SHorizontalBox);
        const auto AddSection = [&](ESection Section, EShipSystemIcon Icon, const FText& Label)
        {
            Sections->AddSlot().FillWidth(1).Padding(0, 0, 6, 0)
            [
                SNew(SButton).ContentPadding(FMargin(8, 5)).HAlign(HAlign_Center).ToolTip(MissionUIStyle::MakeToolTip(Label))
                .ButtonColorAndOpacity_Lambda([this, Section]()
                {
                    return GazetteerSection == Section ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG;
                })
                .OnClicked_Lambda([this, Section]()
                {
                    if (GazetteerSection != Section)
                    {
                        GazetteerSection = Section;
                        SelectCategory(EShipSystemIcon::Gazetteer);
                    }
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(Icon)]
            ];
        };
        AddSection(ESection::Systems, EShipSystemIcon::PlanetMoon, INVTEXT("Systems and worlds"));
        AddSection(ESection::Factions, EShipSystemIcon::Gazetteer, INVTEXT("Factions and field guide"));
        AddSection(ESection::Figures, EShipSystemIcon::Agent, INVTEXT("Major figures"));

        ChildSlot.Padding(20, 16)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [ Filters ]
            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [SNew(SBox).Visibility_Lambda([this](){return ActiveCategory==EShipSystemIcon::Station || ActiveCategory==EShipSystemIcon::Transport?EVisibility::Visible:EVisibility::Collapsed;})[Subfilters]]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [
                SNew(SBox).Visibility_Lambda([this]()
                {
                    return ActiveCategory == EShipSystemIcon::Gazetteer ? EVisibility::Visible : EVisibility::Collapsed;
                })
                [Sections]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
            [
                SAssignNew(Picker, SComboBox<TSharedPtr<FEntry>>).OptionsSource(&Entries).InitiallySelectedItem(Selected)
                .OnGenerateWidget_Lambda([](TSharedPtr<FEntry> Entry) -> TSharedRef<SWidget>
                {
                    return SNew(STextBlock).Text(Entry ? FText::FromString(Entry->DisplayName()) : FText::GetEmpty())
                        .ColorAndOpacity(FLinearColor::White).Font(MissionUIStyle::GetRowFont(16));
                })
                .OnSelectionChanged_Lambda([this](TSharedPtr<FEntry> Entry, ESelectInfo::Type) { if (Entry) SetEntry(MoveTemp(Entry)); })
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).ColorAndOpacity(FLinearColor::White)
                    .Text_Lambda([this]() { return Selected ? FText::FromString(Selected->DisplayName()) : INVTEXT("No entries in this category"); })
                ]
            ]
            + SVerticalBox::Slot().FillHeight(1)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 12, 0)
                [
                    SNew(SBox).Clipping(EWidgetClipping::ClipToBounds).Visibility_Lambda([this]() { return Selected && !Selected->DesignKey.IsNone()
                        ? EVisibility::Visible : EVisibility::Collapsed; })
                    [Args._PreviewWidget.IsValid() ? Args._PreviewWidget.ToSharedRef() : SNullWidget::NullWidget]
                ]
                + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 12, 0)
                [
                    SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
                    .Visibility_Lambda([this]() { return Selected && Selected->Category == EShipSystemIcon::Gazetteer &&
                        !UTacRefDlg::GetGazetteerSystemName(Selected->Name).IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
                    [Args._GazetteerMap.IsValid() ? Args._GazetteerMap.ToSharedRef() : SNullWidget::NullWidget]
                ]
                + SHorizontalBox::Slot().FillWidth(1)
                [
                SNew(SScrollBox)
                + SScrollBox::Slot().Padding(0, 0, 12, 16)
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).ColorAndOpacity(FLinearColor(.48f, .8f, .94f))
                    .AutoWrapText(true).Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Stats) : FText::GetEmpty(); })
                ]
                + SScrollBox::Slot().Padding(0, 0, 12, 0)
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).AutoWrapText(true)
                    .Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Description) : FText::GetEmpty(); })
                ]
                ]
            ]
        ];
    }

private:
    struct FEntry
    {
        EShipSystemIcon Category;
        FString Name;
        FString Stats;
        FString Description;
        FName DesignKey;
        int32 HullCategory=-1;

        FString DisplayName() const
        {
            return Category == EShipSystemIcon::Gazetteer ? UTacRefDlg::GetGazetteerDisplayName(Name) : Name;
        }
    };
public:
    TArray<FString> GetVisibleNames()const {TArray<FString> Names;for(const auto& Entry:Entries)Names.Add(Entry->DisplayName());return Names;}
    void SelectSubcategory(int32 Type){Subcategory=Type;SelectCategory(ActiveCategory);}
    void SelectCategory(EShipSystemIcon Category)
    {
        if(ActiveCategory!=Category)Subcategory=-1;
        ActiveCategory = Category;
        Entries.Reset();
        for (const auto& Entry : Catalog)
        {
            if (Entry->Category == Category && (Subcategory<0 || Entry->HullCategory==Subcategory) && (Category != EShipSystemIcon::Gazetteer ||
                UTacRefDlg::GetGazetteerSection(Entry->Name) == GazetteerSection))
                Entries.Add(Entry);
        }
        const TSharedPtr<FEntry> First = Entries.IsEmpty() ? nullptr : Entries[0];
        if (Picker)
        {
            Picker->ClearSelection();
            Picker->RefreshOptions();
            if (First) Picker->SetSelectedItem(First);
        }
        SetEntry(First);
    }

private:
    void SetEntry(TSharedPtr<FEntry> Entry)
    {
        if (Selected == Entry) return;
        Selected = MoveTemp(Entry);
        if (UpdateGazetteerMap && Selected && Selected->Category == EShipSystemIcon::Gazetteer)
            UpdateGazetteerMap(UTacRefDlg::GetGazetteerSystemName(Selected->Name));
        if (UpdatePreview)
            UpdatePreview(Selected && !Selected->DesignKey.IsNone() ? ShipDesignRegistry::Find(Selected->DesignKey) : nullptr);
    }

    TFunction<void(const FShipDesign*)> UpdatePreview;
    TFunction<void(const FString&)> UpdateGazetteerMap;
    UTacRefDlg::EGazetteerSection GazetteerSection = UTacRefDlg::EGazetteerSection::Systems;
    EShipSystemIcon ActiveCategory = EShipSystemIcon::Ship;
    int32 Subcategory=-1;
    TArray<TSharedPtr<FEntry>> Catalog;
    TArray<TSharedPtr<FEntry>> Entries;
    TSharedPtr<SComboBox<TSharedPtr<FEntry>>> Picker;
    TSharedPtr<FEntry> Selected;
};
