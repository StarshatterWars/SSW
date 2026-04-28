/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026.

	SUBSYSTEM:    Stars.exe
	FILE:         HUDView.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR / STUDIO CREDIT
	===============================
	Original HUDView implementation:
	John DiCamillo / Destroyer Studios LLC
	Copyright (c) 1997-2004. All Rights Reserved.

	OVERVIEW
	========
	View class for Heads Up Display
*/

#include "HUDView.h"

// Unreal (minimal):
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // FMath
#include "Logging/LogMacros.h"         // UE_LOG

// Starshatter / Starshatter Wars:
#include "CameraView.h"
#include "MFDView.h"
#include "SimProjector.h"

#include "HUDSounds.h"
#include "Ship.h"
#include "SimElement.h"        // was Element
#include "Computer.h"
#include "Drive.h"
#include "Instruction.h"
#include "NavSystem.h"
#include "Power.h"
#include "Shield.h"
#include "Sensor.h"
#include "SimContact.h"        // was Contact
#include "ShipDesign.h"
#include "SimShot.h"           // was Shot (keep SimShot*)
#include "Drone.h"
#include "Thruster.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "FlightDeck.h"
#include "SteerAI.h"
#include "Sim.h"
#include "StarSystem.h"        // keep StarSystem*
#include "Starshatter.h"
#include "CameraManager.h"     // was CameraDirector

#include "RadioView.h"
#include "FormatUtil.h"
#include "Hoop.h"
#include "QuantumDrive.h"
#include "KeyMap.h"
#include "AudioConfig.h"
#include "PlayerCharacter.h"
#include "GameStructs.h"
#include "FontManager.h" 
#include "OrbitalBody.h" 

#include "Screen.h"
#include "DataLoader.h"
#include "SimScene.h"          // was Scene
          // provides SystemFont*
#include "Graphic.h"
#include "Sprite.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "MouseController.h"
#include "Polygon.h"
#include "Sound.h"
#include "Bitmap.h"
#include "Game.h"
#include "SimModel.h"


// -------------------------------------------------------------------------------------------------
// Local helpers (FColor math compatible with legacy "Color" ops)
// -------------------------------------------------------------------------------------------------
static inline FVector HUDOtherHand(const FVector& V)
{
	// Starshatter used a handedness conversion helper. Preserve intent (Z flip) for now.
	return FVector((float)V.X, (float)V.Y, (float)-V.Z);
}

static inline FColor ColorMul(const FColor& C, float S)
{
	const FLinearColor L = FLinearColor(C) * S;
	return L.ToFColor(true);
}

static inline FColor ColorDim(const FColor& C, float S)
{
	// Legacy dim() behaved like multiply.
	return ColorMul(C, S);
}

static inline FColor ColorAdd(const FColor& A, const FColor& B)
{
	const FLinearColor L = FLinearColor(A) + FLinearColor(B);
	return L.ToFColor(true);
}

static inline FColor ColorClamp(const FColor& C)
{
	const FLinearColor L = FLinearColor(C).GetClamped(0.0f, 1.0f);
	return L.ToFColor(true);
}

static FORCEINLINE FIntRect ToIntRect(const Rect& R)
{
	// Rect is (x,y,w,h). FIntRect is (MinX,MinY,MaxX,MaxY).
	return FIntRect(R.x, R.y, R.x + R.w, R.y + R.h);
}

static const char* GetContactDisplayName(SimContact* c)
{
	if (!c) return "CONTACT";

	if (Ship* s = c->GetShip())
		return s->Name();          // or s->DesignName() if you prefer

	// If you have other accessors in your port, use them here:
	// if (SimObject* o = c->GetObject()) return o->Name();

	if (c->GetShot())
		return "INCOMING";

	return "CONTACT";
}
// -------------------------------------------------------------------------------------------------

static Bitmap hud_left_air;
static Bitmap hud_right_air;
static Bitmap hud_left_fighter;
static Bitmap hud_right_fighter;
static Bitmap hud_left_starship;
static Bitmap hud_right_starship;
static Bitmap instr_left;
static Bitmap instr_right;
static Bitmap warn_left;
static Bitmap warn_right;
static Bitmap lead;
static Bitmap cross;
static Bitmap cross1;
static Bitmap cross2;
static Bitmap cross3;
static Bitmap cross4;
static Bitmap fpm;
static Bitmap hpm;
static Bitmap pitch_ladder_pos;
static Bitmap pitch_ladder_neg;
static Bitmap chase_left;
static Bitmap chase_right;
static Bitmap chase_top;
static Bitmap chase_bottom;
static Bitmap icon_ship;
static Bitmap icon_target;

static BYTE* hud_left_shade_air = 0;
static BYTE* hud_right_shade_air = 0;
static BYTE* hud_left_shade_fighter = 0;
static BYTE* hud_right_shade_fighter = 0;
static BYTE* hud_left_shade_starship = 0;
static BYTE* hud_right_shade_starship = 0;
static BYTE* instr_left_shade = 0;
static BYTE* instr_right_shade = 0;
static BYTE* warn_left_shade = 0;
static BYTE* warn_right_shade = 0;
static BYTE* lead_shade = 0;
static BYTE* cross_shade = 0;
static BYTE* cross1_shade = 0;
static BYTE* cross2_shade = 0;
static BYTE* cross3_shade = 0;
static BYTE* cross4_shade = 0;
static BYTE* fpm_shade = 0;
static BYTE* hpm_shade = 0;
static BYTE* pitch_ladder_pos_shade = 0;
static BYTE* pitch_ladder_neg_shade = 0;
static BYTE* chase_left_shade = 0;
static BYTE* chase_right_shade = 0;
static BYTE* chase_top_shade = 0;
static BYTE* chase_bottom_shade = 0;
static BYTE* icon_ship_shade = 0;
static BYTE* icon_target_shade = 0;

static Sprite* hud_left_sprite = 0;
static Sprite* hud_right_sprite = 0;
static Sprite* fpm_sprite = 0;
static Sprite* hpm_sprite = 0;
static Sprite* lead_sprite = 0;
static Sprite* aim_sprite = 0;
static Sprite* tgt1_sprite = 0;
static Sprite* tgt2_sprite = 0;
static Sprite* tgt3_sprite = 0;
static Sprite* tgt4_sprite = 0;
static Sprite* chase_sprite = 0;
static Sprite* instr_left_sprite = 0;
static Sprite* instr_right_sprite = 0;
static Sprite* warn_left_sprite = 0;
static Sprite* warn_right_sprite = 0;
static Sprite* icon_ship_sprite = 0;
static Sprite* icon_target_sprite = 0;

static USound* missile_lock_sound = nullptr;

const int NUM_HUD_COLORS = 4;

static FColor standard_hud_colors[NUM_HUD_COLORS] = {
	FColor(130,190,140),  // green
	FColor(130,200,220),  // cyan
	FColor(250,170, 80),  // orange
	FColor(16, 16, 16)   // dark gray
};

static FColor standard_txt_colors[NUM_HUD_COLORS] = {
	FColor(150,200,170),  // green w/ green gray
	FColor(220,220,180),  // cyan  w/ light yellow
	FColor(220,220, 80),  // orange w/ yellow
	FColor(32, 32, 32)   // dark gray
};

static FColor night_vision_colors[NUM_HUD_COLORS] = {
	FColor(20, 80, 20),  // green
	FColor(30, 80, 80),  // cyan
	FColor(80, 80, 20),  // yellow
	FColor(0,  0,  0)   // no night vision
};

static SystemFont* HudFont = nullptr; // was Font*
static SystemFont* BigFont = nullptr; // was Font*

static bool   mouse_in = false;
static int    mouse_latch = 0;
static int    mouse_index = -1;

static SYSTEM_STATUS ship_status = SYSTEM_STATUS::NOMINAL;  // was System::STATUS
static SYSTEM_STATUS tgt_status = SYSTEM_STATUS::NOMINAL;

// +--------------------------------------------------------------------+

static enum TXT {
	MAX_CONTACT = 50,

	TXT_CAUTION_TXT = 0,
	TXT_LAST_CAUTION = 23,
	TXT_CAM_ANGLE,
	TXT_CAM_MODE,
	TXT_PAUSED,
	TXT_GEAR_DOWN,

	TXT_HUD_MODE,
	TXT_PRIMARY_WEP,
	TXT_SECONDARY_WEP,
	TXT_DECOY,
	TXT_SHIELD,
	TXT_AUTO,
	TXT_SHOOT,
	TXT_NAV_INDEX,
	TXT_NAV_ACTION,
	TXT_NAV_FORMATION,
	TXT_NAV_SPEED,
	TXT_NAV_ETR,
	TXT_NAV_HOLD,

	TXT_SPEED,
	TXT_RANGE,
	TXT_CLOSING_SPEED,
	TXT_THREAT_WARN,
	TXT_COMPASS,
	TXT_HEADING,
	TXT_PITCH,
	TXT_ALTITUDE,
	TXT_GFORCE,
	TXT_MISSILE_T1,
	TXT_MISSILE_T2,
	TXT_ICON_SHIP_TYPE,
	TXT_ICON_TARGET_TYPE,
	TXT_TARGET_NAME,
	TXT_TARGET_DESIGN,
	TXT_TARGET_SHIELD,
	TXT_TARGET_HULL,
	TXT_TARGET_SUB,
	TXT_TARGET_ETA,

	TXT_MSG_1,
	TXT_MSG_2,
	TXT_MSG_3,
	TXT_MSG_4,
	TXT_MSG_5,
	TXT_MSG_6,

	TXT_NAV_PT,
	TXT_SELF,
	TXT_SELF_NAME,
	TXT_CONTACT_NAME,
	TXT_CONTACT_INFO = TXT_CONTACT_NAME + MAX_CONTACT,
	TXT_LAST = TXT_CONTACT_INFO + MAX_CONTACT,

	TXT_LAST_ACTIVE = TXT_NAV_HOLD,
	TXT_INSTR_PAGE = TXT_CAUTION_TXT + 6,
};

static HUDText hud_text[TXT_LAST];

// +--------------------------------------------------------------------+

void
HUDView::DrawHUDText(int index, const char* txt, Rect& InRect, int align, int upcase, bool box)
{
	if (index < 0 || index >= TXT_LAST)
		return;

	HUDText& ht = hud_text[index];
	FColor   hc = ht.color;

	char txt_buf[256];
	int  n = (int)strlen(txt);

	if (n > 250) n = 250;

	int i;
	for (i = 0; i < n; i++) {
		if (upcase && islower((unsigned char)txt[i]))
			txt_buf[i] = (char)toupper((unsigned char)txt[i]);
		else
			txt_buf[i] = txt[i];
	}
	txt_buf[i] = 0;

	if (box) {
		ht.font->DrawText(txt_buf, n, InRect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);

		if ((align & DT_CENTER) != 0) {
			const int cx = width / 2;
			InRect.x = cx - InRect.w / 2;
		}
	}

	if (!cockpit_hud_texture && InRect.Contains(Mouse::X(), Mouse::Y())) {
		mouse_in = true;

		if (index <= TXT_LAST_ACTIVE)
			hc = FColor::White;

		if (Mouse::LButton() && !mouse_latch) {
			mouse_latch = 2;
			mouse_index = index;
		}
	}

	if (cockpit_hud_texture &&
		index >= TXT_HUD_MODE &&
		index <= TXT_TARGET_ETA &&
		ht.font != BigFont) {

		Sprite* s = hud_sprite[0];

		const FVector Loc = s->GetLocation();
		const int cx = (int)Loc.X;
		const int cy = (int)Loc.Y;
		const int w2 = s->Width() / 2;
		const int h2 = s->Height() / 2;

		Rect txt_rect(InRect);
		txt_rect.x -= (cx - w2);
		txt_rect.y -= (cy - h2);

		if (index == TXT_ICON_SHIP_TYPE)
			txt_rect = Rect(0, 500, 128, 12);

		else if (index == TXT_ICON_TARGET_TYPE)
			txt_rect = Rect(128, 500, 128, 12);

		ht.font->SetColor(hc);
		ht.font->DrawText(txt_buf, n, txt_rect, align | DT_SINGLELINE, cockpit_hud_texture);
		ht.hidden = false;
	}
	else {
		ht.font->SetColor(hc);
		ht.font->DrawText(txt_buf, n, InRect, align | DT_SINGLELINE);
		ht.rect = InRect;
		ht.hidden = false;

		if (box) {
			Rect boxRect(InRect);
			boxRect.Inflate(3, 2);
			boxRect.h--;
			DrawRect(boxRect, HudColor);
		}
	}
}


void
HUDView::HideHUDText(int index)
{
	if (index >= TXT_LAST)
		return;

	hud_text[index].hidden = true;
}

// +--------------------------------------------------------------------+

HUDView* HUDView::hud_view = nullptr;
bool     HUDView::arcade = false;
bool     HUDView::show_fps = false;
int      HUDView::def_color_set = 1;
int      HUDView::gunsight = 1;

// +--------------------------------------------------------------------+

HUDView::HUDView(Screen* InScreen)
	: View(InScreen, 0, 0,
		InScreen ? InScreen->Width() : 0,
		InScreen ? InScreen->Height() : 0),
	
	projector(nullptr),
	camview(nullptr),
	width(InScreen ? InScreen->Width() : 0),
	height(InScreen ? InScreen->Height() : 0),

	sim(nullptr),
	ship(nullptr),
	target(nullptr),
	active_region(nullptr),
	cockpit_hud_texture(nullptr),
	show_warn(false),
	show_inst(false),
	inst_page(0),
	threat(0),
	mode(EHUDMode::Tactical),
	color(0),
	tactical(0),
	overlay(0),
	transition(false),
	docking(false),
	az_ring(nullptr),
	az_pointer(nullptr),
	el_ring(nullptr),
	el_pointer(nullptr),
	compass_scale(1.0)
{
	hud_view = this;

	sim = Sim::GetSim();
	if (sim)
		sim->ShowGrid(false);

	xcenter = (width / 2.0) - 0.5;
	ycenter = (height / 2.0) + 0.5;

	PrepareBitmap("HUDleftA.pcx", hud_left_air, hud_left_shade_air);
	PrepareBitmap("HUDrightA.pcx", hud_right_air, hud_right_shade_air);
	PrepareBitmap("HUDleft.pcx", hud_left_fighter, hud_left_shade_fighter);
	PrepareBitmap("HUDright.pcx", hud_right_fighter, hud_right_shade_fighter);
	PrepareBitmap("HUDleft1.pcx", hud_left_starship, hud_left_shade_starship);
	PrepareBitmap("HUDright1.pcx", hud_right_starship, hud_right_shade_starship);
	PrepareBitmap("INSTR_left.pcx", instr_left, instr_left_shade);
	PrepareBitmap("INSTR_right.pcx", instr_right, instr_right_shade);
	PrepareBitmap("CAUTION_left.pcx", warn_left, warn_left_shade);
	PrepareBitmap("CAUTION_right.pcx", warn_right, warn_right_shade);
	PrepareBitmap("hud_icon.pcx", icon_ship, icon_ship_shade);
	PrepareBitmap("hud_icon.pcx", icon_target, icon_target_shade);

	PrepareBitmap("lead.pcx", lead, lead_shade);
	PrepareBitmap("cross.pcx", cross, cross_shade);
	PrepareBitmap("cross1.pcx", cross1, cross1_shade);
	PrepareBitmap("cross2.pcx", cross2, cross2_shade);
	PrepareBitmap("cross3.pcx", cross3, cross3_shade);
	PrepareBitmap("cross4.pcx", cross4, cross4_shade);
	PrepareBitmap("fpm.pcx", fpm, fpm_shade);
	PrepareBitmap("hpm.pcx", hpm, hpm_shade);
	PrepareBitmap("chase_l.pcx", chase_left, chase_left_shade);
	PrepareBitmap("chase_r.pcx", chase_right, chase_right_shade);
	PrepareBitmap("chase_t.pcx", chase_top, chase_top_shade);
	PrepareBitmap("chase_b.pcx", chase_bottom, chase_bottom_shade);
	PrepareBitmap("ladder1.pcx", pitch_ladder_pos, pitch_ladder_pos_shade);
	PrepareBitmap("ladder2.pcx", pitch_ladder_neg, pitch_ladder_neg_shade);

	hud_left_air.SetType(Bitmap::BMP_TRANSLUCENT);
	hud_right_air.SetType(Bitmap::BMP_TRANSLUCENT);
	hud_left_fighter.SetType(Bitmap::BMP_TRANSLUCENT);
	hud_right_fighter.SetType(Bitmap::BMP_TRANSLUCENT);
	hud_left_starship.SetType(Bitmap::BMP_TRANSLUCENT);
	hud_right_starship.SetType(Bitmap::BMP_TRANSLUCENT);
	instr_left.SetType(Bitmap::BMP_TRANSLUCENT);
	instr_right.SetType(Bitmap::BMP_TRANSLUCENT);
	warn_left.SetType(Bitmap::BMP_TRANSLUCENT);
	warn_right.SetType(Bitmap::BMP_TRANSLUCENT);
	icon_ship.SetType(Bitmap::BMP_TRANSLUCENT);
	icon_target.SetType(Bitmap::BMP_TRANSLUCENT);
	fpm.SetType(Bitmap::BMP_TRANSLUCENT);
	hpm.SetType(Bitmap::BMP_TRANSLUCENT);
	lead.SetType(Bitmap::BMP_TRANSLUCENT);
	cross.SetType(Bitmap::BMP_TRANSLUCENT);
	cross1.SetType(Bitmap::BMP_TRANSLUCENT);
	cross2.SetType(Bitmap::BMP_TRANSLUCENT);
	cross3.SetType(Bitmap::BMP_TRANSLUCENT);
	cross4.SetType(Bitmap::BMP_TRANSLUCENT);
	chase_left.SetType(Bitmap::BMP_TRANSLUCENT);
	chase_right.SetType(Bitmap::BMP_TRANSLUCENT);
	chase_top.SetType(Bitmap::BMP_TRANSLUCENT);
	chase_bottom.SetType(Bitmap::BMP_TRANSLUCENT);
	pitch_ladder_pos.SetType(Bitmap::BMP_TRANSLUCENT);
	pitch_ladder_neg.SetType(Bitmap::BMP_TRANSLUCENT);

	hud_left_sprite = new Sprite(&hud_left_fighter);
	hud_right_sprite = new Sprite(&hud_right_fighter);
	instr_left_sprite = new Sprite(&instr_left);
	instr_right_sprite = new Sprite(&instr_right);
	warn_left_sprite = new Sprite(&warn_left);
	warn_right_sprite = new Sprite(&warn_right);
	icon_ship_sprite = new Sprite(&icon_ship);
	icon_target_sprite = new Sprite(&icon_target);
	fpm_sprite = new Sprite(&fpm);
	hpm_sprite = new Sprite(&hpm);
	lead_sprite = new Sprite(&lead);
	aim_sprite = new Sprite(&cross);
	tgt1_sprite = new Sprite(&cross1);
	tgt2_sprite = new Sprite(&cross2);
	tgt3_sprite = new Sprite(&cross3);
	tgt4_sprite = new Sprite(&cross4);
	chase_sprite = new Sprite(&chase_left);

	ZeroMemory(hud_sprite, sizeof(hud_sprite));

	hud_sprite[0] = hud_left_sprite;
	hud_sprite[1] = hud_right_sprite;
	hud_sprite[2] = instr_left_sprite;
	hud_sprite[3] = instr_right_sprite;
	hud_sprite[4] = warn_left_sprite;
	hud_sprite[5] = warn_right_sprite;
	hud_sprite[6] = icon_ship_sprite;
	hud_sprite[7] = icon_target_sprite;
	hud_sprite[8] = fpm_sprite;
	hud_sprite[9] = hpm_sprite;
	hud_sprite[10] = lead_sprite;
	hud_sprite[11] = aim_sprite;
	hud_sprite[12] = tgt1_sprite;
	hud_sprite[13] = tgt2_sprite;
	hud_sprite[14] = tgt3_sprite;
	hud_sprite[15] = tgt4_sprite;
	hud_sprite[16] = chase_sprite;

	double pitch_ladder_UV[8] = { 0.125,0.0625, 0.875,0.0625, 0.875,0, 0.125,0 };
	double UV[8];

	int i = 0;

	for (i = 0; i < 15; i++) {
		pitch_ladder[i] = new Sprite(&pitch_ladder_pos);

		CopyMemory(UV, pitch_ladder_UV, sizeof(UV));
		UV[1] = UV[3] = (pitch_ladder_UV[1] * (i));
		UV[5] = UV[7] = (pitch_ladder_UV[1] * (i + 1));

		pitch_ladder[i]->Reshape(192, 16);
		pitch_ladder[i]->SetTexCoords(UV);
		pitch_ladder[i]->SetBlendMode(2);
		pitch_ladder[i]->Hide();
	}

	// zero mark at i=15
	{
		pitch_ladder[i] = new Sprite(&pitch_ladder_pos);

		UV[0] = UV[6] = 0;
		UV[2] = UV[4] = 1;
		UV[1] = UV[3] = (pitch_ladder_UV[1] * (i + 1));
		UV[5] = UV[7] = (pitch_ladder_UV[1] * (pitch_ladder_UV[1] * (i))); // legacy bug-guard; safe fallback

		pitch_ladder[i]->Reshape(256, 16);
		pitch_ladder[i]->SetTexCoords(UV);
		pitch_ladder[i]->SetBlendMode(2);
		pitch_ladder[i]->Hide();
	}

	for (i = 16; i < 31; i++) {
		pitch_ladder[i] = new Sprite(&pitch_ladder_neg);

		CopyMemory(UV, pitch_ladder_UV, sizeof(UV));
		UV[1] = UV[3] = (pitch_ladder_UV[1] * (30 - i));
		UV[5] = UV[7] = (pitch_ladder_UV[1] * (30 - i + 1));

		pitch_ladder[i]->Reshape(192, 16);
		pitch_ladder[i]->SetTexCoords(UV);
		pitch_ladder[i]->SetBlendMode(2);
		pitch_ladder[i]->Hide();
	}

	for (i = 0; i < 3; i++)
		mfd[i] = new MFDView(window, i);

	mfd[0]->SetRect(Rect(8, height - 136, 128, 128));
	mfd[1]->SetRect(Rect(width - 136, height - 136, 128, 128));
	mfd[2]->SetRect(Rect(8, 8, 128, 128));

	hud_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height / 2, 1));
	hud_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height / 2, 1));
	hud_left_sprite->SetBlendMode(2);
	hud_left_sprite->SetFilter(0);
	hud_right_sprite->SetBlendMode(2);
	hud_right_sprite->SetFilter(0);

	instr_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height - 128, 1));
	instr_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height - 128, 1));
	instr_left_sprite->SetBlendMode(2);
	instr_left_sprite->SetFilter(0);
	instr_right_sprite->SetBlendMode(2);
	instr_right_sprite->SetFilter(0);

	warn_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height - 128, 1));
	warn_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height - 128, 1));
	warn_left_sprite->SetBlendMode(2);
	warn_left_sprite->SetFilter(0);
	warn_right_sprite->SetBlendMode(2);
	warn_right_sprite->SetFilter(0);

	icon_ship_sprite->MoveTo(FVector(184.0f, (float)height - 72.0f, 1));
	icon_target_sprite->MoveTo(FVector((float)width - 184.0f, (float)height - 72.0f, 1));
	icon_ship_sprite->SetBlendMode(2);
	icon_ship_sprite->SetFilter(0);
	icon_target_sprite->SetBlendMode(2);
	icon_target_sprite->SetFilter(0);

	fpm_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	hpm_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	lead_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	aim_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	tgt1_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	tgt2_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	tgt3_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));
	tgt4_sprite->MoveTo(FVector((float)width / 2, (float)height / 2, 1));

	fpm_sprite->SetBlendMode(2);
	hpm_sprite->SetBlendMode(2);
	lead_sprite->SetBlendMode(2);
	aim_sprite->SetBlendMode(2);
	tgt1_sprite->SetBlendMode(2);
	tgt2_sprite->SetBlendMode(2);
	tgt3_sprite->SetBlendMode(2);
	tgt4_sprite->SetBlendMode(2);
	chase_sprite->SetBlendMode(2);

	fpm_sprite->SetFilter(0);
	hpm_sprite->SetFilter(0);
	lead_sprite->SetFilter(0);
	aim_sprite->SetFilter(0);
	tgt1_sprite->SetFilter(0);
	tgt2_sprite->SetFilter(0);
	tgt3_sprite->SetFilter(0);
	tgt4_sprite->SetFilter(0);
	chase_sprite->SetFilter(0);

	lead_sprite->Hide();
	aim_sprite->Hide();
	tgt1_sprite->Hide();
	tgt2_sprite->Hide();
	tgt3_sprite->Hide();
	tgt4_sprite->Hide();
	chase_sprite->Hide();
	
	this->aw = chase_left.Width() / 2;
	this->ah = chase_left.Height() / 2;

	mfd[0]->SetMode(EMFDMode::SHIP);
	mfd[1]->SetMode(EMFDMode::FOV);
	mfd[2]->SetMode(EMFDMode::GAME);
	
	for (int mc = 0; mc < 3; mc++) {
		mfd[mc]->SetHUDColor(standard_hud_colors[color]);
	}
	
	SetHUDColor(FColor::Black);
	SetTextColor(FColor::White);
	SetStatusColor(SYSTEM_STATUS::NOMINAL);

	FSlateFontInfo HudFontInfo;

	if (!FontManager::Find("HUD", HudFontInfo))
	{
		HudFontInfo.Size = 16; // fallback
	}

	if (!FontManager::Find("GUI", HudFontInfo))
	{
		HudFontInfo.Size = 18; // fallback
	}
	//HudFont = FontManager::Find("HUD");
	//BigFont = FontManager::Find("GUI");

	for (i = 0; i < TXT_LAST; i++) {
		hud_text[i].font = HudFont;
	}

	hud_text[TXT_THREAT_WARN].font = BigFont;
	hud_text[TXT_SHOOT].font = BigFont;
	hud_text[TXT_AUTO].font = BigFont;

	SetHUDColorSet(def_color_set);
	

	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath("HUD/");

	az_ring = new Solid;
	az_pointer = new Solid;
	el_ring = new Solid;
	el_pointer = new Solid;

	az_ring->Load("CompassRing.mag", compass_scale);
	az_pointer->Load("CompassPointer.mag", compass_scale);
	el_ring->Load("PitchRing.mag", compass_scale);
	el_pointer->Load("CompassPointer.mag", compass_scale);

	loader->SetDataPath("Sounds/");
	loader->LoadSound("MissileLock.wav", missile_lock_sound, USound::LOOP | USound::LOCKED);

	loader->SetDataPath(0);

	for (i = 0; i < MAX_MSG; i++)
		msg_time[i] = 0;
}

HUDView::~HUDView()
{
	HideCompass();

	if (missile_lock_sound) {
		missile_lock_sound->Stop();
		missile_lock_sound->Release();
		missile_lock_sound = nullptr;
	}

	for (int i = 0; i < 3; i++) {
		delete mfd[i];
		mfd[i] = nullptr;
	}

	for (int i = 0; i < 32; i++) {
		delete hud_sprite[i];
		hud_sprite[i] = nullptr;
	}

	fpm.ClearImage();
	hpm.ClearImage();
	lead.ClearImage();
	cross.ClearImage();
	cross1.ClearImage();
	cross2.ClearImage();
	cross3.ClearImage();
	cross4.ClearImage();
	hud_left_air.ClearImage();
	hud_right_air.ClearImage();
	hud_left_fighter.ClearImage();
	hud_right_fighter.ClearImage();
	hud_left_starship.ClearImage();
	hud_right_starship.ClearImage();
	instr_left.ClearImage();
	instr_right.ClearImage();
	warn_left.ClearImage();
	warn_right.ClearImage();
	icon_ship.ClearImage();
	icon_target.ClearImage();
	chase_left.ClearImage();
	chase_right.ClearImage();
	chase_top.ClearImage();
	chase_bottom.ClearImage();
	pitch_ladder_pos.ClearImage();
	pitch_ladder_neg.ClearImage();

	delete[] fpm_shade;
	delete[] hpm_shade;
	delete[] lead_shade;
	delete[] cross_shade;
	delete[] cross1_shade;
	delete[] cross2_shade;
	delete[] cross3_shade;
	delete[] cross4_shade;
	delete[] hud_left_shade_air;
	delete[] hud_right_shade_air;
	delete[] hud_left_shade_fighter;
	delete[] hud_right_shade_fighter;
	delete[] hud_left_shade_starship;
	delete[] hud_right_shade_starship;
	delete[] instr_left_shade;
	delete[] instr_right_shade;
	delete[] warn_left_shade;
	delete[] warn_right_shade;
	delete[] icon_ship_shade;
	delete[] icon_target_shade;
	delete[] chase_left_shade;
	delete[] chase_right_shade;
	delete[] chase_top_shade;
	delete[] chase_bottom_shade;
	delete[] pitch_ladder_pos_shade;
	delete[] pitch_ladder_neg_shade;

	delete az_ring;
	delete az_pointer;
	delete el_ring;
	delete el_pointer;

	fpm_shade = nullptr;
	hpm_shade = nullptr;
	cross_shade = nullptr;
	cross1_shade = nullptr;
	cross2_shade = nullptr;
	cross3_shade = nullptr;
	cross4_shade = nullptr;
	hud_left_shade_air = nullptr;
	hud_right_shade_air = nullptr;
	hud_left_shade_fighter = nullptr;
	hud_right_shade_fighter = nullptr;
	hud_left_shade_starship = nullptr;
	hud_right_shade_starship = nullptr;
	instr_left_shade = nullptr;
	instr_right_shade = nullptr;
	warn_left_shade = nullptr;
	warn_right_shade = nullptr;
	icon_ship_shade = nullptr;
	icon_target_shade = nullptr;
	chase_left_shade = nullptr;
	chase_right_shade = nullptr;
	chase_top_shade = nullptr;
	chase_bottom_shade = nullptr;
	pitch_ladder_pos_shade = nullptr;
	pitch_ladder_neg_shade = nullptr;

	az_ring = nullptr;
	az_pointer = nullptr;
	el_ring = nullptr;
	el_pointer = nullptr;

	hud_view = nullptr;
}

void
HUDView::OnWindowMove()
{
	width = Width();
	height = Height();
	xcenter = (width / 2.0) - 0.5;
	ycenter = (height / 2.0) + 0.5;

	mfd[0]->SetRect(Rect(8, height - 136, 128, 128));
	mfd[1]->SetRect(Rect(width - 136, height - 136, 128, 128));
	mfd[2]->SetRect(Rect(8, 8, 128, 128));

	mfd[0]->SetHUDColor(standard_hud_colors[color]);
	mfd[1]->SetHUDColor(standard_hud_colors[color]);
	mfd[2]->SetHUDColor(standard_hud_colors[color]);

	hud_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height / 2, 1));
	hud_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height / 2, 1));

	instr_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height - 128, 1));
	instr_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height - 128, 1));
	warn_left_sprite->MoveTo(FVector((float)width / 2 - 128, (float)height - 128, 1));
	warn_right_sprite->MoveTo(FVector((float)width / 2 + 128, (float)height - 128, 1));
	icon_ship_sprite->MoveTo(FVector(184.0f, (float)height - 72.0f, 1));
	icon_target_sprite->MoveTo(FVector((float)width - 184.0f, (float)height - 72.0f, 1));

	for (int i = 0; i < TXT_LAST; i++) {
		hud_text[i].font = HudFont;
		hud_text[i].color = standard_txt_colors[color];
	}

	if (BigFont) {
		hud_text[TXT_THREAT_WARN].font = BigFont;
		hud_text[TXT_SHOOT].font = BigFont;
		hud_text[TXT_AUTO].font = BigFont;
	}

	int cx = width / 2;
	int cy = height / 2;
	(void)cx; (void)cy;
}

// +--------------------------------------------------------------------+

void
HUDView::SetTacticalMode(int mode_in)
{
	if (tactical != mode_in) {
		tactical = mode_in;

		if (tactical) {
			hud_left_sprite->Hide();
			hud_right_sprite->Hide();

			for (int i = 0; i < 31; i++)
				pitch_ladder[i]->Hide();
		}
		else if (Game::MaxTexSize() > 128) {
			hud_left_sprite->Show();
			hud_right_sprite->Show();
		}
	}
}

void
HUDView::SetOverlayMode(int mode_in)
{
	if (overlay != mode_in) {
		overlay = mode_in;
	}
}


// +--------------------------------------------------------------------+

bool
HUDView::Update(SimObject* obj)
{
	if (obj == ship) {
		if (target)
			SetTarget(nullptr);

		ship = nullptr;

		for (int i = 0; i < 3; i++)
			mfd[i]->SetShip(ship);
	}

	if (obj == target) {
		target = nullptr;
		PrepareBitmap("hud_icon.pcx", icon_target, icon_target_shade);
		ColorizeBitmap(icon_target, icon_target_shade, TextColor);
	}

	return SimObserver::Update(obj);
}

const char*
HUDView::GetObserverName() const
{
	return "HUDView";
}

// +--------------------------------------------------------------------+

void HUDView::UseCameraView(CameraView* v)
{
	if (v && camview != v)
	{
		camview = v;

		for (int i = 0; i < 3; i++) {
			mfd[i]->UseCameraView(camview);
		}

		projector = camview->GetProjector();
	}
}

// +--------------------------------------------------------------------+

FColor
HUDView::MarkerColor(SimContact* contact)
{
	FColor c(80, 80, 80);

	if (contact) {
		Sim* sim_local = Sim::GetSim();
		Ship* ship_local = sim_local ? sim_local->GetPlayerShip() : nullptr;

		const int c_iff = ship_local ? contact->GetIFF(ship_local) : 0;

		// Legacy: c = Ship::IFFColor(c_iff) * contact->Age();
		c = ColorMul(Ship::IFFColor(c_iff), (float)contact->Age());

		if (contact->GetShot() && ship_local && contact->Threat(ship_local)) {
			if ((Game::RealTime() / 500) & 1)
				c = ColorMul(c, 2.0f);
			else
				c = ColorMul(c, 0.5f);
		}
	}

	return c;
}

// +--------------------------------------------------------------------+

void
HUDView::DrawContactMarkers()
{
	threat = 0;

	for (int i = 0; i < MAX_CONTACT; i++) {
		HideHUDText(TXT_CONTACT_NAME + i);
		HideHUDText(TXT_CONTACT_INFO + i);
	}

	if (!ship)
		return;

	int                  index = 0;
	ListIter<SimContact> contact = ship->ContactList();

	// draw own sensor contacts:
	while (++contact) {
		SimContact* c = contact.value();

		// draw track ladder:
		if (c->TrackLength() > 0 && c->GetShip() != ship) {
			DrawTrack(c);
		}

		DrawContact(c, index++);
	}

	FColor c = ship->MarkerColor();

	// draw own ship track ladder:
	if (CameraManager::GetCameraMode() == CameraManager::MODE_ORBIT && ship->TrackLength() > 0) {
		const int ctl = ship->TrackLength();

		FVector t1 = ship->Location();
		FVector t2 = ship->TrackPoint(0);

		if (t1 != t2)
			DrawTrackSegment(t1, t2, c);

		for (int i = 0; i < ctl - 1; i++) {
			t1 = ship->TrackPoint(i);
			t2 = ship->TrackPoint(i + 1);

			if (t1 != t2)
				DrawTrackSegment(t1, t2, ColorMul(c, (float)((double)(ctl - i) / (double)ctl)));
		}
	}

	// draw own ship marker:
	FVector mark_pt = ship->Location();
	projector->Transform(mark_pt);

	// clip:
	if (CameraManager::GetCameraMode() == CameraManager::MODE_ORBIT && mark_pt.Z > 1.0f) {
		projector->Project(mark_pt);

		const int x = (int)mark_pt.X;
		const int y = (int)mark_pt.Y;

		if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
			DrawDiamond(x, y, 5, c);

			if (tactical) {
				Rect self_rect(x + 8, y - 4, 200, 12);
				DrawHUDText(TXT_SELF, ship->Name(), self_rect, DT_LEFT, HUD_MIXED_CASE);
			}
		}
	}

	// draw life bars on targeted ship:
	if (target && target->Type() == SimObject::SIM_SHIP && target->Rep()) {
		Ship* tgt_ship = (Ship*)target;
		if (!tgt_ship) {
			UE_LOG(LogTemp, Warning, TEXT("Null pointer in HUDView::DrawContactMarkers() - tgt_ship"));
			return;
		}

		Graphic* g = tgt_ship->Rep();
		Rect     r = g->ScreenRect();

		FVector mark_pt2 = tgt_ship->Location();
		projector->Transform(mark_pt2);

		// clip:
		if (mark_pt2.Z > 1.0f) {
			projector->Project(mark_pt2);

			int x = (int)mark_pt2.X;
			int y = r.y;

			if (y >= 2000)
				y = (int)mark_pt2.Y;

			if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
				const int BAR_LENGTH = 40;

				// life bars:
				const int sx = x - BAR_LENGTH / 2;
				const int sy = y - 8;

				const double hull_strength = tgt_ship->Integrity() / tgt_ship->Design()->integrity;

				const int hw = (int)(BAR_LENGTH * hull_strength);
				const int sw = (int)(BAR_LENGTH * (tgt_ship->ShieldStrength() / 100.0));

				SYSTEM_STATUS s = SYSTEM_STATUS::NOMINAL;

				if (hull_strength < 0.30)        s = SYSTEM_STATUS::CRITICAL;
				else if (hull_strength < 0.60)   s = SYSTEM_STATUS::DEGRADED;

				const FColor hc = GetStatusColor(s);
				const FColor sc = HudColor;

				FillRect(sx, sy, sx + hw, sy + 1, hc);
				FillRect(sx, sy + 3, sx + sw, sy + 4, sc);
			}
		}
	}
}
// +--------------------------------------------------------------------+

void HUDView::DrawTarget()
{
	const int bar_width = 256;
	const int bar_height = 192;
	const int box_width = 120;

	SimObject* old_target = target;

	if (mode == EHUDMode::ILS)
	{
		Ship* controller = ship ? ship->GetController() : nullptr;
		if (controller && !target)
			target = controller;
	}

	if (target && target->Rep())
	{
		Sensor* sensor = ship ? ship->GetSensor() : nullptr;
		SimContact* contact = nullptr;

		if (sensor && ship && target->Type() == SimObject::SIM_SHIP)
		{
			contact = sensor->FindContact((Ship*)target);
		}

		const int cx = width / 2;
		const int cy = height / 2;

		const int l = cx - bar_width / 2;
		const int r = cx + bar_width / 2;
		const int t = cy - bar_height / 2;
		const int b = cy + bar_height / 2;

		FVector delta = target->Location() - ship->Location();
		double  distance = delta.Size();

		FVector delta_v = ship->Velocity() - target->Velocity();
		double  speed = delta_v.Size();

		char txt[256] = {};

		if (mode == EHUDMode::ILS && ship->GetInbound() && ship->GetInbound()->GetDeck())
		{
			delta = ship->GetInbound()->GetDeck()->EndPoint() - ship->Location();
			distance = delta.Size();
		}

		// --------------------------------------------------------------------
		// Determine closing speed sign (UE-correct dot usage)
		// --------------------------------------------------------------------
		const FVector ShipVel = ship->Velocity();
		const double  delta_dot_vel = delta.Dot(ShipVel);

		if (delta_dot_vel > 0.0) // target in front
		{
			if (delta_v.Dot(ShipVel) < 0.0) // losing ground
				speed = -speed;
		}
		else // target behind
		{
			if (delta_v.Dot(ShipVel) > 0.0) // passing
				speed = -speed;
		}

		Rect range_rect(r - 20, cy - 5, box_width, 12);

		if (tactical)
			range_rect.x = width - range_rect.w - 8;

		if (contact)
		{
			Sensor* sensor2 = ship->GetSensor();
			double limit = 75e3;

			if (sensor2)
				limit = sensor2->GetBeamRange();

			distance = contact->Range(ship, limit);

			if (!contact->ActLock() && !contact->PasLock())
			{
				strcpy_s(txt, Game::GetText("HUDView.No-Range").data());
				speed = 0;
			}
			else
			{
				FormatNumber(txt, distance);
			}
		}
		else
		{
			FormatNumber(txt, distance);
		}

		DrawHUDText(TXT_RANGE, txt, range_rect, DT_RIGHT);

		if (arcade)
		{
			target = old_target;
			return;
		}

		range_rect.y += 18;
		FormatNumber(txt, speed);
		DrawHUDText(TXT_CLOSING_SPEED, txt, range_rect, DT_RIGHT);

		// --------------------------------------------------------------------
		// Target info
		// --------------------------------------------------------------------
		if (!tactical)
		{
			range_rect.y = cy - 76;
		}
		else
		{
			range_rect.x = width - 2 * box_width - 8;
			range_rect.y = cy - 76;
			range_rect.w = 2 * box_width;
		}

		DrawHUDText(TXT_TARGET_NAME, target->Name(), range_rect, DT_RIGHT);

		if (target->Type() == SimObject::SIM_SHIP)
		{
			Ship* tgt_ship = (Ship*)target;

			range_rect.y += 10;
			DrawHUDText(
				TXT_TARGET_DESIGN,
				tgt_ship->Design()->display_name,
				range_rect,
				DT_RIGHT
			);

			if (mode != EHUDMode::ILS)
			{
				if (tgt_ship->IsStarship())
				{
					range_rect.y += 10;
					sprintf_s(
						txt,
						"%s %03d",
						Game::GetText("HUDView.symbol.shield").data(),
						(int)tgt_ship->ShieldStrength()
					);
					DrawHUDText(TXT_TARGET_SHIELD, txt, range_rect, DT_RIGHT);
				}

				range_rect.y += 10;
				sprintf_s(
					txt,
					"%s %03d",
					Game::GetText("HUDView.symbol.hull").data(),
					(int)(tgt_ship->Integrity() /
						tgt_ship->Design()->integrity * 100)
				);
				DrawHUDText(TXT_TARGET_HULL, txt, range_rect, DT_RIGHT);

				SimSystem* sys = ship->GetSubTarget();
				if (sys)
				{
					FColor       stat = HudColor;
					static DWORD blink = Game::RealTime();

					int blink_delta = Game::RealTime() - blink;
					sprintf_s(txt, "%s %03d", sys->Abbreviation(), (int)sys->Availability());

					switch (sys->GetStatus())
					{
					case SYSTEM_STATUS::DEGRADED:
						stat = FColor(255, 255, 0);
						break;

					case SYSTEM_STATUS::CRITICAL:
					case SYSTEM_STATUS::DESTROYED:
						stat = FColor(255, 0, 0);
						break;

					case SYSTEM_STATUS::MAINT:
						if (blink_delta < 250)
							stat = FColor(8, 8, 8);
						break;
					}

					if (blink_delta > 500)
						blink = Game::RealTime();

					range_rect.y += 10;
					DrawHUDText(TXT_TARGET_SUB, txt, range_rect, DT_RIGHT);
				}
			}
		}
		else if (target->Type() == SimObject::SIM_DRONE)
		{
			Drone* tgt_drone = (Drone*)target;

			range_rect.y += 10;
			DrawHUDText(TXT_TARGET_DESIGN, tgt_drone->DesignName(), range_rect, DT_RIGHT);

			range_rect.y += 10;
			int eta = tgt_drone->GetEta();

			if (eta > 0)
			{
				int minutes = (eta / 60) % 60;
				int seconds = eta % 60;

				char eta_buf[16];
				sprintf_s(eta_buf, "T %d:%02d", minutes, seconds);
				DrawHUDText(TXT_TARGET_ETA, eta_buf, range_rect, DT_RIGHT);
			}
		}
	}
	target = old_target;
}


// +--------------------------------------------------------------------+

void
HUDView::DrawNavInfo()
{
	const int bar_width = 256;
	const int bar_height = 192;
	const int box_width = 120;

	if (arcade) {
		if (ship && ship->IsAutoNavEngaged()) {
			Rect info_rect(width / 2 - box_width, height / 2 + bar_height, box_width * 2, 12);

			if (BigFont)
				hud_text[TXT_NAV_INDEX].font = BigFont;

			DrawHUDText(TXT_NAV_INDEX, Game::GetText("HUDView.Auto-Nav"), info_rect, DT_CENTER);
		}

		return;
	}

	hud_text[TXT_NAV_INDEX].font = HudFont;

	Instruction* navpt = ship ? ship->GetNextNavPoint() : 0;

	if (navpt) {
		int cx = width / 2;
		int cy = height / 2;
		int l = cx - bar_width / 2;
		int r = cx + bar_width / 2;
		int t = cy - bar_height / 2;
		int b = cy + bar_height / 2;

		int    index = ship->GetNavIndex(navpt);
		double distance = ship->RangeToNavPoint(navpt);
		double speed = ship->Velocity().Size();
		int    etr = 0;
		char   txt[256];

		if (speed > 10)
			etr = (int)(distance / speed);

		Rect info_rect(r - 20, cy + 32, box_width, 12);

		if (tactical)
			info_rect.x = width - info_rect.w - 8;

		if (ship->IsAutoNavEngaged())
			sprintf_s(txt, "%s %d", "Auto Nav", index);
		else
			sprintf_s(txt, "%s %d", "Nav", index);

		DrawHUDText(TXT_NAV_INDEX, txt, info_rect, DT_RIGHT);

		info_rect.y += 10;
		const INSTRUCTION_ACTION Action = navpt->GetAction();
		if (Action != INSTRUCTION_ACTION::NONE)
			DrawHUDText(TXT_NAV_ACTION, Instruction::ActionName(Action), info_rect, DT_RIGHT);;

		info_rect.y += 10;
		FormatNumber(txt, navpt->GetSpeed());
		DrawHUDText(TXT_NAV_SPEED, txt, info_rect, DT_RIGHT);

		if (etr > 3600) {
			info_rect.y += 10;
			sprintf_s(txt, "%s XX:XX","ETR");
			DrawHUDText(TXT_NAV_ETR, txt, info_rect, DT_RIGHT);
		}
		else if (etr > 0) {
			info_rect.y += 10;

			int minutes = (etr / 60) % 60;
			int seconds = (etr) % 60;
			sprintf_s(txt, "%s %2d:%02d", "ETR", minutes, seconds);
			DrawHUDText(TXT_NAV_ETR, txt, info_rect, DT_RIGHT);
		}

		if (navpt->GetHoldTime() > 0) {
			info_rect.y += 10;

			int hold = (int)navpt->GetHoldTime();
			int minutes = (hold / 60) % 60;
			int seconds = (hold) % 60;
			sprintf_s(txt, "%s %2d:%02d", "HOLD", minutes, seconds);
			DrawHUDText(TXT_NAV_HOLD, txt, info_rect, DT_RIGHT);
		}
	}
}

// +--------------------------------------------------------------------+

void
HUDView::DrawSight()
{
	if (target && target->Rep()) {
		FVector delta = target->Location() - ship->Location();
		double  distance = delta.Size();

		// draw LCOS on target:
		if (!tactical)
			DrawLCOS(target, distance);
	}
}

// +--------------------------------------------------------------------+

void
HUDView::DrawDesignators()
{
	double     xtarg = xcenter;
	double     ytarg = ycenter;
	SimObject* t1 = 0;
	SimObject* t2 = 0;
	SimObject* t3 = 0;
	Sprite* sprite = 0;

	tgt1_sprite->Hide();
	tgt2_sprite->Hide();
	tgt3_sprite->Hide();
	tgt4_sprite->Hide();

	// fighters just show primary target:
	if (ship->IsDropship()) {
		SimObject* t = ship->GetTarget();
		SimSystem* s = ship->GetSubTarget();

		if (t) {
			FVector tloc = t->Location();

			if (s) {
				tloc = s->MountLocation();
			}
			else if (t->Type() == SimObject::SIM_SHIP) {
				Ship* tgt_ship = (Ship*)t;

				if (tgt_ship->IsGroundUnit())
					tloc += FVector(0, 150, 0);
			}

			projector->Transform(tloc);

			if (tloc.Z > 0) {
				projector->Project(tloc);

				xtarg = tloc.X;
				ytarg = tloc.Y;

				if (xtarg > 0 && xtarg < width - 1 && ytarg>0 && ytarg < height - 1) {
					double range = (t->Location() - ship->Location()).Size();

					// use out-of-range crosshair if out of range:
					if (!ship->GetPrimaryDesign() || ship->GetPrimaryDesign()->max_range < range) {
						tgt4_sprite->Show();
						tgt4_sprite->MoveTo(FVector(xtarg, ytarg, 1));
					}

					// else, use in-range primary crosshair:
					else {
						tgt1_sprite->Show();
						tgt1_sprite->MoveTo(FVector(xtarg, ytarg, 1));
					}
				}
			}
		}
	}

	// starships show up to three targets:
	else {
		ListIter<WeaponGroup> w = ship->GetWeapons();
		while (!t3 && ++w) {
			SimObject* t = w->GetTarget();
			SimSystem* s = w->GetSubTarget();

			if (w->Contains(ship->GetPrimary())) {
				if (t == 0) t = ship->GetTarget();
				t1 = t;
				sprite = tgt1_sprite;
			}

			else if (t && w->Contains(ship->GetSecondary())) {
				t2 = t;
				sprite = tgt2_sprite;

				if (t2 == t1)
					continue;   // don't overlap target designators
			}

			else if (t) {
				t3 = t;
				sprite = tgt3_sprite;

				if (t3 == t1 || t3 == t2)
					continue;   // don't overlap target designators
			}

			if (t) {
				FVector tloc = t->Location();

				if (s)
					tloc = s->MountLocation();

				projector->Transform(tloc);

				if (tloc.Z > 0) {
					projector->Project(tloc);

					xtarg = tloc.X;
					ytarg = tloc.Y;

					if (xtarg > 0 && xtarg < width - 1 && ytarg>0 && ytarg < height - 1) {
						double range = (t->Location() - ship->Location()).Size();

						// flip to out-of-range crosshair
						if (sprite == tgt1_sprite) {
							if (!ship->GetPrimaryDesign() || ship->GetPrimaryDesign()->max_range < range) {
								sprite = tgt4_sprite;
							}
						}

						sprite->Show();
						sprite->MoveTo(FVector(xtarg, ytarg, 1));
					}
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

FColor
HUDView::GetStatusColor(SYSTEM_STATUS status)
{
	FColor sc;

	switch (status) {
	default:
	case SYSTEM_STATUS::NOMINAL:     sc = FColor(32, 192, 32);  break;
	case SYSTEM_STATUS::DEGRADED:    sc = FColor(255, 255, 0);  break;
	case SYSTEM_STATUS::CRITICAL:    sc = FColor(255, 0, 0);  break;
	case SYSTEM_STATUS::DESTROYED:   sc = FColor(0, 0, 0);  break;
	}

	return sc;
}


// +--------------------------------------------------------------------+

static SYSTEM_STATUS GetReactorStatus(Ship* ship)
{
	if (!ship || ship->Reactors().size() < 1)
		return SYSTEM_STATUS::UNKNOWN;

	SYSTEM_STATUS  status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	ListIter<PowerSource> iter = ship->Reactors();
	while (++iter) {
		PowerSource* s = iter.value();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetDriveStatus(Ship* ship)
{
	if (!ship || ship->GetDrives().size() < 1)
		return SYSTEM_STATUS::UNKNOWN;

	SYSTEM_STATUS  status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	ListIter<Drive> iter = ship->GetDrives();
	while (++iter) {
		Drive* s = iter.value();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetQuantumStatus(Ship* ship)
{
	if (!ship || ship->GetQuantumDrive() == 0)
		return SYSTEM_STATUS::UNKNOWN;

	QuantumDrive* s = ship->GetQuantumDrive();
	return s->GetStatus();
}

static SYSTEM_STATUS GetThrusterStatus(Ship* ship)
{
	if (!ship || ship->GetThruster() == 0)
		return SYSTEM_STATUS::UNKNOWN;

	Thruster* s = ship->GetThruster();
	return s->GetStatus();
}

static SYSTEM_STATUS GetShieldStatus(Ship* ship)
{
	if (!ship)
		return SYSTEM_STATUS::UNKNOWN;

	Shield* s = ship->GetShield();
	Weapon* d = ship->GetDecoy();

	if (!s && !d)
		return SYSTEM_STATUS::UNKNOWN;

	SYSTEM_STATUS  status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	if (s) {
		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (d) {
		if (d->GetStatus() < status)
			status = d->GetStatus();
		else if (d->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetWeaponStatus(Ship* ship, int index)
{
	if (!ship || ship->GetWeapons().size() <= index)
		return SYSTEM_STATUS::UNKNOWN;

	WeaponGroup* group = ship->GetWeapons().at(index);

	SYSTEM_STATUS  status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	ListIter<Weapon> iter = group->GetWeapons();
	while (++iter) {
		Weapon* s = iter.value();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetSensorStatus(Ship* ship)
{
	if (!ship || ship->GetSensor() == 0)
		return SYSTEM_STATUS::UNKNOWN;

	Sensor* s = ship->GetSensor();
	Weapon* p = ship->GetProbeLauncher();

	SYSTEM_STATUS status = s->GetStatus();
	bool maint = s->GetStatus() == SYSTEM_STATUS::MAINT;

	if (p) {
		if (p->GetStatus() < status)
			status = p->GetStatus();
		else if (p->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetComputerStatus(Ship* ship)
{
	if (!ship || ship->GetComputers().size() < 1)
		return SYSTEM_STATUS::UNKNOWN;

	SYSTEM_STATUS status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	ListIter<Computer> iter = ship->GetComputers();
	while (++iter) {
		Computer* s = iter.value();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (ship->GetNavSystem()) {
		NavSystem* s = ship->GetNavSystem();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

static SYSTEM_STATUS GetFlightDeckStatus(Ship* ship)
{
	if (!ship || ship->FlightDecks().size() < 1)
		return SYSTEM_STATUS::UNKNOWN;

	SYSTEM_STATUS status = SYSTEM_STATUS::NOMINAL;
	bool maint = false;

	ListIter<FlightDeck> iter = ship->FlightDecks();
	while (++iter) {
		FlightDeck* s = iter.value();

		if (s->GetStatus() < status)
			status = s->GetStatus();
		else if (s->GetStatus() == SYSTEM_STATUS::MAINT)
			maint = true;
	}

	if (maint && status == SYSTEM_STATUS::NOMINAL)
		status = SYSTEM_STATUS::MAINT;

	return status;
}

// +--------------------------------------------------------------------+

void
HUDView::DrawWarningPanel()
{
	int box_width = 75;
	int box_height = 17;
	int row_height = 28;
	int box_left = width / 2 - box_width * 2;

	if (cockpit_hud_texture) {
		box_left = 275;
		box_height = 18;
		row_height = 18;
	}

	if (ship) {
		if (Game::MaxTexSize() > 128) {
			warn_left_sprite->Show();
			warn_right_sprite->Show();
		}

		int x = box_left;
		int y = cockpit_hud_texture ? 410 : height - 97;
		int c = cockpit_hud_texture ? 3 : 4;

		static uint32 blink = (uint32)Game::RealTime();

		for (int index = 0; index < 12; index++) {
			
			SYSTEM_STATUS  stat = SYSTEM_STATUS::UNKNOWN;
			Text abrv = Game::GetText("HUDView.UNKNOWN");

			switch (index) {
			case 0:  stat = GetReactorStatus(ship);    abrv = Game::GetText("HUDView.REACTOR");  break;
			case 1:  stat = GetDriveStatus(ship);      abrv = Game::GetText("HUDView.DRIVE");    break;
			case 2:  stat = GetQuantumStatus(ship);    abrv = Game::GetText("HUDView.QUANTUM");  break;
			case 3:  stat = GetShieldStatus(ship);     abrv = Game::GetText("HUDView.SHIELD");
				if (ship->GetShield() == 0 && ship->GetDecoy())
					abrv = Game::GetText("HUDView.DECOY");
				break;

			case 4:
			case 5:
			case 6:
			case 7:  stat = GetWeaponStatus(ship, index - 4);
				if (stat != SYSTEM_STATUS::UNKNOWN) {
					WeaponGroup* g = ship->GetWeapons().at(index - 4);
					abrv = g->Name();
				}
				break;

			case 8:  stat = GetSensorStatus(ship);    
				abrv = "SENSOR";
				break;
			case 9:  stat = GetComputerStatus(ship);
				abrv = "COMP";
				break;
			case 10: stat = GetThrusterStatus(ship);   
				abrv = "THRUST";
				break;
			case 11: stat = GetFlightDeckStatus(ship); 
				abrv = "FLTDECK"; 
				break;
			}

			Rect warn_rect(x, y, box_width, box_height);

			if (cockpit_hud_texture) {
				cockpit_hud_texture->DrawRect(ToIntRect(warn_rect), FColor(64, 64, 64));
			}

			if (stat != SYSTEM_STATUS::UNKNOWN) {
				SetStatusColor((SYSTEM_STATUS)stat);
				FColor tc = StatusColor;

				if (stat != SYSTEM_STATUS::NOMINAL) {
					if (Game::RealTime() - blink < 250) {
						tc = cockpit_hud_texture ? TextColor : FColor(8, 8, 8);
					}
				}

				if (cockpit_hud_texture) {
					if (tc != TextColor) {
						Rect r2 = warn_rect;
						r2.Inset(1, 1, 1, 1);
						cockpit_hud_texture->FillRect(ToIntRect(r2), tc);
						tc = FColor::Black;
					}

					warn_rect.y += 4;

					HudFont->SetColor(tc);
					HudFont->DrawText(abrv, -1,
						warn_rect,
						DT_CENTER | DT_SINGLELINE,
						cockpit_hud_texture);

					warn_rect.y -= 4;
				}
				else {
					DrawHUDText(TXT_CAUTION_TXT + index,
						abrv,
						warn_rect,
						DT_CENTER);

					hud_text[TXT_CAUTION_TXT + index].color = tc;
				}
			}

			x += box_width;

			if (--c <= 0) {
				c = cockpit_hud_texture ? 3 : 4;
				x = box_left;
				y += row_height;
			}
		}

		if (Game::RealTime() - blink > 500)
			blink = (uint32)Game::RealTime();

		// reset for next time
		SetStatusColor(SYSTEM_STATUS::NOMINAL);
	}
}

// +--------------------------------------------------------------------+

void
HUDView::DrawInstructions()
{
	if (!ship) return;

	if (Game::MaxTexSize() > 128) {
		instr_left_sprite->Show();
		instr_right_sprite->Show();
	}

	int         ninst = 0;
	int         nobj = 0;
	SimElement* elem = ship->GetElement();

	if (elem) {
		ninst = elem->NumInstructions();
		nobj = elem->NumObjectives();
	}

	Rect r = Rect(width / 2 - 143, height - 105, 290, 17);

	if (ninst) {
		int npages = ninst / 6 + (ninst % 6 ? 1 : 0);

		if (inst_page >= npages)
			inst_page = npages - 1;
		else if (inst_page < 0)
			inst_page = 0;

		int first = inst_page * 6;
		int last = first + 6;
		if (last > ninst) last = ninst;

		int n = TXT_CAUTION_TXT;

		for (int i = first; i < last; i++) {
			hud_text[n].color = standard_txt_colors[color];
			DrawHUDText(n++, FormatInstruction(elem->GetInstruction(i)), r, DT_LEFT, HUD_MIXED_CASE);
			r.y += 14;
		}

		char page[32] = { 0 };
		snprintf(page, sizeof(page), "%d / %d", inst_page + 1, npages);
		r = Rect(width / 2 + 40, height - 16, 110, 16);
		DrawHUDText(TXT_INSTR_PAGE, page, r, DT_CENTER, HUD_MIXED_CASE);
	}

	else if (nobj) {
		int n = TXT_CAUTION_TXT;

		for (int i = 0; i < nobj; i++) {
			char desc[256] = { 0 };
			snprintf(desc, sizeof(desc), "* %s", elem->GetObjective(i)->GetShortDescription());
			hud_text[n].color = standard_txt_colors[color];
			DrawHUDText(n++, desc, r, DT_LEFT, HUD_MIXED_CASE);
			r.y += 14;
		}
	}

	else {
		hud_text[TXT_CAUTION_TXT].color = standard_txt_colors[color];
		DrawHUDText(TXT_CAUTION_TXT, Game::GetText("HUDView.No-Instructions"), r, DT_LEFT, HUD_MIXED_CASE);
	}
}

// +--------------------------------------------------------------------+

const char*
HUDView::FormatInstruction(Text instr)
{
	if (!instr.contains('$'))
		return (const char*)instr;

	static char result[256];

	const char* s = (const char*)instr;
	char* d = result;

	KeyMap& keymap = Starshatter::GetInstance()->GetKeyMap();

	while (*s) {
		if (*s == '$') {
			s++;
			char action[32];
			char* a = action;
			while (*s && (isupper(*s) || isdigit(*s) || *s == '_')) *a++ = *s++;
			*a = 0;

			int         act = KeyMap::GetKeyAction(action);
			int         key = keymap.FindMapIndex(act);
			const char* s2 = keymap.DescribeKey(key);

			if (!s2) s2 = action;
			while (*s2) *d++ = *s2++;
		}
		else {
			*d++ = *s++;
		}
	}

	*d = 0;

	return result;
}

// +--------------------------------------------------------------------+

void
HUDView::CycleInstructions(int direction)
{
	if (direction > 0)
		inst_page++;
	else
		inst_page--;
}

// +--------------------------------------------------------------------+

void
HUDView::DrawMessages()
{
	int message_queue_empty = true;

	// age messages:
	for (int i = 0; i < MAX_MSG; i++) {
		if (msg_time[i] > 0) {
			msg_time[i] -= Game::GUITime();

			if (msg_time[i] <= 0) {
				msg_time[i] = 0;
				msg_text[i] = "";
			}

			message_queue_empty = false;
		}
	}

	if (!message_queue_empty) {
		// advance message pipeline:
		for (int i = 0; i < MAX_MSG; i++) {
			if (msg_time[0] == 0) {
				for (int j = 0; j < MAX_MSG - 1; j++) {
					msg_time[j] = msg_time[j + 1];
					msg_text[j] = msg_text[j + 1];
				}

				msg_time[MAX_MSG - 1] = 0;
				msg_text[MAX_MSG - 1] = "";
			}
		}

		// draw messages:
		for (int i = 0; i < MAX_MSG; i++) {
			int index = TXT_MSG_1 + i;

			if (msg_time[i] > 0) {
				Rect msg_rect(10, 95 + i * 10, width - 20, 12);
				DrawHUDText(index, msg_text[i], msg_rect, DT_LEFT, HUD_MIXED_CASE);

				if (msg_time[i] > 1)
					hud_text[index].color = TextColor;
				else {
					// Legacy: txt_color.dim(...)
					// If you still have a dim helper on FColor in your port, keep it.
					// Otherwise, approximate via linear scale:
					const float a = (float)(0.5 + 0.5 * msg_time[i]);
					hud_text[index].color = FColor(
						(uint8)FMath::Clamp((int)(TextColor.R * a), 0, 255),
						(uint8)FMath::Clamp((int)(TextColor.G * a), 0, 255),
						(uint8)FMath::Clamp((int)(TextColor.B * a), 0, 255),
						TextColor.A
					);
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
HUDView::DrawNav()
{
	if (!sim)
		return;

	active_region = sim->GetActiveRegion();

	if (ship) {
		int nav_index = 1;
		Instruction* next = ship->GetNextNavPoint();

		if (mode == EHUDMode::Navigation) {
			if (next && next->GetAction() == INSTRUCTION_ACTION::LAUNCH)
				DrawNavPoint(*next, 0, true);

			ListIter<Instruction> navpt = ship->GetFlightPlan();
			while (++navpt) {
				DrawNavPoint(*navpt.value(), nav_index++, (navpt.value() == next));
			}
		}
		else if (next) {
			DrawNavPoint(*next, 0, true);
		}
	}
}

void
HUDView::DrawILS()
{
	if (ship) {
		bool hoops_drawn = false;
		bool same_sector = false;

		InboundSlot* inbound = ship->GetInbound();
		if (inbound) {
			FlightDeck* fd = inbound->GetDeck();

			if (fd && fd->IsRecoveryDeck() && fd->GetCarrier()) {
				if (fd->GetCarrier()->GetRegion() == ship->GetRegion())
					same_sector = true;

				if (same_sector && mode == EHUDMode::ILS && !transition && !docking) {
					FVector dst = fd->MountLocation();
					projector->Transform(dst);

					if (dst.Z > 1.0) {
						projector->Project(dst);

						int x = (int)dst.X;
						int y = (int)dst.Y;

						if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
							DrawLine(x - 6, y - 6, x + 6, y + 6, HudColor);
							DrawLine(x + 6, y - 6, x - 6, y + 6, HudColor);
						}
					}
				}

				// draw the hoops for this flight deck:
				SimScene* scene = camview->GetScene();
				for (int h = 0; h < fd->NumHoops(); h++) {
					Hoop* hoop = fd->GetHoops() + h;
					if (hoop && scene) {
						if (same_sector && mode == EHUDMode::ILS && !transition && !docking) {
							scene->AddGraphic(hoop);
							hoop->Show();
							hoops_drawn = true;
						}
						else {
							hoop->Hide();
							scene->DelGraphic(hoop);
						}
					}
				}
			}
		}

		if (!hoops_drawn) {
			ListIter<Ship> iter = ship->GetRegion()->GetCarriers();
			while (++iter) {
				Ship* carrier = iter.value();

				bool ours = (carrier->GetIFF() == ship->GetIFF()) || (carrier->GetIFF() == 0);

				for (int i = 0; i < carrier->NumFlightDecks(); i++) {
					FlightDeck* fd = carrier->GetFlightDeck(i);

					if (fd && fd->IsRecoveryDeck()) {
						if (mode == EHUDMode::ILS && ours && !transition && !docking) {
							FVector dst = fd->MountLocation();
							projector->Transform(dst);

							if (dst.Z > 1.0) {
								projector->Project(dst);

								int x = (int)dst.X;
								int y = (int)dst.Y;

								if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
									DrawLine(x - 6, y - 6, x + 6, y + 6, HudColor);
									DrawLine(x + 6, y - 6, x - 6, y + 6, HudColor);
								}
							}
						}

						// draw the hoops for this flight deck:
						SimScene* scene = camview->GetScene();
						for (int h = 0; h < fd->NumHoops(); h++) {
							Hoop* hoop = fd->GetHoops() + h;
							if (hoop && scene) {
								if (mode == EHUDMode::ILS && ours && !transition && !docking) {
									hoop->Show();
									if (!hoop->GetScene())
										scene->AddGraphic(hoop);
								}
								else {
									hoop->Hide();
									if (hoop->GetScene())
										scene->DelGraphic(hoop);
								}
							}
						}
					}
				}
			}
		}
	}
}

void
HUDView::DrawObjective()
{
	if (ship && ship->GetDirector() && ship->GetDirector()->Type() >= SteerAI::SEEKER) {
		SteerAI* steer = (SteerAI*)ship->GetDirector();

		FVector obj = steer->GetObjective();
		projector->Transform(obj);

		if (obj.Z > 1.0) {
			projector->Project(obj);

			int x = (int)obj.X;
			int y = (int)obj.Y;

			if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
				FColor c = FColor::Cyan;
				DrawRect(x - 6, y - 6, x + 6, y + 6, c);
				DrawLine(x - 6, y - 6, x + 6, y + 6, c);
				DrawLine(x + 6, y - 6, x - 6, y + 6, c);
			}
		}

		if (steer->GetOther()) {
			obj = steer->GetOther()->Location();
			projector->Transform(obj);

			if (obj.Z > 1.0) {
				projector->Project(obj);

				int x = (int)obj.X;
				int y = (int)obj.Y;

				if (x > 4 && x < width - 4 && y > 4 && y < height - 4) {
					FColor c = FColor::Orange;
					DrawRect(x - 6, y - 6, x + 6, y + 6, c);
					DrawLine(x - 6, y - 6, x + 6, y + 6, c);
					DrawLine(x + 6, y - 6, x - 6, y + 6, c);
				}
			}
		}
	}
	/***/
}

void HUDView::DrawNavPoint(Instruction& navpt, int index, int next)
{
	if (index >= 15 || !navpt.GetRegion())
		return;

	FVector npt = navpt.GetRegion()->GetLocation() + navpt.GetLocation();

	if (active_region)
		npt -= active_region->GetLocation();
		
	npt = OtherHand(npt);

	projector->Transform(npt);

	if (npt.Z > 1.0)
	{
		projector->Project(npt);

		const int x = (int)npt.X;
		const int y = (int)npt.Y;

		if (x > 4 && x < width - 4 && y > 4 && y < height - 4)
		{
			FColor c = FColor::White;
			if (navpt.GetStatus() > INSTRUCTION_STATUS::ACTIVE && navpt.GetHoldTime() <= 0)
				c = FColor(64, 64, 64);

			if (next)
				DrawEllipse(x - 6, y - 6, x + 5, y + 5, c);

			DrawLine(x - 6, y - 6, x + 6, y + 6, c);
			DrawLine(x + 6, y - 6, x - 6, y + 6, c);

			if (index > 0)
			{
				char npt_buf[32] = { 0 };
				Rect npt_rect(x + 10, y - 4, 200, 12);

				if (navpt.GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt.GetHoldTime() > 0)
				{
					char hold_time[32] = { 0 };
					FormatTime(hold_time, navpt.GetHoldTime());
					snprintf(npt_buf, sizeof(npt_buf), "%d %s", index, hold_time);
				}
				else
				{
					snprintf(npt_buf, sizeof(npt_buf), "%d", index);
				}

				DrawHUDText(TXT_NAV_PT + index, npt_buf, npt_rect, DT_LEFT);
			}
		}
	}

	if (next && mode == EHUDMode::Navigation && navpt.GetRegion() == ship->GetRegion())
	{
		FVector tloc = OtherHand(navpt.GetLocation());
		projector->Transform(tloc);

		const bool behind = (tloc.Z < 0);
		if (behind)
			tloc.Z = -tloc.Z;

		projector->Project(tloc);

		if (behind ||
			tloc.X <= 0 || tloc.X >= width - 1 ||
			tloc.Y <= 0 || tloc.Y >= height - 1)
		{
			if (tloc.X <= 0 || (behind && tloc.X < width / 2))
			{
				if (tloc.Y < ah) tloc.Y = (float)ah;
				else if (tloc.Y >= height - ah) tloc.Y = (float)(height - 1 - ah);

				chase_sprite->Show();
				chase_sprite->SetAnimation(&chase_left);
				chase_sprite->MoveTo(FVector((float)aw, tloc.Y, 1));
			}
			else if (tloc.X >= width - 1 || behind)
			{
				if (tloc.Y < ah) tloc.Y = (float)ah;
				else if (tloc.Y >= height - ah) tloc.Y = (float)(height - 1 - ah);

				chase_sprite->Show();
				chase_sprite->SetAnimation(&chase_right);
				chase_sprite->MoveTo(FVector((float)(width - 1 - aw), tloc.Y, 1));
			}
			else
			{
				if (tloc.X < aw) tloc.X = (float)aw;
				else if (tloc.X >= width - aw) tloc.X = (float)(width - 1 - aw);

				if (tloc.Y <= 0)
				{
					chase_sprite->Show();
					chase_sprite->SetAnimation(&chase_top);
					chase_sprite->MoveTo(FVector(tloc.X, (float)ah, 1));
				}
				else if (tloc.Y >= height - 1)
				{
					chase_sprite->Show();
					chase_sprite->SetAnimation(&chase_bottom);
					chase_sprite->MoveTo(FVector(tloc.X, (float)(height - 1 - ah), 1));
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
HUDView::SetShip(Ship* s)
{
	if (ship != s) {
		double new_scale = 1;

		ship_status = SYSTEM_STATUS::UNKNOWN;
		ship = s;

		if (ship) {
			if (ship->Life() == 0 || ship->IsDying() || ship->IsDead()) {
				ship = 0;
			}
			else {
				Observe(ship);
				new_scale = 1.1 * ship->Radius() / 64;

				if (ship->Design()->hud_icon.Width()) {
					TransferBitmap(ship->Design()->hud_icon, icon_ship, icon_ship_shade);
					ColorizeBitmap(icon_ship, icon_ship_shade, TextColor);
				}
			}
		}

		if (az_ring) {
			az_ring->Rescale(1 / compass_scale);
			az_ring->Rescale(new_scale);
		}

		if (az_pointer) {
			az_pointer->Rescale(1 / compass_scale);
			az_pointer->Rescale(new_scale);
		}

		if (el_ring) {
			el_ring->Rescale(1 / compass_scale);
			el_ring->Rescale(new_scale);
		}

		if (el_pointer) {
			el_pointer->Rescale(1 / compass_scale);
			el_pointer->Rescale(new_scale);
		}

		compass_scale = new_scale;
		inst_page = 0;

		if (ship && ship->GetElement() && ship->GetElement()->NumInstructions() > 0)
			if (!show_inst)
				CycleHUDInst();
	}

	else if (ship && ship->Design()->hud_icon.Width()) {
		bool update = false;
		SYSTEM_STATUS sstat = SYSTEM_STATUS::NOMINAL;
		int integrity = (int)(ship->Integrity() / ship->Design()->integrity * 100);

		if (integrity < 30)        sstat = SYSTEM_STATUS::CRITICAL;
		else if (integrity < 60)   sstat = SYSTEM_STATUS::DEGRADED;

		if (sstat != ship_status) {
			ship_status = sstat;
			update = true;
		}

		if (update) {
			SetStatusColor((SYSTEM_STATUS)ship_status);
			ColorizeBitmap(icon_ship, icon_ship_shade, StatusColor);
		}
	}

	if (ship && ship->Cockpit()) {
		Solid* cockpit = (Solid*)ship->Cockpit();

		bool change = false;

		if (cockpit->Hidden()) {
			if (cockpit_hud_texture)
				change = true;

			cockpit_hud_texture = 0;
		}
		else {
			if (!cockpit_hud_texture)
				change = true;

			SimModel* cockpit_model = cockpit->GetModel();
			Material* hud_material = 0;

			if (cockpit_model) {
				hud_material = (Material*)cockpit_model->FindMaterial("HUD");
				if (hud_material) {
					cockpit_hud_texture = hud_material->tex_emissive;
				}
			}
		}

		if (change) {
			SetHUDColorSet(color);
		}
	}
}

void
HUDView::SetTarget(SimObject* t)
{
	bool update = false;

	if (target != t) {
		tgt_status = SYSTEM_STATUS::UNKNOWN;
		target = t;
		if (target) Observe(target);
		update = true;
	}

	if (target && target->Type() == SimObject::SIM_SHIP) {
		SYSTEM_STATUS sstat = SYSTEM_STATUS::NOMINAL;
		Ship* tship = (Ship*)target;
		int integrity = (int)(tship->Integrity() / tship->Design()->integrity * 100);

		if (integrity < 30)        sstat = SYSTEM_STATUS::CRITICAL;
		else if (integrity < 60)   sstat = SYSTEM_STATUS::DEGRADED;

		if (sstat != tgt_status) {
			tgt_status = sstat;
			update = true;
		}
	}

	if (update) {
		if (target && target->Type() == SimObject::SIM_SHIP) {
			Ship* tship = (Ship*)target;
			TransferBitmap(tship->Design()->hud_icon, icon_target, icon_target_shade);
		}
		else {
			PrepareBitmap("hud_icon.pcx", icon_target, icon_target_shade);
		}

		SetStatusColor((SYSTEM_STATUS)tgt_status);
		ColorizeBitmap(icon_target, icon_target_shade, StatusColor);
	}
}

// +--------------------------------------------------------------------+

MFDView*
HUDView::GetMFD(int n) const
{
	if (n >= 0 && n < 3)
		return mfd[n];

	return 0;
}

// +--------------------------------------------------------------------+

void
HUDView::Refresh()
{
	sim = Sim::GetSim();
	mouse_in = false;

	if (!sim || !camview || !projector) {
		return;
	}

	if (Mouse::LButton() == 0) {
		mouse_latch = 0;
		mouse_index = -1;
	}

	int mouse_index_old = mouse_index;

	SetShip(sim->GetPlayerShip());

	if (mode == EHUDMode::Off) {
		if (cockpit_hud_texture) {
			cockpit_hud_texture->FillRect(0, 0, 512, 256, FColor::Black);
		}

		sim->ShowGrid(false);
		return;
	}

	if (cockpit_hud_texture && cockpit_hud_texture->Width() == 512) {
		Bitmap* hud_bmp = 0;

		if (hud_sprite[0]) {
			hud_bmp = hud_sprite[0]->Frame();
			int bmp_w = hud_bmp->Width();
			int bmp_h = hud_bmp->Height();

			cockpit_hud_texture->BitBlt(0, 0, *hud_bmp, 0, 0, bmp_w, bmp_h);
		}

		if (hud_sprite[1]) {
			hud_bmp = hud_sprite[1]->Frame();
			int bmp_w = hud_bmp->Width();
			int bmp_h = hud_bmp->Height();

			cockpit_hud_texture->BitBlt(256, 0, *hud_bmp, 0, 0, bmp_w, bmp_h);
		}

		if (hud_sprite[6]) {
			if (hud_sprite[6]->Hidden()) {
				cockpit_hud_texture->FillRect(0, 384, 128, 512, FColor::Black);
			}
			else {
				hud_bmp = hud_sprite[6]->Frame();
				int bmp_w = hud_bmp->Width();
				int bmp_h = hud_bmp->Height();

				cockpit_hud_texture->BitBlt(0, 384, *hud_bmp, 0, 0, bmp_w, bmp_h);
			}
		}

		if (hud_sprite[7]) {
			if (hud_sprite[7]->Hidden()) {
				cockpit_hud_texture->FillRect(128, 384, 256, 512, FColor::Black);
			}
			else {
				hud_bmp = hud_sprite[7]->Frame();
				int bmp_w = hud_bmp->Width();
				int bmp_h = hud_bmp->Height();

				cockpit_hud_texture->BitBlt(128, 384, *hud_bmp, 0, 0, bmp_w, bmp_h);
			}
		}

		for (int i = 8; i < 32; i++) {
			if (hud_sprite[i] && !hud_sprite[i]->Hidden()) {
				Sprite* s = hud_sprite[i];

				int cx = (int)s->GetLocation().X;
				int cy = (int)s->GetLocation().Y;
				int w2 = s->Width() / 2;
				int h2 = s->Height() / 2;

				DrawBitmap(cx - w2, cy - h2, cx + w2, cy + h2, s->Frame(), Video::BLEND_ALPHA);
			}
		}
	}
	else {
		for (int i = 0; i < 32; i++) {
			if (hud_sprite[i] && !hud_sprite[i]->Hidden()) {
				Sprite* s = hud_sprite[i];

				int cx = (int)s->GetLocation().X;
				int cy = (int)s->GetLocation().Y;
				int w2 = s->Width() / 2;
				int h2 = s->Height() / 2;

				DrawBitmap(cx - w2, cy - h2, cx + w2, cy + h2, s->Frame(), Video::BLEND_ALPHA);
			}
		}

		Video* video = Video::GetInstance();

		for (int i = 0; i < 31; i++) {
			Sprite* s = pitch_ladder[i];

			if (s && !s->Hidden()) {
				s->Render2D(video);
			}
		}
	}

	//DrawStarSystem();
	DrawMessages();

	if (ship) {
		// no hud in transition:
		if (ship->InTransition()) {
			transition = true;
			HideAll();
			return;
		}

		else if (transition) {
			transition = false;
			RestoreHUD();
		}

		CameraManager* cam_mgr = CameraManager::GetInstance();

		// everything is off during docking, except the final message:
		if (cam_mgr && cam_mgr->GetMode() == CameraManager::MODE_DOCKING) {
			docking = true;
			HideAll();

			if (ship->GetFlightPhase() == Ship::DOCKING) {
				Rect dock_rect(width / 2 - 100, height / 6, 200, 20);

				if (ship->IsAirborne())
					DrawHUDText(TXT_AUTO, Game::GetText("HUDView.SUCCESSFUL-LANDING"), dock_rect, DT_CENTER);
				else
					DrawHUDText(TXT_AUTO, Game::GetText("HUDView.DOCKING-COMPLETE"), dock_rect, DT_CENTER);
			}
			return;
		}
		else if (docking) {
			docking = false;
			RestoreHUD();
		}

		// go to NAV mode during autopilot:
		if (ship->GetNavSystem() && ship->GetNavSystem()->AutoNavEngaged() && !arcade)
			mode = EHUDMode::Navigation;

		SetTarget(ship->GetTarget());

		// internal view of HUD reticule
		if (CameraManager::GetCameraMode() <= CameraManager::MODE_CHASE)
			SetTacticalMode(0);

		// external view
		else
			SetTacticalMode(!cockpit_hud_texture);

		sim->ShowGrid(tactical &&
			!ship->IsAirborne() &&
			CameraManager::GetCameraMode() != CameraManager::MODE_VIRTUAL);

		// draw HUD bars:
		DrawBars();

		if (missile_lock_sound) {
			if (threat > 1) {
				long max_vol = AudioConfig::WrnVolume();
				long volume = -1500;

				if (volume > max_vol)
					volume = max_vol;

				missile_lock_sound->SetVolume(volume);
				missile_lock_sound->Play();
			}
			else {
				missile_lock_sound->Stop();
			}
		}

		DrawNav();
		DrawILS();

		// FOR DEBUG PURPOSES ONLY:
		// DrawObjective();

		if (!overlay) {
			Rect fov_rect(0, 10, width, 10);
			int  fov_degrees = 180 - 2 * (int)(projector->XAngle() * 180 / PI);

			if (fov_degrees > 90)
				DrawHUDText(TXT_CAM_ANGLE, Game::GetText("HUDView.Wide-Angle"), fov_rect, DT_CENTER);

			fov_rect.y = 20;
			DrawHUDText(TXT_CAM_MODE, CameraManager::GetModeName(), fov_rect, DT_CENTER);
		}

		DrawMFDs();

		instr_left_sprite->Hide();
		instr_right_sprite->Hide();
		warn_left_sprite->Hide();
		warn_right_sprite->Hide();

		if (cockpit_hud_texture)
			cockpit_hud_texture->FillRect(256, 384, 512, 512, FColor::Black);

		if (show_inst) {
			DrawInstructions();
		}

		else if (!arcade) {
			if (ship->MasterCaution() && !show_warn)
				ShowHUDWarn();

			if (show_warn)
				DrawWarningPanel();
		}

		if (width > 640 || (!show_inst && !show_warn)) {
			Rect icon_rect(120, height - 24, 128, 16);

			if (ship)
				DrawHUDText(TXT_ICON_SHIP_TYPE, ship->DesignName(), icon_rect, DT_CENTER);

			icon_rect.x = width - 248;

			if (target && target->Type() == SimObject::SIM_SHIP) {
				Ship* tship = (Ship*)target;
				DrawHUDText(TXT_ICON_TARGET_TYPE, tship->DesignName(), icon_rect, DT_CENTER);
			}
		}
	}
	else {
		if (target) {
			SetTarget(0);
		}
	}

	// latch mouse down to prevent dragging into a control:
	if (Mouse::LButton() == 1)
		mouse_latch = 1;

	if (mouse_index > -1 && mouse_index_old != mouse_index)
		MouseFrame();
}

void
HUDView::DrawMFDs()
{
	for (int i = 0; i < 3; i++) {
		mfd[i]->Show();
		mfd[i]->SetShip(ship);
		mfd[i]->SetCockpitHUDTexture(cockpit_hud_texture);
		mfd[i]->Draw();
	}
}

// +--------------------------------------------------------------------+

void
HUDView::DrawStarSystem()
{
	if (sim && sim->GetStarSystem()) {
		StarSystem* sys = sim->GetStarSystem();

		ListIter<OrbitalBody> iter = sys->GetBodies();
		while (++iter) {
			OrbitalBody* body = iter.value();
			DrawOrbitalBody(body);
		}
	}
}

void
HUDView::DrawOrbitalBody(OrbitalBody* body)
{
	if (body) {
		FVector p = body->Rep()->Location();

		projector->Transform(p);

		if (p.Z > 100) {
			float r = (float)body->Radius();
			r = projector->ProjectRadius(p, r);
			projector->Project(p, false);

			DrawEllipse((int)(p.X - r),
				(int)(p.Y - r),
				(int)(p.X + r),
				(int)(p.Y + r),
				FColor::Cyan);
		}

		ListIter<OrbitalBody> iter = body->Satellites();
		while (++iter) {
			OrbitalBody* child = iter.value();
			DrawOrbitalBody(child);
		}
	}
}

// +--------------------------------------------------------------------+

void
HUDView::ExecFrame()
{
	// update the position of HUD elements that are
	// part of the 3D scene (like fpm and lcos sprites)
	HideCompass();

	if (ship && !transition && !docking && mode != EHUDMode::Off) {
		PlayerCharacter* p = PlayerCharacter::GetCurrentPlayer();
		gunsight = p->Gunsight();

		if (ship->IsStarship()) {
			if (tactical) {
				hud_left_sprite->Hide();
				hud_right_sprite->Hide();
			}

			else if (hud_left_sprite->Frame() != &hud_left_starship) {
				hud_left_sprite->SetAnimation(&hud_left_starship);
				hud_right_sprite->SetAnimation(&hud_right_starship);

				hud_left_sprite->MoveTo(FVector((float)(width / 2 - 128), (float)(height / 2), 1));
				hud_right_sprite->MoveTo(FVector((float)(width / 2 + 128), (float)(height / 2), 1));
			}
		}

		else if (!ship->IsStarship()) {
			if (ship->IsAirborne() && hud_left_sprite->Frame() != &hud_left_air) {
				hud_left_sprite->SetAnimation(&hud_left_air);
				hud_right_sprite->SetAnimation(&hud_right_air);
			}

			else if (!ship->IsAirborne() && hud_left_sprite->Frame() != &hud_left_fighter) {
				hud_left_sprite->SetAnimation(&hud_left_fighter);
				hud_right_sprite->SetAnimation(&hud_right_fighter);
			}
		}

		if (!tactical) {
			if (Game::MaxTexSize() > 128) {
				hud_left_sprite->Show();
				hud_right_sprite->Show();
			}

			if (!arcade)
				DrawFPM();

			if (ship->IsStarship() && ship->GetFLCSMode() == Ship::FLCS_HELM)
				DrawHPM();
			else if (!arcade)
				DrawPitchLadder();
		}

		else {
			if (ship->IsStarship() && ship->GetFLCSMode() == Ship::FLCS_HELM)
				DrawCompass();
		}

		if (mode == EHUDMode::Tactical) {
			DrawSight();
			DrawDesignators();
		}

		if (width > 640 || (!show_inst && !show_warn)) {
			icon_ship_sprite->Show();
			icon_target_sprite->Show();
		}
		else {
			icon_ship_sprite->Hide();
			icon_target_sprite->Hide();
		}
	}

	// if the hud is off or prohibited,
	// hide all of the sprites:
	else {
		hud_left_sprite->Hide();
		hud_right_sprite->Hide();
		instr_left_sprite->Hide();
		instr_right_sprite->Hide();
		warn_left_sprite->Hide();
		warn_right_sprite->Hide();
		icon_ship_sprite->Hide();
		icon_target_sprite->Hide();
		fpm_sprite->Hide();
		hpm_sprite->Hide();
		lead_sprite->Hide();
		aim_sprite->Hide();
		tgt1_sprite->Hide();
		tgt2_sprite->Hide();
		tgt3_sprite->Hide();
		tgt4_sprite->Hide();
		chase_sprite->Hide();

		for (int i = 0; i < 3; i++)
			mfd[i]->Hide();

		for (int i = 0; i < 31; i++)
			pitch_ladder[i]->Hide();

		DrawILS();
	}
}



// +--------------------------------------------------------------------+
// Local helpers
// +--------------------------------------------------------------------+

static inline FColor DimColor(const FColor& In, float Scalar)
{
	Scalar = FMath::Clamp(Scalar, 0.0f, 1.0f);

	const uint8 R = (uint8)FMath::Clamp(FMath::RoundToInt(In.R * Scalar), 0, 255);
	const uint8 G = (uint8)FMath::Clamp(FMath::RoundToInt(In.G * Scalar), 0, 255);
	const uint8 B = (uint8)FMath::Clamp(FMath::RoundToInt(In.B * Scalar), 0, 255);

	return FColor(R, G, B, In.A);
}

static const FColor HUD_DarkGray(64, 64, 64, 255);
static const FColor HUD_Orange(255, 165, 0, 255);

// +--------------------------------------------------------------------+

void
HUDView::CycleMFDMode(int mfd_index)
{
	if (mfd_index < 0 || mfd_index > 2) return;

	EMFDMode Mode = mfd[mfd_index]->GetMode();
	int32 M = static_cast<int32>(Mode);
	++M;
	Mode = static_cast<EMFDMode>(M);

	if (mfd_index == 2)
	{
		if (Mode > EMFDMode::SHIP)
			Mode = EMFDMode::OFF;
	}
	else
	{
		if (Mode > EMFDMode::RADAR3D)
			Mode = EMFDMode::OFF;

		if (Mode == EMFDMode::GAME)
			Mode = static_cast<EMFDMode>(static_cast<int32>(Mode) + 1);

		if (mfd_index != 0 && Mode == EMFDMode::SHIP)
			Mode = static_cast<EMFDMode>(static_cast<int32>(Mode) + 1);
	}

	mfd[mfd_index]->SetMode(Mode);
	HUDSounds::PlaySound(HUDSounds::SND_MFD_MODE);
}

// +--------------------------------------------------------------------+

void
HUDView::ShowHUDWarn()
{
	if (!show_warn) {
		show_warn = true;

		if (ship && ship->HullStrength() <= 40) {
			// TOO OBNOXIOUS!!
			HUDSounds::PlaySound(HUDSounds::SND_RED_ALERT);
		}
	}
}

void
HUDView::ShowHUDInst()
{
	show_inst = true;
}

// +--------------------------------------------------------------------+

void
HUDView::HideHUDWarn()
{
	show_warn = false;

	if (ship) {
		ship->ClearCaution();
		HUDSounds::StopSound(HUDSounds::SND_RED_ALERT);
	}
}

void
HUDView::HideHUDInst()
{
	show_inst = false;
}

// +--------------------------------------------------------------------+

void
HUDView::CycleHUDWarn()
{
	HUDSounds::PlaySound(HUDSounds::SND_HUD_WIDGET);
	show_warn = !show_warn;

	if (ship && !show_warn) {
		ship->ClearCaution();
		HUDSounds::StopSound(HUDSounds::SND_RED_ALERT);
	}
}

void
HUDView::CycleHUDInst()
{
	show_inst = !show_inst;
	HUDSounds::PlaySound(HUDSounds::SND_HUD_WIDGET);
}

// +--------------------------------------------------------------------+

void
HUDView::SetHUDMode(EHUDMode m)
{
	if (mode != m) {
		mode = m;

		if (mode > EHUDMode::ILS || mode < EHUDMode::Off)
			mode = EHUDMode::Off;

		if (ship && !ship->IsDropship() && mode == EHUDMode::ILS)
			mode = EHUDMode::Off;

		RestoreHUD();
	}
}

void HUDView::CycleHUDMode()
{
	mode = static_cast<EHUDMode>(static_cast<int>(mode) + 1);

	if (arcade && mode != EHUDMode::Tactical)
		mode = EHUDMode::Off;
	else if (mode > EHUDMode::ILS) // enum class: don’t use mode < Off
		mode = EHUDMode::Off;
	else if (ship && !ship->IsDropship() && mode == EHUDMode::ILS)
		mode = EHUDMode::Off;

	RestoreHUD();
	HUDSounds::PlaySound(HUDSounds::SND_HUD_MODE);
}

void
HUDView::RestoreHUD()
{
	if (mode == EHUDMode::Off) {
		HideAll();
	}
	else {
		for (int i = 0; i < 3; i++)
			mfd[i]->Show();

		if (width > 640 || (!show_inst && !show_warn)) {
			icon_ship_sprite->Show();
			icon_target_sprite->Show();
		}
		else {
			icon_ship_sprite->Hide();
			icon_target_sprite->Hide();
		}

		if (!tactical && Game::MaxTexSize() > 128) {
			hud_left_sprite->Show();
			hud_right_sprite->Show();
		}

		fpm_sprite->Show();

		if (ship && ship->IsStarship())
			hpm_sprite->Show();

		if (gunsight == 0)
			aim_sprite->Show();
		else
			lead_sprite->Show();
	}
}

void
HUDView::HideAll()
{
	for (int i = 0; i < 3; i++)
		mfd[i]->Hide();

	hud_left_sprite->Hide();
	hud_right_sprite->Hide();
	instr_left_sprite->Hide();
	instr_right_sprite->Hide();
	warn_left_sprite->Hide();
	warn_right_sprite->Hide();
	icon_ship_sprite->Hide();
	icon_target_sprite->Hide();
	fpm_sprite->Hide();
	hpm_sprite->Hide();
	lead_sprite->Hide();
	aim_sprite->Hide();
	tgt1_sprite->Hide();
	tgt2_sprite->Hide();
	tgt3_sprite->Hide();
	tgt4_sprite->Hide();
	chase_sprite->Hide();

	sim->ShowGrid(false);

	for (int i = 0; i < 31; i++)
		pitch_ladder[i]->Hide();

	if (missile_lock_sound)
		missile_lock_sound->Stop();

	HideCompass();
	DrawILS();
	Mouse::Show(false);
}

// +--------------------------------------------------------------------+

FColor
HUDView::Ambient() const
{
	if (!sim || !ship || mode == EHUDMode::Off)
		return FColor::Black;

	SimRegion* rgn = sim->GetActiveRegion();

	if (!rgn || !rgn->IsAirSpace())
		return FColor::Black;

	// Legacy: Color c = sim->GetStarSystem()->Ambient();
	// Now: assume StarSystem ambient returns FColor (or adapt at call site).
	FColor c = sim->GetStarSystem()->GetAmbient();

	if (c.R > 32 || c.G > 32 || c.B > 32)
		return FColor::Black;

	// if we get this far, the night-vision aid is on
	return night_vision_colors[color];
}

FColor
HUDView::CycleHUDColor()
{
	HUDSounds::PlaySound(HUDSounds::SND_HUD_MODE);
	SetHUDColorSet(color + 1);
	return HudColor;
}

void
HUDView::SetHUDColorSet(int c)
{
	color = c;
	if (color > NUM_HUD_COLORS - 1) color = 0;

	HudColor = standard_hud_colors[color];
	TextColor = standard_txt_colors[color];

	ColorizeBitmap(fpm, fpm_shade, HudColor, true);
	ColorizeBitmap(hpm, hpm_shade, HudColor, true);
	ColorizeBitmap(lead, lead_shade, DimColor(TextColor, 1.0f), true); // legacy txt_color * 1.25
	ColorizeBitmap(cross, cross_shade, HudColor, true);
	ColorizeBitmap(cross1, cross1_shade, HudColor, true);
	ColorizeBitmap(cross2, cross2_shade, HudColor, true);
	ColorizeBitmap(cross3, cross3_shade, HudColor, true);
	ColorizeBitmap(cross4, cross4_shade, HudColor, true);

	if (Game::MaxTexSize() > 128) {
		ColorizeBitmap(hud_left_air, hud_left_shade_air, HudColor);
		ColorizeBitmap(hud_right_air, hud_right_shade_air, HudColor);
		ColorizeBitmap(hud_left_fighter, hud_left_shade_fighter, HudColor);
		ColorizeBitmap(hud_right_fighter, hud_right_shade_fighter, HudColor);
		ColorizeBitmap(hud_left_starship, hud_left_shade_starship, HudColor);
		ColorizeBitmap(hud_right_starship, hud_right_shade_starship, HudColor);

		ColorizeBitmap(instr_left, instr_left_shade, HudColor);
		ColorizeBitmap(instr_right, instr_right_shade, HudColor);
		ColorizeBitmap(warn_left, warn_left_shade, HudColor);
		ColorizeBitmap(warn_right, warn_right_shade, HudColor);

		ColorizeBitmap(pitch_ladder_pos, pitch_ladder_pos_shade, HudColor);
		ColorizeBitmap(pitch_ladder_neg, pitch_ladder_neg_shade, HudColor);
	}

	ColorizeBitmap(icon_ship, icon_ship_shade, TextColor);
	ColorizeBitmap(icon_target, icon_target_shade, TextColor);

	ColorizeBitmap(chase_left, chase_left_shade, HudColor, true);
	ColorizeBitmap(chase_right, chase_right_shade, HudColor, true);
	ColorizeBitmap(chase_top, chase_top_shade, HudColor, true);
	ColorizeBitmap(chase_bottom, chase_bottom_shade, HudColor, true);

	Hoop::SetColor(HudColor);

	for (int i = 0; i < 3; i++) {
		mfd[i]->SetText3DColor(TextColor);
		mfd[i]->SetHUDColor(HudColor);
	}

	FSlateFontInfo HudFontInfo;
	SystemFont* ffont = FontManager::Find("HUD");

	if (!FontManager::Find("HUD", HudFontInfo))
	{
		HudFontInfo.Size = 18; // fallback
	}
	if (ffont)
		ffont->SetColor(TextColor);

	for (int i = 0; i < TXT_LAST; i++)
		hud_text[i].color = TextColor;
}

// +--------------------------------------------------------------------+

void
HUDView::Message(const char* fmt, ...)
{
	if (!fmt)
		return;

	char msg[512] = { 0 };

	va_list args;
	va_start(args, fmt);
#if defined(_MSC_VER)
	vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
#else
	vsnprintf(msg, sizeof(msg), fmt, args);
#endif
	va_end(args);

	if (char* newline = strchr(msg, '\n'))
		*newline = 0;

	// Legacy Print -> UE_LOG
	UE_LOG(LogTemp, Log, TEXT("%s"), UTF8_TO_TCHAR(msg));

	if (hud_view) {
		int index = -1;

		for (int i = 0; i < MAX_MSG; i++) {
			if (hud_view->msg_time[i] <= 0) {
				index = i;
				break;
			}
		}

		// no space; advance pipeline:
		if (index < 0) {
			for (int i = 0; i < MAX_MSG - 1; i++) {
				hud_view->msg_text[i] = hud_view->msg_text[i + 1];
				hud_view->msg_time[i] = hud_view->msg_time[i + 1];
			}

			index = MAX_MSG - 1;
		}

		hud_view->msg_text[index] = msg;
		hud_view->msg_time[index] = 10;
	}
}

// +--------------------------------------------------------------------+

void
HUDView::ClearMessages()
{
	if (hud_view) {
		for (int i = 0; i < MAX_MSG - 1; i++) {
			hud_view->msg_text[i] = Text();
			hud_view->msg_time[i] = 0;
		}
	}
}

// +--------------------------------------------------------------------+

void
HUDView::PrepareBitmap(const char* name, Bitmap& img, uint8*& shades)
{
	delete[] shades;
	shades = nullptr;

	DataLoader* loader = DataLoader::GetLoader();

	loader->SetDataPath("HUD/");
	int loaded = loader->LoadGameBitmap(name, img, Bitmap::BMP_TRANSPARENT);
	loader->SetDataPath(nullptr);

	if (!loaded)
		return;

	const int w = img.Width();
	const int h = img.Height();

	shades = new uint8[w * h];

	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			shades[y * w + x] = (uint8)(img.GetColor(x, y).R * 0.66f);
}

void
HUDView::TransferBitmap(const Bitmap& src, Bitmap& img, uint8*& shades)
{
	delete[] shades;
	shades = nullptr;

	if (src.Width() != img.Width() || src.Height() != img.Height())
		return;

	img.CopyBitmap(src);
	img.SetType(Bitmap::BMP_TRANSLUCENT);

	const int w = img.Width();
	const int h = img.Height();

	shades = new uint8[w * h];

	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			shades[y * w + x] = (uint8)(img.GetColor(x, y).R * 0.5f);
}

void
HUDView::ColorizeBitmap(Bitmap& img, uint8* shades, FColor color, bool force_alpha)
{
	if (!shades) return;

	const int max_tex_size = Game::MaxTexSize();

	if (max_tex_size < 128)
		Game::SetMaxTexSize(128);

	if (hud_view && hud_view->cockpit_hud_texture && !force_alpha) {
		img.FillColor(FColor::Black);
		FColor* dst = img.HiPixels();
		uint8* src = shades;

		for (int y = 0; y < img.Height(); y++) {
			for (int x = 0; x < img.Width(); x++) {
				if (*src)
					*dst = DimColor(color, (*src) / 200.0f);
				else
					*dst = FColor::Black;

				dst++;
				src++;
			}
		}
		img.MakeTexture();
	}
	else {
		img.FillColor(color);
		img.CopyAlphaImage(img.Width(), img.Height(), shades);
		img.MakeTexture();
	}

	if (max_tex_size < 128)
		Game::SetMaxTexSize(max_tex_size);
}

// +--------------------------------------------------------------------+

void
HUDView::MouseFrame()
{
	MouseController* ctrl = MouseController::GetInstance();
	if (ctrl && ctrl->Active())
		return;

	if (mouse_index >= TXT_CAUTION_TXT && mouse_index <= TXT_LAST_CAUTION) {
		if (show_inst) {
			if (mouse_index == TXT_INSTR_PAGE) {
				if (Mouse::X() > width / 2 + 125)
					CycleInstructions(1);
				else if (Mouse::X() < width / 2 + 65)
					CycleInstructions(-1);
			}
			else {
				show_inst = false;
			}
		}
		else {
			CycleHUDWarn();
		}
		return;
	}

	Starshatter* stars = Starshatter::GetInstance();
	if (mouse_index == TXT_PAUSED)
		stars->Pause(!Game::Paused());

	if (mouse_index == TXT_GEAR_DOWN)
		ship->ToggleGear();

	if (mouse_index == TXT_HUD_MODE) {
		CycleHUDMode();

		if (mode == EHUDMode::Off)
			CycleHUDMode();
	}

	if (mouse_index == TXT_PRIMARY_WEP) {
		HUDSounds::PlaySound(HUDSounds::SND_WEP_MODE);
		ship->CyclePrimary();
	}

	if (mouse_index == TXT_SECONDARY_WEP) {
		HUDSounds::PlaySound(HUDSounds::SND_WEP_MODE);
		ship->CycleSecondary();
	}

	if (mouse_index == TXT_DECOY)
		ship->FireDecoy();

	if (mouse_index == TXT_SHIELD) {
		Shield* shield = ship->GetShield();

		if (shield) {
			double level = shield->GetPowerLevel();

			const Rect& r = hud_text[TXT_SHIELD].rect;
			if (Mouse::X() < r.x + r.w * 0.75)
				shield->SetPowerLevel(level - 10);
			else
				shield->SetPowerLevel(level + 10);

			HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
		}
	}

	if (mouse_index == TXT_AUTO)
		ship->TimeSkip();

	if (mouse_index >= TXT_NAV_INDEX && mouse_index <= TXT_NAV_ETR) {
		ship->SetAutoNav(!ship->IsAutoNavEngaged());
		SetHUDMode(EHUDMode::Tactical);
	}
}

// +--------------------------------------------------------------------+

bool
HUDView::IsMouseLatched()
{
	bool result = mouse_in;

	if (!result) {
		HUDView* hud = HUDView::GetInstance();

		for (int i = 0; i < 3; i++)
			result = result || hud->mfd[i]->IsMouseLatched();
	}

	return result;
}

// +--------------------------------------------------------------------+

bool
HUDView::IsNameCrowded(int x, int y)
{
	for (int i = 0; i < MAX_CONTACT; i++) {
		HUDText& test = hud_text[TXT_CONTACT_NAME + i];

		if (!test.hidden) {
			Rect r = test.rect;

			int dx = r.x - x;
			int dy = r.y - y;
			int d = dx * dx + dy * dy;

			if (d <= 400)
				return true;
		}

		test = hud_text[TXT_CONTACT_INFO + i];

		if (!test.hidden) {
			Rect r = test.rect;

			int dx = r.x - x;
			int dy = r.y - y;
			int d = dx * dx + dy * dy;

			if (d <= 400)
				return true;
		}
	}

	return false;
}

void
HUDView::DrawDiamond(int x, int y, int r, FColor c)
{
	// Avoid Windows POINT; keep a tiny local POD that matches legacy usage.
	struct SPolyPoint { int x; int y; };
	SPolyPoint diamond[4];

	diamond[0].x = x;
	diamond[0].y = y - r;

	diamond[1].x = x + r;
	diamond[1].y = y;

	diamond[2].x = x;
	diamond[2].y = y + r;

	diamond[3].x = x - r;
	diamond[3].y = y;

	DrawPoly(4, (FVector*)diamond, c);
}

void HUDView::DrawBars()
{
	// Minimal tactical HUD text bars (speed, heading, paused)
	if (!ship)
		return;

	char txt[128] = { 0 };

	// SPEED (left)
	{
		const double speed = ship->Velocity().Size();
		FormatNumber(txt, speed);

		Rect r(20, height / 2 - 6, 120, 12);
		if (tactical) r.y = height - 40;
		DrawHUDText(TXT_SPEED, txt, r, DT_LEFT);
	}

	// HEADING (center top) — no Ship::Orientation() available
	{
		// Derive heading from velocity direction (XY plane).
		// If nearly stopped, heading is undefined; show 000.
		const FVector v = ship->Velocity();
		const double vx = (double)v.X;
		const double vy = (double)v.Y;

		int headingDeg = 0;

		const double v2 = vx * vx + vy * vy;
		if (v2 > 1e-4) // moving enough to define a direction
		{
			const double headingRad = FMath::Atan2(vy, vx);
			headingDeg = (int)FMath::RoundToInt((float)FMath::RadiansToDegrees((float)headingRad));
			if (headingDeg < 0) headingDeg += 360;
		}

#if defined(_MSC_VER)
		sprintf_s(txt, "HDG %03d", headingDeg);
#else
		snprintf(txt, sizeof(txt), "HDG %03d", headingDeg);
#endif

		Rect r(0, 10, width, 12);
		if (tactical) r.y = 8;
		DrawHUDText(TXT_HEADING, txt, r, DT_CENTER);
	}

	// PAUSED indicator
	if (Game::Paused())
	{
		Rect r(0, height / 6, width, 20);
		DrawHUDText(TXT_PAUSED, Game::GetText("HUDView.PAUSED"), r, DT_CENTER);
	}
}

void HUDView::DrawContact(SimContact* c, int index)
{
	if (!c || !ship || !projector)
		return;

	// Contact position in world -> view -> screen
	FVector p = c->Location();
	projector->Transform(p);

	// Clip to front of camera
	if (p.Z <= 1.0f)
		return;

	projector->Project(p);

	const int x = (int)p.X;
	const int y = (int)p.Y;

	// Screen clip
	if (x <= 4 || x >= width - 4 || y <= 4 || y >= height - 4)
		return;

	const FColor mc = MarkerColor(c);

	// Marker
	DrawDiamond(x, y, 4, mc);

	// Name/Info (avoid crowding)
	if (tactical && index < MAX_CONTACT && !IsNameCrowded(x, y))
	{
		// ---------------------------
		// NAME (SimContact has no Name())
		// Prefer the underlying ship name; fall back to shot; then generic.
		// ---------------------------
		const char* contact_name = "CONTACT";

		if (Ship* s = c->GetShip())
		{
			contact_name = s->Name(); // assumes legacy const char*
		}
		else if (c->GetShot())
		{
			contact_name = "INCOMING";
		}
		// NOTE: Remove c->GetObject() unless your SimContact actually has it.
		// else if (SimObject* o = c->GetObject()) { contact_name = o->Name(); }

		Rect name_rect(x + 10, y - 4, 200, 12);
		DrawHUDText(TXT_CONTACT_NAME + index, contact_name, name_rect, DT_LEFT, HUD_MIXED_CASE);

		// ---------------------------
		// INFO (range)
		// ---------------------------
		char info[128] = { 0 };

		double limit = 75e3;
		if (ship && ship->GetSensor())
			limit = ship->GetSensor()->GetBeamRange();

		const double rng = c->Range(ship, limit);
		FormatNumber(info, rng);

		Rect info_rect(x + 10, y + 6, 200, 12);
		DrawHUDText(TXT_CONTACT_INFO + index, info, info_rect, DT_LEFT, HUD_MIXED_CASE);
	}

	// Threat counting (for missile lock sound logic)
	if (c->GetShot() && c->Threat(ship))
		++threat;
}

void HUDView::DrawTrack(SimContact* c)
{
	if (!c || !projector)
		return;

	// Track ladder: draw the contact's stored track points, fading by age.
	const int ctl = c->TrackLength();
	if (ctl <= 0)
		return;

	FColor base = MarkerColor(c);

	FVector t1 = c->TrackPoint(0);
	FVector t0 = c->Location();

	// Segment from current to first track point:
	if (t0 != t1)
		DrawTrackSegment(t0, t1, base);

	for (int i = 0; i < ctl - 1; i++)
	{
		FVector a = c->TrackPoint(i);
		FVector b = c->TrackPoint(i + 1);
		if (a == b)
			continue;

		const float fade = (float)((double)(ctl - i) / (double)ctl);
		DrawTrackSegment(a, b, ColorMul(base, fade));
	}
}

void HUDView::DrawTrackSegment(FVector& t1, FVector& t2, FColor c)
{
	if (!projector)
		return;

	FVector a = t1;
	FVector b = t2;

	projector->Transform(a);
	projector->Transform(b);

	// Both behind camera
	if (a.Z <= 1.0f && b.Z <= 1.0f)
		return;

	// Project what we can; if one is behind, still try to draw a short hint line.
	if (a.Z > 1.0f) projector->Project(a);
	if (b.Z > 1.0f) projector->Project(b);

	const int ax = (int)a.X;
	const int ay = (int)a.Y;
	const int bx = (int)b.X;
	const int by = (int)b.Y;

	// Simple bounds reject
	if ((ax < -1000 || ax > width + 1000) && (bx < -1000 || bx > width + 1000))
		return;
	if ((ay < -1000 || ay > height + 1000) && (by < -1000 || by > height + 1000))
		return;

	DrawLine(ax, ay, bx, by, c);
}

void HUDView::DrawRect(SimObject* targ)
{
	if (!targ || !targ->Rep())
		return;

	Graphic* g = targ->Rep();
	Rect r = g->ScreenRect();

	// Clamp and outline
	if (r.w <= 0 || r.h <= 0)
		return;

	DrawRect(r, HudColor);
}

void HUDView::DrawLCOS(SimObject* targ, double dist)
{
	// LCOS: Lead-Computing Optical Sight (simplified).
	// Legacy computed ballistic lead based on weapon/projectile speed, target motion, etc.
	// Here we do a safe fallback: aim at the projected target (or subtarget mount) and use aim/lead sprite.

	if (!ship || !targ || !projector)
		return;

	FVector p = targ->Location();
	projector->Transform(p);

	if (p.Z <= 1.0f)
		return;

	projector->Project(p);

	const double x = p.X;
	const double y = p.Y;

	if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1)
		return;

	// Choose sprite based on gunsight selection:
	Sprite* s = (gunsight == 0) ? aim_sprite : lead_sprite;
	if (!s)
		return;

	s->Show();
	s->MoveTo(FVector((float)x, (float)y, 1));

	// If you want a little “range cue”, draw a tiny box:
	if (!arcade)
	{
		const int ix = (int)x;
		const int iy = (int)y;
		DrawRect(ix - 3, iy - 3, ix + 3, iy + 3, HudColor);
	}
}

void HUDView::DrawFPM()
{
	// Flight Path Marker (simplified):
	if (!ship || !projector || !fpm_sprite)
		return;

	// Best-effort: use velocity vector projected from ship origin.
	FVector p = ship->Location() + ship->Velocity().GetSafeNormal() * 500.0f;
	projector->Transform(p);

	if (p.Z <= 1.0f)
	{
		// fallback to center
		fpm_sprite->Show();
		fpm_sprite->MoveTo(FVector((float)xcenter, (float)ycenter, 1));
		return;
	}

	projector->Project(p);

	fpm_sprite->Show();
	fpm_sprite->MoveTo(FVector((float)p.X, (float)p.Y, 1));
}

void HUDView::DrawHPM()
{
	// Helm Path Marker (simplified, no Ship::Orientation()):
	if (!ship || !projector || !hpm_sprite)
		return;

	// Use velocity direction as a forward proxy:
	const FVector v = ship->Velocity();

	// If we're basically not moving, just center it:
	if (v.SizeSquared() < 1e-4f)
	{
		hpm_sprite->Show();
		hpm_sprite->MoveTo(FVector((float)xcenter, (float)ycenter, 1));
		return;
	}

	const FVector dir = v.GetSafeNormal();

	// Project a point ahead of the ship along motion vector:
	FVector p = ship->Location() + dir * 500.0f;

	projector->Transform(p);

	// If behind/too close to camera plane, center it:
	if (p.Z <= 1.0f)
	{
		hpm_sprite->Show();
		hpm_sprite->MoveTo(FVector((float)xcenter, (float)ycenter, 1));
		return;
	}

	projector->Project(p);

	hpm_sprite->Show();
	hpm_sprite->MoveTo(FVector((float)p.X, (float)p.Y, 1));
}

void HUDView::DrawCompass()
{
	// Minimal 2D compass overlay (compile-safe)
	if (!ship)
		return;

	// Compass center (top-middle of screen)
	const int cx = width / 2;
	const int cy = 50;

	// Draw compass ring
	DrawEllipse(cx - 28, cy - 28, cx + 28, cy + 28, HudColor);

	// Use velocity as heading proxy
	const FVector v = ship->Velocity();

	// If nearly stationary, draw centered marker
	if (v.SizeSquared() < 1e-4f)
	{
		DrawLine(cx, cy - 18, cx, cy + 18, HudColor);
		return;
	}

	const FVector dir = v.GetSafeNormal();

	// Heading angle in screen space
	const float heading = FMath::Atan2(dir.Y, dir.X);

	const float dx = FMath::Cos(heading) * 22.0f;
	const float dy = FMath::Sin(heading) * 22.0f;

	DrawLine(cx, cy, cx + (int)dx, cy + (int)dy, HudColor);

	// Heading label
	char txt[32] = {};
	int headingDeg = (int)FMath::RoundToInt(FMath::RadiansToDegrees(heading));
	if (headingDeg < 0)
		headingDeg += 360;

	snprintf(txt, sizeof(txt), "%03d", headingDeg);

	Rect r(cx - 30, cy + 30, 60, 12);
	DrawHUDText(TXT_COMPASS, txt, r, DT_CENTER);
}

void HUDView::HideCompass()
{
	// If you later wire az_ring/el_ring into the scene, detach/hide here.
	// For now, compass is drawn 2D so nothing to do.
}

void HUDView::DrawPitchLadder()
{
	// Minimal pitch ladder (compile-safe fallback)
	// Hide all ladder sprites, then show the zero mark near center.
	for (int i = 0; i < 31; i++)
	{
		if (pitch_ladder[i])
			pitch_ladder[i]->Hide();
	}

	if (!ship)
		return;

	// Fallback pitch source: velocity direction
	const FVector v = ship->Velocity();

	// If basically stationary, just show zero rung centered:
	if (v.SizeSquared() < 1e-4f)
	{
		const int idxZero = 15;
		if (pitch_ladder[idxZero])
		{
			pitch_ladder[idxZero]->Show();
			pitch_ladder[idxZero]->MoveTo(FVector((float)xcenter, (float)ycenter, 1));
		}
		return;
	}

	const FVector dir = v.GetSafeNormal();

	// Pitch from direction Z component:
	const float pitchRad = FMath::Asin(FMath::Clamp(dir.Z, -1.0f, 1.0f));
	const float pitchDeg = FMath::RadiansToDegrees(pitchRad);

	// Map +-90 deg to +-120 px (screen Y is down, so invert sign)
	const float yOff = FMath::Clamp(-pitchDeg / 90.0f, -1.0f, 1.0f) * 120.0f;

	// Show “zero” rung (index 15) at offset center.
	const int idxZero = 15;
	if (pitch_ladder[idxZero])
	{
		pitch_ladder[idxZero]->Show();
		pitch_ladder[idxZero]->MoveTo(FVector((float)xcenter, (float)(ycenter + yOff), 1));
	}
}


void HUDView::DrawLine(int x1, int y1, int x2, int y2, FColor c)
{
	// Calls the View::DrawLine you imported with 'using View::DrawLine;'
	View::DrawLine(x1, y1, x2, y2, c);
}

void HUDView::FillRect(int x1, int y1, int x2, int y2, FColor c)
{
	// Calls the View::FillRect you imported with 'using View::FillRect;'
	View::FillRect(x1, y1, x2, y2, c);
}

void HUDView::DrawRect(int x1, int y1, int x2, int y2, FColor c)
{
	// Calls the View::DrawRect you imported with 'using View::DrawRect;'
	View::DrawRect(x1, y1, x2, y2, c);
}

void HUDView::DrawEllipse(int x1, int y1, int x2, int y2, FColor c)
{
	// Calls the View::DrawEllipse you imported with 'using View::DrawEllipse;'
	View::DrawEllipse(x1, y1, x2, y2, c);
}


void HUDView::DrawRect(const Rect& r, const FColor& c)
{
	const int x1 = r.x;
	const int y1 = r.y;
	const int x2 = r.x + r.w;
	const int y2 = r.y + r.h;

	// Outline rectangle using your existing DrawLine
	DrawLine(x1, y1, x2, y1, c); // top
	DrawLine(x2, y1, x2, y2, c); // right
	DrawLine(x2, y2, x1, y2, c); // bottom
	DrawLine(x1, y2, x1, y1, c); // left
}

void HUDView::DrawRect(const Rect& r, const FLinearColor& c)
{
	DrawRect(r, c.ToFColor(true));
}


void HUDView::DrawLine(const FVector2D& A, const FVector2D& B, const FLinearColor& C, float Thickness) const
{
	if (!paint_ctx || !paint_geo) return;

	TArray<FVector2D> Pts;
	Pts.Add(A);
	Pts.Add(B);

	FSlateDrawElement::MakeLines(
		paint_ctx->OutDrawElements,
		paint_layer,
		paint_geo->ToPaintGeometry(),
		Pts,
		ESlateDrawEffect::None,
		C,
		true,
		Thickness
	);
}

void HUDView::DrawRectOutline(const FSlateRect& R, const FLinearColor& C, float Thickness) const
{
	DrawLine(FVector2D(R.Left, R.Top), FVector2D(R.Right, R.Top), C, Thickness);
	DrawLine(FVector2D(R.Right, R.Top), FVector2D(R.Right, R.Bottom), C, Thickness);
	DrawLine(FVector2D(R.Right, R.Bottom), FVector2D(R.Left, R.Bottom), C, Thickness);
	DrawLine(FVector2D(R.Left, R.Bottom), FVector2D(R.Left, R.Top), C, Thickness);
}

void HUDView::FillRect(const FSlateRect& R, const FLinearColor& C) const
{
	if (!paint_ctx || !paint_geo) return;

	const FVector2D Pos(R.Left, R.Top);
	const FVector2D Size(R.Right - R.Left, R.Bottom - R.Top);

	FSlateDrawElement::MakeBox(
		paint_ctx->OutDrawElements,
		paint_layer,
		paint_geo->ToPaintGeometry(Pos, Size),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		C
	);
}

void HUDView::DrawEllipseOutline(const FVector2D& Center, float Rx, float Ry, const FLinearColor& C, float Thickness, int32 Segments) const
{
	if (!paint_ctx || !paint_geo) return;
	if (Segments < 8) Segments = 8;

	TArray<FVector2D> Pts;
	Pts.Reserve(Segments + 1);

	for (int32 i = 0; i <= Segments; i++)
	{
		const float t = (2.0f * PI) * ((float)i / (float)Segments);
		Pts.Add(Center + FVector2D(FMath::Cos(t) * Rx, FMath::Sin(t) * Ry));
	}

	FSlateDrawElement::MakeLines(
		paint_ctx->OutDrawElements,
		paint_layer,
		paint_geo->ToPaintGeometry(),
		Pts,
		ESlateDrawEffect::None,
		C,
		true,
		Thickness
	);
}

void HUDView::DrawTextAt(const FVector2D& Pos, const FString& Text, const FSlateFontInfo& FontInfo, const FLinearColor& C) const
{
	if (!paint_ctx || !paint_geo) return;

	FSlateDrawElement::MakeText(
		paint_ctx->OutDrawElements,
		paint_layer,
		paint_geo->ToPaintGeometry(Pos, FVector2D(1, 1)),
		Text,
		FontInfo,
		ESlateDrawEffect::None,
		C
	);
}

FLinearColor HUDView::ToLinear(const Color& C)
{
	// Legacy Color likely 0..255 channels
	return FLinearColor((float)C.Red() / 255.0f, (float)C.Green() / 255.0f, (float)C.Blue() / 255.0f, 1.0f);
}

FSlateRect HUDView::ToSlateRect(const Rect& R)
{
	return FSlateRect((float)R.x, (float)R.y, (float)(R.x + R.w), (float)(R.y + R.h));
}

