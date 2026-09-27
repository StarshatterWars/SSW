/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         FormDef.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Form and Control Definition Resources
*/

#pragma once

// Starshatter core types:
#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "List.h"

// Unreal (minimal):
#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath
#include "CoreTypes.h"                // uint32, uint16, uint8

// +--------------------------------------------------------------------+

class FormDef;    // values defining form style and control placement
class CtrlDef;    // values defining control style
class WinDef;     // base class for FormDef and CtrlDef
class TermStruct; // used for parsing

enum WinType {
	WIN_DEF_FORM,
	WIN_DEF_LABEL,
	WIN_DEF_BUTTON,
	WIN_DEF_COMBO,
	WIN_DEF_EDIT,
	WIN_DEF_IMAGE,
	WIN_DEF_SLIDER,
	WIN_DEF_LIST,
	WIN_DEF_RICH
};

// +--------------------------------------------------------------------+

class ColumnDef
{
public:
	static const char* TYPENAME() { return "ColumnDef"; }

	ColumnDef();
	ColumnDef(const char* title, int width, int align, int sort);

	Text   title;
	int    width;
	int    align;
	int    sort;
	FColor color;
	bool   use_color;
};

// +--------------------------------------------------------------------+

class LayoutDef
{
public:
	static const char* TYPENAME() { return "LayoutDef"; }

	// KEEP Starshatter List (no std::vector):
	List<uint32> x_mins;
	List<uint32> y_mins;
	List<float>  x_weights;
	List<float>  y_weights;
};

// +--------------------------------------------------------------------+

class WinDef
{
	friend class FormDef;

public:
	static const char* TYPENAME() { return "WinDef"; }

	WinDef(uint32 id, uint32 type, const char* text = 0, uint32 style = 0);
	virtual ~WinDef() {}

	int operator == (const WinDef& w) const { return id == w.id; }

	uint32         GetID() const { return id; }
	void           SetID(uint32 id);
	uint32         GetParentID() const { return pid; }
	void           SetParentID(uint32 id);
	uint32         GetType() const { return type; }
	void           SetType(uint32 type);

	void           SetRect(const Rect& r);
	Rect           GetRect() const { return rect; }
	int            GetX()    const { return rect.x; }
	int            GetY()    const { return rect.y; }
	int            GetW()    const { return rect.w; }
	int            GetH()    const { return rect.h; }

	void           SetEnabled(bool enable = true);
	bool           IsEnabled() const { return enabled; }

	void           SetStyle(uint32 s);
	uint32         GetStyle() const { return style; }

	void           SetFont(const char* t);
	const Text& GetFont() const { return font; }
	void           SetText(const char* t);
	const Text& GetText() const { return text; }
	void           SetAltText(const char* t);
	const Text& GetAltText() const { return alt_text; }
	void           SetTexture(const char* t);
	const Text& GetTexture() const { return texture; }

	void           SetBackColor(const FColor& c);
	FColor         GetBackColor()    const { return back_color; }
	void           SetBaseColor(const FColor& c);
	FColor         GetBaseColor()    const { return base_color; }
	void           SetForeColor(const FColor& c);
	FColor         GetForeColor()    const { return fore_color; }
	void           SetSingleLine(bool a);
	bool           GetSingleLine()   const { return single_line; }
	void           SetTextAlign(uint32 a);
	uint32         GetTextAlign()    const { return text_align; }
	void           SetTransparent(bool t);
	bool           GetTransparent()  const { return transparent; }
	void           SetHidePartial(bool a);
	bool           GetHidePartial()  const { return hide_partial; }

	void           SetMargins(const Insets& m);
	const Insets& GetMargins()      const { return margins; }
	void           SetTextInsets(const Insets& t);
	const Insets& GetTextInsets()   const { return text_insets; }
	void           SetCellInsets(const Insets& t);
	const Insets& GetCellInsets()   const { return cell_insets; }
	void           SetCells(const Rect& r);
	const Rect& GetCells()        const { return cells; }

	void           SetFixedWidth(int w) { fixed_width = w; }
	int            GetFixedWidth()   const { return fixed_width; }
	void           SetFixedHeight(int h) { fixed_height = h; }
	int            GetFixedHeight()  const { return fixed_height; }

	const LayoutDef& GetLayout()     const { return layout; }

protected:
	uint32         id;
	uint32         pid;
	uint32         type;
	Rect           rect;
	Text           font;
	Text           text;
	Text           alt_text;
	Text           texture;
	Text           picture;
	uint32         style;
	uint32         text_align;
	bool           single_line;
	bool           enabled;
	bool           transparent;
	bool           hide_partial;

	FColor         back_color;
	FColor         base_color;
	FColor         fore_color;

	Insets         margins;
	Insets         text_insets;
	Insets         cell_insets;
	Rect           cells;
	int            fixed_width;
	int            fixed_height;

	LayoutDef      layout;
};

// +--------------------------------------------------------------------+

class CtrlDef : public WinDef
{
public:
	static const char* TYPENAME() { return "CtrlDef"; }

	CtrlDef(uint32 id = 0, uint32 type = WIN_DEF_LABEL, const char* text = 0, uint32 style = 0);
	virtual ~CtrlDef();

	virtual CtrlDef& operator=(const CtrlDef& ctrl);

	bool     GetActive() const;
	void     SetActive(bool c);

	FColor   GetActiveColor() const;
	void     SetActiveColor(const FColor& c);

	bool     GetAnimated() const;
	void     SetAnimated(bool bNewValue);

	short    GetBevelWidth() const;
	void     SetBevelWidth(short nNewValue);

	bool     GetBorder() const;
	void     SetBorder(bool bNewValue);

	FColor   GetBorderColor() const;
	void     SetBorderColor(const FColor& c);

	bool     GetDropShadow() const;
	void     SetDropShadow(bool bNewValue);

	bool     GetIndent() const;
	void     SetIndent(bool bNewValue);

	bool     GetInvertLabel() const;
	void     SetInvertLabel(bool bNewValue);

	int      GetOrientation() const;
	void     SetOrientation(int o);

	Text     GetPicture() const;
	void     SetPicture(const Text& img_name);

	short    GetPictureLocation() const;
	void     SetPictureLocation(short nNewValue);

	short    GetPictureType() const;
	void     SetPictureType(short nNewValue);

	bool     GetSticky() const;
	void     SetSticky(bool bNewValue);

	int      GetNumLeds() const;
	void     SetNumLeds(int nNewValue);

	int      NumItems() const;
	Text     GetItem(int i) const;
	void     AddItem(const char* t);

	int         NumColumns() const;
	ColumnDef* GetColumn(int i) const;
	void        AddColumn(const char* t, int w, int a, int s);

	int      NumTabs() const;
	int      GetTab(int i) const;
	void     SetTab(int i, int t);
	void     AddTab(int i);

	bool     GetShowHeadings() const;
	void     SetShowHeadings(bool bNewValue);

	int      GetLeading() const;
	void     SetLeading(int nNewValue);

	int      GetLineHeight() const;
	void     SetLineHeight(int nNewValue);

	int      GetMultiSelect() const;
	void     SetMultiSelect(int nNewValue);

	int      GetDragDrop() const;
	void     SetDragDrop(int nNewValue);

	int      GetScrollBarVisible() const;
	void     SetScrollBarVisible(int nNewValue);

	bool     GetSmoothScroll() const;
	void     SetSmoothScroll(bool bNewValue);

	short    GetItemStyle() const;
	void     SetItemStyle(short nNewValue);

	short    GetSelectedStyle() const;
	void     SetSelectedStyle(short nNewValue);

	char     GetPasswordChar() const;
	void     SetPasswordChar(char c);

	Text     GetStandardImage() const;
	void     SetStandardImage(const Text& img_name);

	Text     GetActivatedImage() const;
	void     SetActivatedImage(const Text& img_name);

	Text     GetTransitionImage() const;
	void     SetTransitionImage(const Text& img_name);

protected:
	uint16            ctrl_flags;
	short             bevel_width;

	FColor            active_color;
	FColor            border_color;

	Text              picture;
	short             picture_loc;
	short             picture_type;

	Text              standard_image;
	Text              activated_image;
	Text              transition_image;

	bool              active;
	bool              show_headings;
	int               leading;
	int               line_height;
	int               multiselect;
	int               dragdrop;
	int               scroll_bar;
	int               orientation;
	int               num_leds;

	short             item_style;
	short             selected_style;

	bool              smooth_scroll;

	List<Text>        items;
	List<ColumnDef>   columns;

	int               ntabs;
	int               tabs[10];
	char              pass_char;
};

// +--------------------------------------------------------------------+

class FormDef : public WinDef
{
public:
	static const char* TYPENAME() { return "FormDef"; }

	FormDef(const char* text = 0, uint32 style = 0);
	virtual ~FormDef();

	void     Load(const char* filename);

	void     AddCtrl(CtrlDef* def);
	CtrlDef* FindCtrl(uint8 ctrl_id);

	ListIter<CtrlDef> GetControls() const;

protected:
	void     ParseCtrlDef(CtrlDef* ctrl, const TermStruct* val);
	void     ParseColumnDef(CtrlDef* ctrl, const TermStruct* val);
	void     ParseLayoutDef(LayoutDef* def, const TermStruct* val);

	void     ParseCtrlDef(CtrlDef* ctrl, TermStruct* val) { ParseCtrlDef(ctrl, (const TermStruct*)val); }
	void     ParseColumnDef(CtrlDef* ctrl, TermStruct* val) { ParseColumnDef(ctrl, (const TermStruct*)val); }
	void     ParseLayoutDef(LayoutDef* def, TermStruct* val) { ParseLayoutDef(def, (const TermStruct*)val); }

	CtrlDef       defctrl;
	List<CtrlDef> controls;
};
