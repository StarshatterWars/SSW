/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    DIALOG:         PlayerDlg
    FILE:           PlayerDlg.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Player profile dialog (authoritative-only).

    - Reads/writes FS_PlayerGameInfo via UStarshatterPlayerSubsystem
    - DOES NOT use legacy PlayerCharacter
    - DOES NOT use lambdas
    - Fixes common “nothing shows up” issues:
        * edt_callsign was never created
        * empire row incorrectly used txt_rank
        * callsign was being written into edt_squadron
        * RootCanvas fallback robustness
=============================================================================*/

#include "PlayerDlg.h"

// UMG
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Image.h"

// Helpers
#include "FormattingUtils.h"
#include "AwardInfoRegistry.h"

// Manager
#include "MenuScreen.h"

// Subsystem + data
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "GameStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayerDlg, Log, All);

static UStarshatterGameDataSubsystem* GetGameDataSS(const UObject* WorldContext)
{
    if (!WorldContext) return nullptr;
    if (const UGameInstance* GI = WorldContext->GetWorld() ? WorldContext->GetWorld()->GetGameInstance() : nullptr)
        return GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    return nullptr;
}

static void SetImageTextureFixed64(UImage* Img, UTexture2D* Tex)
{
    if (!Img) return;

    FSlateBrush Brush = Img->GetBrush();

    Brush.SetResourceObject(Tex);
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.ImageSize = FVector2D(64.f, 64.f);   // <- lock brush size

    Img->SetBrush(Brush);

    Img->SetDesiredSizeOverride(FVector2D(64.f, 64.f)); 
}

static int32 MedalFlagToSlotIndex(uint32 MedalFlag)
{
    // MedalFlag must be power-of-two: 1,2,4,8...
    if (MedalFlag == 0 || (MedalFlag & (MedalFlag - 1)) != 0)
        return INDEX_NONE;

    // log2 for power-of-two:
    return FMath::CountTrailingZeros(MedalFlag); // 1->0, 2->1, 4->2, 8->3 ...
}

UPlayerDlg::UPlayerDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UPlayerDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UPlayerDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();
}

void UPlayerDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    EnsureLayout();
    EnsureScrollHostsStats();
}

void UPlayerDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind base buttons (from BaseScreen)
    if (ApplyButton)
    {
        ApplyButton->OnClicked.RemoveAll(this);
        ApplyButton->OnClicked.AddUniqueDynamic(this, &UPlayerDlg::OnApply);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveAll(this);
        CancelButton->OnClicked.AddUniqueDynamic(this, &UPlayerDlg::OnCancel);
    }

    // Optional buttons (present in some blueprints)
    if (AddPlayerButton)
    {
        AddPlayerButton->OnClicked.RemoveAll(this);
        AddPlayerButton->OnClicked.AddUniqueDynamic(this, &UPlayerDlg::OnAdd);
    }

    if (DeletePlayerButton)
    {
        DeletePlayerButton->OnClicked.RemoveAll(this);
        DeletePlayerButton->OnClicked.AddUniqueDynamic(this, &UPlayerDlg::OnDel);
    }

    BuildMedalsGrid(4, 4); 
    Debug_ShowAllMedals();   // TEST: force show all 16


    RefreshUIFromSubsystem(); 

    EnsureLayout();
    EnsureScrollHostsStats();

    BuildRosterPanel();
    BuildStatsRows();

    RefreshUIFromSubsystem();
}

void UPlayerDlg::NativeDestruct()
{
    if (AddPlayerButton)    AddPlayerButton->OnClicked.RemoveAll(this);
    if (DeletePlayerButton) DeletePlayerButton->OnClicked.RemoveAll(this);
    if (ApplyButton)        ApplyButton->OnClicked.RemoveAll(this);
    if (CancelButton)       CancelButton->OnClicked.RemoveAll(this);

    Super::NativeDestruct();
}

void UPlayerDlg::HandleAccept() { OnApply(); }
void UPlayerDlg::HandleCancel() { OnCancel(); }

void UPlayerDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UPlayerDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

// ------------------------------------------------------------------------
// Subsystem access
// ------------------------------------------------------------------------

UStarshatterPlayerSubsystem* UPlayerDlg::GetPlayerSS() const
{
    if (!GetGameInstance())
        return nullptr;

    return GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
}

// ------------------------------------------------------------------------
// Layout
// ------------------------------------------------------------------------

void UPlayerDlg::EnsureLayout()
{
    if (!WidgetTree)
        return;

    // Prefer BaseScreen::RootCanvas if available; else attempt alternate bind.
    if (!RootCanvas)
    {
        RootCanvas = RootCanvasPlayer;

        if (!RootCanvas)
            RootCanvas = Cast<UCanvasPanel>(GetRootWidget());

        if (!RootCanvas)
            RootCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RootCanvas")));

        if (!RootCanvas)
            RootCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RootCanvasPlayer")));
    }

    if (!RootCanvas)
    {
        UE_LOG(LogPlayerDlg, Error, TEXT("[PlayerDlg] EnsureLayout: RootCanvas is NULL"));
        return;
    }

    if (MainRow)
        return;

    MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainRow"));
    if (!MainRow)
        return;

    if (UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(MainRow))
    {
        S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        S->SetAlignment(FVector2D(0.f, 0.f));
        S->SetOffsets(FMargin(24.f, 64.f, 24.f, 24.f));
        S->SetAutoSize(false);
        S->SetZOrder(100);
    }

    LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));
    StatsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("StatsScroll"));
    StatsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatsVBox"));

    if (!LeftPanel || !StatsScroll || !StatsVBox)
        return;

    if (UHorizontalBoxSlot* L = MainRow->AddChildToHorizontalBox(LeftPanel))
    {
        L->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        L->SetHorizontalAlignment(HAlign_Left);
        L->SetVerticalAlignment(VAlign_Fill);
        L->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
    }

    if (UHorizontalBoxSlot* R = MainRow->AddChildToHorizontalBox(StatsScroll))
    {
        R->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        R->SetHorizontalAlignment(HAlign_Fill);
        R->SetVerticalAlignment(VAlign_Fill);
        R->SetPadding(FMargin(0.f));
    }

    StatsScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    StatsScroll->SetAnimateWheelScrolling(true);
    StatsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);

    // BaseScreen helper target (if you use it elsewhere)
    AutoVBox = StatsVBox;
}

void UPlayerDlg::EnsureScrollHostsStats()
{
    if (!StatsScroll || !StatsVBox)
        return;

    if (StatsVBox->GetParent())
        StatsVBox->RemoveFromParent();

    StatsScroll->ClearChildren();
    StatsScroll->AddChild(StatsVBox);

    StatsVBox->SetVisibility(ESlateVisibility::Visible);
}

// ------------------------------------------------------------------------
// Options-style row builders
// ------------------------------------------------------------------------

UTextBlock* UPlayerDlg::MakeValueText()
{
    if (!WidgetTree)
        return nullptr;

    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!T)
        return nullptr;

    T->SetJustification(ETextJustify::Right);
    return T;
}

UEditableTextBox* UPlayerDlg::MakeEditBox(bool bPassword)
{
    if (!WidgetTree)
        return nullptr;

    UEditableTextBox* E = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    if (!E)
        return nullptr;

    if (bPassword)
        E->SetIsPassword(true);

    return E;
}

UHorizontalBox* UPlayerDlg::AddStatRow(const FText& Label, UWidget* RightWidget, float RightWidth, float RowPadY)
{
    if (!WidgetTree || !StatsVBox || !RightWidget)
        return nullptr;

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!Row)
        return nullptr;

    if (UVerticalBoxSlot* VSlot = StatsVBox->AddChildToVerticalBox(Row))
    {
        VSlot->SetHorizontalAlignment(HAlign_Fill);
        VSlot->SetVerticalAlignment(VAlign_Top);
        VSlot->SetPadding(FMargin(0.f, RowPadY, 0.f, RowPadY));
    }

    UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (Lbl)
    {
        Lbl->SetText(Label);
        Lbl->SetJustification(ETextJustify::Left);

        if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(Lbl))
        {
            LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LS->SetHorizontalAlignment(HAlign_Left);
            LS->SetVerticalAlignment(VAlign_Center);
            LS->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        }
    }

    USizeBox* Wrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    if (Wrap)
    {
        Wrap->SetWidthOverride(RightWidth);
        Wrap->AddChild(RightWidget);

        if (UHorizontalBoxSlot* RS = Row->AddChildToHorizontalBox(Wrap))
        {
            RS->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            RS->SetHorizontalAlignment(HAlign_Right);
            RS->SetVerticalAlignment(VAlign_Center);
        }
    }

    return Row;
}

// ------------------------------------------------------------------------
// Left panel
// ------------------------------------------------------------------------

void UPlayerDlg::BuildRosterPanel()
{
    if (!WidgetTree || !LeftPanel)
        return;

    LeftPanel->ClearChildren();

    UTextBlock* Hdr = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProfileHdr"));
    if (Hdr)
    {
        Hdr->SetText(FText::FromString(TEXT("CURRENT PROFILE")));
        if (UVerticalBoxSlot* S = LeftPanel->AddChildToVerticalBox(Hdr))
            S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    }

    txt_profile_name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProfileName"));
    if (txt_profile_name)
    {
        txt_profile_name->SetText(FText::FromString(TEXT("")));
        if (UVerticalBoxSlot* S = LeftPanel->AddChildToVerticalBox(txt_profile_name))
            S->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
    }
}

// ------------------------------------------------------------------------
// Right panel
// ------------------------------------------------------------------------

void UPlayerDlg::BuildStatsRows()
{
    if (!WidgetTree || !StatsVBox)
        return;

    StatsVBox->ClearChildren();
    MedalImages.Reset();
    MacroEdits.Reset();

    // EDITABLES (FIXED: Callsign edit is created)
    edt_name = MakeEditBox(false);
    edt_password = MakeEditBox(true);
    edt_callsign = MakeEditBox(false);
    edt_squadron = MakeEditBox(false);
    edt_signature = MakeEditBox(false);

    AddStatRow(FText::FromString(TEXT("NAME")), edt_name);
    AddStatRow(FText::FromString(TEXT("PASSWORD")), edt_password);
    AddStatRow(FText::FromString(TEXT("CALLSIGN")), edt_callsign);
    AddStatRow(FText::FromString(TEXT("SQUADRON")), edt_squadron);
    AddStatRow(FText::FromString(TEXT("SIGNATURE")), edt_signature);


    ApplyDefaultEditBoxStyle(edt_name, 18);
    ApplyDefaultEditBoxStyle(edt_password, 18);
    ApplyDefaultEditBoxStyle(edt_callsign, 18);
    ApplyDefaultEditBoxStyle(edt_squadron, 18);
    ApplyDefaultEditBoxStyle(edt_signature, 18);

    // READ-ONLY FIELDS
    txt_created = MakeValueText();
    txt_flighttime = MakeValueText();
    txt_missions = MakeValueText();
    txt_kills = MakeValueText();
    txt_losses = MakeValueText();
    txt_points = MakeValueText();
    txt_rank = MakeValueText();
    txt_empire = MakeValueText();

    AddStatRow(FText::FromString(TEXT("CREATED")), txt_created);
    AddStatRow(FText::FromString(TEXT("FLIGHT TIME")), txt_flighttime);
    AddStatRow(FText::FromString(TEXT("MISSIONS")), txt_missions);
    AddStatRow(FText::FromString(TEXT("KILLS")), txt_kills);
    AddStatRow(FText::FromString(TEXT("LOSSES")), txt_losses);
    AddStatRow(FText::FromString(TEXT("POINTS")), txt_points);
    AddStatRow(FText::FromString(TEXT("RANK")), txt_rank);

    // FIXED: EMPIRE row uses txt_empire, not txt_rank
    AddStatRow(FText::FromString(TEXT("EMPIRE")), txt_empire);

    img_rank = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RankImage"));
    if (img_rank)
        AddStatRow(FText::FromString(TEXT("INSIGNIA")), img_rank, 64.0f);

    // MEDALS
    {
        UTextBlock* MedalsHdr = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MedalsHdr"));
        if (MedalsHdr)
        {
            MedalsHdr->SetText(FText::FromString(TEXT("MEDALS")));
            if (UVerticalBoxSlot* S = StatsVBox->AddChildToVerticalBox(MedalsHdr))
                S->SetPadding(FMargin(0.f, 18.f, 0.f, 8.f));
        }

        medals_grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("MedalsGrid"));

        if (medals_grid)
        {
            // Wrap grid to prevent stretching
            USizeBox* GridBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MedalsGridBox"));

            // 4 medals × 64px + spacing (let's assume 4px spacing)
            GridBox->SetWidthOverride(4 * 128.0f + 3 * 4.f);

            GridBox->AddChild(medals_grid);

            if (UVerticalBoxSlot* S = StatsVBox->AddChildToVerticalBox(GridBox))
            {
                S->SetHorizontalAlignment(HAlign_Right);  // prevents full width stretch
                S->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
            }
        }
        BuildMedalsGrid(4, 4);
    }

    // CHAT MACROS
    {
        UTextBlock* MacroHdr = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MacroHdr"));
        if (MacroHdr)
        {
            MacroHdr->SetText(FText::FromString(TEXT("CHAT MACROS")));
            if (UVerticalBoxSlot* S = StatsVBox->AddChildToVerticalBox(MacroHdr))
                S->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
        }

        BuildChatMacrosRows();
    }
}

void UPlayerDlg::BuildMedalsGrid(int32 Cols, int32 Rows)
{
    if (!medals_grid || !WidgetTree)
        return;

    medals_grid->ClearChildren();
    MedalImages.Reset();

    const int32 Total = Cols * Rows; // 16

    for (int32 i = 0; i < Total; ++i)
    {
        USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());

        if (!SB || !Img)
            continue;

        SB->SetWidthOverride(128.0f);
        SB->SetHeightOverride(64.f);
        SB->AddChild(Img);

        // Optional: start dim/empty
        Img->SetOpacity(0.2f);
        Img->SetVisibility(ESlateVisibility::HitTestInvisible);

        // Force brush size
        FSlateBrush B = Img->GetBrush();
        B.ImageSize = FVector2D(128.0f, 64.f);
        Img->SetBrush(B);

        const int32 Row = i / Cols;
        const int32 Col = i % Cols;

        if (UUniformGridSlot* CSlot = medals_grid->AddChildToUniformGrid(SB, Row, Col))
        {
            CSlot->SetHorizontalAlignment(HAlign_Center);
            CSlot->SetVerticalAlignment(VAlign_Center);
        }

        MedalImages.Add(Img);
    }
}

void UPlayerDlg::BuildChatMacrosRows()
{
    if (!WidgetTree || !StatsVBox)
        return;

    MacroEdits.SetNum(10);

    // 1..9 then 0 (NO LAMBDAS)
    for (int32 i = 1; i <= 9; ++i)
    {
        UEditableTextBox* E = MakeEditBox(false);
        MacroEdits[i] = E;

        AddStatRow(FText::FromString(FString::FromInt(i)), E);
    }

    {
        UEditableTextBox* E = MakeEditBox(false);
        MacroEdits[0] = E;

        AddStatRow(FText::FromString(TEXT("0")), E);
    }
}

// ------------------------------------------------------------------------
// Model <-> UI (SUBSYSTEM)
// ------------------------------------------------------------------------

void UPlayerDlg::UpdatePlayerFromUI_Subsystem()
{
    UStarshatterPlayerSubsystem* SS = GetPlayerSS();
    if (!SS)
        return;

    // Make sure it has loaded at least once
    if (!SS->HasLoaded())
        SS->LoadPlayer();

    FS_PlayerGameInfo& Info = SS->GetMutablePlayerInfo();

    if (edt_name)
    {
        const FString NewName = edt_name->GetText().ToString().TrimStartAndEnd();
        if (!NewName.IsEmpty())
            Info.Name = NewName;
    }

    if (edt_callsign)
        Info.Callsign = edt_callsign->GetText().ToString().TrimStartAndEnd();

    if (edt_squadron)
        Info.Squadron = edt_squadron->GetText().ToString().TrimStartAndEnd();

    if (edt_signature)
        Info.Signature = edt_signature->GetText().ToString().TrimStartAndEnd();

    // Chat macros (10)
    if (MacroEdits.Num() == 10)
    {
        Info.ChatMacros.SetNum(10);
        for (int32 i = 0; i < 10; ++i)
        {
            if (MacroEdits[i])
                Info.ChatMacros[i] = MacroEdits[i]->GetText().ToString();
        }
    }

    SS->SavePlayer(true);
}

void UPlayerDlg::RefreshUIFromSubsystem()
{
    UStarshatterPlayerSubsystem* SS = GetPlayerSS();
    if (!SS)
    {
        UE_LOG(LogPlayerDlg, Warning, TEXT("[PlayerDlg] RefreshUIFromSubsystem: PlayerSubsystem is NULL"));
        return;
    }

    if (!SS->HasLoaded())
        SS->LoadPlayer();

    const FS_PlayerGameInfo& Info = SS->GetPlayerInfo();

    UE_LOG(LogPlayerDlg, Warning, TEXT("[PlayerDlg] Refresh: Name='%s' Callsign='%s' Empire=%d Rank=%d"),
        *Info.Name, *Info.Callsign, Info.Empire, Info.Rank);

    // ------------------------------------------------------------
    // Left panel
    // ------------------------------------------------------------
    if (txt_profile_name)
        txt_profile_name->SetText(FText::FromString(Info.Name.IsEmpty() ? TEXT("<UNNAMED>") : Info.Name));

    // Editable fields
    if (edt_name)      edt_name->SetText(FText::FromString(Info.Name));
    if (edt_callsign)  edt_callsign->SetText(FText::FromString(Info.Callsign));
    if (edt_squadron)  edt_squadron->SetText(FText::FromString(Info.Squadron));
    if (edt_signature) edt_signature->SetText(FText::FromString(Info.Signature));

    // Password not in FS_PlayerGameInfo (leave blank)
    if (edt_password)
        edt_password->SetText(FText::GetEmpty());

    // Read-only
    if (txt_created)    txt_created->SetText(FText::FromString(UFormattingUtils::FormatDateFromUnixSeconds(Info.CreateTime)));
    if (txt_flighttime) txt_flighttime->SetText(FText::FromString(UFormattingUtils::FormatTimeHMS((double)Info.FlightTime)));
    if (txt_missions)   txt_missions->SetText(FText::AsNumber(Info.PlayerMissions));
    if (txt_kills)      txt_kills->SetText(FText::AsNumber(Info.PlayerKills));
    if (txt_losses)     txt_losses->SetText(FText::AsNumber(Info.PlayerLosses));
    if (txt_points)     txt_points->SetText(FText::AsNumber(Info.PlayerPoints));

    // ------------------------------------------------------------
    // Empire text
    // ------------------------------------------------------------
    if (txt_empire)
    {
        const UEnum* Enum = StaticEnum<EEMPIRE_NAME>();
        if (Enum && Enum->IsValidEnumValue(Info.Empire))
            txt_empire->SetText(Enum->GetDisplayNameTextByValue((int64)Info.Empire));
        else
            txt_empire->SetText(FText::FromString(TEXT("UNKNOWN EMPIRE")));
    }

    // ------------------------------------------------------------
    // GameData subsystem for Rank + Medals
    // Rank is 0-based in save; tables are 1-based -> +1.
    // ------------------------------------------------------------
    const int32 RankId = Info.Rank + 1;

    UStarshatterGameDataSubsystem* GD = nullptr;
    if (UWorld* W = GetWorld())
    {
        if (UGameInstance* GI = W->GetGameInstance())
            GD = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    }

    if (GD)
    {
        // -------------------------
        // RANK (text + 64x64 icon)
        // -------------------------
        FRankInfo RankRow;
        const bool bHaveRank = GD->GetRankInfo(RankId, RankRow);

        if (txt_rank)
        {
            const FString RankNameStr = bHaveRank ? RankRow.Name : FString();
            txt_rank->SetText(!RankNameStr.IsEmpty()
                ? FText::FromString(RankNameStr)
                : FText::FromString(TEXT("UNKNOWN RANK")));
        }

        if (img_rank)
        {
            UTexture2D* Tex = nullptr;

            if (bHaveRank)
            {
                // NOTE: adjust field name if your struct uses SmallRankTexture etc.
                if (RankRow.SmallImage.IsValid())
                    Tex = RankRow.SmallImage.Get();
                else if (!RankRow.SmallImage.IsNull())
                    Tex = RankRow.SmallImage.LoadSynchronous();
            }

            img_rank->SetBrushFromTexture(Tex, true);
            SetImageTextureFixed64(img_rank, Tex); // ensure brush ImageSize=64x64 + desired size override
            img_rank->SetVisibility(Tex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        }

        // -------------------------
        // MEDALS (fixed 16-slot 4x4 board)
        // bit 0..15 => MedalId 1..16
        // -------------------------
        //ApplyMedalsMask((uint32)Info.MedalsMask);
        ApplyMedalsMask(0xFFFF);
    }
    else
    {
        // Fallback: legacy registry for text only
        if (txt_rank)
        {
            const TCHAR* RankName = UAwardInfoRegistry::RankName(RankId);
            txt_rank->SetText((RankName && FCString::Strlen(RankName) > 0)
                ? FText::FromString(RankName)
                : FText::FromString(TEXT("UNKNOWN RANK")));
        }

        if (img_rank)
            img_rank->SetVisibility(ESlateVisibility::Collapsed);

        // If no GD, just dim/clear medals
        ApplyMedalsMask((uint32)Info.MedalsMask);
    }

    // ------------------------------------------------------------
    // Macros
    // ------------------------------------------------------------
    if (MacroEdits.Num() == 10)
    {
        for (int32 i = 0; i < 10; ++i)
        {
            if (!MacroEdits[i])
                continue;

            const FString V = Info.ChatMacros.IsValidIndex(i) ? Info.ChatMacros[i] : FString();
            MacroEdits[i]->SetText(FText::FromString(V));
        }
    }
}

// ------------------------------------------------------------------------
// Actions
// ------------------------------------------------------------------------

void UPlayerDlg::OnAdd()
{
    UE_LOG(LogPlayerDlg, Warning, TEXT("[PlayerDlg] Add is disabled for single-profile subsystem flow."));
}

void UPlayerDlg::OnDel()
{
    UE_LOG(LogPlayerDlg, Warning, TEXT("[PlayerDlg] Delete is disabled for single-profile subsystem flow."));
}

void UPlayerDlg::OnApply()
{
    UpdatePlayerFromUI_Subsystem();

    if (manager)
        manager->ShowMenuDlg();
    else
        HideDlg();
}

void UPlayerDlg::OnCancel()
{
    RefreshUIFromSubsystem();

    if (manager)
        manager->ShowMenuDlg();
    else
        HideDlg();
}

void UPlayerDlg::ApplyMedalsMask(uint32 MedalsMask)
{
    // Safety
    if (MedalImages.Num() == 0)
        return;

    UStarshatterGameDataSubsystem* GD = nullptr;
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            GD = GI->GetSubsystem<UStarshatterGameDataSubsystem>();

    // ------------------------------------------------------------
    // Clear all 16 slots first
    // ------------------------------------------------------------
   // Clear all slots to transparent
    for (int32 i = 0; i < MedalImages.Num(); ++i)
    {
        if (UImage* Img = MedalImages[i])
        {
            Img->SetBrushFromTexture(nullptr, true);
            Img->SetOpacity(0.f);   // fully transparent
            Img->SetVisibility(ESlateVisibility::HitTestInvisible);

            // Still force 64x64 so layout doesn't collapse
            FSlateBrush B = Img->GetBrush();
            B.ImageSize = FVector2D(64.f, 64.f);
            Img->SetBrush(B);
        }
    }

    if (!GD)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ApplyMedalsMask] GameDataSubsystem missing."));
        return;
    }

    // ------------------------------------------------------------
    // For each of the 16 possible medal flags
    // ------------------------------------------------------------
    const int32 MaxSlots = FMath::Min(16, MedalImages.Num());

    for (int32 XSlot = 0; XSlot < MaxSlots; ++XSlot)
    {
        const uint32 MedalFlag = (1u << XSlot);

        // Player does not have this medal
        if ((MedalsMask & MedalFlag) == 0)
            continue;

        FMedalInfo Row;
        if (!GD->GetMedalInfoByFlag(MedalFlag, Row))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[ApplyMedalsMask] Owned medal flag %u but no row found."),
                MedalFlag);
            continue;
        }

        UTexture2D* Tex = nullptr;

        if (Row.SmallImage.IsValid())
            Tex = Row.SmallImage.Get();
        else if (!Row.SmallImage.IsNull())
            Tex = Row.SmallImage.LoadSynchronous();

        if (UImage* Img = MedalImages[XSlot])
        {
            Img->SetOpacity(1.0f);
            Img->SetBrushFromTexture(Tex, true);

            FSlateBrush B = Img->GetBrush();
            B.ImageSize = FVector2D(64.f, 64.f);
            Img->SetBrush(B);
        }
    }
}

void UPlayerDlg::Debug_ShowAllMedals()
{
    const int32 Count = FMath::Min(MedalImages.Num(), 16);

    UStarshatterGameDataSubsystem* GD = nullptr;
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            GD = GI->GetSubsystem<UStarshatterGameDataSubsystem>();

    for (int32 i = 0; i < Count; ++i)
    {
        UImage* Img = MedalImages[i];
        if (!Img) continue;

        const int32 MedalId = i + 1;

        UTexture2D* MedalTex = nullptr;

        if (GD)
        {
            FMedalInfo Row;
            if (GD->GetMedalInfo(MedalId, Row))
            {
                if (Row.SmallImage.IsValid())
                {
                    MedalTex = Row.SmallImage.Get();
                }
                else if (!Row.SmallImage.IsNull())
                {
                    MedalTex = Row.SmallImage.LoadSynchronous();
                }

                UE_LOG(LogTemp, Warning, TEXT("[MedalTest] MedalId=%d Soft=%s Tex=%s"),
                    MedalId,
                    *Row.SmallImage.ToSoftObjectPath().ToString(),
                    MedalTex ? *MedalTex->GetName() : TEXT("NULL"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[MedalTest] MedalId=%d NOT FOUND in cache/table"), MedalId);
            }
        }

        Img->SetVisibility(ESlateVisibility::HitTestInvisible);
        Img->SetOpacity(1.0f);
        Img->SetBrushFromTexture(MedalTex, true);

        // Enforce 64x64 brush size
        FSlateBrush B = Img->GetBrush();
        B.ImageSize = FVector2D(64.f, 64.f);
        Img->SetBrush(B);
    }
}



