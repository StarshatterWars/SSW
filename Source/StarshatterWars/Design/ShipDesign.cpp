/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShipDesign.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR / STUDIO
	========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Starship Design parameters class
*/

#include "ShipDesign.h"
#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Engine/Texture2D.h"

#include "Ship.h"
#include "SimShot.h"
#include "Power.h"
#include "HardPoint.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "Shield.h"
#include "Sensor.h"
#include "NavLight.h"
#include "NavSystem.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Farcaster.h"
#include "Thruster.h"
#include "FlightDeck.h"
#include "LandingGear.h"
#include "Computer.h"
#include "SystemDesign.h"
#include "SimComponent.h"

#include "Game.h"
#include "Solid.h"
#include "SimModel.h"
#include "Skin.h"
#include "Sprite.h"
#include "SimLight.h"
#include "Sound.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "List.h"
#include "Text.h"
#include "GameStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipDesign, Log, All);

// +--------------------------------------------------------------------+

const char* ship_design_class_name[32] = {
	"Drone",          "Fighter",
	"Attack",         "LCA",
	"Courier",        "Cargo",
	"Corvette",       "Freighter",

	"Frigate",        "Destroyer",
	"Cruiser",        "Battleship",
	"Carrier",        "Dreadnaught",

	"Station",        "Farcaster",

	"Mine",           "DEFSAT",
	"COMSAT",         "SWACS",

	"Building",       "Factory",
	"SAM",            "EWR",
	"C3I",            "Starbase",

	"0x04000000",     "0x08000000",
	"0x10000000",     "0x20000000",
	"0x40000000",     "0x80000000"
};

// +--------------------------------------------------------------------+

static bool          degrees = false;

struct ShipCatalogEntry {
	static const char* TYPENAME() { return "ShipCatalogEntry"; }

	ShipCatalogEntry() : hide(false), design(0) {}

	ShipCatalogEntry(const char* n, const char* t, const char* p, const char* f, bool h = false) :
		name(n), type(t), path(p), file(f), hide(h), design(0) {
	}

	~ShipCatalogEntry() { delete design; }

	Text        name;
	Text        type;
	Text        path;
	Text        file;
	bool        hide;

	ShipDesign* design;
};

static List<ShipCatalogEntry> catalog;
static List<ShipCatalogEntry> mod_catalog;

// +--------------------------------------------------------------------+

#define GET_DEF_BOOL(n) if (defname==(#n)) GetDefBool((n),   def, filename)
#define GET_DEF_TEXT(n) if (defname==(#n)) GetDefText((n),   def, filename)
#define GET_DEF_NUM(n)  if (defname==(#n)) GetDefNumber((n), def, filename)
#define GET_DEF_VEC(n)  if (defname==(#n)) GetDefVec((n),    def, filename)

static char        cockpit_name[80];
static List<Text>  detail[4];
static List<FVector> offset[4];

static char errmsg[256];

// +--------------------------------------------------------------------+

ShipLoad::ShipLoad()
{
	mass = 0;
}

ShipSquadron::ShipSquadron()
{
	name[0] = 0;
	design = 0;
	count = 4;
	avail = 4;
}

static void PrepareModel(SimModel& model)
{
	bool uses_bumps = false;

	ListIter<Material> iter = model.GetMaterials();
	while (++iter && !uses_bumps) {
		Material* mtl = iter.value();
		if (mtl->tex_bumpmap != 0 && mtl->bump != 0)
			uses_bumps = true;
	}

	if (uses_bumps)
		model.ComputeTangents();
}

// +--------------------------------------------------------------------+

ShipDesign::ShipDesign()
	: sensor(0), navsys(0), shield(0), type(0), decoy(0),
	probe(0), gear(0), valid(false), secret(false), auto_roll(1), cockpit_model(0),
	bolt_hit_sound_resource(0), beam_hit_sound_resource(0), lod_levels(0)
{
	for (int i = 0; i < 4; i++)
		feature_size[i] = 0.0f;
}

// +--------------------------------------------------------------------+

ShipDesign::ShipDesign(const char* n, const char* p, const char* fname, bool s)
	: sensor(0), navsys(0), shield(0), type(0),
	quantum_drive(0), farcaster(0), thruster(0), shield_model(0), decoy(0),
	probe(0), gear(0), valid(false), secret(s), auto_roll(1), cockpit_model(0),
	bolt_hit_sound_resource(0), beam_hit_sound_resource(0), lod_levels(0)
{
	strcpy_s(name, n);

	if (!strstr(fname, ".def"))
		sprintf_s(filename, "%s.def", fname);
	else
		strcpy_s(filename, fname);

	for (int i = 0; i < 4; i++)
		feature_size[i] = 0.0f;

	scale = 1.0f;

	agility = 2e2f;
	air_factor = 0.1f;
	vlimit = 8e3f;
	drag = 2.5e-5f;
	arcade_drag = 1.0f;
	roll_drag = 5.0f;
	pitch_drag = 5.0f;
	yaw_drag = 5.0f;

	roll_rate = 0.0f;
	pitch_rate = 0.0f;
	yaw_rate = 0.0f;

	trans_x = 0.0f;
	trans_y = 0.0f;
	trans_z = 0.0f;

	turn_bank = (float)(PI / 8);

	CL = 0.0f;
	CD = 0.0f;
	stall = 0.0f;

	prep_time = 30.0f;
	avoid_time = 0.0f;
	avoid_fighter = 0.0f;
	avoid_strike = 0.0f;
	avoid_target = 0.0f;
	commit_range = 0.0f;

	splash_radius = -1.0f;
	scuttle = 5e3f;
	repair_speed = 1.0f;
	repair_teams = 2;
	repair_auto = true;
	repair_screen = true;
	wep_screen = true;

	chase_vec = FVector(0.f, -100.f, 20.f);
	bridge_vec = FVector(0.f, 0.f, 0.f);
	beauty_cam = FVector(0.f, 0.f, 0.f);
	cockpit_scale = 1.0f;

	radius = 1.0f;
	integrity = 500.0f;

	primary = 0;
	secondary = 1;
	main_drive = -1;

	pcs = 3.0f;
	acs = 1.0f;
	detet = 250.0e3f;
	e_factor[0] = 0.1f;
	e_factor[1] = 0.3f;
	e_factor[2] = 1.0f;

	explosion_scale = 0.0f;
	death_spiral_time = 3.0f;

	if (!secret) {
		UE_LOG(LogShipDesign, Log, TEXT("Loading ShipDesign '%s'"), ANSI_TO_TCHAR(name));
	}

	strcpy_s(path_name, p);
	if (path_name[strlen(path_name) - 1] != '/')
		strcat_s(path_name, "/");

	// Load Design File:
	DataLoader* Loader = DataLoader::GetLoader();
	Loader->SetDataPath(path_name);

	BYTE* Block = 0;
	int BlockLen = Loader->LoadBuffer(filename, Block, true);

	// file not found:
	if (BlockLen <= 4) {
		valid = false;
		if (Block) Loader->ReleaseBuffer(Block);
		return;
	}

	Parser ParserInst(new BlockReader((const char*)Block, BlockLen));
	Term* TermInst = ParserInst.ParseTerm();

	if (!TermInst) {
		UE_LOG(LogShipDesign, Error, TEXT("ERROR: could not parse '%s'"), ANSI_TO_TCHAR(filename));
		valid = false;
		Loader->ReleaseBuffer(Block);
		return;
	}
	else {
		TermText* FileType = TermInst->isText();
		if (!FileType || FileType->value() != "SHIP") {
			UE_LOG(LogShipDesign, Error, TEXT("ERROR: invalid ship design file '%s'"), ANSI_TO_TCHAR(filename));
			valid = false;
			delete TermInst;
			Loader->ReleaseBuffer(Block);
			return;
		}
	}

	cockpit_name[0] = 0;
	valid = true;
	degrees = false;

	do {
		delete TermInst;
		TermInst = ParserInst.ParseTerm();

		if (TermInst) {
			TermDef* Def = TermInst->isDef();
			if (Def) {
				ParseShip(Def);
			}
			else {
				UE_LOG(LogShipDesign, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
				TermInst->print();
			}
		}
	} while (TermInst);

	// -----------------------------------------------------------------
	// LOD / OFFSETS (FIXED FOR List<FVector>::append)
	// -----------------------------------------------------------------

	for (int LodIndex = 0; LodIndex < 4; LodIndex++) {
		int OffsetIndex = 0;

		ListIter<Text> ModelIter = detail[LodIndex];
		while (++ModelIter) {
			const char* ModelName = ModelIter.value()->data();

			SimModel* ModelInst = new SimModel;
			if (!ModelInst->Load(ModelName, scale)) {
				UE_LOG(LogShipDesign, Error, TEXT("ERROR: Could not load detail %d, model '%s'"), LodIndex, ANSI_TO_TCHAR(ModelName));
				delete ModelInst;
				ModelInst = 0;
				valid = false;
			}
			else {
				lod_levels = LodIndex + 1;

				if (ModelInst->GetRadius() > radius)
					radius = (float)ModelInst->GetRadius();

				models[LodIndex].append(ModelInst);
				PrepareModel(*ModelInst);

				// IMPORTANT:
				// offsets[] must store FVector* (pointer ownership), not FVector values and not FVector::FReal.
				if (offset[LodIndex].size()) {
					FVector* OffsetPtr = offset[LodIndex].at(OffsetIndex);
					if (OffsetPtr) {
						*OffsetPtr *= scale;
						offsets[LodIndex].append(OffsetPtr); // transfer ownership
					}
					else {
						offsets[LodIndex].append(new FVector(0.f, 0.f, 0.f));
					}
				}
				else {
					offsets[LodIndex].append(new FVector(0.f, 0.f, 0.f));
				}

				OffsetIndex++;
			}
		}

		detail[LodIndex].destroy();
	}

	if (!secret) {
		UE_LOG(LogShipDesign, Log, TEXT("   Ship Design Radius = %f"), radius);
	}

	if (cockpit_name[0]) {
		const char* CockpitModelName = cockpit_name;

		cockpit_model = new SimModel;
		if (!cockpit_model->Load(CockpitModelName, cockpit_scale)) {
			UE_LOG(LogShipDesign, Error, TEXT("ERROR: Could not load cockpit model '%s'"), ANSI_TO_TCHAR(CockpitModelName));
			delete cockpit_model;
			cockpit_model = 0;
		}
		else {
			if (!secret) {
				UE_LOG(LogShipDesign, Log, TEXT("   Loaded cockpit model '%s', preparing tangents"), ANSI_TO_TCHAR(CockpitModelName));
			}
			PrepareModel(*cockpit_model);
		}
	}

	if (beauty.Width() < 1 && Loader->FindFile("beauty.pcx"))
		Loader->LoadGameBitmap("beauty.pcx", beauty);

	if (hud_icon.Width() < 1 && Loader->FindFile("hud_icon.pcx"))
		Loader->LoadGameBitmap("hud_icon.pcx", hud_icon);

	Loader->ReleaseBuffer(Block);
	Loader->SetDataPath("");

	if (abrv[0] == 0) {
		switch (type) {
		case (int) CLASSIFICATION::DRONE:     
			strcpy_s(abrv, "DR");   
			break;
		case (int)CLASSIFICATION::FIGHTER:
			strcpy_s(abrv, "F");
			break;
		case (int)CLASSIFICATION::ATTACK: 
			strcpy_s(abrv, "F/A");
			break;
		case (int)CLASSIFICATION::LCA: 
			strcpy_s(abrv, "LCA"); 
			break;
		case (int)CLASSIFICATION::CORVETTE: 
			strcpy_s(abrv, "FC");  
			break;
		case (int)CLASSIFICATION::COURIER:
		case (int)CLASSIFICATION::CARGO:
		case (int)CLASSIFICATION::FREIGHTER:
			strcpy_s(abrv, "MV"); 
			break;
		case (int)CLASSIFICATION::FRIGATE:
			strcpy_s(abrv, "FF");    
			break;
		case (int)CLASSIFICATION::DESTROYER:  
			strcpy_s(abrv, "DD"); 
			break;
		case (int)CLASSIFICATION::CRUISER:
			strcpy_s(abrv, "CA"); 
			break;
		case (int)CLASSIFICATION::BATTLESHIP:
			strcpy_s(abrv, "BB");
			break;
		case (int)CLASSIFICATION::CARRIER:   
			strcpy_s(abrv, "CV");   
			break;
		case (int)CLASSIFICATION::DREADNAUGHT:
			strcpy_s(abrv, "DN");   
			break;
		case (int)CLASSIFICATION::MINE: 
			strcpy_s(abrv, "MINE"); 
			break;
		case (int)CLASSIFICATION::COMSAT: 
			strcpy_s(abrv, "COMS");
			break;
		case (int)CLASSIFICATION::DEFSAT:  
			strcpy_s(abrv, "DEFS");  
			break;
		case (int)CLASSIFICATION::SWACS:  
			strcpy_s(abrv, "SWAC");
			break;
		default:    
			break;
		}
	}

	if (scuttle < 1)
		scuttle = 1;

	if (splash_radius < 0)
		splash_radius = radius * 12.0f;

	if (repair_speed <= 1e-6f)
		repair_speed = 1.0e-6f;

	if (commit_range <= 0) {
		if (type <= (int)CLASSIFICATION::LCA)
			commit_range = 80.0e3f;
		else
			commit_range = 200.0e3f;
	}

	// calc standard loadout weights:
	ListIter<ShipLoad> LoadIter = loadouts;
	while (++LoadIter) {
		for (int HpIndex = 0; HpIndex < hard_points.size(); HpIndex++) {
			HardPoint* Hp = hard_points[HpIndex];
			LoadIter->mass += Hp->GetCarryMass(LoadIter->load[HpIndex]);
		}
	}
}

// +--------------------------------------------------------------------+

ShipDesign::~ShipDesign()
{
	/*delete bolt_hit_sound_resource;
	delete beam_hit_sound_resource;
	delete cockpit_model;
	delete navsys;
	delete sensor;
	delete shield;
	delete thruster;
	delete farcaster;
	delete quantum_drive;
	delete decoy;
	delete probe;
	delete gear;

	navlights.destroy();
	flight_decks.destroy();
	hard_points.destroy();
	computers.destroy();
	weapons.destroy();
	drives.destroy();
	reactors.destroy();
	loadouts.destroy();
	map_sprites.destroy();

	delete shield_model;
	for (int i = 0; i < 4; i++) {
		models[i].destroy();
		offsets[i].destroy();
	}

	spin_rates.destroy();

	for (int i = 0; i < 10; i++) {
		delete debris[i].model;
	}*/
}

const char*
ShipDesign::DisplayName() const
{
	if (display_name[0])
		return display_name;

	return name;
}

// +--------------------------------------------------------------------+

void AddModCatalogEntry(const char* design_name, const char* design_path)
{
	if (!design_name || !*design_name)
		return;

	ShipCatalogEntry* entry = 0;

	for (int i = 0; i < catalog.size(); i++) {
		ShipCatalogEntry* e = catalog[i];
		if (e->name == design_name) {
			if (design_path && *design_path && e->path != design_path)
				continue;
			entry = e;
			return;
		}
	}

	for (int i = 0; i < mod_catalog.size(); i++) {
		ShipCatalogEntry* e = mod_catalog[i];
		if (e->name == design_name) {
			if (design_path && *design_path) {
				Text full_path = "Mods/Ships/";
				full_path += design_path;

				if (e->path != full_path)
					continue;
			}

			entry = e;
			return;
		}
	}

	// still here? notxfound yet:
	Text file = Text(design_name) + ".def";
	Text path = Text("Mods/Ships/");
	Text name;
	Text type;
	bool valid = false;

	if (design_path && *design_path)
		path += design_path;
	else
		path += design_name;

	path += "/";

	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path);

	BYTE* block;
	int blocklen = loader->LoadBuffer(file, block, true);

	// file notxfound:
	if (blocklen <= 4) {
		return;
	}

	Parser parser(new  BlockReader((const char*)block, blocklen));
	Term* term = parser.ParseTerm();

	if (!term) {
		UE_LOG(LogShipDesign, Error, TEXT("ERROR: could notxparse '%s'"), ANSI_TO_TCHAR(file.data()));
		delete block;
		return;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "SHIP") {
			UE_LOG(LogShipDesign, Error, TEXT("ERROR: invalid ship design file '%s'"), ANSI_TO_TCHAR(file.data()));
			delete block;
			return;
		}
	}

	valid = true;

	do {
		delete term;

		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				Text defname = def->name()->value();
				defname.setSensitive(false);

				if (defname == "class") {
					if (!GetDefText(type, def, file)) {
						UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship class in '%s'"), ANSI_TO_TCHAR(file.data()));
						valid = false;
					}
				}

				else if (defname == "name") {
					if (!GetDefText(name, def, file)) {
						UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship name in '%s'"), ANSI_TO_TCHAR(file.data()));
						valid = false;
					}
				}
			}
			else {
				UE_LOG(LogShipDesign, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(file.data()));
				term->print();
			}
		}
	} while (term && valid && (name.length() < 1 || type.length() < 1));

	delete block;

	if (valid && name.length() && type.length()) {
		UE_LOG(LogShipDesign, Log, TEXT("Add Mod Catalog Entry '%s' Class '%s'"),
			ANSI_TO_TCHAR(name.data()), ANSI_TO_TCHAR(type.data()));

		ShipCatalogEntry* entry2 = new  ShipCatalogEntry(name, type, path, file);
		mod_catalog.append(entry2);
	}
}

void
ShipDesign::Initialize()
{
	if (catalog.size()) return;

	LoadCatalog("Ships/", "catalog.def");
	LoadSkins("Mods/Skins/");

	List<Text>  mod_designs;
	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath("Mods/Ships/");
	loader->ListFiles("*.def", mod_designs, true);

	for (int i = 0; i < mod_designs.size(); i++) {
		Text full_name = *mod_designs[i];
		full_name.setSensitive(false);

		if (full_name.contains('/') && !full_name.contains("catalog")) {
			char path[1024];
			strcpy_s(path, full_name.data());

			char* name = path + full_name.length();
			while (*name != '/')
				name--;

			*name++ = 0;

			char* p = strrchr(name, '.');
			if (p && strlen(p) > 3) {
				if ((p[1] == 'd' || p[1] == 'D') &&
					(p[2] == 'e' || p[2] == 'E') &&
					(p[3] == 'f' || p[3] == 'F')) {

					*p = 0;
				}
			}

			// Quick parse to add info to catalog (do notxpreload heavy assets).
			AddModCatalogEntry(name, path);
		}
	}

	mod_designs.destroy();
	loader->SetDataPath("");
}

void
ShipDesign::Close()
{
	mod_catalog.destroy();
	catalog.destroy();
}

// +--------------------------------------------------------------------+

int
ShipDesign::LoadCatalog(const char* path, const char* fname, bool mod)
{
	int result = 0;

	// Load Design Catalog File:
	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path);

	char filename[NAMELEN];
	FMemory::Memzero(filename, NAMELEN);
	strncpy(filename, fname, NAMELEN - 1);

	UE_LOG(LogShipDesign, Log, TEXT("Loading ship design catalog: %s%s"), ANSI_TO_TCHAR(path), ANSI_TO_TCHAR(filename));

	BYTE* block;
	int blocklen = loader->LoadBuffer(filename, block, true);
	Parser parser(new BlockReader((const char*)block, blocklen));
	Term* term = parser.ParseTerm();

	if (!term) {
		UE_LOG(LogShipDesign, Error, TEXT("ERROR: could notxparse '%s'"), ANSI_TO_TCHAR(filename));
		loader->ReleaseBuffer(block);
		loader->SetDataPath("");
		return result;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "SHIPCATALOG") {
			UE_LOG(LogShipDesign, Error, TEXT("ERROR: invalid ship catalog file '%s'"), ANSI_TO_TCHAR(filename));
			loader->ReleaseBuffer(block);
			loader->SetDataPath("");
			return result;
		}
	}

	do {
		delete term;

		term = parser.ParseTerm();

		Text name, type, fname2, path2;
		bool hide = false;

		if (term) {
			TermDef* def = term->isDef();
			if (def && def->term() && def->term()->isStruct()) {
				TermStruct* val = def->term()->isStruct();

				for (int i = 0; i < val->elements()->size(); i++) {
					TermDef* pdef = val->elements()->at(i)->isDef();
					if (pdef) {
						Text defname = pdef->name()->value();
						defname.setSensitive(false);

						if (defname == "name") {
							if (!GetDefText(name, pdef, filename))
								UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship name in '%s'"), ANSI_TO_TCHAR(filename));
						}
						else if (defname == "type") {
							if (!GetDefText(type, pdef, filename))
								UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship type in '%s'"), ANSI_TO_TCHAR(filename));
						}
						else if (defname == "path") {
							if (!GetDefText(path2, pdef, filename))
								UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship path in '%s'"), ANSI_TO_TCHAR(filename));
						}
						else if (defname == "file") {
							if (!GetDefText(fname2, pdef, filename))
								UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship file in '%s'"), ANSI_TO_TCHAR(filename));
						}
						else if (defname == "hide" || defname == "secret") {
							GetDefBool(hide, pdef, filename);
						}
					}
				}

				ShipCatalogEntry* entry = new  ShipCatalogEntry(name, type, path2, fname2, hide);

				if (mod) mod_catalog.append(entry);
				else     catalog.append(entry);

				result++;
			}
			else {
				UE_LOG(LogShipDesign, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
				term->print();
			}
		}
	} while (term);

	loader->ReleaseBuffer(block);
	loader->SetDataPath("");

	return result;
}

// +--------------------------------------------------------------------+

void
ShipDesign::LoadSkins(const char* path, const char* archive)
{
	// Load MOD Skin Files:
	List<Text>  list;
	DataLoader* loader = DataLoader::GetLoader();
	bool        oldfs = loader->IsFileSystemEnabled();

	loader->UseFileSystem(true);
	loader->SetDataPath(path);

	ListIter<Text> iter = list;
	while (++iter) {
		Text  filename = *iter.value();
		BYTE* block;
		int   blocklen = loader->LoadBuffer(filename, block, true);

		// file notxfound:
		if (blocklen <= 4) {
			continue;
		}

		Parser      parser(new  BlockReader((const char*)block, blocklen));
		Term* term = parser.ParseTerm();
		ShipDesign* design = 0;

		if (!term) {
			UE_LOG(LogShipDesign, Error, TEXT("ERROR: could notxparse '%s'"), ANSI_TO_TCHAR(filename.data()));
			return;
		}
		else {
			TermText* file_type = term->isText();
			if (!file_type || file_type->value() != "SKIN") {
				UE_LOG(LogShipDesign, Error, TEXT("ERROR: invalid skin file '%s'"), ANSI_TO_TCHAR(filename.data()));
				return;
			}
		}

		do {
			delete term;

			term = parser.ParseTerm();

			if (term) {
				TermDef* def = term->isDef();
				if (def) {
					Text defname = def->name()->value();
					defname.setSensitive(false);

					if (defname == "name") {
						Text name2;
						GetDefText(name2, def, filename);
						design = Get(name2);
					}

					else if (defname == "skin" && design != 0) {
						if (!def->term() || !def->term()->isStruct()) {
							UE_LOG(LogShipDesign, Warning, TEXT("WARNING: skin struct missing in '%s'"), ANSI_TO_TCHAR(filename.data()));
						}
						else {
							TermStruct* val = def->term()->isStruct();
							Skin* skin = design->ParseSkin(val);

							if (skin)
								skin->SetPath(archive);
						}
					}
				}
			}
		} while (term);
	}

	loader->UseFileSystem(oldfs);
}

// +--------------------------------------------------------------------+

int
ShipDesign::StandardCatalogSize()
{
	return catalog.size();
}

void
ShipDesign::PreloadCatalog(int index)
{
	if (index >= 0 && index < catalog.size()) {
		ShipCatalogEntry* entry = catalog[index];

		if (entry->hide)
			return;

		int ship_class = ClassForName(entry->type);
		if (ship_class > (int) CLASSIFICATION::STARSHIPS)
			return;

		if (!entry->path.contains("Alliance_"))
			return;

		if (!entry->design) {
			entry->design = new  ShipDesign(entry->name,
				entry->path,
				entry->file,
				entry->hide);
		}
	}

	else {
		ListIter<ShipCatalogEntry> iter = catalog;
		while (++iter) {
			ShipCatalogEntry* entry = iter.value();

			if (!entry->design) {
				entry->design = new  ShipDesign(entry->name,
					entry->path,
					entry->file,
					entry->hide);
			}
		}
	}
}

// +--------------------------------------------------------------------+

bool
ShipDesign::CheckName(const char* design_name)
{
	ShipCatalogEntry* entry = 0;

	for (int i = 0; i < catalog.size(); i++) {
		if (catalog.at(i)->name == design_name) {
			entry = catalog.at(i);
			break;
		}
	}

	if (!entry) {
		for (int i = 0; i < mod_catalog.size(); i++) {
			if (mod_catalog.at(i)->name == design_name) {
				entry = mod_catalog.at(i);
				break;
			}
		}
	}

	return entry != 0;
}

// +--------------------------------------------------------------------+

ShipDesign*
ShipDesign::Get(const char* design_name, const char* design_path)
{
	if (!design_name || !*design_name)
		return 0;

	ShipCatalogEntry* entry = 0;

	for (int i = 0; i < catalog.size(); i++) {
		ShipCatalogEntry* e = catalog[i];
		if (e->name == design_name) {
			if (design_path && *design_path && e->path != design_path)
				continue;
			entry = e;
			break;
		}
	}

	if (!entry) {
		for (int i = 0; i < mod_catalog.size(); i++) {
			ShipCatalogEntry* e = mod_catalog[i];
			if (e->name == design_name) {
				if (design_path && *design_path) {
					Text full_path = "Mods/Ships/";
					full_path += design_path;

					if (e->path != full_path)
						continue;
				}

				entry = e;
				break;
			}
		}
	}

	if (entry) {
		if (!entry->design) {
			entry->design = new  ShipDesign(entry->name,
				entry->path,
				entry->file,
				entry->hide);
		}
		return entry->design;
	}
	else {
		UE_LOG(LogShipDesign, Warning, TEXT("ShipDesign: no catalog entry for design '%s', checking mods..."), ANSI_TO_TCHAR(design_name));
		return ShipDesign::FindModDesign(design_name, design_path);
	}
}

ShipDesign*
ShipDesign::FindModDesign(const char* design_name, const char* design_path)
{
	Text file = Text(design_name) + ".def";
	Text path = Text("Mods/Ships/");

	if (design_path && *design_path)
		path += design_path;
	else
		path += design_name;

	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path.data());

	ShipDesign* design = new  ShipDesign(design_name, path, file);

	if (design->valid) {
		UE_LOG(LogShipDesign, Log, TEXT("ShipDesign: found mod design '%s'"), ANSI_TO_TCHAR(design->name));

		ShipCatalogEntry* entry = new  ShipCatalogEntry(design->name,
			ClassName(design->type),
			path,
			file);
		mod_catalog.append(entry);
		entry->design = design;
		return entry->design;
	}
	else {
		delete design;
	}

	return 0;
}

void
ShipDesign::ClearModCatalog()
{
	mod_catalog.destroy();

	for (int i = 0; i < catalog.size(); i++) {
		ShipCatalogEntry* e = catalog[i];

		if (e && e->design) {
			ListIter<Skin> iter = e->design->skins;

			while (++iter) {
				Skin* skin = iter.value();
				if (*skin->Path())
					iter.removeItem();
			}
		}
	}
}

// +--------------------------------------------------------------------+

int
ShipDesign::GetDesignList(int type, List<Text>& designs)
{
	designs.clear();

	for (int i = 0; i < catalog.size(); i++) {
		ShipCatalogEntry* e = catalog[i];

		int etype = ClassForName(e->type);
		if (etype & type) {
			if (!e->design)
				e->design = new  ShipDesign(e->name,
					e->path,
					e->file,
					e->hide);

			if (e->hide || !e->design || !e->design->valid || e->design->secret)
				continue;

			designs.append(&e->name);
		}
	}

	for (int i = 0; i < mod_catalog.size(); i++) {
		ShipCatalogEntry* e = mod_catalog[i];

		int etype = ClassForName(e->type);
		if (etype & type) {
			designs.append(&e->name);
		}
	}

	return designs.size();
}

// +--------------------------------------------------------------------+

int
ShipDesign::ClassForName(const char* name)
{
	if (!name || !name[0])
		return 0;

	for (int i = 0; i < 32; i++) {
		if (!_stricmp(name, ship_design_class_name[i])) {
			return 1 << i;
		}
	}

	return 0;
}

const char*
ShipDesign::ClassName(int type)
{
	if (type != 0) {
		int index = 0;

		while (!(type & 1)) {
			type >>= 1;
			index++;
		}

		if (index >= 0 && index < 32)
			return ship_design_class_name[index];
	}

	return "Unknown";
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseShip(TermDef* def)
{
	char    detail_name[NAMELEN];
	FVector off_loc(0, 0, 0);
	FVector spin(0, 0, 0);
	Text    defname = def->name()->value();

	defname.setSensitive(false);

	if (defname == "cockpit_model") {
		if (!GetDefText(cockpit_name, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing cockpit_model in '%s'"), ANSI_TO_TCHAR(filename));
		}
	}

	else if (defname == "model" || defname == "detail_0") {
		if (!GetDefText(detail_name, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing model in '%s'"), ANSI_TO_TCHAR(filename));
		}

		detail[0].append(new Text(detail_name));
	}

	else if (defname == "detail_1") {
		if (!GetDefText(detail_name, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing detail_1 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		detail[1].append(new Text(detail_name));
	}

	else if (defname == "detail_2") {
		if (!GetDefText(detail_name, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing detail_2 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		detail[2].append(new Text(detail_name));
	}

	else if (defname == "detail_3") {
		if (!GetDefText(detail_name, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing detail_3 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		detail[3].append(new Text(detail_name));
	}

	else if (defname == "spin") {
		if (!GetDefVec(spin, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing spin in '%s'"), ANSI_TO_TCHAR(filename));
		}

		// Point/Vec3 -> FVector
		spin_rates.append(new FVector(spin));
	}

	else if (defname == "offset_0") {
		if (!GetDefVec(off_loc, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing offset_0 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		offset[0].append(new FVector(off_loc));
	}

	else if (defname == "offset_1") {
		if (!GetDefVec(off_loc, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing offset_1 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		offset[1].append(new FVector(off_loc));
	}

	else if (defname == "offset_2") {
		if (!GetDefVec(off_loc, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing offset_2 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		offset[2].append(new FVector(off_loc));
	}

	else if (defname == "offset_3") {
		if (!GetDefVec(off_loc, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing offset_3 in '%s'"), ANSI_TO_TCHAR(filename));
		}

		offset[3].append(new FVector(off_loc));
	}

	else if (defname == "beauty") {
		if (def->term() && def->term()->isArray()) {
			GetDefVec(beauty_cam, def, filename);

			if (degrees) {
				beauty_cam.X *= (float) DEGREES;
				beauty_cam.Y *= (float) DEGREES;
			}
		}
		else {
			char beauty_name[64];
			if (!GetDefText(beauty_name, def, filename)) {
				UE_LOG(LogTemp, Warning,
					TEXT("WARNING: invalid or missing beauty in '%s'"),
					ANSI_TO_TCHAR(filename));
			}

			DataLoader* loader = DataLoader::GetLoader();
			loader->LoadGameBitmap(beauty_name, beauty);
		}
	}

	else if (defname == "hud_icon") {
		char hud_icon_name[64];
		if (!GetDefText(hud_icon_name, def, filename)) {
			
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: invalid or missing hud_icon in '%s'"),
				ANSI_TO_TCHAR(filename));
		}

		DataLoader* loader = DataLoader::GetLoader();
		loader->LoadGameBitmap(hud_icon_name, hud_icon);
	}

	else if (defname == "feature_0") {
		if (!GetDefNumber(feature_size[0], def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing feature_0 in '%s'"), ANSI_TO_TCHAR(filename));
		}
	}

	else if (defname == "feature_1") {
		if (!GetDefNumber(feature_size[1], def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing feature_1 in '%s'"), ANSI_TO_TCHAR(filename));
		}
	}

	else if (defname == "feature_2") {
		if (!GetDefNumber(feature_size[2], def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing feature_2 in '%s'"), ANSI_TO_TCHAR(filename));
		}
	}

	else if (defname == "feature_3") {
		if (!GetDefNumber(feature_size[3], def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing feature_3 in '%s'"), ANSI_TO_TCHAR(filename));
		}
	}

	else if (defname == "class") {
		char typestr[64];
		if (!GetDefText(typestr, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing ship class in '%s'"), ANSI_TO_TCHAR(filename));
		}

		type = ClassForName(typestr);

		if (type <= (int)CLASSIFICATION::LCA) {
			repair_auto   = false;
			repair_screen = false;
			wep_screen    = false;
		}
	}

	else GET_DEF_TEXT(name);
	else GET_DEF_TEXT(description);
	else GET_DEF_TEXT(display_name);
	else GET_DEF_TEXT(abrv);
	else GET_DEF_NUM(pcs);
	else GET_DEF_NUM(acs);
	else GET_DEF_NUM(detet);
	else GET_DEF_NUM(scale);
	else GET_DEF_NUM(explosion_scale);
	else GET_DEF_NUM(mass);
	else GET_DEF_NUM(vlimit);
	else GET_DEF_NUM(agility);
	else GET_DEF_NUM(air_factor);
	else GET_DEF_NUM(roll_rate);
	else GET_DEF_NUM(pitch_rate);
	else GET_DEF_NUM(yaw_rate);
	else GET_DEF_NUM(integrity);
	else GET_DEF_NUM(drag);
	else GET_DEF_NUM(arcade_drag);
	else GET_DEF_NUM(roll_drag);
	else GET_DEF_NUM(pitch_drag);
	else GET_DEF_NUM(yaw_drag);
	else GET_DEF_NUM(trans_x);
	else GET_DEF_NUM(trans_y);
	else GET_DEF_NUM(trans_z);
	else GET_DEF_NUM(turn_bank);
	else GET_DEF_NUM(cockpit_scale);
	else GET_DEF_NUM(auto_roll);

	else GET_DEF_NUM(CL);
	else GET_DEF_NUM(CD);
	else GET_DEF_NUM(stall);

	else GET_DEF_NUM(prep_time);
	else GET_DEF_NUM(avoid_time);
	else GET_DEF_NUM(avoid_fighter);
	else GET_DEF_NUM(avoid_strike);
	else GET_DEF_NUM(avoid_target);
	else GET_DEF_NUM(commit_range);

	else GET_DEF_NUM(splash_radius);
	else GET_DEF_NUM(scuttle);
	else GET_DEF_NUM(repair_speed);
	else GET_DEF_NUM(repair_teams);
	else GET_DEF_BOOL(secret);
	else GET_DEF_BOOL(repair_auto);
	else GET_DEF_BOOL(repair_screen);
	else GET_DEF_BOOL(wep_screen);
	else GET_DEF_BOOL(degrees);

	else if (defname == "emcon_1") {
		GetDefNumber(e_factor[0], def, filename);
	}

	else if (defname == "emcon_2") {
		GetDefNumber(e_factor[1], def, filename);
	}

	else if (defname == "emcon_3") {
		GetDefNumber(e_factor[2], def, filename);
	}

	else if (defname == "chase") {
		if (!GetDefVec(chase_vec, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing chase cam loc in '%s'"), ANSI_TO_TCHAR(filename));
		}

		chase_vec *= (float) scale;
	}

	else if (defname == "bridge") {
		if (!GetDefVec(bridge_vec, def, filename)) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing bridge cam loc in '%s'"), ANSI_TO_TCHAR(filename));
		}

		bridge_vec *= (float) scale;
	}

	else if (defname == "power") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: power source struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParsePower(val);
		}
	}

	else if (defname == "main_drive" || defname == "drive") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: main drive struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseDrive(val);
		}
	}

	else if (defname == "quantum" || defname == "quantum_drive") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: quantum_drive struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseQuantumDrive(val);
		}
	}

	else if (defname == "sender" || defname == "farcaster") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: farcaster struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseFarcaster(val);
		}
	}

	else if (defname == "thruster") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: thruster struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseThruster(val);
		}
	}

	else if (defname == "navlight") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: navlight struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseNavlight(val);
		}
	}

	else if (defname == "flightdeck") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: flightdeck struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseFlightDeck(val);
		}
	}

	else if (defname == "gear") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: gear struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseLandingGear(val);
		}
	}

	else if (defname == "weapon") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: weapon struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseWeapon(val);
		}
	}

	else if (defname == "hardpoint") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: hardpoint struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseHardPoint(val);
		}
	}

	else if (defname == "loadout") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: loadout struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseLoadout(val);
		}
	}

	else if (defname == "decoy") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: decoy struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseWeapon(val);
		}
	}

	else if (defname == "probe") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: probe struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseWeapon(val);
		}
	}

	else if (defname == "sensor") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: sensor struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseSensor(val);
		}
	}

	else if (defname == "nav") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: nav struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseNavsys(val);
		}
	}

	else if (defname == "computer") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: computer struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseComputer(val);
		}
	}

	else if (defname == "shield") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: shield struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseShield(val);
		}
	}

	else if (defname == "death_spiral") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: death spiral struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseDeathSpiral(val);
		}
	}

	else if (defname == "map") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: map struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseMap(val);
		}
	}

	else if (defname == "squadron") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: squadron struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseSquadron(val);
		}
	}

	else if (defname == "skin") {
		if (!def->term() || !def->term()->isStruct()) {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: skin struct missing in '%s'"), ANSI_TO_TCHAR(filename));
		}
		else {
			TermStruct* val = def->term()->isStruct();
			ParseSkin(val);
		}
	}

	else {
		UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unknown parameter '%s' in '%s'"),
			ANSI_TO_TCHAR(defname.data()),
			ANSI_TO_TCHAR(filename));
	}

	if (description.length())
		description = Game::GetText(description);
}

void
ShipDesign::ParsePower(TermStruct* val)
{
	int    stype = 0;
	float  output = 1000.0f;
	float  fuel = 0.0f;
	FVector loc(0.0f, 0.0f, 0.0f);
	float  size = 0.0f;
	float  hull = 0.5f;
	Text   design_name;
	Text   pname;
	Text   pabrv;
	int    etype = 0;
	int    emcon_1 = -1;
	int    emcon_2 = -1;
	int    emcon_3 = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "type") {
				TermText* tname = pdef->term()->isText();

				if (tname) {
					if (tname->value()[0] == 'B') stype = PowerSource::BATTERY;
					else if (tname->value()[0] == 'A') stype = PowerSource::AUX;
					else if (tname->value()[0] == 'F') stype = PowerSource::FUSION;
					else {
						UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unknown power source type '%s' in '%s'"),
							ANSI_TO_TCHAR(tname->value().data()),
							ANSI_TO_TCHAR(filename));
					}
				}
			}

			else if (defname == "name") {
				GetDefText(pname, pdef, filename);
			}

			else if (defname == "abrv") {
				GetDefText(pabrv, pdef, filename);
			}

			else if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}

			else if (defname == "max_output") {
				GetDefNumber(output, pdef, filename);
			}

			else if (defname == "fuel_range") {
				GetDefNumber(fuel, pdef, filename);
			}

			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}

			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}

			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}

			else if (defname == "explosion") {
				GetDefNumber(etype, pdef, filename);
			}

			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}

			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}

			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}
		}
	}

	PowerSource* source = new  PowerSource((PowerSource::SUBTYPE)stype, output);
	if (pname.length()) source->SetName(pname);
	if (pabrv.length()) source->SetName(pabrv);
	source->SetFuelRange(fuel);
	source->Mount(loc, size, hull);
	source->SetExplosionType(etype);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			source->SetDesign(sd);
	}

	if (emcon_1 >= 0 && emcon_1 <= 100)
		source->SetEMCONPower(1, emcon_1);

	if (emcon_2 >= 0 && emcon_2 <= 100)
		source->SetEMCONPower(2, emcon_2);

	if (emcon_3 >= 0 && emcon_3 <= 100)
		source->SetEMCONPower(3, emcon_3);

	reactors.append(source);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseDrive(TermStruct* val)
{
	Text   dname;
	Text   dabrv;
	int    dtype = 0;
	int    etype = 0;
	float  dthrust = 1.0f;
	float  daug = 0.0f;
	float  dscale = 1.0f;
	FVector loc(0.0f, 0.0f, 0.0f);
	float  size = 0.0f;
	float  hull = 0.5f;
	Text   design_name;
	int    emcon_1 = -1;
	int    emcon_2 = -1;
	int    emcon_3 = -1;
	bool   trail = true;
	Drive* drive = 0;

	for (int elemIdx = 0; elemIdx < val->elements()->size(); elemIdx++) {
		TermDef* pdef = val->elements()->at(elemIdx)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "type") {
				TermText* tname = pdef->term()->isText();

				if (tname) {
					Text tval = tname->value();
					tval.setSensitive(false);

					if (tval == "Plasma")            dtype = Drive::PLASMA;
					else if (tval == "Fusion")       dtype = Drive::FUSION;
					else if (tval == "Alien")        dtype = Drive::GREEN;
					else if (tval == "Green")        dtype = Drive::GREEN;
					else if (tval == "Red")          dtype = Drive::RED;
					else if (tval == "Blue")         dtype = Drive::BLUE;
					else if (tval == "Yellow")       dtype = Drive::YELLOW;
					else if (tval == "Stealth")      dtype = Drive::STEALTH;
					else {
						UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unknown drive type '%s' in '%s'"),
							ANSI_TO_TCHAR(tname->value().data()),
							ANSI_TO_TCHAR(filename));
					}
				}
			}

			else if (defname == "name") {
				if (!GetDefText(dname, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing name for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "abrv") {
				if (!GetDefText(dabrv, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing abrv for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "design") {
				if (!GetDefText(design_name, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing design for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "thrust") {
				if (!GetDefNumber(dthrust, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing thrust for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "augmenter") {
				if (!GetDefNumber(daug, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing augmenter for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "scale") {
				if (!GetDefNumber(dscale, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing scale for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "port") {
				FVector port(0, 0, 0);
				float   flare_scale = 0;

				if (pdef->term()->isArray()) {
					GetDefVec(port, pdef, filename);
					port *= scale;
					flare_scale = dscale;
				}

				else if (pdef->term()->isStruct()) {
					TermStruct* portStruct = pdef->term()->isStruct();

					for (int portElemIdx = 0; portElemIdx < portStruct->elements()->size(); portElemIdx++) {
						TermDef* pdef2 = portStruct->elements()->at(portElemIdx)->isDef();
						if (pdef2) {
							Text portDefName = pdef2->name()->value();
							portDefName.setSensitive(false);

							if (portDefName == "loc") {
								GetDefVec(port, pdef2, filename);
								port *= scale;
							}

							else if (portDefName == "scale") {
								GetDefNumber(flare_scale, pdef2, filename);
							}
						}
					}

					if (flare_scale <= 0)
						flare_scale = dscale;
				}

				if (!drive)
					drive = new Drive((Drive::SUBTYPE)dtype, dthrust, daug, trail);

				drive->AddPort(port, flare_scale);
			}

			else if (defname == "loc") {
				if (!GetDefVec(loc, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing loc for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
				loc *= (float)scale;
			}

			else if (defname == "size") {
				if (!GetDefNumber(size, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing size for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
				size *= (float)scale;
			}

			else if (defname == "hull_factor") {
				if (!GetDefNumber(hull, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing hull_factor for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "explosion") {
				if (!GetDefNumber(etype, pdef, filename)) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid or missing explosion for drive in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}

			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}

			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}

			else if (defname == "trail" || defname == "show_trail") {
				GetDefBool(trail, pdef, filename);
			}
		}
	}

	if (!drive)
		drive = new Drive((Drive::SUBTYPE)dtype, dthrust, daug, trail);

	drive->SetSourceIndex(reactors.size() - 1);
	drive->Mount(loc, size, hull);
	if (dname.length()) drive->SetName(dname);
	if (dabrv.length()) drive->SetAbbreviation(dabrv);
	drive->SetExplosionType(etype);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			drive->SetDesign(sd);
	}

	if (emcon_1 >= 0 && emcon_1 <= 100)
		drive->SetEMCONPower(1, emcon_1);

	if (emcon_2 >= 0 && emcon_2 <= 100)
		drive->SetEMCONPower(2, emcon_2);

	if (emcon_3 >= 0 && emcon_3 <= 100)
		drive->SetEMCONPower(3, emcon_3);

	main_drive = drives.size();
	drives.append(drive);
}


// +--------------------------------------------------------------------+

void
ShipDesign::ParseQuantumDrive(TermStruct* val)
{
	// UE-style local naming to avoid member hiding and match your convention:
	double Capacity = 250e3;
	double Consumption = 1e3;
	FVector Loc(0.0f, 0.0f, 0.0f);
	float   Size = 0.0f;
	float   HullFactor = 0.5f;
	float   Countdown = 5.0f;

	Text    DesignName;
	Text    TypeName;
	Text    Abrv;

	int     Subtype = QuantumDrive::QUANTUM;
	int     Emcon1 = -1;
	int     Emcon2 = -1;
	int     Emcon3 = -1;

	for (int32 ElemIndex = 0; ElemIndex < val->elements()->size(); ElemIndex++) {
		TermDef* Def = val->elements()->at(ElemIndex)->isDef();
		if (!Def)
			continue;

		Text DefName = Def->name()->value();
		DefName.setSensitive(false);

		if (DefName == "design") {
			GetDefText(DesignName, Def, filename);
		}
		else if (DefName == "abrv") {
			GetDefText(Abrv, Def, filename);
		}
		else if (DefName == "type") {
			GetDefText(TypeName, Def, filename);
			TypeName.setSensitive(false);

			if (TypeName.contains("hyper")) {
				Subtype = QuantumDrive::HYPER;
			}
		}
		else if (DefName == "capacity") {
			GetDefNumber(Capacity, Def, filename);
		}
		else if (DefName == "consumption") {
			GetDefNumber(Consumption, Def, filename);
		}
		else if (DefName == "loc") {
			GetDefVec(Loc, Def, filename);
			Loc *= (float)scale;
		}
		else if (DefName == "size") {
			GetDefNumber(Size, Def, filename);
			Size *= (float)scale;
		}
		else if (DefName == "hull_factor") {
			GetDefNumber(HullFactor, Def, filename);
		}
		else if (DefName == "jump_time") {
			GetDefNumber(Countdown, Def, filename);
		}
		else if (DefName == "countdown") {
			GetDefNumber(Countdown, Def, filename);
		}
		else if (DefName == "emcon_1") {
			GetDefNumber(Emcon1, Def, filename);
		}
		else if (DefName == "emcon_2") {
			GetDefNumber(Emcon2, Def, filename);
		}
		else if (DefName == "emcon_3") {
			GetDefNumber(Emcon3, Def, filename);
		}
	}

	QuantumDrive* Drive = new QuantumDrive((QuantumDrive::SUBTYPE)Subtype, Capacity, Consumption);
	Drive->SetSourceIndex(reactors.size() - 1);
	Drive->Mount(Loc, Size, HullFactor);
	Drive->SetCountdown(Countdown);

	if (DesignName.length()) {
		SystemDesign* SysDesign = SystemDesign::Find(DesignName);
		if (SysDesign)
			Drive->SetDesign(SysDesign);
	}

	if (Abrv.length())
		Drive->SetAbbreviation(Abrv);

	if (Emcon1 >= 0 && Emcon1 <= 100)
		Drive->SetEMCONPower(1, Emcon1);

	if (Emcon2 >= 0 && Emcon2 <= 100)
		Drive->SetEMCONPower(2, Emcon2);

	if (Emcon3 >= 0 && Emcon3 <= 100)
		Drive->SetEMCONPower(3, Emcon3);

	quantum_drive = Drive;
}


// +--------------------------------------------------------------------+

void
ShipDesign::ParseFarcaster(TermStruct* val)
{
	Text    design_name;
	double  capacity = 300e3;
	double  consumption = 15e3;  // twenty second recharge
	int     napproach = 0;
	FVector approach[Farcaster::NUM_APPROACH_PTS];
	FVector loc(0.0f, 0.0f, 0.0f);
	FVector start(0.0f, 0.0f, 0.0f);
	FVector end(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;
	int     emcon_1 = -1;
	int     emcon_2 = -1;
	int     emcon_3 = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}
			else if (defname == "capacity") {
				GetDefNumber(capacity, pdef, filename);
			}
			else if (defname == "consumption") {
				GetDefNumber(consumption, pdef, filename);
			}
			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}
			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}
			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}
			else if (defname == "start") {
				GetDefVec(start, pdef, filename);
				start *= (float)scale;
			}
			else if (defname == "end") {
				GetDefVec(end, pdef, filename);
				end *= (float)scale;
			}
			else if (defname == "approach") {
				if (napproach < Farcaster::NUM_APPROACH_PTS) {
					GetDefVec(approach[napproach], pdef, filename);
					approach[napproach++] *= (float)scale;
				}
				else {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: farcaster approach point ignored in '%s' (max=%d)"),
						ANSI_TO_TCHAR(filename),
						Farcaster::NUM_APPROACH_PTS);
				}
			}
			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}
			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}
			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}
		}
	}

	Farcaster* caster = new  Farcaster(capacity, consumption);
	caster->SetSourceIndex(reactors.size() - 1);
	caster->Mount(loc, size, hull);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			caster->SetDesign(sd);
	}

	caster->SetStartPoint(start);
	caster->SetEndPoint(end);

	for (int i = 0; i < napproach; i++)
		caster->SetApproachPoint(i, approach[i]);

	if (emcon_1 >= 0 && emcon_1 <= 100)
		caster->SetEMCONPower(1, emcon_1);

	if (emcon_2 >= 0 && emcon_2 <= 100)
		caster->SetEMCONPower(2, emcon_2);

	if (emcon_3 >= 0 && emcon_3 <= 100)
		caster->SetEMCONPower(3, emcon_3);

	farcaster = caster;
}

// +--------------------------------------------------------------------+
void
ShipDesign::ParseThruster(TermStruct* val)
{
	if (thruster) {
		UE_LOG(LogShipDesign, Warning,
			TEXT("WARNING: additional thruster ignored in '%s'"),
			ANSI_TO_TCHAR(filename));
		return;
	}

	double  thrust = 100;
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;
	Text    design_name;
	float   tscale = 1.0f;
	int     emcon_1 = -1;
	int     emcon_2 = -1;
	int     emcon_3 = -1;
	int     dtype = 0;

	Thruster* drive = nullptr;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		Text defname = pdef->name()->value();
		defname.setSensitive(false);

		if (defname == "type") {
			TermText* tname = pdef->term()->isText();
			if (tname) {
				Text tval = tname->value();
				tval.setSensitive(false);

				if (tval == "Plasma")  dtype = Drive::PLASMA;
				else if (tval == "Fusion")  dtype = Drive::FUSION;
				else if (tval == "Alien")   dtype = Drive::GREEN;
				else if (tval == "Green")   dtype = Drive::GREEN;
				else if (tval == "Red")     dtype = Drive::RED;
				else if (tval == "Blue")    dtype = Drive::BLUE;
				else if (tval == "Yellow")  dtype = Drive::YELLOW;
				else if (tval == "Stealth") dtype = Drive::STEALTH;
				else {
					UE_LOG(LogShipDesign, Warning,
						TEXT("WARNING: unknown thruster type '%s' in '%s'"),
						ANSI_TO_TCHAR(tname->value().data()),
						ANSI_TO_TCHAR(filename));
				}
			}
		}
		else if (defname == "thrust") {
			GetDefNumber(thrust, pdef, filename);
		}
		else if (defname == "design") {
			GetDefText(design_name, pdef, filename);
		}
		else if (defname == "loc") {
			GetDefVec(loc, pdef, filename);
			loc *= (float)scale;
		}
		else if (defname == "size") {
			GetDefNumber(size, pdef, filename);
			size *= (float)scale;
		}
		else if (defname == "hull_factor") {
			GetDefNumber(hull, pdef, filename);
		}
		else if (defname == "scale") {
			GetDefNumber(tscale, pdef, filename);
		}
		else if (defname.contains("port") && pdef->term()) {
			FVector port(0.0f, 0.0f, 0.0f);
			float   port_scale = 0.0f;
			uint32  fire = 0;

			if (pdef->term()->isArray()) {
				GetDefVec(port, pdef, filename);
				port *= scale;
				port_scale = tscale;
			}
			else if (pdef->term()->isStruct()) {
				TermStruct* port_struct = pdef->term()->isStruct();

				for (int j = 0; j < port_struct->elements()->size(); j++) {
					TermDef* pdef2 = port_struct->elements()->at(j)->isDef();
					if (!pdef2)
						continue;

					if (pdef2->name()->value() == "loc") {
						GetDefVec(port, pdef2, filename);
						port *= scale;
					}
					else if (pdef2->name()->value() == "fire") {
						int fire_i = 0;
						GetDefNumber(fire_i, pdef2, filename);
						fire = (uint32)fire_i;
					}
					else if (pdef2->name()->value() == "scale") {
						GetDefNumber(port_scale, pdef2, filename);
					}
				}

				if (port_scale <= 0.0f)
					port_scale = tscale;
			}

			if (!drive)
				drive = new Thruster(dtype, thrust, tscale);

			if (defname == "port" || defname == "port_bottom")
				drive->AddPort(Thruster::BOTTOM, port, fire, port_scale);
			else if (defname == "port_top")
				drive->AddPort(Thruster::TOP, port, fire, port_scale);
			else if (defname == "port_left")
				drive->AddPort(Thruster::LEFT, port, fire, port_scale);
			else if (defname == "port_right")
				drive->AddPort(Thruster::RIGHT, port, fire, port_scale);
			else if (defname == "port_fore")
				drive->AddPort(Thruster::FORE, port, fire, port_scale);
			else if (defname == "port_aft")
				drive->AddPort(Thruster::AFT, port, fire, port_scale);
		}
		else if (defname == "emcon_1") {
			GetDefNumber(emcon_1, pdef, filename);
		}
		else if (defname == "emcon_2") {
			GetDefNumber(emcon_2, pdef, filename);
		}
		else if (defname == "emcon_3") {
			GetDefNumber(emcon_3, pdef, filename);
		}
	}

	if (!drive)
		drive = new Thruster(dtype, thrust, tscale);

	drive->SetSourceIndex(reactors.size() - 1);
	drive->Mount(loc, size, hull);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			drive->SetDesign(sd);
	}

	if (emcon_1 >= 0 && emcon_1 <= 100)
		drive->SetEMCONPower(1, emcon_1);

	if (emcon_2 >= 0 && emcon_2 <= 100)
		drive->SetEMCONPower(2, emcon_2);

	if (emcon_3 >= 0 && emcon_3 <= 100)
		drive->SetEMCONPower(3, emcon_3);

	thruster = drive;
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseNavlight(TermStruct* val)
{
	Text   dname;
	Text   dabrv;
	Text   design_name;
	int    nlights = 0;
	float  dscale = 1.0f;
	float  period = 10.0f;
	FVector bloc[NavLight::MAX_LIGHTS];
	int    btype[NavLight::MAX_LIGHTS];
	uint32 pattern[NavLight::MAX_LIGHTS];

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		Text defname = pdef->name()->value();
		defname.setSensitive(false);

		if (defname == "name") {
			GetDefText(dname, pdef, filename);
		}
		else if (defname == "abrv") {
			GetDefText(dabrv, pdef, filename);
		}
		else if (defname == "design") {
			GetDefText(design_name, pdef, filename);
		}
		else if (defname == "scale") {
			GetDefNumber(dscale, pdef, filename);
		}
		else if (defname == "period") {
			GetDefNumber(period, pdef, filename);
		}
		else if (defname == "light") {
			if (!pdef->term() || !pdef->term()->isStruct()) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: light struct missing for ship '%s' in '%s'"),
					ANSI_TO_TCHAR(name),
					ANSI_TO_TCHAR(filename));
				continue;
			}

			TermStruct* light_struct = pdef->term()->isStruct();

			FVector loc(0.0f, 0.0f, 0.0f);
			int     t = 0;
			uint32  ptn = 0;

			for (int j = 0; j < light_struct->elements()->size(); j++) {
				TermDef* pdef2 = light_struct->elements()->at(j)->isDef();
				if (!pdef2)
					continue;

				Text defname2 = pdef2->name()->value();
				defname2.setSensitive(false);

				if (defname2 == "type") {
					GetDefNumber(t, pdef2, filename);
				}
				else if (defname2 == "loc") {
					GetDefVec(loc, pdef2, filename);
				}
				else if (defname2 == "pattern") {
					int ptn_i = 0;
					GetDefNumber(ptn_i, pdef2, filename);
					ptn = (uint32)ptn_i;
				}
			}

			if (t < 1 || t > 4)
				t = 1;

			if (nlights < NavLight::MAX_LIGHTS) {
				bloc[nlights] = loc * scale;
				btype[nlights] = t - 1;
				pattern[nlights] = ptn;
				nlights++;
			}
			else {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: Too many lights ship '%s' in '%s'"),
					ANSI_TO_TCHAR(name),
					ANSI_TO_TCHAR(filename));
			}
		}
	}

	NavLight* nav = new NavLight(period, dscale);
	if (dname.length()) nav->SetName(dname);
	if (dabrv.length()) nav->SetAbbreviation(dabrv);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			nav->SetDesign(sd);
	}

	for (int k = 0; k < nlights; k++)
		nav->AddBeacon(bloc[k], pattern[k], btype[k]);

	navlights.append(nav);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseFlightDeck(TermStruct* val)
{
	Text   dname;
	Text   dabrv;
	Text   design_name;
	float  dscale = 1.0f;
	float  az = 0.0f;
	int    etype = 0;

	bool   launch = false;
	bool   recovery = false;
	int    nslots = 0;
	int    napproach = 0;
	int    nrunway = 0;

	uint32  filters[10] = { 0 };
	FVector spots[10];
	FVector approach[FlightDeck::NUM_APPROACH_PTS];
	FVector runway[2];

	FVector loc(0, 0, 0);
	FVector start(0, 0, 0);
	FVector end(0, 0, 0);
	FVector cam(0, 0, 0);
	FVector box(0, 0, 0);

	float  cycle_time = 0.0f;
	float  size = 0.0f;
	float  hull = 0.5f;
	float  light = 0.0f;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		Text defname = pdef->name()->value();
		defname.setSensitive(false);

		if (defname == "name")
			GetDefText(dname, pdef, filename);

		else if (defname == "abrv")
			GetDefText(dabrv, pdef, filename);

		else if (defname == "design")
			GetDefText(design_name, pdef, filename);

		else if (defname == "start") {
			GetDefVec(start, pdef, filename);
			start *= (float)scale;
		}

		else if (defname == "end") {
			GetDefVec(end, pdef, filename);
			end *= (float)scale;
		}

		else if (defname == "cam") {
			GetDefVec(cam, pdef, filename);
			cam *= (float)scale;
		}

		else if (defname == "box" || defname == "bounding_box") {
			GetDefVec(box, pdef, filename);
			box *= (float)scale;
		}

		else if (defname == "approach") {
			if (napproach < FlightDeck::NUM_APPROACH_PTS) {
				GetDefVec(approach[napproach], pdef, filename);
				approach[napproach++] *= (float)scale;
			}
			else {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: flight deck approach point ignored in '%s' (max=%d)"),
					ANSI_TO_TCHAR(filename),
					FlightDeck::NUM_APPROACH_PTS);
			}
		}

		else if (defname == "runway") {
			if (nrunway < 2) {
				GetDefVec(runway[nrunway], pdef, filename);
				runway[nrunway++] *= (float)scale;
			}
		}

		else if (defname == "spot") {
			if (pdef->term()->isStruct()) {
				TermStruct* s = pdef->term()->isStruct();

				for (int j = 0; j < s->elements()->size(); j++) {
					TermDef* d = s->elements()->at(j)->isDef();
					if (!d)
						continue;

					if (d->name()->value() == "loc") {
						GetDefVec(spots[nslots], d, filename);
						spots[nslots] *= (float)scale;
					}
					else if (d->name()->value() == "filter") {
						int filter_tmp = 0;
						GetDefNumber(filter_tmp, d, filename);
						filters[nslots] = (uint32)filter_tmp;
					}
				}

				nslots++;
			}
			else if (pdef->term()->isArray()) {
				GetDefVec(spots[nslots], pdef, filename);
				spots[nslots] *= (float)scale;
				filters[nslots++] = 0xf;
			}
		}

		else if (defname == "light") {
			GetDefNumber(light, pdef, filename);
		}

		else if (defname == "cycle_time") {
			GetDefNumber(cycle_time, pdef, filename);
		}

		else if (defname == "launch") {
			GetDefBool(launch, pdef, filename);
		}

		else if (defname == "recovery") {
			GetDefBool(recovery, pdef, filename);
		}

		else if (defname == "azimuth") {
			GetDefNumber(az, pdef, filename);
			if (degrees)
				az *= (float)DEGREES;
		}

		else if (defname == "loc") {
			GetDefVec(loc, pdef, filename);
			loc *= (float)scale;
		}

		else if (defname == "size") {
			GetDefNumber(size, pdef, filename);
			size *= (float)scale;
		}

		else if (defname == "hull_factor") {
			GetDefNumber(hull, pdef, filename);
		}

		else if (defname == "explosion") {
			GetDefNumber(etype, pdef, filename);
		}
	}

	FlightDeck* deck = new FlightDeck();
	deck->Mount(loc, size, hull);

	if (dname.length()) deck->SetName(dname);
	if (dabrv.length()) deck->SetAbbreviation(dabrv);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			deck->SetDesign(sd);
	}

	if (launch)
		deck->SetLaunchDeck();
	else if (recovery)
		deck->SetRecoveryDeck();

	deck->SetAzimuth(az);
	deck->SetBoundingBox(box);
	deck->SetStartPoint(start);
	deck->SetEndPoint(end);
	deck->SetCamLoc(cam);
	deck->SetExplosionType(etype);

	if (light > 0)
		deck->SetLight(light);

	for (int a = 0; a < napproach; a++)
		deck->SetApproachPoint(a, approach[a]);

	for (int r = 0; r < nrunway; r++)
		deck->SetRunwayPoint(r, runway[r]);

	for (int s2 = 0; s2 < nslots; s2++)
		deck->AddSlot(spots[s2], filters[s2]);

	if (cycle_time > 0)
		deck->SetCycleTime(cycle_time);

	flight_decks.append(deck);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseLandingGear(TermStruct* val)
{
	Text    dname;
	Text    dabrv;
	Text    design_name;
	int     ngear = 0;

	FVector start[LandingGear::MAX_GEAR];
	FVector end[LandingGear::MAX_GEAR];
	SimModel* model[LandingGear::MAX_GEAR];

	// Ensure arrays are in a known state:
	for (int k = 0; k < LandingGear::MAX_GEAR; k++) {
		start[k] = FVector(0, 0, 0);
		end[k] = FVector(0, 0, 0);
		model[k] = 0;
	}

	for (int idx = 0; idx < val->elements()->size(); idx++) {
		TermDef* pdef_outer = val->elements()->at(idx)->isDef();
		if (!pdef_outer)
			continue;

		Text defname = pdef_outer->name()->value();
		defname.setSensitive(false);

		if (defname == "name")
			GetDefText(dname, pdef_outer, filename);

		else if (defname == "abrv")
			GetDefText(dabrv, pdef_outer, filename);

		else if (defname == "design")
			GetDefText(design_name, pdef_outer, filename);

		else if (defname == "gear") {
			if (!pdef_outer->term() || !pdef_outer->term()->isStruct()) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: gear struct missing for ship '%s' in '%s'"),
					ANSI_TO_TCHAR(name),
					ANSI_TO_TCHAR(filename));
			}
			else {
				TermStruct* gear_struct = pdef_outer->term()->isStruct();

				FVector v1(0, 0, 0);
				FVector v2(0, 0, 0);
				char    mod_name[256];

				FMemory::Memzero(mod_name, sizeof(mod_name));

				for (int subIdx = 0; subIdx < gear_struct->elements()->size(); subIdx++) {
					TermDef* pdef_inner = gear_struct->elements()->at(subIdx)->isDef();
					if (!pdef_inner)
						continue;

					Text inner_name = pdef_inner->name()->value();
					inner_name.setSensitive(false);

					if (inner_name == "model") {
						GetDefText(mod_name, pdef_inner, filename);
					}
					else if (inner_name == "start") {
						GetDefVec(v1, pdef_inner, filename);
					}
					else if (inner_name == "end") {
						GetDefVec(v2, pdef_inner, filename);
					}
				}

				if (ngear < LandingGear::MAX_GEAR) {
					SimModel* m = new SimModel;

					if (!m->Load(mod_name, scale)) {
						UE_LOG(LogShipDesign, Warning,
							TEXT("WARNING: Could not load landing gear model '%s'"),
							ANSI_TO_TCHAR(mod_name));
						delete m;
						m = 0;
					}
					else {
						model[ngear] = m;
						start[ngear] = v1 * scale;
						end[ngear] = v2 * scale;
						ngear++;
					}
				}
				else {
					UE_LOG(LogShipDesign, Warning,
						TEXT("WARNING: Too many landing gear ship '%s' in '%s'"),
						ANSI_TO_TCHAR(name),
						ANSI_TO_TCHAR(filename));
				}
			}
		}
	}

	gear = new LandingGear();
	if (dname.length()) gear->SetName(dname);
	if (dabrv.length()) gear->SetAbbreviation(dabrv);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			gear->SetDesign(sd);
	}

	for (int g = 0; g < ngear; g++)
		gear->AddGear(model[g], start[g], end[g]);
}

void
ShipDesign::ParseWeapon(TermStruct* val)
{
	Text    wtype;
	Text    wname;
	Text    wabrv;
	Text    design_name;
	Text    wgroup_name;
	int     nmuz = 0;
	FVector muzzles[Weapon::MAX_BARRELS];
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;
	float   az = 0.0f;
	float   el = 0.0f;
	float   az_max = 1e6f;
	float   az_min = 1e6f;
	float   el_max = 1e6f;
	float   el_min = 1e6f;
	float   az_rest = 1e6f;
	float   el_rest = 1e6f;
	int     etype = 0;
	int     emcon_1 = -1;
	int     emcon_2 = -1;
	int     emcon_3 = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "type")
				GetDefText(wtype, pdef, filename);

			else if (defname == "name")
				GetDefText(wname, pdef, filename);

			else if (defname == "abrv")
				GetDefText(wabrv, pdef, filename);

			else if (defname == "design")
				GetDefText(design_name, pdef, filename);

			else if (defname == "group")
				GetDefText(wgroup_name, pdef, filename);

			else if (defname == "muzzle") {
				if (nmuz < Weapon::MAX_BARRELS) {
					GetDefVec(muzzles[nmuz], pdef, filename);
					nmuz++;
				}
				else {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: too many muzzles (max=%d) for weapon in '%s'"),
						Weapon::MAX_BARRELS,
						ANSI_TO_TCHAR(filename));
				}
			}

			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}

			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}

			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}

			else if (defname == "azimuth") {
				GetDefNumber(az, pdef, filename);
				if (degrees) az *= (float)DEGREES;
			}

			else if (defname == "elevation") {
				GetDefNumber(el, pdef, filename);
				if (degrees) el *= (float)DEGREES;
			}

			else if (defname == "aim_az_max") {
				GetDefNumber(az_max, pdef, filename);
				if (degrees) az_max *= (float)DEGREES;
				az_min = 0.0f - az_max;
			}

			else if (defname == "aim_el_max") {
				GetDefNumber(el_max, pdef, filename);
				if (degrees) el_max *= (float)DEGREES;
				el_min = 0.0f - el_max;
			}

			else if (defname == "aim_az_min") {
				GetDefNumber(az_min, pdef, filename);
				if (degrees) az_min *= (float)DEGREES;
			}

			else if (defname == "aim_el_min") {
				GetDefNumber(el_min, pdef, filename);
				if (degrees) el_min *= (float)DEGREES;
			}

			else if (defname == "aim_az_rest") {
				GetDefNumber(az_rest, pdef, filename);
				if (degrees) az_rest *= (float)DEGREES;
			}

			else if (defname == "aim_el_rest") {
				GetDefNumber(el_rest, pdef, filename);
				if (degrees) el_rest *= (float)DEGREES;
			}

			else if (defname == "rest_azimuth") {
				GetDefNumber(az_rest, pdef, filename);
				if (degrees) az_rest *= (float)DEGREES;
			}

			else if (defname == "rest_elevation") {
				GetDefNumber(el_rest, pdef, filename);
				if (degrees) el_rest *= (float)DEGREES;
			}

			else if (defname == "explosion") {
				GetDefNumber(etype, pdef, filename);
			}

			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}

			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}

			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}

			else {
				UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unknown weapon parameter '%s' in '%s'"),
					ANSI_TO_TCHAR(defname.data()),
					ANSI_TO_TCHAR(filename));
			}
		}
	}

	WeaponDesign* meta = WeaponDesign::Find(wtype);
	if (!meta) {
		UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unusual weapon name '%s' in '%s'"),
			ANSI_TO_TCHAR((const char*)wtype),
			ANSI_TO_TCHAR(filename));
	}
	else {
		// non-turret weapon muzzles are relative to ship scale:
		if (meta->turret_model == 0) {
			for (int i = 0; i < nmuz; i++)
				muzzles[i] *= (float)scale;
		}

		// turret weapon muzzles are relative to weapon scale:
		else {
			for (int i = 0; i < nmuz; i++)
				muzzles[i] *= (float)meta->scale;
		}

		Weapon* gun = new  Weapon(meta, nmuz, muzzles, az, el);
		gun->SetSourceIndex(reactors.size() - 1);
		gun->Mount(loc, size, hull);

		if (az_max < 1e6f)  gun->SetAzimuthMax(az_max);
		if (az_min < 1e6f)  gun->SetAzimuthMin(az_min);
		if (az_rest < 1e6f) gun->SetRestAzimuth(az_rest);

		if (el_max < 1e6f)  gun->SetElevationMax(el_max);
		if (el_min < 1e6f)  gun->SetElevationMin(el_min);
		if (el_rest < 1e6f) gun->SetRestElevation(el_rest);

		if (emcon_1 >= 0 && emcon_1 <= 100)
			gun->SetEMCONPower(1, emcon_1);

		if (emcon_2 >= 0 && emcon_2 <= 100)
			gun->SetEMCONPower(2, emcon_2);

		if (emcon_3 >= 0 && emcon_3 <= 100)
			gun->SetEMCONPower(3, emcon_3);

		if (wname.length()) gun->SetName(wname);
		if (wabrv.length()) gun->SetAbbreviation(wabrv);

		if (design_name.length()) {
			SystemDesign* sd = SystemDesign::Find(design_name);
			if (sd)
				gun->SetDesign(sd);
		}

		if (wgroup_name.length())
			gun->SetGroup(wgroup_name);

		gun->SetExplosionType(etype);

		if (meta->decoy_type && !decoy)
			decoy = gun;
		else if (meta->probe && !probe)
			probe = gun;
		else
			weapons.append(gun);
	}

	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path_name);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseHardPoint(TermStruct* val)
{
	Text     wtypes[8];
	Text     wname;
	Text     wabrv;
	Text     design;

	FVector  muzzle(0.0f, 0.0f, 0.0f);
	FVector  loc(0.0f, 0.0f, 0.0f);

	float    size = 0.0f;
	float    hull = 0.5f;
	float    az = 0.0f;
	float    el = 0.0f;

	int      ntypes = 0;
	int      emcon_1 = -1;
	int      emcon_2 = -1;
	int      emcon_3 = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		Text defname = pdef->name()->value();
		defname.setSensitive(false);

		if (defname == "type") {
			if (ntypes < 8)
				GetDefText(wtypes[ntypes++], pdef, filename);
		}
		else if (defname == "name")
			GetDefText(wname, pdef, filename);

		else if (defname == "abrv")
			GetDefText(wabrv, pdef, filename);

		else if (defname == "design")
			GetDefText(design, pdef, filename);

		else if (defname == "muzzle") {
			GetDefVec(muzzle, pdef, filename);
			muzzle *= (float)scale;
		}

		else if (defname == "loc") {
			GetDefVec(loc, pdef, filename);
			loc *= (float)scale;
		}

		else if (defname == "size") {
			GetDefNumber(size, pdef, filename);
			size *= (float)scale;
		}

		else if (defname == "hull_factor") {
			GetDefNumber(hull, pdef, filename);
		}

		else if (defname == "azimuth") {
			GetDefNumber(az, pdef, filename);
			if (degrees)
				az *= (float)DEGREES;
		}

		else if (defname == "elevation") {
			GetDefNumber(el, pdef, filename);
			if (degrees)
				el *= (float)DEGREES;
		}

		else if (defname == "emcon_1")
			GetDefNumber(emcon_1, pdef, filename);

		else if (defname == "emcon_2")
			GetDefNumber(emcon_2, pdef, filename);

		else if (defname == "emcon_3")
			GetDefNumber(emcon_3, pdef, filename);

		else {
			UE_LOG(LogShipDesign, Warning,
				TEXT("WARNING: unknown weapon parameter '%s' in '%s'"),
				ANSI_TO_TCHAR(defname.data()),
				ANSI_TO_TCHAR(filename));
		}
	}

	HardPoint* hp = new HardPoint(muzzle, az, el);
	if (!hp)
		return;

	for (int w = 0; w < ntypes; w++) {
		WeaponDesign* meta = WeaponDesign::Find(wtypes[w]);
		if (!meta) {
			UE_LOG(LogShipDesign, Warning,
				TEXT("WARNING: unusual weapon name '%s' in '%s'"),
				ANSI_TO_TCHAR(wtypes[w].data()),
				ANSI_TO_TCHAR(filename));
		}
		else {
			hp->AddDesign(meta);
		}
	}

	hp->Mount(loc, size, hull);

	if (wname.length())  hp->SetName(wname);
	if (wabrv.length())  hp->SetAbbreviation(wabrv);
	if (design.length()) hp->SetDesign(design);

	hard_points.append(hp);

	DataLoader::GetLoader()->SetDataPath(path_name);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseLoadout(TermStruct* val)
{
	ShipLoad* load = new  ShipLoad;
	if (!load)
		return;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "name")
				GetDefText(load->name, pdef, filename);

			else if (defname == "stations")
				GetDefArray(load->load, 16, pdef, filename);

			else {
				UE_LOG(LogShipDesign, Warning, TEXT("WARNING: unknown loadout parameter '%s' in '%s'"),
					ANSI_TO_TCHAR(defname.data()),
					ANSI_TO_TCHAR(filename));
			}
		}
	}

	loadouts.append(load);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseSensor(TermStruct* val)
{
	Text    design_name;
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;
	int     nranges = 0;
	float   ranges[8];
	int     emcon_1 = -1;
	int     emcon_2 = -1;
	int     emcon_3 = -1;

	FMemory::Memzero(ranges, sizeof(ranges));

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "range") {
				if (nranges < 8)
					GetDefNumber(ranges[nranges++], pdef, filename);
				else
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: too many sensor ranges (max=8) in '%s'"),
						ANSI_TO_TCHAR(filename));
			}

			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}

			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}

			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}

			else if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}

			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}

			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}

			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}
		}
	}

	if (!sensor) {
		sensor = new  Sensor();

		if (design_name.length()) {
			SystemDesign* sd = SystemDesign::Find(design_name);
			if (sd)
				sensor->SetDesign(sd);
		}

		for (int i = 0; i < nranges; i++)
			sensor->AddRange(ranges[i]);

		if (emcon_1 >= 0 && emcon_1 <= 100)
			sensor->SetEMCONPower(1, emcon_1);

		if (emcon_2 >= 0 && emcon_2 <= 100)
			sensor->SetEMCONPower(2, emcon_2);

		if (emcon_3 >= 0 && emcon_3 <= 100)
			sensor->SetEMCONPower(3, emcon_3);

		sensor->Mount(loc, size, hull);
		sensor->SetSourceIndex(reactors.size() - 1);
	}
	else {
		UE_LOG(LogShipDesign, Warning, TEXT("WARNING: additional sensor ignored in '%s'"),
			ANSI_TO_TCHAR(filename));
	}
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseNavsys(TermStruct* val)
{
	Text    design_name;
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}
			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}
			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}
			else if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}
		}
	}

	if (!navsys) {
		navsys = new  NavSystem;

		if (design_name.length()) {
			SystemDesign* sd = SystemDesign::Find(design_name);
			if (sd)
				navsys->SetDesign(sd);
		}

		navsys->Mount(loc, size, hull);
		navsys->SetSourceIndex(reactors.size() - 1);
	}
	else {
		UE_LOG(LogShipDesign, Warning, TEXT("WARNING: additional nav system ignored in '%s'"),
			ANSI_TO_TCHAR(filename));
	}
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseComputer(TermStruct* val)
{
	Text    comp_name("Computer");
	Text    comp_abrv("Comp");
	Text    design_name;
	int     comp_type = 1;
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "name") {
				GetDefText(comp_name, pdef, filename);
			}
			else if (defname == "abrv") {
				GetDefText(comp_abrv, pdef, filename);
			}
			else if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}
			else if (defname == "type") {
				GetDefNumber(comp_type, pdef, filename);
			}
			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}
			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}
			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}
		}
	}

	Computer* comp = new  Computer(comp_type, comp_name);
	comp->Mount(loc, size, hull);
	comp->SetAbbreviation(comp_abrv);
	comp->SetSourceIndex(reactors.size() - 1);

	if (design_name.length()) {
		SystemDesign* sd = SystemDesign::Find(design_name);
		if (sd)
			comp->SetDesign(sd);
	}

	computers.append(comp);
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseShield(TermStruct* val)
{
	Text    dname;
	Text    dabrv;
	Text    design_name;
	Text    model_name;
	double  factor = 0;
	double  capacity = 0;
	double  consumption = 0;
	double  cutoff = 0;
	double  curve = 0;
	double  def_cost = 1;
	int     shield_type = 0;
	FVector loc(0.0f, 0.0f, 0.0f);
	float   size = 0.0f;
	float   hull = 0.5f;
	int     etype = 0;
	bool    shield_capacitor = false;
	bool    shield_bubble = false;
	int     emcon_1 = -1;
	int     emcon_2 = -1;
	int     emcon_3 = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "type") {
				GetDefNumber(shield_type, pdef, filename);
			}
			else if (defname == "name") {
				GetDefText(dname, pdef, filename);
			}
			else if (defname == "abrv") {
				GetDefText(dabrv, pdef, filename);
			}
			else if (defname == "design") {
				GetDefText(design_name, pdef, filename);
			}
			else if (defname == "model") {
				GetDefText(model_name, pdef, filename);
			}

			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
				loc *= (float)scale;
			}
			else if (defname == "size") {
				GetDefNumber(size, pdef, filename);
				size *= (float)scale;
			}
			else if (defname == "hull_factor") {
				GetDefNumber(hull, pdef, filename);
			}

			else if (defname.contains("factor")) {
				GetDefNumber(factor, pdef, filename);
			}
			else if (defname.contains("cutoff")) {
				GetDefNumber(cutoff, pdef, filename);
			}
			else if (defname.contains("curve")) {
				GetDefNumber(curve, pdef, filename);
			}
			else if (defname.contains("capacitor")) {
				GetDefBool(shield_capacitor, pdef, filename);
			}
			else if (defname.contains("bubble")) {
				GetDefBool(shield_bubble, pdef, filename);
			}
			else if (defname == "capacity") {
				GetDefNumber(capacity, pdef, filename);
			}
			else if (defname == "consumption") {
				GetDefNumber(consumption, pdef, filename);
			}
			else if (defname == "deflection_cost") {
				GetDefNumber(def_cost, pdef, filename);
			}
			else if (defname == "explosion") {
				GetDefNumber(etype, pdef, filename);
			}

			else if (defname == "emcon_1") {
				GetDefNumber(emcon_1, pdef, filename);
			}
			else if (defname == "emcon_2") {
				GetDefNumber(emcon_2, pdef, filename);
			}
			else if (defname == "emcon_3") {
				GetDefNumber(emcon_3, pdef, filename);
			}

			else if (defname == "bolt_hit_sound") {
				GetDefText(bolt_hit_sound, pdef, filename);
			}
			else if (defname == "beam_hit_sound") {
				GetDefText(beam_hit_sound, pdef, filename);
			}
		}
	}

	if (!shield) {
		if (shield_type) {
			shield = new  Shield((Shield::SUBTYPE)shield_type);
			shield->SetSourceIndex(reactors.size() - 1);
			shield->Mount(loc, size, hull);
			if (dname.length()) shield->SetName(dname);
			if (dabrv.length()) shield->SetAbbreviation(dabrv);

			if (design_name.length()) {
				SystemDesign* sd = SystemDesign::Find(design_name);
				if (sd)
					shield->SetDesign(sd);
			}

			shield->SetExplosionType(etype);
			shield->SetShieldCapacitor(shield_capacitor);
			shield->SetShieldBubble(shield_bubble);

			if (factor > 0) shield->SetShieldFactor(factor);
			if (capacity > 0) shield->SetCapacity(capacity);
			if (cutoff > 0) shield->SetShieldCutoff(cutoff);
			if (consumption > 0) shield->SetConsumption(consumption);
			if (def_cost > 0) shield->SetDeflectionCost(def_cost);
			if (curve > 0) shield->SetShieldCurve(curve);

			if (emcon_1 >= 0 && emcon_1 <= 100)
				shield->SetEMCONPower(1, emcon_1);

			if (emcon_2 >= 0 && emcon_2 <= 100)
				shield->SetEMCONPower(2, emcon_2);

			if (emcon_3 >= 0 && emcon_3 <= 100)
				shield->SetEMCONPower(3, emcon_3);

			if (model_name.length()) {
				shield_model = new SimModel;
				if (!shield_model->Load(model_name, scale)) {
					UE_LOG(LogShipDesign, Error, TEXT("ERROR: Could notxload shield model '%s'"),
						ANSI_TO_TCHAR(model_name.data()));
					delete shield_model;
					shield_model = 0;
					valid = false;
				}
				else {
					shield_model->SetDynamic(true);
					shield_model->SetLuminous(true);
				}
			}

			DataLoader* loader = DataLoader::GetLoader();
			const uint32 SOUND_FLAGS = USound::LOCALIZED | USound::LOC_3D;

			if (bolt_hit_sound.length()) {
				if (!loader->LoadSound(bolt_hit_sound, bolt_hit_sound_resource, SOUND_FLAGS, true)) {
					loader->SetDataPath("Sounds/");
					loader->LoadSound(bolt_hit_sound, bolt_hit_sound_resource, SOUND_FLAGS);
					loader->SetDataPath(path_name);
				}
			}

			if (beam_hit_sound.length()) {
				if (!loader->LoadSound(beam_hit_sound, beam_hit_sound_resource, SOUND_FLAGS, true)) {
					loader->SetDataPath("Sounds/");
					loader->LoadSound(beam_hit_sound, beam_hit_sound_resource, SOUND_FLAGS);
					loader->SetDataPath(path_name);
				}
			}
		}
		else {
			UE_LOG(LogShipDesign, Warning, TEXT("WARNING: invalid shield type in '%s'"),
				ANSI_TO_TCHAR(filename));
		}
	}
	else {
		UE_LOG(LogShipDesign, Warning, TEXT("WARNING: additional shield ignored in '%s'"),
			ANSI_TO_TCHAR(filename));
	}
} 

void
ShipDesign::ParseDeathSpiral(TermStruct* val)
{
	int exp_index = -1;
	int debris_index = -1;
	int fire_index = -1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* def = val->elements()->at(i)->isDef();
		if (def) {
			Text defname = def->name()->value();
			defname.setSensitive(false);

			if (defname == "time") {
				GetDefNumber(death_spiral_time, def, filename);
			}

			else if (defname == "explosion") {
				if (!def->term() || !def->term()->isStruct()) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: explosion struct missing in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
				else {
					TermStruct* v = def->term()->isStruct();
					ParseExplosion(v, ++exp_index);
				}
			}

			// BACKWARD COMPATIBILITY:
			else if (defname == "explosion_type") {
				GetDefNumber(explosion[++exp_index].type, def, filename);
			}

			else if (defname == "explosion_time") {
				GetDefNumber(explosion[exp_index].time, def, filename);
			}

			else if (defname == "explosion_loc") {
				GetDefVec(explosion[exp_index].loc, def, filename);
				explosion[exp_index].loc *= (float)scale;
			}

			else if (defname == "final_type") {
				GetDefNumber(explosion[++exp_index].type, def, filename);
				explosion[exp_index].final = true;
			}

			else if (defname == "final_loc") {
				GetDefVec(explosion[exp_index].loc, def, filename);
				explosion[exp_index].loc *= (float)scale;
			}

			else if (defname == "debris") {
				if (def->term() && def->term()->isText()) {
					Text model_name;
					GetDefText(model_name, def, filename);

					SimModel* model = new SimModel;
					if (!model->Load(model_name, scale)) {
						UE_LOG(LogShipDesign, Warning, TEXT("Could notxload debris model '%s'"),
							ANSI_TO_TCHAR(model_name.data()));
						delete model;
						return;
					}

					PrepareModel(*model);
					debris[++debris_index].model = model;
					fire_index = -1;
				}
				else if (!def->term() || !def->term()->isStruct()) {
					UE_LOG(LogShipDesign, Warning, TEXT("WARNING: debris struct missing in '%s'"),
						ANSI_TO_TCHAR(filename));
				}
				else {
					TermStruct* v = def->term()->isStruct();
					ParseDebris(v, ++debris_index);
				}
			}

			else if (defname == "debris_mass") {
				GetDefNumber(debris[debris_index].mass, def, filename);
			}

			else if (defname == "debris_speed") {
				GetDefNumber(debris[debris_index].speed, def, filename);
			}

			else if (defname == "debris_drag") {
				GetDefNumber(debris[debris_index].drag, def, filename);
			}

			else if (defname == "debris_loc") {
				GetDefVec(debris[debris_index].loc, def, filename);
				debris[debris_index].loc *= (float)scale;
			}

			else if (defname == "debris_count") {
				GetDefNumber(debris[debris_index].count, def, filename);
			}

			else if (defname == "debris_life") {
				GetDefNumber(debris[debris_index].life, def, filename);
			}

			else if (defname == "debris_fire") {
				if (++fire_index < 5) {
					GetDefVec(debris[debris_index].fire_loc[fire_index], def, filename);
					debris[debris_index].fire_loc[fire_index] *= (float)scale;
				}
			}

			else if (defname == "debris_fire_type") {
				GetDefNumber(debris[debris_index].fire_type, def, filename);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseExplosion(TermStruct* val, int index)
{
	ShipExplosion* exp = &explosion[index];

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* def = val->elements()->at(i)->isDef();
		if (def) {
			Text defname = def->name()->value();
			defname.setSensitive(false);

			if (defname == "time") {
				GetDefNumber(exp->time, def, filename);
			}

			else if (defname == "type") {
				GetDefNumber(exp->type, def, filename);
			}

			else if (defname == "loc") {
				GetDefVec(exp->loc, def, filename);
				exp->loc *= (float)scale;
			}

			else if (defname == "final") {
				GetDefBool(exp->final, def, filename);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseDebris(TermStruct* val, int index)
{
	char        model_name[NAMELEN];
	int         fire_index = 0;
	ShipDebris* deb = &debris[index];

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* def = val->elements()->at(i)->isDef();
		if (def) {
			Text defname = def->name()->value();
			defname.setSensitive(false);

			if (defname == "model") {
				GetDefText(model_name, def, filename);

				SimModel* model = new SimModel;
				if (!model->Load(model_name, scale)) {
					UE_LOG(LogShipDesign, Warning, TEXT("Could notxload debris model '%s'"),
						ANSI_TO_TCHAR(model_name));
					delete model;
					return;
				}

				PrepareModel(*model);
				deb->model = model;
			}

			else if (defname == "mass") {
				GetDefNumber(deb->mass, def, filename);
			}

			else if (defname == "speed") {
				GetDefNumber(deb->speed, def, filename);
			}

			else if (defname == "drag") {
				GetDefNumber(deb->drag, def, filename);
			}

			else if (defname == "loc") {
				GetDefVec(deb->loc, def, filename);
				deb->loc *= (float)scale;
			}

			else if (defname == "count") {
				GetDefNumber(deb->count, def, filename);
			}

			else if (defname == "life") {
				GetDefNumber(deb->life, def, filename);
			}

			else if (defname == "fire") {
				if (fire_index < 5) {
					GetDefVec(deb->fire_loc[fire_index], def, filename);
					deb->fire_loc[fire_index] *= (float)scale;
					fire_index++;
				}
			}

			else if (defname == "fire_type") {
				GetDefNumber(deb->fire_type, def, filename);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
ShipDesign::ParseMap(TermStruct* val)
{
	char  sprite_name[NAMELEN];

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "sprite") {
				GetDefText(sprite_name, pdef, filename);

				Bitmap* sprite = new Bitmap();
				DataLoader* loader = DataLoader::GetLoader();
				loader->LoadGameBitmap(sprite_name, *sprite, Bitmap::BMP_TRANSLUCENT);

				map_sprites.append(sprite);
			}
		}
	}
}

// +--------------------------------------------------------------------+
void
ShipDesign::ParseSquadron(TermStruct* val)
{
	char squad_name[NAMELEN];
	char design_name[NAMELEN];
	int  count = 4;
	int  avail = 4;

	squad_name[0] = 0;
	design_name[0] = 0;

	for (int idx = 0; idx < val->elements()->size(); idx++) {
		TermDef* pdef = val->elements()->at(idx)->isDef();
		if (!pdef)
			continue;

		Text defname = pdef->name()->value();
		defname.setSensitive(false);

		if (defname == "name") {
			GetDefText(squad_name, pdef, filename);
		}
		else if (defname == "design") {
			GetDefText(design_name, pdef, filename);
		}
		else if (defname == "count") {
			GetDefNumber(count, pdef, filename);
		}
		else if (defname == "avail") {
			GetDefNumber(avail, pdef, filename);
		}
	}

	ShipSquadron* s = new ShipSquadron;
	strcpy_s(s->name, squad_name);

	s->design = Get(design_name);
	s->count = count;
	s->avail = avail;

	squadrons.append(s);
}

// +--------------------------------------------------------------------+

Skin*
ShipDesign::ParseSkin(TermStruct* val)
{
	Skin* skin = 0;

	// Renamed from 'name' to avoid hiding ShipDesign::name (C4458)
	char  skin_name[NAMELEN];
	skin_name[0] = 0;

	for (int idx = 0; idx < val->elements()->size(); idx++) {
		TermDef* def = val->elements()->at(idx)->isDef();
		if (!def)
			continue;

		Text defname = def->name()->value();
		defname.setSensitive(false);

		if (defname == "name") {
			GetDefText(skin_name, def, filename);

			skin = new Skin(skin_name);
		}
		else if (defname == "material" || defname == "mtl") {
			if (!def->term() || !def->term()->isStruct()) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: skin material struct missing in '%s'"),
					ANSI_TO_TCHAR(filename));
			}
			else {
				TermStruct* mtl_struct = def->term()->isStruct();
				ParseSkinMtl(mtl_struct, skin);
			}
		}
	}

	if (skin && skin->NumCells()) {
		skins.append(skin);
	}
	else if (skin) {
		delete skin;
		skin = 0;
	}

	return skin;
}

void
ShipDesign::ParseSkinMtl(TermStruct* val, Skin* skin)
{
	Material* mtl = new Material;
	if (!mtl || !val) {
		delete mtl;
		return;
	}

	for (int idx = 0; idx < val->elements()->size(); idx++) {
		TermDef* def = val->elements()->at(idx)->isDef();
		if (!def)
			continue;

		Text defname = def->name()->value();
		defname.setSensitive(false);

		if (defname == "name") {
			GetDefText(mtl->name, def, filename);
		}
		else if (defname == "Ka") {
			// Unreal port: use FColor overload (or bridge helper), not legacy Color
			GetDefFColor(mtl->Ka, def, filename);
		}
		else if (defname == "Kd") {
			GetDefFColor(mtl->Kd, def, filename);
		}
		else if (defname == "Ks") {
			GetDefFColor(mtl->Ks, def, filename);
		}
		else if (defname == "Ke") {
			GetDefFColor(mtl->Ke, def, filename);
		}
		else if (defname == "Ns" || defname == "power") {
			GetDefNumber(mtl->power, def, filename);
		}
		else if (defname == "bump") {
			GetDefNumber(mtl->bump, def, filename);
		}
		else if (defname == "luminous") {
			GetDefBool(mtl->luminous, def, filename);
		}
		else if (defname == "blend") {
			if (def->term() && def->term()->isNumber()) {
				GetDefNumber(mtl->blend, def, filename);
			}
			else if (def->term() && def->term()->isText()) {
				Text blend_val;
				GetDefText(blend_val, def, filename);
				blend_val.setSensitive(false);

				if (blend_val == "alpha" || blend_val == "translucent")
					mtl->blend = Material::MTL_TRANSLUCENT;
				else if (blend_val == "additive")
					mtl->blend = Material::MTL_ADDITIVE;
				else
					mtl->blend = Material::MTL_SOLID;
			}
		}
		else if (defname.indexOf("tex_d") == 0) {
			char tex_name[64] = { 0 };
			if (!GetDefText(tex_name, def, filename)) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: invalid or missing tex_diffuse in '%s'"),
					ANSI_TO_TCHAR(filename));
			}
			else {
				DataLoader* loader = DataLoader::GetLoader();
				loader->LoadTexture(tex_name, mtl->tex_diffuse);
			}
		}
		else if (defname.indexOf("tex_s") == 0) {
			char tex_name[64] = { 0 };
			if (!GetDefText(tex_name, def, filename)) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: invalid or missing tex_specular in '%s'"),
					ANSI_TO_TCHAR(filename));
			}
			else {
				DataLoader* loader = DataLoader::GetLoader();
				loader->LoadTexture(tex_name, mtl->tex_specular);
			}
		}
		else if (defname.indexOf("tex_b") == 0) {
			char tex_name[64] = { 0 };
			if (!GetDefText(tex_name, def, filename)) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: invalid or missing tex_bumpmap in '%s'"),
					ANSI_TO_TCHAR(filename));
			}
			else {
				DataLoader* loader = DataLoader::GetLoader();
				loader->LoadTexture(tex_name, mtl->tex_bumpmap);
			}
		}
		else if (defname.indexOf("tex_e") == 0) {
			char tex_name[64] = { 0 };
			if (!GetDefText(tex_name, def, filename)) {
				UE_LOG(LogShipDesign, Warning,
					TEXT("WARNING: invalid or missing tex_emissive in '%s'"),
					ANSI_TO_TCHAR(filename));
			}
			else {
				DataLoader* loader = DataLoader::GetLoader();
				loader->LoadTexture(tex_name, mtl->tex_emissive);
			}
		}
	}

	if (skin)
		skin->AddMaterial(mtl);
	else
		delete mtl;
}

const Skin*
ShipDesign::FindSkin(const char* skin_name) const
{
	int n = skins.size();

	for (int i = 0; i < n; i++) {
		Skin* s = skins[n - 1 - i];

		if (!strcmp(s->Name(), skin_name))
			return s;
	}

	return 0;
}

ShipExplosion::ShipExplosion()
{

}
