/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    FILE: GameStructs_UI.h

    OVERVIEW
    ========
    Shared UI data contracts:
    - DT row definitions
    - enums
    - no runtime logic
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Styling/SlateTypes.h"
#include "GameStructs_UI.generated.h"

// ------------------------------------------------------------
// Form control type
// ------------------------------------------------------------

// ------------------------------------------------------------
// Control Type (frm "type")
// ------------------------------------------------------------
UENUM(BlueprintType)
enum class EUIControlType : uint8
{
	Label      UMETA(DisplayName = "Label"),
	Button     UMETA(DisplayName = "Button"),
	Combo      UMETA(DisplayName = "Combo"),
	Edit       UMETA(DisplayName = "Edit"),
	Image      UMETA(DisplayName = "Image"),
	Slider     UMETA(DisplayName = "Slider"),
	List       UMETA(DisplayName = "List"),
	RichText   UMETA(DisplayName = "RichText"),
	Panel      UMETA(DisplayName = "Panel"),       
	Background UMETA(DisplayName = "Background"),  
};

// -----------------------------------------------------------------------------
// ESSWThemeFont
// -----------------------------------------------------------------------------
// Enumerates supported UI font families for Starshatter Wars.
// -----------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESSWThemeFont : uint8
{
	LimerickBold      UMETA(DisplayName = "Limerick Bold"),
	VerdanaItalic     UMETA(DisplayName = "Verdana Italic"),
	Serpntb           UMETA(DisplayName = "Serpent Bold")
};

// ------------------------------------------------------------
// Layout Def (frm "layout")
// x_mins / cols, y_mins / rows, x_weights / col_wts, y_weights / row_wts
// ------------------------------------------------------------
USTRUCT(BlueprintType)
struct FS_LayoutDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> XMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> YMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> XWeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> YWeight;
};

// ------------------------------------------------------------
// Single UI Control Def (frm "defctrl" and "ctrl")
// Notes:
// - Rect is pixel-space in legacy; keep as FIntRect for now.
// - Insets/margins are UE-native FMargin.
// - Colors are UE-native FColor.
// - Style/Align are stored as legacy numeric flags for later mapping.
// ------------------------------------------------------------
USTRUCT(BlueprintType)
struct FS_UIControlDef
{
	GENERATED_BODY()

	// Identity
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Id = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ParentId = 0;

	// Type
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EUIControlType Type = EUIControlType::Label;

	// Textual
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Caption;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Alt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Font;

	// Geometry
	// IMPORTANT: Decide your Rect convention globally (XYWH vs LTRB).
	// Recommended for legacy FRM: treat as XYWH (X,Y,Width,Height) during parsing,
	// then store as: Min=(X,Y) Max=(X+W, Y+H) in FIntRect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntRect Rect = FIntRect(0, 0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin Margins = FMargin(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin TextInsets = FMargin(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin CellInsets = FMargin(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntRect Cells = FIntRect(0, 0, 0, 0);

	// Fixed sizing constraints (if present)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FixedWidth = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FixedHeight = 0;

	// Colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor ActiveColor = FColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor BackColor = FColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor BaseColor = FColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor BorderColor = FColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor ForeColor = FColor::White;

	// Assets / images
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Texture;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString StandardImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ActivatedImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString TransitionImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Picture;

	// List / combo items
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Item;

	// Password
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Password;

	// Legacy flags / options
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSmoothScroll = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSingleLine = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAnimated = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBorder = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDropShadow = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bShowHeadings = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSticky = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTransparent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHidePartial = false;

	// Misc numeric
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Tab = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Orientation = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Leading = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LineHeight = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MultiSelect = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 DragDrop = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ScrollBar = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PictureLoc = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PictureType = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 NumLeds = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ItemStyle = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SelectedStyle = 0;

	// Raw legacy style bits (frm "style")
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Style = 0;

	// Raw legacy bevel width (frm "bevel_width")
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BevelWidth = 0;

	// Raw legacy alignment (frm "align" or "text_align")
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Align = 0; // DT_LEFT/DT_RIGHT/DT_CENTER, etc.
};

// ------------------------------------------------------------
// Form Design (frm "FORM" / "form")
// ------------------------------------------------------------
USTRUCT(BlueprintType)
struct FS_FormDesign : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Caption;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Id = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntRect Rect = FIntRect(0, 0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor BackColor = FColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor BaseColor = FColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor ForeColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin Insets = FMargin(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin TextInsets = FMargin(0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FMargin CellInsets = FMargin(0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntRect Cells = FIntRect(0, 0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Style = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Align = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FS_LayoutDef LayoutDef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FS_UIControlDef DefaultCtrl;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FS_UIControlDef> Controls;
};
