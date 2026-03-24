/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterFormSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Loads and parses legacy Starshatter .frm UI definitions into Unreal-native
    structs (FS_FormDesign, FS_UIControlDef, FS_LayoutDef).

    This subsystem performs NO widget creation and NO rendering.
*/

#include "StarshatterFormSubsystem.h" // MUST be first include

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"
#include "Engine/DataTable.h"

#if WITH_EDITOR
#include "Editor.h"
#include "DataTableEditorUtils.h"
#include "UObject/Package.h"
#include "Misc/ScopedSlowTask.h"
#endif

// Legacy parser stack:
#include "DataLoader.h"
#include "Parser.h"
#include "Term.h"
#include "ParseUtil.h"

// Optional: if your parse helpers depend on these legacy headers:
#include "Text.h"
#include "FormatUtil.h"
#include "Random.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterForms, Log, All);

// ------------------------------------------------------------
// Local helpers (static, no reliance on `this`)
// ------------------------------------------------------------

static void SetRectXYWH(FIntRect& Out, int32 X, int32 Y, int32 W, int32 H)
{
    Out.Min.X = X;
    Out.Min.Y = Y;
    Out.Max.X = X + W;
    Out.Max.Y = Y + H;
}

static void SetMargin(FMargin& Out, const Insets& In)
{
    Out.Left = (float)In.left;
    Out.Top = (float)In.top;
    Out.Right = (float)In.right;
    Out.Bottom = (float)In.bottom;
}

static FColor ColorFromVec3_255(const FVector& V)
{
    return FColor(
        (uint8)FMath::Clamp(FMath::RoundToInt(V.X), 0, 255),
        (uint8)FMath::Clamp(FMath::RoundToInt(V.Y), 0, 255),
        (uint8)FMath::Clamp(FMath::RoundToInt(V.Z), 0, 255),
        255
    );
}

static void ParseLayoutDef_Internal(TermStruct* Val, const char* Fn, FS_LayoutDef& OutLayout)
{
    if (!Val)
        return;

    std::vector<DWORD> x_mins;
    std::vector<DWORD> y_mins;
    std::vector<float> x_weights;
    std::vector<float> y_weights;

    FS_LayoutDef NewLayout;

    for (int i = 0; i < (int)Val->elements()->size(); i++)
    {
        TermDef* PDef = Val->elements()->at(i)->isDef();
        if (!PDef)
            continue;

        const Text& Key = PDef->name()->value();

        if (Key == "x_mins" || Key == "cols")
        {
            GetDefArray(x_mins, PDef, Fn);
            NewLayout.XMin.SetNum((int32)x_mins.size());
            for (int32 idx = 0; idx < (int32)x_mins.size(); idx++)
                NewLayout.XMin[idx] = (int32)x_mins[idx];
        }
        else if (Key == "y_mins" || Key == "rows")
        {
            GetDefArray(y_mins, PDef, Fn);
            NewLayout.YMin.SetNum((int32)y_mins.size());
            for (int32 idx = 0; idx < (int32)y_mins.size(); idx++)
                NewLayout.YMin[idx] = (int32)y_mins[idx];
        }
        else if (Key == "x_weights" || Key == "col_wts")
        {
            GetDefArray(x_weights, PDef, Fn);
            NewLayout.XWeight.SetNum((int32)x_weights.size());
            for (int32 idx = 0; idx < (int32)x_weights.size(); idx++)
                NewLayout.XWeight[idx] = x_weights[idx];
        }
        else if (Key == "y_weights" || Key == "row_wts")
        {
            GetDefArray(y_weights, PDef, Fn);
            NewLayout.YWeight.SetNum((int32)y_weights.size());
            for (int32 idx = 0; idx < (int32)y_weights.size(); idx++)
                NewLayout.YWeight[idx] = y_weights[idx];
        }
    }

    OutLayout = NewLayout;
}

static void ParseCtrlDef_Internal(TermStruct* Val, const char* Fn, FS_UIControlDef& OutCtrl)
{
    if (!Val)
        return;

    Text Buf;

    for (int i = 0; i < (int)Val->elements()->size(); i++)
    {
        TermDef* PDef = Val->elements()->at(i)->isDef();
        if (!PDef)
            continue;

        const Text& Key = PDef->name()->value();

        // Strings
        if (Key == "text") { GetDefText(Buf, PDef, Fn); OutCtrl.Text = ANSI_TO_TCHAR(Buf); }
        else if (Key == "caption") { GetDefText(Buf, PDef, Fn); OutCtrl.Caption = ANSI_TO_TCHAR(Buf); }
        else if (Key == "alt") { GetDefText(Buf, PDef, Fn); OutCtrl.Alt = ANSI_TO_TCHAR(Buf); }
        else if (Key == "font") { GetDefText(Buf, PDef, Fn); OutCtrl.Font = ANSI_TO_TCHAR(Buf); }
        else if (Key == "texture") { GetDefText(Buf, PDef, Fn); OutCtrl.Texture = ANSI_TO_TCHAR(Buf); }
        else if (Key == "standard_image") { GetDefText(Buf, PDef, Fn); OutCtrl.StandardImage = ANSI_TO_TCHAR(Buf); }
        else if (Key == "activated_image") { GetDefText(Buf, PDef, Fn); OutCtrl.ActivatedImage = ANSI_TO_TCHAR(Buf); }
        else if (Key == "transition_image") { GetDefText(Buf, PDef, Fn); OutCtrl.TransitionImage = ANSI_TO_TCHAR(Buf); }
        else if (Key == "picture") { GetDefText(Buf, PDef, Fn); OutCtrl.Picture = ANSI_TO_TCHAR(Buf); }
        else if (Key == "item") { GetDefText(Buf, PDef, Fn); OutCtrl.Item = ANSI_TO_TCHAR(Buf); }
        else if (Key == "password") { Text Pw; GetDefText(Pw, PDef, Fn); OutCtrl.Password = ANSI_TO_TCHAR(Pw); }

        // Ids
        else if (Key == "id") { DWORD id = 0; GetDefNumber(id, PDef, Fn); OutCtrl.Id = (int32)id; }
        else if (Key == "pid") { DWORD pid = 0; GetDefNumber(pid, PDef, Fn); OutCtrl.ParentId = (int32)pid; }

        // Type
        else if (Key == "type")
        {
            GetDefText(Buf, PDef, Fn);
            Text TypeName(Buf);

            if (TypeName == "button")      OutCtrl.Type = EUIControlType::Button;
            else if (TypeName == "combo")  OutCtrl.Type = EUIControlType::Combo;
            else if (TypeName == "edit")   OutCtrl.Type = EUIControlType::Edit;
            else if (TypeName == "image")  OutCtrl.Type = EUIControlType::Image;
            else if (TypeName == "slider") OutCtrl.Type = EUIControlType::Slider;
            else if (TypeName == "list")   OutCtrl.Type = EUIControlType::List;
            else if (TypeName == "rich" || TypeName == "text" || TypeName == "rich_text")
                OutCtrl.Type = EUIControlType::RichText;
            else
                OutCtrl.Type = EUIControlType::Label;
        }

        // Geometry
        else if (Key == "rect") { Rect r{}; GetDefRect(r, PDef, Fn); SetRectXYWH(OutCtrl.Rect, (int32)r.x, (int32)r.y, (int32)r.w, (int32)r.h); }
        else if (Key == "margins") { Insets m{}; GetDefInsets(m, PDef, Fn); SetMargin(OutCtrl.Margins, m); }
        else if (Key == "text_insets") { Insets t{}; GetDefInsets(t, PDef, Fn); SetMargin(OutCtrl.TextInsets, t); }
        else if (Key == "cell_insets") { Insets ci{}; GetDefInsets(ci, PDef, Fn); SetMargin(OutCtrl.CellInsets, ci); }
        else if (Key == "cells") { Rect c{}; GetDefRect(c, PDef, Fn); SetRectXYWH(OutCtrl.Cells, (int32)c.x, (int32)c.y, (int32)c.w, (int32)c.h); }
        else if (Key == "fixed_width") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.FixedWidth = n; }
        else if (Key == "fixed_height") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.FixedHeight = n; }

        // Colors
        else if (Key == "active_color") { FVector v{}; GetDefVec(v, PDef, Fn); OutCtrl.ActiveColor = ColorFromVec3_255(v); }
        else if (Key == "back_color") { FVector v{}; GetDefVec(v, PDef, Fn); OutCtrl.BackColor = ColorFromVec3_255(v); }
        else if (Key == "base_color") { FVector v{}; GetDefVec(v, PDef, Fn); OutCtrl.BaseColor = ColorFromVec3_255(v); }
        else if (Key == "border_color") { FVector v{}; GetDefVec(v, PDef, Fn); OutCtrl.BorderColor = ColorFromVec3_255(v); }
        else if (Key == "fore_color") { FVector v{}; GetDefVec(v, PDef, Fn); OutCtrl.ForeColor = ColorFromVec3_255(v); }

        // Bools
        else if (Key == "enabled") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bEnabled = b; }
        else if (Key == "smooth_scroll") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bSmoothScroll = b; }
        else if (Key == "single_line") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bSingleLine = b; }
        else if (Key == "active") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bActive = b; }
        else if (Key == "animated") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bAnimated = b; }
        else if (Key == "border") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bBorder = b; }
        else if (Key == "drop_shadow") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bDropShadow = b; }
        else if (Key == "show_headings") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bShowHeadings = b; }
        else if (Key == "sticky") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bSticky = b; }
        else if (Key == "transparent") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bTransparent = b; }
        else if (Key == "hide_partial") { bool b = false; GetDefBool(b, PDef, Fn); OutCtrl.bHidePartial = b; }

        // Ints
        else if (Key == "tab") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.Tab = n; }
        else if (Key == "orientation") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.Orientation = n; }
        else if (Key == "leading") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.Leading = n; }
        else if (Key == "line_height") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.LineHeight = n; }
        else if (Key == "multiselect") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.MultiSelect = n; }
        else if (Key == "dragdrop") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.DragDrop = n; }
        else if (Key == "scroll_bar") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.ScrollBar = n; }
        else if (Key == "picture_loc") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.PictureLoc = n; }
        else if (Key == "picture_type") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.PictureType = n; }
        else if (Key == "num_leds") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.NumLeds = n; }
        else if (Key == "item_style") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.ItemStyle = n; }
        else if (Key == "selected_style") { int n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.SelectedStyle = n; }

        // Raw bits
        else if (Key == "style") { DWORD n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.Style = (int32)n; }
        else if (Key == "bevel_width") { DWORD n = 0; GetDefNumber(n, PDef, Fn); OutCtrl.BevelWidth = (int32)n; }

        else if (Key == "align" || Key == "text_align")
        {
            DWORD a = DT_LEFT;

            if (GetDefText(Buf, PDef, Fn))
            {
                if (!_stricmp(Buf, "left"))        a = DT_LEFT;
                else if (!_stricmp(Buf, "right"))  a = DT_RIGHT;
                else if (!_stricmp(Buf, "center")) a = DT_CENTER;
            }
            else
            {
                GetDefNumber(a, PDef, Fn);
            }

            OutCtrl.Align = (int32)a;
        }
    }
}

// ------------------------------------------------------------
// Subsystem lifecycle
// ------------------------------------------------------------

void UStarshatterFormSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] Initialize()"));

    FormDefDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_FormDef.DT_FormDef"));

    if (!FormDefDataTable)
    {
        UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] DT_FormDef not found (ok). Using in-memory cache only."));
    }
}

void UStarshatterFormSubsystem::Deinitialize()
{
    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] Deinitialize()"));

    FormsByName.Empty();
    bFormsLoaded = false;

    Super::Deinitialize();
}

void UStarshatterFormSubsystem::BootLoadForms()
{
    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] BootLoadForms()"));
    LoadForms();
}

const FS_FormDesign* UStarshatterFormSubsystem::FindFormByName(const FName& FormName) const
{
    return FormsByName.Find(FormName);
}

void UStarshatterFormSubsystem::LoadForms()
{
    if (bFormsLoaded)
    {
        UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] LoadForms() skipped (already loaded)"));
        return;
    }

    FormsByName.Empty();

    const FString FormsDir = FPaths::ProjectContentDir() / TEXT("GameData/Screens/");
    const FString Wildcard = FormsDir / TEXT("*.frm");

    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *Wildcard, /*Files*/true, /*Directories*/false);

    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] Found %d .frm files in %s"), Files.Num(), *FormsDir);

    for (const FString& Leaf : Files)
    {
        const FString FullPath = FormsDir / Leaf;
        UE_LOG(LogStarshatterForms, Verbose, TEXT("[FORMS] Parsing: %s"), *FullPath);

        const FTCHARToUTF8 Utf8Path(*FullPath);
        LoadForm(Utf8Path.Get()); // IMPORTANT: LoadForm must only cache, not write DT
    }

    bFormsLoaded = true;

    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] LoadForms complete. Cached=%d"), FormsByName.Num());
}

// ------------------------------------------------------------
// Per-file parsing
// ------------------------------------------------------------

void UStarshatterFormSubsystem::LoadForm(const char* InFilename)
{
    if (!InFilename || !*InFilename)
    {
        UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] LoadForm called with null/empty filename"));
        return;
    }

    // NOTE: InFilename should be UTF-8 in your pipeline (FTCHARToUTF8::Get()).
    const FString FormFilePath = UTF8_TO_TCHAR(InFilename);

    if (!FPaths::FileExists(FormFilePath))
    {
        UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] Form file not found: %s"), *FormFilePath);
        return;
    }

    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *FormFilePath))
    {
        UE_LOG(LogStarshatterForms, Error, TEXT("[FORMS] Failed to read form file: %s"), *FormFilePath);
        return;
    }

    Bytes.Add(0); // null terminate for BlockReader

    // Stable UTF-8 filename for legacy GetDef* helpers:
    const FTCHARToUTF8 FnUtf8(*FormFilePath);
    const char* Fn = FnUtf8.Get();

    Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    Term* TermPtr = ParserObj.ParseTerm();

    if (!TermPtr)
    {
        UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] Failed to parse form file (no terms): %s"), *FormFilePath);
        return;
    }

    // Header check: first token must be FORM
    {
        TermText* FileType = TermPtr->isText();
        if (!FileType || FileType->value() != "FORM")
        {
            UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] Invalid form header (expected FORM): %s"), *FormFilePath);
            delete TermPtr;
            return;
        }
    }

    FS_FormDesign NewForm;

    // FormName: Content/GameData/Screens/Foo.frm -> Foo
    FString FormName = FormFilePath;
    FormName.ReplaceInline(TEXT("\\"), TEXT("/"));

    FString Root = (FPaths::ProjectContentDir() / TEXT("GameData/Screens/"));
    Root.ReplaceInline(TEXT("\\"), TEXT("/"));

    FormName.RemoveFromStart(Root);
    FormName.RemoveFromEnd(TEXT(".frm"));

    NewForm.Name = FormName;

    FS_LayoutDef LocalLayout; // per-form scratch

    // Parse remaining terms:
    while (true)
    {
        delete TermPtr;
        TermPtr = ParserObj.ParseTerm();
        if (!TermPtr)
            break;

        TermDef* Def = TermPtr->isDef();
        if (!Def)
            continue;

        if (Def->name()->value() != "form")
            continue;

        if (!Def->term() || !Def->term()->isStruct())
        {
            UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] form structure missing in '%s'"), *FormFilePath);
            continue;
        }

        TermStruct* FormStruct = Def->term()->isStruct();

        for (int32 i = 0; i < (int32)FormStruct->elements()->size(); ++i)
        {
            TermDef* PDef = FormStruct->elements()->at(i)->isDef();
            if (!PDef)
                continue;

            const Text& Key = PDef->name()->value();
            Text Buf;

            if (Key == "text" || Key == "caption")
            {
                GetDefText(Buf, PDef, Fn);
                NewForm.Caption = ANSI_TO_TCHAR(Buf);
            }
            else if (Key == "id")
            {
                DWORD id = 0;
                GetDefNumber(id, PDef, Fn);
                NewForm.Id = (int32)id;
            }
            else if (Key == "pid")
            {
                DWORD pid = 0;
                GetDefNumber(pid, PDef, Fn);
                NewForm.PId = (int32)pid;
            }
            else if (Key == "rect")
            {
                Rect r{};
                GetDefRect(r, PDef, Fn);
                SetRectXYWH(NewForm.Rect, (int32)r.x, (int32)r.y, (int32)r.w, (int32)r.h);
            }
            else if (Key == "font")
            {
                GetDefText(Buf, PDef, Fn);
                NewForm.Font = ANSI_TO_TCHAR(Buf);
            }
            else if (Key == "back_color")
            {
                FVector c{};
                GetDefVec(c, PDef, Fn);
                NewForm.BackColor = ColorFromVec3_255(c);
            }
            else if (Key == "base_color")
            {
                FVector c{};
                GetDefVec(c, PDef, Fn);
                NewForm.BaseColor = ColorFromVec3_255(c);
            }
            else if (Key == "fore_color")
            {
                FVector c{};
                GetDefVec(c, PDef, Fn);
                NewForm.ForeColor = ColorFromVec3_255(c);
            }
            else if (Key == "margins")
            {
                Insets m{};
                GetDefInsets(m, PDef, Fn);
                SetMargin(NewForm.Insets, m);
            }
            else if (Key == "text_insets")
            {
                Insets t{};
                GetDefInsets(t, PDef, Fn);
                SetMargin(NewForm.TextInsets, t);
            }
            else if (Key == "cell_insets")
            {
                Insets ci{};
                GetDefInsets(ci, PDef, Fn);
                SetMargin(NewForm.CellInsets, ci);
            }
            else if (Key == "cells")
            {
                Rect c{};
                GetDefRect(c, PDef, Fn);
                SetRectXYWH(NewForm.Cells, (int32)c.x, (int32)c.y, (int32)c.w, (int32)c.h);
            }
            else if (Key == "texture")
            {
                GetDefText(Buf, PDef, Fn);
                NewForm.Texture = ANSI_TO_TCHAR(Buf);
            }
            else if (Key == "transparent")
            {
                bool b = false;
                GetDefBool(b, PDef, Fn);
                NewForm.bTransparent = b;
            }
            else if (Key == "style")
            {
                DWORD s = 0;
                GetDefNumber(s, PDef, Fn);
                NewForm.Style = (int32)s;
            }
            else if (Key == "align" || Key == "text_align")
            {
                DWORD a = DT_LEFT;

                if (GetDefText(Buf, PDef, Fn))
                {
                    if (!_stricmp(Buf, "left"))        a = DT_LEFT;
                    else if (!_stricmp(Buf, "right"))  a = DT_RIGHT;
                    else if (!_stricmp(Buf, "center")) a = DT_CENTER;
                }
                else
                {
                    GetDefNumber(a, PDef, Fn);
                }

                NewForm.Align = (int32)a;
            }
            else if (Key == "layout")
            {
                if (PDef->term() && PDef->term()->isStruct())
                {
                    ParseLayoutDef_Internal(PDef->term()->isStruct(), Fn, LocalLayout);
                    NewForm.LayoutDef = LocalLayout;
                }
                else
                {
                    UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] layout structure missing in '%s'"), *FormFilePath);
                }
            }
            else if (Key == "defctrl")
            {
                if (PDef->term() && PDef->term()->isStruct())
                {
                    NewForm.DefaultCtrl = FS_UIControlDef{};
                    ParseCtrlDef_Internal(PDef->term()->isStruct(), Fn, NewForm.DefaultCtrl);
                }
                else
                {
                    UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] defctrl structure missing in '%s'"), *FormFilePath);
                }
            }
            else if (Key == "ctrl")
            {
                if (PDef->term() && PDef->term()->isStruct())
                {
                    FS_UIControlDef Ctrl = NewForm.DefaultCtrl; // inherit defaults
                    ParseCtrlDef_Internal(PDef->term()->isStruct(), Fn, Ctrl);
                    NewForm.Controls.Add(Ctrl);
                }
                else
                {
                    UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] ctrl structure missing in '%s'"), *FormFilePath);
                }
            }
        }
    }

    if (TermPtr)
    {
        delete TermPtr;
        TermPtr = nullptr;
    }

    // ------------------------------------------------------------
    // Authoritative in-memory cache
    // ------------------------------------------------------------
    const FName RowName(*FormName);
    FormsByName.Add(RowName, NewForm);

    // ------------------------------------------------------------
    // Optional DT mirror (runtime-friendly; persistence is editor-only)
    // ------------------------------------------------------------
    if (!FormDefDataTable)
    {
        UE_LOG(LogStarshatterForms, Verbose, TEXT("[FORMS] No DT loaded; cached '%s' only"), *FormName);
        return;
    }

    if (FormDefDataTable->GetRowStruct() != FS_FormDesign::StaticStruct())
    {
        UE_LOG(LogStarshatterForms, Error,
            TEXT("[FORMS] DT RowStruct mismatch. Expected '%s' got '%s' (form '%s' not written)"),
            *FS_FormDesign::StaticStruct()->GetName(),
            FormDefDataTable->GetRowStruct() ? *FormDefDataTable->GetRowStruct()->GetName() : TEXT("NULL"),
            *FormName);
        return;
    }
    // Your RemoveRow/AddRow block here

    // Overwrite-safe:
    if (FormDefDataTable->FindRow<FS_FormDesign>(RowName, TEXT("LoadForm"), false))
    {
        FormDefDataTable->RemoveRow(RowName);
    }

    FormDefDataTable->AddRow(RowName, NewForm);

    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] Wrote DT row '%s'"), *RowName.ToString());

}

void UStarshatterFormSubsystem::RebuildFormDefDataTable_EditorOnly()
{
#if WITH_EDITOR
    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] RebuildFormDefDataTable_EditorOnly()"));

    if (!FormDefDataTable)
    {
        UE_LOG(LogStarshatterForms, Error, TEXT("[FORMS] DT_FormDef is null"));
        return;
    }

    if (FormDefDataTable->GetRowStruct() != FS_FormDesign::StaticStruct())
    {
        UE_LOG(LogStarshatterForms, Error,
            TEXT("[FORMS] DT RowStruct mismatch. Expected '%s' got '%s'"),
            *FS_FormDesign::StaticStruct()->GetName(),
            FormDefDataTable->GetRowStruct() ? *FormDefDataTable->GetRowStruct()->GetName() : TEXT("NULL"));
        return;
    }

    // Do not mutate asset DT during PIE:
    if (GEditor && GEditor->PlayWorld)
    {
        UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] Refusing to rebuild DT during PIE. Stop play first."));
        return;
    }

    // ------------------------------------------------------------
    // Build authoritative cache (parse all .frm)
    // ------------------------------------------------------------
    bFormsLoaded = false;     // force LoadForms() to run
    FormsByName.Reset();

    LoadForms();             // IMPORTANT: LoadForms/LoadForm must ONLY cache, not write DT

    // ------------------------------------------------------------
    // Clear + rebuild DT asset (editor-safe)
    // ------------------------------------------------------------
    FormDefDataTable->Modify();

    // Clear all rows:
    {
        const TArray<FName> Existing = FormDefDataTable->GetRowNames();
        for (const FName& Row : Existing)
        {
            FDataTableEditorUtils::RemoveRow(FormDefDataTable, Row);
        }
    }

    // Add rows from cache:
    for (const TPair<FName, FS_FormDesign>& It : FormsByName)
    {
        const FName RowName = It.Key;
        const FS_FormDesign& RowData = It.Value;

        FDataTableEditorUtils::AddRow(FormDefDataTable, RowName);

        uint8* DestRow = FormDefDataTable->FindRowUnchecked(RowName);
        check(DestRow);

        FS_FormDesign::StaticStruct()->CopyScriptStruct(DestRow, &RowData);
    }

    FormDefDataTable->MarkPackageDirty();
    FormDefDataTable->PostEditChange();

    UE_LOG(LogStarshatterForms, Log, TEXT("[FORMS] Rebuilt DT_FormDef: %d rows"), FormsByName.Num());
#else
    UE_LOG(LogStarshatterForms, Warning, TEXT("[FORMS] RebuildFormDefDataTable_EditorOnly called in non-editor build"));
#endif
}
