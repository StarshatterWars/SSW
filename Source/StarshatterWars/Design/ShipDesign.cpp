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
#include "ShipDesignRegistry.h"
#include "Skin.h"

// Runtime designs are owned by ShipDesignRegistry, populated from ships.json.
// DEF import and migration live in the editor ingestion subsystem.
static TArray<TUniquePtr<Text>> RegistryDesignNames;

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

ShipDesign::ShipDesign()
	:
    type(0),
    auto_roll(1),
    valid(false),
    secret(false),
    lod_levels(0),
    cockpit_model(0),
    sensor(0),
    navsys(0),
    shield(0),
    decoy(0),
    probe(0),
    gear(0),
    bolt_hit_sound_resource(0),
    beam_hit_sound_resource(0)
{
	for (int i = 0; i < 4; i++)
		feature_size[i] = 0.0f;
}


ShipDesign::~ShipDesign() {}
const char* ShipDesign::DisplayName() const { return display_name[0]?display_name:name; }
void ShipDesign::Initialize() {}
void ShipDesign::Close() { RegistryDesignNames.Reset(); }
bool ShipDesign::CheckName(const char* Name) { return Name && ShipDesignRegistry::FindLegacy(Name)!=nullptr; }
ShipDesign* ShipDesign::Get(const char* Name,const char*)
{
    return Name && Name[0]?ShipDesignRegistry::FindLegacy(Name):nullptr;
}
ShipDesign* ShipDesign::GetDesignName(const char* Name,const char* Path) { return Get(Name,Path); }
int ShipDesign::GetDesignList(int Type,List<Text>& Designs)
{
    Designs.clear();RegistryDesignNames.Reset();
    TArray<FName> Keys;ShipDesignRegistry::GetAll().GetKeys(Keys);Keys.Sort(FNameLexicalLess());
    for(const FName Key:Keys)
        if(const ShipDesign* Design=ShipDesignRegistry::FindLegacy(Key);Design && Design->valid && !Design->secret && (Design->type & Type))
        {
            RegistryDesignNames.Add(MakeUnique<Text>(Design->name));
            Designs.append(RegistryDesignNames.Last().Get());
        }
    return Designs.size();
}

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

const char* ShipDesign::GetDesignClass(int type)
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


