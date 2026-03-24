/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         FormDef.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Form and Control Definition Resources
*/

#include "FormDef.h"
#include "ParseUtil.h"
#include "DataLoader.h"
#include "Bitmap.h"
#include "Game.h"

// Unreal:
#include "GameStructs.h"
#include "HAL/PlatformMemory.h"
#include "Logging/LogMacros.h"
#include "Misc/CString.h"

DEFINE_LOG_CATEGORY_STATIC(LogFormDef, Log, All);

// +----------------------------------------------------------------------+

ColumnDef::ColumnDef()
	: width(10)
	, align(0)
	, sort(0)
	, color(FColor::White)
	, use_color(false)
{
}

ColumnDef::ColumnDef(const char* t, int w, int a, int s)
	: title(t)
	, width(w)
	, align(a)
	, sort(s)
	, color(FColor::White)
	, use_color(false)
{
}

// +----------------------------------------------------------------------+

WinDef::WinDef(uint32 a_id, uint32 a_type, const char* a_text, uint32 a_style)
	: id(a_id)
	, pid(0)
	, type(a_type)
	, text(a_text)
	, style(a_style)
{
	rect = Rect(0, 0, 0, 0);
	text_align = 0;
	single_line = false;
	enabled = true;
	transparent = false;
	hide_partial = true;

	back_color = FColor(128, 128, 128, 255);
	base_color = FColor(128, 128, 128, 255);
	fore_color = FColor::Black;

	fixed_width = 0;
	fixed_height = 0;
}

void WinDef::SetID(uint32 i) { id = i; }
void WinDef::SetParentID(uint32 i) { pid = i; }
void WinDef::SetType(uint32 t) { type = t; }
void WinDef::SetRect(const Rect& r) { rect = r; }
void WinDef::SetEnabled(bool e) { enabled = e; }
void WinDef::SetStyle(uint32 s) { style = s; }
void WinDef::SetFont(const char* t) { font = t; }
void WinDef::SetText(const char* t) { text = t; }
void WinDef::SetAltText(const char* t) { alt_text = t; }
void WinDef::SetTexture(const char* t) { texture = t; }
void WinDef::SetBackColor(const FColor& c) { back_color = c; }
void WinDef::SetBaseColor(const FColor& c) { base_color = c; }
void WinDef::SetForeColor(const FColor& c) { fore_color = c; }
void WinDef::SetTextAlign(uint32 a) { text_align = a; }
void WinDef::SetSingleLine(bool a) { single_line = a; }
void WinDef::SetTransparent(bool t) { transparent = t; }
void WinDef::SetHidePartial(bool a) { hide_partial = a; }

void WinDef::SetMargins(const Insets& m) { margins = m; }
void WinDef::SetTextInsets(const Insets& t) { text_insets = t; }
void WinDef::SetCellInsets(const Insets& c) { cell_insets = c; }
void WinDef::SetCells(const Rect& r) { cells = r; }

// +----------------------------------------------------------------------+

#define CTRL_DEF_ANIMATED        0x0001
#define CTRL_DEF_BORDER          0x0002
#define CTRL_DEF_DROP_SHADOW     0x0004
#define CTRL_DEF_INDENT          0x0008
#define CTRL_DEF_INVERT_LABEL    0x0010
#define CTRL_DEF_GLOW            0x0020
#define CTRL_DEF_SIMPLE          0x0040
#define CTRL_DEF_STICKY          0x0080

CtrlDef::CtrlDef(uint32 a_id, uint32 a_type, const char* a_text, uint32 a_style)
	: WinDef(a_id, a_type, a_text, a_style)
{
	ctrl_flags = CTRL_DEF_ANIMATED | CTRL_DEF_BORDER | CTRL_DEF_INDENT;
	bevel_width = 5;
	picture_loc = 1; // North
	picture_type = Bitmap::BMP_SOLID;

	active = false;
	show_headings = false;

	leading = 0;
	line_height = 0;
	multiselect = 0;
	dragdrop = 0;
	orientation = 0;
	scroll_bar = 1;
	num_leds = 1;

	smooth_scroll = false;

	item_style = 0;
	selected_style = 0;
	pass_char = 0;

	items.destroy();

	FMemory::Memzero(tabs, sizeof(tabs));
	ntabs = 0;
}

CtrlDef::~CtrlDef()
{
	items.destroy();
	columns.destroy();
}

CtrlDef& CtrlDef::operator=(const CtrlDef& ctrl)
{
	WinDef::operator=(ctrl);

	ctrl_flags = ctrl.ctrl_flags;
	bevel_width = ctrl.bevel_width;
	picture_loc = ctrl.picture_loc;
	picture_type = ctrl.picture_type;

	active = ctrl.active;
	show_headings = ctrl.show_headings;

	leading = ctrl.leading;
	line_height = ctrl.line_height;
	multiselect = ctrl.multiselect;
	dragdrop = ctrl.dragdrop;
	orientation = ctrl.orientation;
	scroll_bar = ctrl.scroll_bar;

	active_color = ctrl.active_color;
	border_color = ctrl.border_color;

	smooth_scroll = ctrl.smooth_scroll;

	item_style = ctrl.item_style;
	selected_style = ctrl.selected_style;
	pass_char = ctrl.pass_char;

	standard_image = ctrl.standard_image;
	activated_image = ctrl.activated_image;
	transition_image = ctrl.transition_image;

	picture = ctrl.picture;
	picture_loc = ctrl.picture_loc;
	picture_type = ctrl.picture_type;

	num_leds = ctrl.num_leds;

	return *this;
}

int CtrlDef::GetOrientation() const
{
	return orientation;
}

void CtrlDef::SetOrientation(int o)
{
	orientation = o;
}

bool CtrlDef::GetActive() const
{
	return active;
}

void CtrlDef::SetActive(bool c)
{
	active = c;
}

FColor CtrlDef::GetActiveColor() const
{
	return active_color;
}

void CtrlDef::SetActiveColor(const FColor& c)
{
	active_color = c;
}

bool CtrlDef::GetAnimated() const
{
	return (ctrl_flags & CTRL_DEF_ANIMATED) != 0;
}

void CtrlDef::SetAnimated(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_ANIMATED;
	else
		ctrl_flags &= ~CTRL_DEF_ANIMATED;
}

short CtrlDef::GetBevelWidth() const
{
	return bevel_width;
}

void CtrlDef::SetBevelWidth(short nNewValue)
{
	bevel_width = nNewValue;
}

bool CtrlDef::GetBorder() const
{
	return (ctrl_flags & CTRL_DEF_BORDER) != 0;
}

void CtrlDef::SetBorder(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_BORDER;
	else
		ctrl_flags &= ~CTRL_DEF_BORDER;
}

FColor CtrlDef::GetBorderColor() const
{
	return border_color;
}

void CtrlDef::SetBorderColor(const FColor& c)
{
	border_color = c;
}

bool CtrlDef::GetDropShadow() const
{
	return (ctrl_flags & CTRL_DEF_DROP_SHADOW) != 0;
}

void CtrlDef::SetDropShadow(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_DROP_SHADOW;
	else
		ctrl_flags &= ~CTRL_DEF_DROP_SHADOW;
}

bool CtrlDef::GetIndent() const
{
	return (ctrl_flags & CTRL_DEF_INDENT) != 0;
}

void CtrlDef::SetIndent(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_INDENT;
	else
		ctrl_flags &= ~CTRL_DEF_INDENT;
}

bool CtrlDef::GetInvertLabel() const
{
	return (ctrl_flags & CTRL_DEF_INVERT_LABEL) != 0;
}

void CtrlDef::SetInvertLabel(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_INVERT_LABEL;
	else
		ctrl_flags &= ~CTRL_DEF_INVERT_LABEL;
}

Text CtrlDef::GetPicture() const
{
	return picture;
}

void CtrlDef::SetPicture(const Text& img_name)
{
	picture = img_name;
}

short CtrlDef::GetPictureLocation() const
{
	return picture_loc;
}

void CtrlDef::SetPictureLocation(short nNewValue)
{
	picture_loc = nNewValue;
}

short CtrlDef::GetPictureType() const
{
	return picture_type;
}

void CtrlDef::SetPictureType(short nNewValue)
{
	picture_type = nNewValue;
}

bool CtrlDef::GetSticky() const
{
	return (ctrl_flags & CTRL_DEF_STICKY) != 0;
}

void CtrlDef::SetSticky(bool bNewValue)
{
	if (bNewValue)
		ctrl_flags |= CTRL_DEF_STICKY;
	else
		ctrl_flags &= ~CTRL_DEF_STICKY;
}

int CtrlDef::GetNumLeds() const
{
	return num_leds;
}

void CtrlDef::SetNumLeds(int n)
{
	if (n > 0)
		num_leds = n;
}

int CtrlDef::NumItems() const
{
	return items.size();
}

Text CtrlDef::GetItem(int i) const
{
	Text result;

	if (i >= 0 && i < items.size())
		result = *(items[i]);

	return result;
}

void CtrlDef::AddItem(const char* t)
{
	// Remove (__FILE__, __LINE__) allocator usage:
	items.append(new Text(t));
}

int CtrlDef::NumColumns() const
{
	return columns.size();
}

ColumnDef* CtrlDef::GetColumn(int i) const
{
	ColumnDef* result = 0;

	if (i >= 0 && i < columns.size())
		result = columns[i];

	return result;
}

void CtrlDef::AddColumn(const char* t, int w, int a, int s)
{
	// Remove (__FILE__, __LINE__) allocator usage:
	columns.append(new ColumnDef(t, w, a, s));
}

int CtrlDef::NumTabs() const
{
	return ntabs;
}

int CtrlDef::GetTab(int i) const
{
	if (i >= 0 && i < ntabs)
		return tabs[i];
	return 0;
}

void CtrlDef::SetTab(int i, int t)
{
	if (i >= 0 && i < 10) {
		tabs[i] = t;
		if (i >= ntabs)
			ntabs = i + 1;
	}
}

void CtrlDef::AddTab(int i)
{
	if (ntabs < 10)
		tabs[ntabs++] = i;
}

bool CtrlDef::GetShowHeadings() const
{
	return show_headings;
}

void CtrlDef::SetShowHeadings(bool bNewValue)
{
	show_headings = bNewValue;
}

int CtrlDef::GetLeading() const
{
	return leading;
}

void CtrlDef::SetLeading(int nNewValue)
{
	leading = nNewValue;
}

int CtrlDef::GetLineHeight() const
{
	return line_height;
}

void CtrlDef::SetLineHeight(int nNewValue)
{
	line_height = nNewValue;
}

int CtrlDef::GetMultiSelect() const
{
	return multiselect;
}

void CtrlDef::SetMultiSelect(int nNewValue)
{
	multiselect = nNewValue;
}

int CtrlDef::GetDragDrop() const
{
	return dragdrop;
}

void CtrlDef::SetDragDrop(int nNewValue)
{
	dragdrop = nNewValue;
}

int CtrlDef::GetScrollBarVisible() const
{
	return scroll_bar;
}

void CtrlDef::SetScrollBarVisible(int nNewValue)
{
	scroll_bar = nNewValue;
}

bool CtrlDef::GetSmoothScroll() const
{
	return smooth_scroll;
}

void CtrlDef::SetSmoothScroll(bool bNewValue)
{
	smooth_scroll = bNewValue;
}

short CtrlDef::GetItemStyle() const
{
	return item_style;
}

void CtrlDef::SetItemStyle(short nNewValue)
{
	item_style = nNewValue;
}

short CtrlDef::GetSelectedStyle() const
{
	return selected_style;
}

void CtrlDef::SetSelectedStyle(short nNewValue)
{
	selected_style = nNewValue;
}

char CtrlDef::GetPasswordChar() const
{
	return pass_char;
}

void CtrlDef::SetPasswordChar(char nNewValue)
{
	pass_char = nNewValue;
}

Text CtrlDef::GetStandardImage() const
{
	return standard_image;
}

void CtrlDef::SetStandardImage(const Text& img_name)
{
	standard_image = img_name;
}

Text CtrlDef::GetActivatedImage() const
{
	return activated_image;
}

void CtrlDef::SetActivatedImage(const Text& img_name)
{
	activated_image = img_name;
}

Text CtrlDef::GetTransitionImage() const
{
	return transition_image;
}

void CtrlDef::SetTransitionImage(const Text& img_name)
{
	transition_image = img_name;
}

// +----------------------------------------------------------------------+

FormDef::FormDef(const char* a_text, uint32 a_style)
	: WinDef(0, WIN_DEF_FORM, a_text, a_style)
{
}

FormDef::~FormDef()
{
	controls.destroy();
}

void FormDef::AddCtrl(CtrlDef* def)
{
	if (def)
		controls.append(def);
}

CtrlDef* FormDef::FindCtrl(uint8 ctrl_id)
{
	if (ctrl_id > 0) {
		CtrlDef test(ctrl_id, 0);
		return controls.find(&test);
	}

	return 0;
}

ListIter<CtrlDef>
FormDef::GetControls() const
{
	// Legacy pattern: ListIter requires non-const source.
	FormDef* f = (FormDef*)this;
	return f->controls;
}

// +----------------------------------------------------------------------+

static char filename[64];
static char path_name[64];

void
FormDef::Load(const char* fname)
{
	FCStringAnsi::Sprintf(filename, "%s.frm", fname);

	UE_LOG(LogFormDef, Log, TEXT("Loading Form '%s'"), ANSI_TO_TCHAR(fname));

	FCStringAnsi::Sprintf(path_name, "Screens/");

	// Load Design File:
	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path_name);

	BYTE* block = nullptr;
	int blocklen = loader->LoadBuffer(filename, block, true);

	if (!block || blocklen < 4)
		return;

	Parser parser(new BlockReader((const char*)block, blocklen));
	Term* term = parser.ParseTerm();

	if (!term) {
		UE_LOG(LogFormDef, Error, TEXT("ERROR: could not parse '%s'"), ANSI_TO_TCHAR(filename));
		return;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "FORM") {
			UE_LOG(LogFormDef, Error, TEXT("ERROR: invalid form file '%s'"), ANSI_TO_TCHAR(filename));
			return;
		}
	}

	do {
		delete term;

		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				if (def->name()->value() == "form") {

					if (!def->term() || !def->term()->isStruct()) {
						UE_LOG(LogFormDef, Warning, TEXT("WARNING: form structure missing in '%s'"), ANSI_TO_TCHAR(filename));
					}
					else {
						FormDef* form = this;
						TermStruct* val = def->term()->isStruct();

						for (int i = 0; i < val->elements()->size(); i++) {
							char buf[256];

							TermDef* pdef = val->elements()->at(i)->isDef();
							if (pdef) {
								if (pdef->name()->value() == "text" ||
									pdef->name()->value() == "caption") {

									GetDefText(buf, pdef, filename);
									form->SetText(Game::GetText(buf));
								}

								else if (pdef->name()->value() == "id") {
									int32 idValue = 0;
									GetDefNumber(idValue, pdef, filename);
									form->SetID(idValue);
								}

								else if (pdef->name()->value() == "pid") {
									int32 idValue = 0;
									GetDefNumber(idValue, pdef, filename);
									form->SetParentID(idValue);
								}

								else if (pdef->name()->value() == "rect") {
									Rect r;
									GetDefRect(r, pdef, filename);
									form->SetRect(r);
								}

								else if (pdef->name()->value() == "font") {
									GetDefText(buf, pdef, filename);
									form->SetFont(buf);
								}

								else if (pdef->name()->value() == "back_color") {
									FColor c;
									GetDefFColor(c, pdef, filename);
									form->SetBackColor(c);
								}

								else if (pdef->name()->value() == "base_color") {
									FColor c;
									GetDefFColor(c, pdef, filename);
									form->SetBaseColor(c);
								}

								else if (pdef->name()->value() == "fore_color") {
									FColor c;
									GetDefFColor(c, pdef, filename);
									form->SetForeColor(c);
								}

								else if (pdef->name()->value() == "margins") {
									GetDefInsets(form->margins, pdef, filename);
								}

								else if (pdef->name()->value() == "text_insets") {
									GetDefInsets(form->text_insets, pdef, filename);
								}

								else if (pdef->name()->value() == "cell_insets") {
									GetDefInsets(form->cell_insets, pdef, filename);
								}

								else if (pdef->name()->value() == "cells") {
									GetDefRect(form->cells, pdef, filename);
								}

								else if (pdef->name()->value() == "texture") {
									GetDefText(buf, pdef, filename);

									if (*buf && !FCStringAnsi::Strchr(buf, '.'))
										FCStringAnsi::Strcat(buf, ".pcx");

									form->SetTexture(buf);
								}

								else if (pdef->name()->value() == "transparent") {
									bool b = false;
									GetDefBool(b, pdef, filename);
									form->SetTransparent(b);
								}

								else if (pdef->name()->value() == "style") {
									int32 s = 0;
									GetDefNumber(s, pdef, filename);
									form->SetStyle(s);
								}

								else if (pdef->name()->value() == "align" ||
									pdef->name()->value() == "text_align") {
									int32 a = DT_LEFT;

									if (GetDefText(buf, pdef, filename)) {
										if (!FCStringAnsi::Stricmp(buf, "left"))
											a = DT_LEFT;
										else if (!FCStringAnsi::Stricmp(buf, "right"))
											a = DT_RIGHT;
										else if (!FCStringAnsi::Stricmp(buf, "center"))
											a = DT_CENTER;
									}
									else {
										GetDefNumber(a, pdef, filename);
									}

									form->SetTextAlign(a);
								}

								// layout constraints:
								else if (pdef->name()->value() == "layout") {

									if (!pdef->term() || !pdef->term()->isStruct()) {
										UE_LOG(LogFormDef, Warning, TEXT("WARNING: layout structure missing in '%s'"), ANSI_TO_TCHAR(filename));
									}
									else {
										TermStruct* layoutVal = pdef->term()->isStruct();
										ParseLayoutDef(&form->layout, layoutVal);
									}
								}

								// controls:
								else if (pdef->name()->value() == "defctrl") {

									if (!pdef->term() || !pdef->term()->isStruct()) {
										UE_LOG(LogFormDef, Warning, TEXT("WARNING: defctrl structure missing in '%s'"), ANSI_TO_TCHAR(filename));
									}
									else {
										TermStruct* defVal = pdef->term()->isStruct();
										ParseCtrlDef(&form->defctrl, defVal);
									}
								}

								else if (pdef->name()->value() == "ctrl") {

									if (!pdef->term() || !pdef->term()->isStruct()) {
										UE_LOG(LogFormDef, Warning, TEXT("WARNING: ctrl structure missing in '%s'"), ANSI_TO_TCHAR(filename));
									}
									else {
										CtrlDef* ctrl = new CtrlDef;
										TermStruct* ctrlVal = pdef->term()->isStruct();

										form->AddCtrl(ctrl);
										*ctrl = form->defctrl;  // copy default params

										ParseCtrlDef(ctrl, ctrlVal);
									}
								}
								// end of controls.
							}
						}     // end form params
					}        // end form struct
				}           // end form

				else {
					UE_LOG(LogFormDef, Warning,
						TEXT("WARNING: unknown object '%s' in '%s'"),
						ANSI_TO_TCHAR(def->name()->value().data()),
						ANSI_TO_TCHAR(filename));
				}
			}
			else {
				UE_LOG(LogFormDef, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
				term->print();
			}
		}
	} while (term);

	loader->ReleaseBuffer(block);
	loader->SetDataPath(0);
}

// +----------------------------------------------------------------------+

void FormDef::ParseCtrlDef(CtrlDef* ctrl, const TermStruct* val)
{
	if (!ctrl || !val || !val->elements())
		return;

	Text Buf;
	ctrl->SetText("");

	for (int32 Index = 0; Index < val->elements()->size(); ++Index) {
		TermDef* ParamDef = val->elements()->at(Index)->isDef();
		if (!ParamDef)
			continue;

		const Text ParamName = ParamDef->name()->value();

		if (ParamName == "text" || ParamName == "caption") {
			GetDefText(Buf, ParamDef, filename);
			ctrl->SetText(Game::GetText(Buf));
		}
		else if (ParamName == "id") {
			int32 NewId = 0;
			GetDefNumber(NewId, ParamDef, filename);
			ctrl->SetID((uint32)NewId);
		}
		else if (ParamName == "pid") {
			int32 NewParentId = 0;
			GetDefNumber(NewParentId, ParamDef, filename);
			ctrl->SetParentID((uint32)NewParentId);
		}
		else if (ParamName == "alt") {
			GetDefText(Buf, ParamDef, filename);
			ctrl->SetAltText(Game::GetText(Buf));
		}
		else if (ParamName == "type") {
			int32 NewType = WIN_DEF_LABEL;

			GetDefText(Buf, ParamDef, filename);
			Text TypeName(Buf);

			if (TypeName == "button")                          NewType = WIN_DEF_BUTTON;
			else if (TypeName == "combo")                      NewType = WIN_DEF_COMBO;
			else if (TypeName == "edit")                       NewType = WIN_DEF_EDIT;
			else if (TypeName == "image")                      NewType = WIN_DEF_IMAGE;
			else if (TypeName == "slider")                     NewType = WIN_DEF_SLIDER;
			else if (TypeName == "list")                       NewType = WIN_DEF_LIST;
			else if (TypeName == "rich" ||
				TypeName == "text" ||
				TypeName == "rich_text")                  NewType = WIN_DEF_RICH;

			ctrl->SetType((uint32)NewType);
		}
		else if (ParamName == "rect") {
			Rect R;
			GetDefRect(R, ParamDef, filename);
			ctrl->SetRect(R);
		}
		else if (ParamName == "font") {
			GetDefText(Buf, ParamDef, filename);
			ctrl->SetFont(Buf);
		}
		else if (ParamName == "active_color") {
			FColor C;
			GetDefFColor(C, ParamDef, filename);
			ctrl->SetActiveColor(C);
		}
		else if (ParamName == "back_color") {
			FColor C;
			GetDefFColor(C, ParamDef, filename);
			ctrl->SetBackColor(C);
		}
		else if (ParamName == "base_color") {
			FColor C;
			GetDefFColor(C, ParamDef, filename);
			ctrl->SetBaseColor(C);
		}
		else if (ParamName == "border_color") {
			FColor C;
			GetDefFColor(C, ParamDef, filename);
			ctrl->SetBorderColor(C);
		}
		else if (ParamName == "fore_color") {
			FColor C;
			GetDefFColor(C, ParamDef, filename);
			ctrl->SetForeColor(C);
		}
		else if (ParamName == "texture") {
			GetDefText(Buf, ParamDef, filename);

			if (Buf.length() > 0 && !Buf.contains('.'))
				Buf.append(".pcx");

			ctrl->SetTexture(Buf);
		}
		else if (ParamName == "margins") {
			GetDefInsets(ctrl->margins, ParamDef, filename);
		}
		else if (ParamName == "text_insets") {
			GetDefInsets(ctrl->text_insets, ParamDef, filename);
		}
		else if (ParamName == "cell_insets") {
			GetDefInsets(ctrl->cell_insets, ParamDef, filename);
		}
		else if (ParamName == "cells") {
			GetDefRect(ctrl->cells, ParamDef, filename);
		}
		else if (ParamName == "fixed_width") {
			GetDefNumber(ctrl->fixed_width, ParamDef, filename);
		}
		else if (ParamName == "fixed_height") {
			GetDefNumber(ctrl->fixed_height, ParamDef, filename);
		}
		else if (ParamName == "standard_image") {
			GetDefText(Buf, ParamDef, filename);

			if (Buf.length() > 0 && !Buf.contains('.'))
				Buf.append(".pcx");

			ctrl->SetStandardImage(Buf);
		}
		else if (ParamName == "activated_image") {
			GetDefText(Buf, ParamDef, filename);

			if (Buf.length() > 0 && !Buf.contains('.'))
				Buf.append(".pcx");

			ctrl->SetActivatedImage(Buf);
		}
		else if (ParamName == "transition_image") {
			GetDefText(Buf, ParamDef, filename);

			if (Buf.length() > 0 && !Buf.contains('.'))
				Buf.append(".pcx");

			ctrl->SetTransitionImage(Buf);
		}
		else if (ParamName == "picture") {
			GetDefText(Buf, ParamDef, filename);

			if (Buf.length() > 0 && !Buf.contains('.'))
				Buf.append(".pcx");

			ctrl->SetPicture(Buf);
		}
		else if (ParamName == "enabled") {
			bool bEnabled = true;
			GetDefBool(bEnabled, ParamDef, filename);
			ctrl->SetEnabled(bEnabled);
		}
		else if (ParamName == "item") {
			GetDefText(Buf, ParamDef, filename);
			ctrl->AddItem(Game::GetText(Buf));
		}
		else if (ParamName == "tab") {
			int32 TabIndex = 0;
			GetDefNumber(TabIndex, ParamDef, filename);
			ctrl->AddTab(TabIndex);
		}
		else if (ParamName == "column") {
			if (!ParamDef->term() || !ParamDef->term()->isStruct()) {
				UE_LOG(LogFormDef, Warning, TEXT("WARNING: column structure missing in '%s'"), ANSI_TO_TCHAR(filename));
			}
			else {
				const TermStruct* ColumnStruct = ParamDef->term()->isStruct();
				ParseColumnDef(ctrl, ColumnStruct);
			}
		}
		else if (ParamName == "orientation") {
			int32 Orientation = 0;
			GetDefNumber(Orientation, ParamDef, filename);
			ctrl->SetOrientation(Orientation);
		}
		else if (ParamName == "leading") {
			int32 Leading = 0;
			GetDefNumber(Leading, ParamDef, filename);
			ctrl->SetLeading(Leading);
		}
		else if (ParamName == "line_height") {
			int32 LineHeight = 0;
			GetDefNumber(LineHeight, ParamDef, filename);
			ctrl->SetLineHeight(LineHeight);
		}
		else if (ParamName == "multiselect") {
			int32 MultiSelect = 0;
			GetDefNumber(MultiSelect, ParamDef, filename);
			ctrl->SetMultiSelect(MultiSelect);
		}
		else if (ParamName == "dragdrop") {
			int32 DragDrop = 0;
			GetDefNumber(DragDrop, ParamDef, filename);
			ctrl->SetDragDrop(DragDrop);
		}
		else if (ParamName == "scroll_bar") {
			int32 ScrollBar = 0;
			GetDefNumber(ScrollBar, ParamDef, filename);
			ctrl->SetScrollBarVisible(ScrollBar);
		}
		else if (ParamName == "smooth_scroll") {
			bool bSmooth = false;
			GetDefBool(bSmooth, ParamDef, filename);
			ctrl->SetSmoothScroll(bSmooth);
		}
		else if (ParamName == "picture_loc") {
			int32 PictureLoc = 0;
			GetDefNumber(PictureLoc, ParamDef, filename);
			ctrl->SetPictureLocation((short)PictureLoc);
		}
		else if (ParamName == "picture_type") {
			int32 PictureType = 0;
			GetDefNumber(PictureType, ParamDef, filename);
			ctrl->SetPictureType((short)PictureType);
		}
		else if (ParamName == "style") {
			int32 NewStyle = 0;
			GetDefNumber(NewStyle, ParamDef, filename);
			ctrl->SetStyle((uint32)NewStyle);
		}
		else if (ParamName == "align" || ParamName == "text_align") {
			int32 TextAlign = DT_LEFT;

			if (GetDefText(Buf, ParamDef, filename)) {
				if (!_stricmp(Buf.data(), "left"))        TextAlign = DT_LEFT;
				else if (!_stricmp(Buf.data(), "right"))  TextAlign = DT_RIGHT;
				else if (!_stricmp(Buf.data(), "center")) TextAlign = DT_CENTER;
			}
			else {
				GetDefNumber(TextAlign, ParamDef, filename);
			}

			ctrl->SetTextAlign((uint32)TextAlign);
		}
		else if (ParamName == "single_line") {
			bool bSingle = false;
			GetDefBool(bSingle, ParamDef, filename);
			ctrl->SetSingleLine(bSingle);
		}
		else if (ParamName == "bevel_width") {
			int32 BevelWidth = 0;
			GetDefNumber(BevelWidth, ParamDef, filename);
			ctrl->SetBevelWidth((short)BevelWidth);
		}
		else if (ParamName == "active") {
			bool bActive = false;
			GetDefBool(bActive, ParamDef, filename);
			ctrl->SetActive(bActive);
		}
		else if (ParamName == "animated") {
			bool bAnimated = false;
			GetDefBool(bAnimated, ParamDef, filename);
			ctrl->SetAnimated(bAnimated);
		}
		else if (ParamName == "border") {
			bool bBorder = false;
			GetDefBool(bBorder, ParamDef, filename);
			ctrl->SetBorder(bBorder);
		}
		else if (ParamName == "drop_shadow") {
			bool bDropShadow = false;
			GetDefBool(bDropShadow, ParamDef, filename);
			ctrl->SetDropShadow(bDropShadow);
		}
		else if (ParamName == "show_headings") {
			bool bShow = false;
			GetDefBool(bShow, ParamDef, filename);
			ctrl->SetShowHeadings(bShow);
		}
		else if (ParamName == "sticky") {
			bool bSticky = false;
			GetDefBool(bSticky, ParamDef, filename);
			ctrl->SetSticky(bSticky);
		}
		else if (ParamName == "transparent") {
			bool bTransparent = false;
			GetDefBool(bTransparent, ParamDef, filename);
			ctrl->SetTransparent(bTransparent);
		}
		else if (ParamName == "hide_partial") {
			bool bHidePartial = false;
			GetDefBool(bHidePartial, ParamDef, filename);
			ctrl->SetHidePartial(bHidePartial);
		}
		else if (ParamName == "num_leds") {
			int32 NumLeds = 0;
			GetDefNumber(NumLeds, ParamDef, filename);
			ctrl->SetNumLeds(NumLeds);
		}
		else if (ParamName == "item_style") {
			int32 ItemStyle = 0;
			GetDefNumber(ItemStyle, ParamDef, filename);
			ctrl->SetItemStyle((short)ItemStyle);
		}
		else if (ParamName == "selected_style") {
			int32 SelectedStyle = 0;
			GetDefNumber(SelectedStyle, ParamDef, filename);
			ctrl->SetSelectedStyle((short)SelectedStyle);
		}
		else if (ParamName == "password") {
			Text Password;
			GetDefText(Password, ParamDef, filename);
			ctrl->SetPasswordChar((char)Password[0]);
		}
		else if (ParamName == "layout") {
			if (!ParamDef->term() || !ParamDef->term()->isStruct()) {
				UE_LOG(LogFormDef, Warning, TEXT("WARNING: layout structure missing in '%s'"), ANSI_TO_TCHAR(filename));
			}
			else {
				const TermStruct* LayoutStruct = ParamDef->term()->isStruct();
				ParseLayoutDef(&ctrl->layout, LayoutStruct);
			}
		}
	}
}


void FormDef::ParseColumnDef(CtrlDef* ctrl, const TermStruct* val)
{
	if (!ctrl || !val || !val->elements())
		return;

	Text ColumnText;
	char AnsiBuf[256] = { 0 };

	int32 ColumnWidth = 0;
	int32 ColumnAlign = 0;
	int32 ColumnSort = 0;

	FColor ColumnColor = FColor::White;
	bool   bUseColor = false;

	for (int32 Index = 0; Index < val->elements()->size(); ++Index) {
		TermDef* ParamDef = val->elements()->at(Index)->isDef();
		if (!ParamDef)
			continue;

		const Text ParamName = ParamDef->name()->value();

		if (ParamName == "text" || ParamName == "title") {
			GetDefText(AnsiBuf, ParamDef, filename);
			ColumnText = Game::GetText(AnsiBuf);
		}
		else if (ParamName == "width") {
			GetDefNumber(ColumnWidth, ParamDef, filename);
		}
		else if (ParamName == "align") {
			ColumnAlign = DT_LEFT;

			if (GetDefText(AnsiBuf, ParamDef, filename)) {
				if (!_stricmp(AnsiBuf, "left"))
					ColumnAlign = DT_LEFT;
				else if (!_stricmp(AnsiBuf, "right"))
					ColumnAlign = DT_RIGHT;
				else if (!_stricmp(AnsiBuf, "center"))
					ColumnAlign = DT_CENTER;
			}
			else {
				GetDefNumber(ColumnAlign, ParamDef, filename);
			}
		}
		else if (ParamName == "sort") {
			GetDefNumber(ColumnSort, ParamDef, filename);
		}
		else if (ParamName == "color") {
			GetDefFColor(ColumnColor, ParamDef, filename);
			bUseColor = true;
		}
	}

	ctrl->AddColumn(ColumnText, (int)ColumnWidth, (int)ColumnAlign, (int)ColumnSort);

	if (bUseColor) {
		const int32 ColumnIndex = ctrl->NumColumns() - 1;
		ColumnDef* ColumnDefPtr = ctrl->GetColumn((int)ColumnIndex);

		if (ColumnDefPtr) {
			ColumnDefPtr->color = ColumnColor;
			ColumnDefPtr->use_color = true;
		}
	}
}

void FormDef::ParseLayoutDef(LayoutDef* def, const TermStruct* val)
{
	if (!def || !val || !val->elements())
		return;

	for (int32 Index = 0; Index < val->elements()->size(); ++Index) {
		TermDef* ParamDef = val->elements()->at(Index)->isDef();
		if (!ParamDef || !ParamDef->term())
			continue;

		const Text Name = ParamDef->name()->value();
		TermArray* Arr = ParamDef->term()->isArray();

		if (!Arr || !Arr->elements())
			continue;

		// --------------------------------------------------
		// x_mins / cols  (uint32)
		// --------------------------------------------------
		if (Name == "x_mins" || Name == "cols") {
			def->x_mins.destroy();

			for (int32 i = 0; i < Arr->elements()->size(); ++i) {
				Term* Elem = Arr->elements()->at(i);
				if (Elem && Elem->isNumber()) {
					uint32 v = (uint32)Elem->isNumber()->value();
					def->x_mins.append(new uint32(v));
				}
			}
		}

		// --------------------------------------------------
		// y_mins / rows  (uint32)
		// --------------------------------------------------
		else if (Name == "y_mins" || Name == "rows") {
			def->y_mins.destroy();

			for (int32 i = 0; i < Arr->elements()->size(); ++i) {
				Term* Elem = Arr->elements()->at(i);
				if (Elem && Elem->isNumber()) {
					uint32 v = (uint32)Elem->isNumber()->value();
					def->y_mins.append(new uint32(v));
				}
			}
		}

		// --------------------------------------------------
		// x_weights / col_wts  (float)
		// --------------------------------------------------
		else if (Name == "x_weights" || Name == "col_wts") {
			def->x_weights.destroy();

			for (int32 i = 0; i < Arr->elements()->size(); ++i) {
				Term* Elem = Arr->elements()->at(i);
				if (Elem && Elem->isNumber()) {
					float v = (float)Elem->isNumber()->value();
					def->x_weights.append(new float(v));
				}
			}
		}

		// --------------------------------------------------
		// y_weights / row_wts  (float)
		// --------------------------------------------------
		else if (Name == "y_weights" || Name == "row_wts") {
			def->y_weights.destroy();

			for (int32 i = 0; i < Arr->elements()->size(); ++i) {
				Term* Elem = Arr->elements()->at(i);
				if (Elem && Elem->isNumber()) {
					float v = (float)Elem->isNumber()->value();
					def->y_weights.append(new float(v));
				}
			}
		}
	}
}

