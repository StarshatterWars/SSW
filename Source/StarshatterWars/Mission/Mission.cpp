/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Mission.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC


	OVERVIEW
	========
	Mission classes
*/

#include "Mission.h"

#include "MissionEvent.h"
#include "StarSystem.h"
#include "Galaxy.h"
#include "Starshatter.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Instruction.h"
#include "WeaponDesign.h"
#include "Sim.h"
#include "MissionLoad.h"
#include "MissionShip.h"

#include "Game.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "FormatUtil.h"
#include "Random.h"
#include "Skin.h"
#include "GameStructs.h"
#include "FormattingUtils.h"
#include "ShipDesignRegistry.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

// Logging:
DEFINE_LOG_CATEGORY_STATIC(LogStarshatterMission, Log, All);

// +--------------------------------------------------------------------+
template <typename TMissionData>
bool Mission::LoadMissionCommon(const TMissionData& InData, bool bFullReset)
{
	if (bFullReset)
	{
		ok = false;
		elements.destroy();
		events.destroy();
		errmsg = "";
		subtitles = "";
		target = nullptr;
		ward = nullptr;
	}

	if (!InData.MissionName.IsEmpty())
		name = TCHAR_TO_ANSI(*InData.MissionName);

	if (!InData.MissionSystem.IsEmpty())
		system = TCHAR_TO_ANSI(*InData.MissionSystem);

	if (!InData.MissionRegion.IsEmpty())
		region = TCHAR_TO_ANSI(*InData.MissionRegion);

	if constexpr (std::is_same_v<TMissionData, FS_CampaignMission>)
	{
		id = InData.MissionId;
		desc = TCHAR_TO_ANSI(*InData.Desc);
		script = TCHAR_TO_ANSI(*InData.Scene);
		objective = TCHAR_TO_ANSI(*InData.Objective);
		sitrep = TCHAR_TO_ANSI(*InData.Sitrep);
		type = static_cast<int32>(InData.MissionType);
		team = InData.Team;
		degrees = InData.Degrees;
		stardate = InData.Stardate;
		start = UFormattingUtils::ParseStarshatterTime(InData.StartTime);
		end = 0;
	}

	//--------------------------------------------------
	// ELEMENTS
	//--------------------------------------------------

	for (const FS_MissionElement& SrcElem : InData.Element)
	{
		MissionElement* Elem = new MissionElement();
		if (!Elem)
			continue;

		UE_LOG(LogTemp, Warning,
			TEXT("[Mission.cpp] Copy SrcElem -> Elem: Name='%s' SrcRegion='%s'"),
			*SrcElem.Name,
			*SrcElem.RegionName);

		Elem->SetName(TCHAR_TO_ANSI(*SrcElem.Name));
		Elem->SetCarrier(TCHAR_TO_ANSI(*SrcElem.Carrier));
		Elem->SetCommander(TCHAR_TO_ANSI(*SrcElem.Commander));
		Elem->SetSquadron(TCHAR_TO_ANSI(*SrcElem.Squadron));

		Elem->SetRegion(TCHAR_TO_ANSI(*SrcElem.RegionName));
		Elem->SetIFF(SrcElem.IFFCode);
		Elem->SetCount(SrcElem.Count);
		Elem->SetPlayer(SrcElem.Player ? 1 : 0);
		Elem->SetAlert(SrcElem.Alert);
		Elem->SetPlayable(SrcElem.Playable);
		Elem->SetInvulnerable(SrcElem.Invulnerable);
		Elem->SetRogue(SrcElem.Rogue);
		Elem->SetCommandAI(SrcElem.CommandAI);
		Elem->SetRespawnCount(SrcElem.Respawns);
		Elem->SetHoldTime(SrcElem.HoldTime);
		Elem->SetZoneLock(SrcElem.ZoneLock);
		Elem->SetHeading(SrcElem.Heading);

		// LOCATION
		Elem->SetLocation(SrcElem.Location);

		Elem->mission_role = static_cast<int32>(SrcElem.RoleName);
		Elem->intel = static_cast<int32>(SrcElem.Intel);

		//--------------------------------------------------
		// NAVPOINT ->INSTRUCTION BRIDGE
		//--------------------------------------------------

		for (const FS_MissionInstruction& SrcNav : SrcElem.Navpoint)
		{
			Instruction* Nav = new Instruction(
				TCHAR_TO_ANSI(*SrcNav.OrderRegionName),
				SrcNav.Location,
				INSTRUCTION_ACTION::VECTOR);

			if (!Nav)
				continue;

			Nav->SetSpeed(SrcNav.Speed);
			Nav->SetPriority(SrcNav.Priority);
			Nav->SetFarcast(SrcNav.Farcast);

			if (!SrcNav.TargetName.IsEmpty())
			{
				Nav->SetTarget(SrcNav.TargetName);
			}

			Elem->AddNavPoint(Nav);

			UE_LOG(LogTemp, Warning,
				TEXT("[Mission::LoadMissionCommon] Added navpoint Elem='%s' Cmd='%s' Region='%s' Target='%s' Loc=%s Speed=%d"),
				*SrcElem.Name,
				*SrcNav.OrderName,
				*SrcNav.OrderRegionName,
				*SrcNav.TargetName,
				*SrcNav.Location.ToString(),
				SrcNav.Speed);
		}

		//--------------------------------------------------

		AddElement(Elem);
	}

	//--------------------------------------------------
	// EVENTS
	//--------------------------------------------------

	for (const FS_MissionEvent& SrcEvent : InData.Event)
	{
		MissionEvent* Ev = new MissionEvent();
		if (!Ev)
			continue;

		Ev->id = SrcEvent.EventId;
		Ev->time = SrcEvent.EventTime;
		Ev->delay = SrcEvent.EventDelay;
		Ev->event = static_cast<int32>(SrcEvent.EventType);
		Ev->trigger = static_cast<int32>(SrcEvent.EventTrigger);
		Ev->event_ship = TCHAR_TO_ANSI(*SrcEvent.EventShip);
		Ev->event_source = TCHAR_TO_ANSI(*SrcEvent.EventSource);
		Ev->event_target = TCHAR_TO_ANSI(*SrcEvent.EventTarget);
		Ev->event_message = TCHAR_TO_ANSI(*SrcEvent.EventMessage);
		Ev->event_sound = TCHAR_TO_ANSI(*SrcEvent.EventSound);

		AddEvent(Ev);
	}

	//--------------------------------------------------
	// TARGET / WARD
	//--------------------------------------------------

	if constexpr (std::is_same_v<TMissionData, FS_CampaignMission>)
	{
		if (!InData.TargetName.IsEmpty())
			target = FindElement(TCHAR_TO_ANSI(*InData.TargetName));

		if (!InData.WardName.IsEmpty())
			ward = FindElement(TCHAR_TO_ANSI(*InData.WardName));
	}

	ok = true;
	Validate();

	UE_LOG(LogTemp, Warning,
		TEXT("[Mission.cpp] LoadMissionCommon COMPLETE Elements=%d"),
		elements.size());

	return ok;
}

Mission::Mission()
	: id(0)
{
}

Mission::Mission(int identity, const char* fname, const char* pname)
	: id(identity),
	type(0),
	team(1),
	ok(false),
	active(false),
	complete(false),
	star_system(0),
	start(33 * 3600),
	stardate(0),
	target(0),
	ward(0),
	current(0),
	degrees(false)
{
	objective = "Mission Unspecified";
	sitrep = "Mission Unknown";

	if (fname)
		strcpy_s(filename, fname);
	else
		ZeroMemory(filename, sizeof(filename));

	if (pname)
		strcpy_s(path, pname);
	else
		strcpy_s(path, "Missions/");
}

Mission::~Mission()
{
	UE_LOG(LogStarshatterMission, Verbose, TEXT("Mission::~Mission() id=%d name='%hs'"), id, name.data());
	elements.destroy();
	events.destroy();
}

// +--------------------------------------------------------------------+

const char*
Mission::GetSubtitles() const
{
	return subtitles;
}

// +--------------------------------------------------------------------+

void
Mission::AddElement(MissionElement* elem)
{
	if (elem)
		elements.append(elem);
}

// +--------------------------------------------------------------------+

MissionElement*
Mission::FindElement(const char* n)
{
	ListIter<MissionElement> iter = elements;
	while (++iter) {
		MissionElement* elem = iter.value();

		if (elem->GetName() == n)
			return elem;
	}

	return 0;
}

// +--------------------------------------------------------------------+

void
Mission::IncreaseElemPriority(int elem_index)
{
	if (elem_index > 0 && elem_index < elements.size()) {
		MissionElement* elem1 = elements.at(elem_index - 1);
		MissionElement* elem2 = elements.at(elem_index);

		elements.at(elem_index - 1) = elem2;
		elements.at(elem_index) = elem1;
	}
}

void
Mission::DecreaseElemPriority(int elem_index)
{
	if (elem_index >= 0 && elem_index < elements.size() - 1) {
		MissionElement* elem1 = elements.at(elem_index);
		MissionElement* elem2 = elements.at(elem_index + 1);

		elements.at(elem_index) = elem2;
		elements.at(elem_index + 1) = elem1;
	}
}

// +--------------------------------------------------------------------+

void
Mission::IncreaseEventPriority(int event_index)
{
	if (event_index > 0 && event_index < events.size()) {
		MissionEvent* event1 = events.at(event_index - 1);
		MissionEvent* event2 = events.at(event_index);

		events.at(event_index - 1) = event2;
		events.at(event_index) = event1;
	}
}

void
Mission::DecreaseEventPriority(int event_index)
{
	if (event_index >= 0 && event_index < events.size() - 1) {
		MissionEvent* event1 = events.at(event_index);
		MissionEvent* event2 = events.at(event_index + 1);

		events.at(event_index) = event2;
		events.at(event_index + 1) = event1;
	}
}

// +--------------------------------------------------------------------+

void
Mission::SetStarSystem(StarSystem* s)
{
	if (star_system != s) {
		star_system = s;

		if (!system_list.contains(s))
			system_list.append(s);
	}
}

void
Mission::ClearSystemList()
{
	star_system = 0;
	system_list.clear();
}

// +--------------------------------------------------------------------+

void Mission::SetPlayer(MissionElement* player_element)
{
	if (!player_element)
	{
		return;
	}

	bool bFound = false;

	ListIter<MissionElement> elem = elements;
	while (++elem)
	{
		MissionElement* element = elem.value();
		if (element == player_element)
		{
			element->player = 1;
			bFound = true;
		}
		else
		{
			element->player = 0;
		}
	}

	if (!bFound)
	{
		AddElement(player_element);

		ListIter<MissionElement> elem2 = elements;
		while (++elem2)
		{
			MissionElement* element = elem2.value();
			element->player = (element == player_element) ? 1 : 0;
		}
	}
}

MissionElement*
Mission::GetPlayer()
{
	MissionElement* p = 0;

	ListIter<MissionElement> elem = elements;
	while (++elem) {
		if (elem->player > 0)
			p = elem.value();
	}

	return p;
}

// +--------------------------------------------------------------------+

MissionEvent*
Mission::FindEvent(int event_type) const
{
	Mission* pThis = (Mission*)this;
	ListIter<MissionEvent> iter = pThis->events;
	while (++iter) {
		MissionEvent* event = iter.value();

		if (event->Event() == event_type)
			return event;
	}

	return 0;
}

void
Mission::AddEvent(MissionEvent* event)
{
	if (event)
		events.append(event);
}

// +--------------------------------------------------------------------+

bool
Mission::ParseMission(const char* block)
{
	Parser parser(new BlockReader(block));
	Term* term = parser.ParseTerm();
	char   err[256];

	if (!term) {
		sprintf_s(err, "ERROR: could not parse '%s'\n", filename);
		AddError(err);
		return ok;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "MISSION") {
			sprintf_s(err, "ERROR: invalid mission file '%s'\n", filename);
			AddError(err);
			term->print(10);
			return ok;
		}
	}

	ok = true;

	char  target_name[256];
	char  ward_name[256];

	target_name[0] = 0;
	ward_name[0] = 0;

	do {
		delete term; term = 0;
		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				Text defname = def->name()->value();
				defname.setSensitive(false);

				if (defname == "name") {
					GetDefText(name, def, filename);
					name = Game::GetText(name);
				}

				else if (defname == "desc") {
					GetDefText(desc, def, filename);
					if (desc.length() > 0 && desc.length() < 32)
						desc = Game::GetText(desc);
				}

				else if (defname == "type") {
					char typestr[64];
					GetDefText(typestr, def, filename);
					type = TypeFromName(typestr);
				}

				else if (defname == "system") {
					char  sysname[64];
					GetDefText(sysname, def, filename);

					Galaxy* galaxyx = Galaxy::GetInstance();

					if (galaxyx) {
						SetStarSystem(galaxyx->GetSystem(sysname));
					}
				}

				else if (defname == "degrees")
					GetDefBool(degrees, def, filename);

				else if (defname == "region")
					GetDefText(region, def, filename);

				else if (defname == "objective") {
					GetDefText(objective, def, filename);
					if (objective.length() > 0 && objective.length() < 32)
						objective = Game::GetText(objective);
				}

				else if (defname == "sitrep") {
					GetDefText(sitrep, def, filename);
					if (sitrep.length() > 0 && sitrep.length() < 32)
						sitrep = Game::GetText(sitrep);
				}

				else if (defname == "subtitles") {
					Text        subtitles_path;
					DataLoader* loader = DataLoader::GetLoader();
					BYTE* sub_block = 0;

					GetDefText(subtitles_path, def, filename);
					loader->SetDataPath(0);
					loader->LoadBuffer(subtitles_path, sub_block, true);

					subtitles = Text("\n") + (const char*)sub_block;

					loader->ReleaseBuffer(sub_block);
				}

				else if (defname == "start")
					GetDefTime(start, def, filename);

				else if (defname == "stardate")
					GetDefNumber(stardate, def, filename);

				else if (defname == "team")
					GetDefNumber(team, def, filename);

				else if (defname == "target")
					GetDefText(target_name, def, filename);

				else if (defname == "ward")
					GetDefText(ward_name, def, filename);

				else if ((defname == "element") ||
					(defname == "ship") ||
					(defname == "station")) {

					if (!def->term() || !def->term()->isStruct()) {
						sprintf_s(err, "ERROR: element struct missing in '%s'\n", filename);
						AddError(err);
					}
					else {
						TermStruct* val = def->term()->isStruct();
						MissionElement* elem = ParseElement(val);
						AddElement(elem);
					}
				}

				else if (defname == "event") {
					if (!def->term() || !def->term()->isStruct()) {
						sprintf_s(err, "ERROR: event struct missing in '%s'\n", filename);
						AddError(err);
					}
					else {
						TermStruct* val = def->term()->isStruct();
						MissionEvent* event = ParseEvent(val);
						AddEvent(event);
					}
				}
			}     // def
		}        // term
	} while (term);

	if (ok) {
		if (target_name[0])
			target = FindElement(target_name);

		if (ward_name[0])
			ward = FindElement(ward_name);
	}

	return ok;
}

// +--------------------------------------------------------------------+

bool
Mission::Save()
{
	Validate();

	if (!filename[0] || !path[0]) {
		AddError(Game::GetText("Mission.error.no-file"));
		return ok;
	}

	Text content = Serialize();

	if (content.length() < 8) {
		AddError(Game::GetText("Mission.error.no-serial"));
		return ok;
	}

	// Preserve legacy behavior: missions are saved relative to the current working data root.
	// Prefer UE-safe directory creation via IPlatformFile:
	const FString BaseDir = FString(ANSI_TO_TCHAR(path));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

	if (!_stricmp(path, "mods/missions/")) {
		PF.CreateDirectoryTree(*(FString(TEXT("Mods"))));
		PF.CreateDirectoryTree(*(FString(TEXT("Mods/Missions"))));
	}
	else if (!_stricmp(path, "multiplayer/")) {
		PF.CreateDirectoryTree(*(FString(TEXT("Multiplayer"))));
	}

	char fname[256];
	sprintf_s(fname, "%s%s", path, filename);

	FILE* f = nullptr;
	fopen_s(&f, fname, "w");
	if (f) {
		fwrite(content.data(), content.length(), 1, f);
		fclose(f);
	}

	return ok;
}

// +--------------------------------------------------------------------+

void
Mission::Validate()
{
	char err[256];

	ok = true;

	if (elements.isEmpty())
	{
		const FString Err = FString::Printf(
			TEXT("Mission error: no elements in '%s'"),
			*FString(ANSI_TO_TCHAR(filename))
		);

		AddError(TCHAR_TO_ANSI(*Err));
	}
	else {
		bool found_player = false;

		for (int i = 0; i < elements.size(); i++) {
			MissionElement* elem = elements.at(i);

			if (elem->GetName().length() < 1) {
				sprintf_s(err, Game::GetText("Mission.error.unnamed-elem").data(), filename);
				AddError(err);
			}

			if (elem->IsPlayer()) {
				if (!found_player) {
					found_player = true;

					if (elem->GetRegion() != GetRegion()) {
						sprintf_s(err, Game::GetText("Mission.error.wrong-sector").data(),
							elem->GetName().data(),
							GetRegion());
						AddError(err);
					}
				}
				else {
					sprintf_s(err, Game::GetText("Mission.error.extra-player").data(),
						elem->GetName().data(),
						filename);
					AddError(err);
				}
			}
		}

		if (!found_player) {
			sprintf_s(err, Game::GetText("Mission.error.no-player").data(), filename);
			AddError(err);
		}
	}
}

void
Mission::AddError(Text err)
{
	UE_LOG(LogStarshatterMission, Warning, TEXT("%hs"), err.data());
	errmsg += err;

	ok = false;
}

// +--------------------------------------------------------------------+

EMISSIONTYPE Mission::EnumFromName(const char* n)
{
	if (!n || !*n)
	{
		return EMISSIONTYPE::OTHER;
	}

	const FString Name = ANSI_TO_TCHAR(n);

	static const TMap<FString, EMISSIONTYPE> MissionTypeMap =
	{
		{TEXT("PATROL"), EMISSIONTYPE::PATROL},
		{TEXT("SWEEP"), EMISSIONTYPE::SWEEP},
		{TEXT("INTERCEPT"), EMISSIONTYPE::INTERCEPT},
		{TEXT("AIR_PATROL"), EMISSIONTYPE::AIR_PATROL},
		{TEXT("AIR_SWEEP"), EMISSIONTYPE::AIR_SWEEP},
		{TEXT("AIR_INTERCEPT"), EMISSIONTYPE::AIR_INTERCEPT},
		{TEXT("STRIKE"), EMISSIONTYPE::STRIKE},
		{TEXT("ASSAULT"), EMISSIONTYPE::ASSAULT},
		{TEXT("DEFEND"), EMISSIONTYPE::DEFEND},
		{TEXT("ESCORT"), EMISSIONTYPE::ESCORT},
		{TEXT("ESCORT_FREIGHT"), EMISSIONTYPE::ESCORT_FREIGHT},
		{TEXT("ESCORT_SHUTTLE"), EMISSIONTYPE::ESCORT_SHUTTLE},
		{TEXT("ESCORT_STRIKE"), EMISSIONTYPE::ESCORT_STRIKE},
		{TEXT("INTEL"), EMISSIONTYPE::INTEL},
		{TEXT("SCOUT"), EMISSIONTYPE::SCOUT},
		{TEXT("RECON"), EMISSIONTYPE::RECON},
		{TEXT("BLOCKADE"), EMISSIONTYPE::BLOCKADE},
		{TEXT("FLEET"), EMISSIONTYPE::FLEET},
		{TEXT("BOMBARDMENT"), EMISSIONTYPE::BOMBARDMENT},
		{TEXT("FLIGHT_OPS"), EMISSIONTYPE::FLIGHT_OPS},
		{TEXT("TRANSPORT"), EMISSIONTYPE::TRANSPORT},
		{TEXT("CARGO"), EMISSIONTYPE::CARGO},
		{TEXT("TRAINING"), EMISSIONTYPE::TRAINING},
		{TEXT("OTHER"), EMISSIONTYPE::OTHER}
	};

	const FString UpperName = Name.ToUpper();

	if (const EMISSIONTYPE* FoundType = MissionTypeMap.Find(UpperName))
	{
		return *FoundType;
	}

	for (int32 i = static_cast<int32>(EMISSIONTYPE::PATROL);
		i <= static_cast<int32>(EMISSIONTYPE::OTHER);
		++i)
	{
		if (!FCStringAnsi::Stricmp(n, GetRoleName(i)))
		{
			return static_cast<EMISSIONTYPE>(i);
		}
	}

	return EMISSIONTYPE::OTHER;
}

int32 Mission::TypeFromName(const char* n)
{
	if (!n || !*n)
	{
		return -1;
	}

	const FString Name = ANSI_TO_TCHAR(n);

	static const TMap<FString, EMISSIONTYPE> MissionTypeMap =
	{
		{TEXT("PATROL"), EMISSIONTYPE::PATROL},
		{TEXT("SWEEP"), EMISSIONTYPE::SWEEP},
		{TEXT("INTERCEPT"), EMISSIONTYPE::INTERCEPT},
		{TEXT("AIR_PATROL"), EMISSIONTYPE::AIR_PATROL},
		{TEXT("AIR_SWEEP"), EMISSIONTYPE::AIR_SWEEP},
		{TEXT("AIR_INTERCEPT"), EMISSIONTYPE::AIR_INTERCEPT},
		{TEXT("STRIKE"), EMISSIONTYPE::STRIKE},
		{TEXT("ASSAULT"), EMISSIONTYPE::ASSAULT},
		{TEXT("DEFEND"), EMISSIONTYPE::DEFEND},
		{TEXT("ESCORT"), EMISSIONTYPE::ESCORT},
		{TEXT("ESCORT_FREIGHT"), EMISSIONTYPE::ESCORT_FREIGHT},
		{TEXT("ESCORT_SHUTTLE"), EMISSIONTYPE::ESCORT_SHUTTLE},
		{TEXT("ESCORT_STRIKE"), EMISSIONTYPE::ESCORT_STRIKE},
		{TEXT("INTEL"), EMISSIONTYPE::INTEL},
		{TEXT("SCOUT"), EMISSIONTYPE::SCOUT},
		{TEXT("RECON"), EMISSIONTYPE::RECON},
		{TEXT("BLOCKADE"), EMISSIONTYPE::BLOCKADE},
		{TEXT("FLEET"), EMISSIONTYPE::FLEET},
		{TEXT("BOMBARDMENT"), EMISSIONTYPE::BOMBARDMENT},
		{TEXT("FLIGHT_OPS"), EMISSIONTYPE::FLIGHT_OPS},
		{TEXT("TRANSPORT"), EMISSIONTYPE::TRANSPORT},
		{TEXT("CARGO"), EMISSIONTYPE::CARGO},
		{TEXT("TRAINING"), EMISSIONTYPE::TRAINING},
		{TEXT("OTHER"), EMISSIONTYPE::OTHER}
	};

	const FString UpperName = Name.ToUpper();

	if (const EMISSIONTYPE* FoundType = MissionTypeMap.Find(UpperName))
	{
		return static_cast<int32>(*FoundType);
	}

	for (int32 i = static_cast<int32>(EMISSIONTYPE::PATROL);
		i <= static_cast<int32>(EMISSIONTYPE::OTHER);
		i++)
	{
		if (!FCStringAnsi::Stricmp(n, GetRoleName(i)))
		{
			return i;
		}
	}

	return -1;
}

// +--------------------------------------------------------------------+

static int elem_id = 351;

MissionElement*
Mission::ParseElement(TermStruct* val)
{
	Text  design;
	Text  skin_name;
	Text  role_name;
	int   deck = 1;
	char  err[256];

	MissionElement* element = new MissionElement();
	const FString ExistingRegion = FString(ANSI_TO_TCHAR(element->GetRegion())).TrimStartAndEnd();

	if (ExistingRegion.IsEmpty())
	{
		element->SetRegion(region);
	}
	element->elem_id = elem_id++;

	current = element;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "name")
				GetDefText(element->name, pdef, filename);

			else if (defname == "carrier")
				GetDefText(element->carrier, pdef, filename);

			else if (defname == "commander")
				GetDefText(element->commander, pdef, filename);

			else if (defname == "squadron")
				GetDefText(element->squadron, pdef, filename);

			else if (defname == "path")
				GetDefText(element->path, pdef, filename);

			else if (defname == "design") {
				GetDefText(design, pdef, filename);
				const FShipDesign* DesignRow = nullptr;

				if (design.length() > 0)
				{
					DesignRow = ShipDesignRegistry::Find(design.data());
				}

				element->SetShipDesign(DesignRow);

				if (!DesignRow) {
					sprintf_s(err, Game::GetText("Mission.error.unknown-ship").data(), design.data(), filename);
					AddError(err);
				}
			}

			else if (defname == "skin") {
				if (!element->ship_design) {
					sprintf_s(err, Game::GetText("Mission.error.out-of-order").data(), filename);
					AddError(err);
				}

				else if (pdef->term()->isText()) {
					GetDefText(skin_name, pdef, filename);

					// Skin lookup is still legacy-only. Until skins are migrated to FShipDesign,
					// leave skin unset here.
					element->skin = nullptr;
				}

				else if (pdef->term()->isStruct()) {
					sprintf_s(err, Game::GetText("Mission.error.bad-skin").data(), filename);
					AddError(err);
				}
			}

			else if (defname == "mission") {
				GetDefText(role_name, pdef, filename);
				element->mission_role = TypeFromName(role_name);
			}

			else if (defname == "intel") {
				GetDefText(role_name, pdef, filename);
				element->intel = Intel::IntelFromName(role_name);
			}

			else if (defname == "loc") {
				Vec3 loc;
				GetDefVec(loc, pdef, filename);
				element->SetLocation(FVector((float)loc.X, (float)loc.Y, (float)loc.Z));
			}

			else if (defname == "rloc") {
				if (pdef->term()->isStruct()) {
					RLoc* rloc = ParseRLoc(pdef->term()->isStruct());
					element->SetRLoc(*rloc);
					delete rloc;
				}
			}

			else if (defname.indexOf("head") == 0) {
				if (pdef->term()->isArray()) {
					Vec3 head;
					GetDefVec(head, pdef, filename);
					if (degrees) head.Z *= (float)DEGREES;
					element->heading = head.Z;
				}
				else if (pdef->term()->isNumber()) {
					double heading = 0;
					GetDefNumber(heading, pdef, filename);
					if (degrees) heading *= DEGREES;
					element->heading = heading;
				}
			}

			else if (defname == "region" || defname == "rgn")
				GetDefText(element->rgn_name, pdef, filename);

			else if (defname == "iff")
				GetDefNumber(element->IFF_code, pdef, filename);

			else if (defname == "count")
				GetDefNumber(element->count, pdef, filename);

			else if (defname == "maint_count")
				GetDefNumber(element->maint_count, pdef, filename);

			else if (defname == "dead_count")
				GetDefNumber(element->dead_count, pdef, filename);

			else if (defname == "player")
				GetDefNumber(element->player, pdef, filename);

			else if (defname == "alert")
				GetDefBool(element->alert, pdef, filename);

			else if (defname == "playable")
				GetDefBool(element->playable, pdef, filename);

			else if (defname == "rogue")
				GetDefBool(element->rogue, pdef, filename);

			else if (defname == "invulnerable")
				GetDefBool(element->invulnerable, pdef, filename);

			else if (defname == "command_ai")
				GetDefNumber(element->command_ai, pdef, filename);

			else if (defname.indexOf("respawn") == 0)
				GetDefNumber(element->respawns, pdef, filename);

			else if (defname.indexOf("hold") == 0)
				GetDefNumber(element->hold_time, pdef, filename);

			else if (defname.indexOf("zone") == 0) {
				if (pdef->term() && pdef->term()->isBool()) {
					bool locked = false;
					GetDefBool(locked, pdef, filename);
					element->zone_lock = locked;
				}
				else {
					GetDefNumber(element->zone_lock, pdef, filename);
				}
			}

			else if (defname == "objective") {
				if (!pdef->term() || !pdef->term()->isStruct()) {
					sprintf_s(err, Game::GetText("Mission.error.no-objective").data(), element->name.data(), filename);
					AddError(err);
				}
				else {
					TermStruct* v = pdef->term()->isStruct();
					Instruction* obj = ParseInstruction(v, element);
					element->objectives.append(obj);
				}
			}

			else if (defname == "instr") {
				Text* obj = new Text;
				if (GetDefText(*obj, pdef, filename))
					element->instructions.append(obj);
				else
					delete obj;
			}

			else if (defname == "ship") {
				if (!pdef->term() || !pdef->term()->isStruct()) {
					sprintf_s(err, Game::GetText("Mission.error.no-ship").data(), element->name.data(), filename);
					AddError(err);
				}
				else {
					TermStruct* v = pdef->term()->isStruct();
					MissionShip* s = ParseShip(v, element);
					element->ships.append(s);

					if (s->GetIntegrity() < 0 && element->GetShipDesign())
						s->SetIntegrity(element->GetShipDesign()->Integrity);
				}
			}

			else if (defname == "order" || defname == "navpt") {
				if (!pdef->term() || !pdef->term()->isStruct()) {
					sprintf_s(err, Game::GetText("Mission.error.no-navpt").data(), element->name.data(), filename);
					AddError(err);
				}
				else {
					TermStruct* v = pdef->term()->isStruct();
					Instruction* npt = ParseInstruction(v, element);
					element->navlist.append(npt);
				}
			}

			else if (defname == "loadout") {
				if (!pdef->term() || !pdef->term()->isStruct()) {
					sprintf_s(err, Game::GetText("Mission.error.no-loadout").data(), element->name.data(), filename);
					AddError(err);
				}
				else {
					TermStruct* v = pdef->term()->isStruct();
					ParseLoadout(v, element);
				}
			}
		}
	}

	if (element->name.length() < 1) {
		sprintf_s(err, Game::GetText("Mission.error.unnamed-elem").data(), filename);
		AddError(err);
	}

	else if (element->GetShipDesign() == nullptr) {
		sprintf_s(err, Game::GetText("Mission.error.unknown-ship").data(), element->name.data(), filename);
		AddError(err);
	}

	current = 0;

	return element;
}

MissionEvent*
Mission::ParseEvent(TermStruct* val)
{
	MissionEvent* event = new MissionEvent;
	Text           event_name;
	Text           trigger_name;
	static int     event_id = 1;
	static double  event_time = 0;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "event") {
				GetDefText(event_name, pdef, filename);
				event->event = MissionEvent::EventForName(event_name);
			}

			else if (defname == "trigger") {
				GetDefText(trigger_name, pdef, filename);
				event->trigger = MissionEvent::TriggerForName(trigger_name);
			}

			else if (defname == "id")
				GetDefNumber(event_id, pdef, filename);

			else if (defname == "time")
				GetDefNumber(event_time, pdef, filename);

			else if (defname == "delay")
				GetDefNumber(event->delay, pdef, filename);

			else if (defname == "event_param" || defname == "param" || defname == "color") {

				if (pdef->term()->isNumber()) {
					GetDefNumber(event->event_param[0], pdef, filename);
					event->event_nparams = 1;
				}

				else if (pdef->term()->isArray()) {
					std::vector<float> plist;
					GetDefArray(plist, pdef, filename);

					for (int j = 0; j < 10 && j < (int)plist.size(); j++) {
						float f = plist[j];
						event->event_param[j] = (int)f;
						event->event_nparams = j + 1;
					}
				}
			}

			else if (defname == "trigger_param") {
				if (pdef->term()->isNumber()) {
					GetDefNumber(event->trigger_param[0], pdef, filename);
					event->trigger_nparams = 1;
				}

				else if (pdef->term()->isArray()) {
					std::vector<float> plist;
					GetDefArray(plist, pdef, filename);

					for (int j = 0; j < 10 && j < (int)plist.size(); j++) {
						float f = plist[j];
						event->trigger_param[j] = (int)f;
						event->trigger_nparams = j + 1;
					}
				}
			}

			else if (defname == "event_ship" || defname == "ship")
				GetDefText(event->event_ship, pdef, filename);

			else if (defname == "event_source" || defname == "source" || defname == "font")
				GetDefText(event->event_source, pdef, filename);

			else if (defname == "event_target" || defname == "target" || defname == "image")
				GetDefText(event->event_target, pdef, filename);

			else if (defname == "event_message" || defname == "message") {
				Text raw_msg;
				GetDefText(raw_msg, pdef, filename);
				raw_msg = Game::GetText(raw_msg);
				event->event_message = FormatTextEscape(raw_msg);
			}

			else if (defname == "event_chance" || defname == "chance")
				GetDefNumber(event->event_chance, pdef, filename);

			else if (defname == "event_sound" || defname == "sound")
				GetDefText(event->event_sound, pdef, filename);

			else if (defname == "loc" || defname == "vec" || defname == "fade")
				GetDefVec(event->event_point, pdef, filename);

			else if (defname == "rect")
				GetDefRect(event->event_rect, pdef, filename);

			else if (defname == "trigger_ship")
				GetDefText(event->trigger_ship, pdef, filename);

			else if (defname == "trigger_target")
				GetDefText(event->trigger_target, pdef, filename);
		}
	}

	event->id = event_id++;
	event->time = event_time;
	return event;
}

MissionShip*
Mission::ParseShip(TermStruct* Val, MissionElement* Element)
{
	MissionShip* MissionShipObj = new MissionShip;

	Text Name;
	Text SkinName;
	Text RegNum;
	Text Region;
	char ErrorText[256];

	Vec3 Location(-1.0e9f, -1.0e9f, -1.0e9f);
	Vec3 Velocity(-1.0e9f, -1.0e9f, -1.0e9f);

	int Respawns = -1;
	double Heading = -1e9;
	double Integrity = -1;

	int Ammo[16];
	int Fuel[4];

	for (int32 AmmoIndex = 0; AmmoIndex < 16; ++AmmoIndex) {
		Ammo[AmmoIndex] = -10;
	}

	for (int32 FuelIndex = 0; FuelIndex < 4; ++FuelIndex) {
		Fuel[FuelIndex] = -10;
	}

	for (int32 ElementIndex = 0; ElementIndex < (int32)Val->elements()->size(); ++ElementIndex) {
		TermDef* Def = Val->elements()->at(ElementIndex)->isDef();
		if (Def) {
			Text DefName = Def->name()->value();
			DefName.setSensitive(false);

			if (DefName == "name") {
				GetDefText(Name, Def, filename);
			}

			else if (DefName == "skin") {
				if (!Element || !Element->GetShipDesign()) {
					sprintf_s(ErrorText, Game::GetText("Mission.error.out-of-order").data(), filename);
					AddError(ErrorText);
				}

				else if (Def->term()->isText()) {
					GetDefText(SkinName, Def, filename);

					// Temporary: skins not yet migrated to FShipDesign
					//MissionShipObj->GetSkin() = nullptr;
				}
			}

			else if (DefName == "regnum") {
				GetDefText(RegNum, Def, filename);
			}

			else if (DefName == "region") {
				GetDefText(Region, Def, filename);
			}

			else if (DefName == "loc") {
				GetDefVec(Location, Def, filename);
			}

			else if (DefName == "velocity") {
				GetDefVec(Velocity, Def, filename);
			}

			else if (DefName == "respawns") {
				GetDefNumber(Respawns, Def, filename);
			}

			else if (DefName == "heading") {
				if (Def->term()->isArray()) {
					Vec3 HeadingVec;
					GetDefVec(HeadingVec, Def, filename);

					if (degrees) {
						HeadingVec.Z *= (float)DEGREES;
					}

					Heading = HeadingVec.Z;
				}
				else if (Def->term()->isNumber()) {
					double HeadingValue = 0.0;
					GetDefNumber(HeadingValue, Def, filename);

					if (degrees) {
						HeadingValue *= DEGREES;
					}

					Heading = HeadingValue;
				}
			}

			else if (DefName == "integrity") {
				GetDefNumber(Integrity, Def, filename);
			}

			else if (DefName == "ammo") {
				GetDefArray(Ammo, 16, Def, filename);
			}

			else if (DefName == "fuel") {
				GetDefArray(Fuel, 4, Def, filename);
			}
		}
	}

	MissionShipObj->SetName(Name);
	MissionShipObj->SetRegNum(RegNum);
	MissionShipObj->SetRegion(Region);
	MissionShipObj->SetIntegrity(Integrity);

	if (Location.X > -1e9f) {
		MissionShipObj->SetLocation(FVector((float)Location.X, (float)Location.Y, (float)Location.Z));
	}

	if (Velocity.X > -1e9f) {
		MissionShipObj->SetVelocity(FVector((float)Velocity.X, (float)Velocity.Y, (float)Velocity.Z));
	}

	if (Respawns > -1) {
		MissionShipObj->SetRespawns(Respawns);
	}

	if (Heading > -1e9) {
		MissionShipObj->SetHeading(Heading);
	}

	if (Ammo[0] > -10) {
		MissionShipObj->SetAmmo(Ammo);
	}

	if (Fuel[0] > -10) {
		MissionShipObj->SetFuel(Fuel);
	}

	return MissionShipObj;
}

Instruction*
Mission::ParseInstruction(TermStruct* val, MissionElement* element)
{
	INSTRUCTION_ACTION order = INSTRUCTION_ACTION::VECTOR;
	INSTRUCTION_STATUS status = INSTRUCTION_STATUS::PENDING;
	INSTRUCTION_FORMATION formation = INSTRUCTION_FORMATION::NONE;
	
	int FormationValue = static_cast<int>(formation);

	int   speed = 0;
	int   priority = 1;
	int   farcast = 0;
	int   hold = 0;
	int   emcon = 0;
	Vec3  loc(0, 0, 0);
	RLoc* rloc = 0;
	Text  order_name;
	Text  status_name;
	Text  order_rgn_name;
	Text  tgt_name;
	Text  tgt_desc;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "cmd") {
				GetDefText(order_name, pdef, filename);

				for (int cmd = 0; cmd < (int) INSTRUCTION_ACTION::NUM_ACTIONS; cmd++) {
					const INSTRUCTION_ACTION Action =
						static_cast<INSTRUCTION_ACTION>(cmd);

					if (!_stricmp(order_name, Instruction::ActionName(Action))) {
						order = Action;
						break;
					}
				}
			}

			else if (defname == "status") {
				GetDefText(status_name, pdef, filename);

				for (int n = 0; n < (int)INSTRUCTION_STATUS::NUM_STATUS; n++) {
					const INSTRUCTION_STATUS Stat =
						static_cast<INSTRUCTION_STATUS>(n);

					if (!_stricmp(status_name, Instruction::StatusName(Stat))) {
						status = Stat;
						break;
					}
				}
			}

			else if (defname == "loc") {
				GetDefVec(loc, pdef, filename);
			}

			else if (defname == "rloc") {
				if (pdef->term()->isStruct())
					rloc = ParseRLoc(pdef->term()->isStruct());
			}

			else if (defname == "rgn") {
				GetDefText(order_rgn_name, pdef, filename);
			}
			else if (defname == "speed") {
				GetDefNumber(speed, pdef, filename);
			}
			else if (defname == "formation") {
				GetDefNumber(FormationValue, pdef, filename);
			}
			else if (defname == "emcon") {
				GetDefNumber(emcon, pdef, filename);
			}
			else if (defname == "priority") {
				GetDefNumber(priority, pdef, filename);
			}
			else if (defname == "farcast") {
				if (pdef->term()->isBool()) {
					bool f = false;
					GetDefBool(f, pdef, filename);
					farcast = f;
				}
				else {
					GetDefNumber(farcast, pdef, filename);
				}
			}
			else if (defname == "tgt") {
				GetDefText(tgt_name, pdef, filename);
			}
			else if (defname == "tgt_desc") {
				GetDefText(tgt_desc, pdef, filename);
			}
			else if (defname.indexOf("hold") == 0) {
				GetDefNumber(hold, pdef, filename);
			}
		}
	}

	Text rgn;

	if (order_rgn_name.length() > 0)
		rgn = order_rgn_name;

	else if (element->navlist.size() > 0)
		rgn = element->navlist[element->navlist.size() - 1]->GetRegionName();

	else
		rgn = region;

	if (tgt_desc.length() && tgt_name.length())
		tgt_desc = tgt_desc + " " + tgt_name;

	Instruction* instr = new Instruction(rgn, FVector((float)loc.X, (float)loc.Y, (float)loc.Z), order);

	instr->SetStatus(status);
	instr->SetEMCON(emcon);
	instr->SetFormation(formation);
	instr->SetSpeed(speed);
	instr->SetTarget(FString(tgt_name.data()));
	instr->SetTargetDesc(tgt_desc);
	instr->SetPriority(priority - 1);
	instr->SetFarcast(farcast);
	instr->SetHoldTime(hold);

	if (rloc) {
		instr->GetRLoc() = *rloc;
		delete rloc;
	}

	return instr;
}

void
Mission::ParseLoadout(TermStruct* Val, MissionElement* Element)
{
	int ShipIndex = -1;
	int Stations[16];
	Text LoadoutName;

	ZeroMemory(Stations, sizeof(Stations));

	for (int32 ElementIndex = 0; ElementIndex < (int32)Val->elements()->size(); ++ElementIndex) {
		TermDef* Def = Val->elements()->at(ElementIndex)->isDef();
		if (Def) {
			Text DefName = Def->name()->value();
			DefName.setSensitive(false);

			if (DefName == "ship") {
				GetDefNumber(ShipIndex, Def, filename);
			}
			else if (DefName == "name") {
				GetDefText(LoadoutName, Def, filename);
			}
			else if (DefName == "stations") {
				GetDefArray(Stations, 16, Def, filename);
			}
		}
	}

	MissionLoad* Load = new MissionLoad(ShipIndex);

	if (LoadoutName.length()) {
		Load->SetName(LoadoutName);
	}

	for (int32 StationIndex = 0; StationIndex < 16; ++StationIndex) {
		Load->SetStation(StationIndex, Stations[StationIndex]);
	}

	Element->loadouts.append(Load);
}


RLoc*
Mission::ParseRLoc(TermStruct* val)
{
	Vec3     base_loc;
	RLoc* rloc = new RLoc;
	RLoc* ref = 0;

	double   dex = 0;
	double   dex_var = 5e3;
	double   az = 0;
	double   az_var = 3.1415;
	double   el = 0;
	double   el_var = 0.1;

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (pdef) {
			Text defname = pdef->name()->value();
			defname.setSensitive(false);

			if (defname == "dex") {
				GetDefNumber(dex, pdef, filename);
				rloc->SetDistance(dex);
			}
			else if (defname == "dex_var") {
				GetDefNumber(dex_var, pdef, filename);
				rloc->SetDistanceVar(dex_var);
			}
			else if (defname == "az") {
				GetDefNumber(az, pdef, filename);
				if (degrees) az *= DEGREES;
				rloc->SetAzimuth(az);
			}
			else if (defname == "az_var") {
				GetDefNumber(az_var, pdef, filename);
				if (degrees) az_var *= DEGREES;
				rloc->SetAzimuthVar(az_var);
			}
			else if (defname == "el") {
				GetDefNumber(el, pdef, filename);
				if (degrees) el *= DEGREES;
				rloc->SetElevation(el);
			}
			else if (defname == "el_var") {
				GetDefNumber(el_var, pdef, filename);
				if (degrees) el_var *= DEGREES;
				rloc->SetElevationVar(el_var);
			}
			else if (defname == "loc") {
				GetDefVec(base_loc, pdef, filename);
				rloc->SetBaseLocation(FVector((float)base_loc.X, (float)base_loc.Y, (float)base_loc.Z));
			}

			else if (defname == "ref") {
				Text refstr;
				GetDefText(refstr, pdef, filename);

				int sep = refstr.indexOf(':');

				if (sep >= 0) {
					Text elem_name = refstr.substring(0, sep);
					Text nav_name = refstr.substring(sep + 1, refstr.length());
					MissionElement* elem = 0;

					if (elem_name == "this")
						elem = current;
					else
						elem = FindElement(elem_name);

					if (elem && elem->NavList().size() > 0) {
						int index = atoi(nav_name) - 1;
						if (index < 0)
							index = 0;
						else if (index >= elem->NavList().size())
							index = elem->NavList().size() - 1;

						ref = &elem->NavList()[index]->GetRLoc();
						rloc->SetReferenceLoc(ref);
					}
					else {
						UE_LOG(LogStarshatterMission, Warning, TEXT("No ref found for rloc '%hs' in elem '%hs'"), refstr.data(), current ? current->GetName().data() : "");
						rloc->SetBaseLocation(RandomPoint());
					}
				}
				else {
					MissionElement* elem = 0;

					if (refstr == "this")
						elem = current;
					else
						elem = FindElement(refstr);

					if (elem) {
						ref = &elem->GetRLoc();
						rloc->SetReferenceLoc(ref);
					}
					else {
						UE_LOG(LogStarshatterMission, Warning, TEXT("No ref found for rloc '%hs' in elem '%hs'"), refstr.data(), current ? current->GetName().data() : "");
						rloc->SetBaseLocation(RandomPoint());
					}
				}
			}
		}
	}

	return rloc;
}

const char* Mission::GetRoleName(int32 role)
{
	switch (static_cast<EMISSIONTYPE>(role))
	{
	case EMISSIONTYPE::PATROL:         return "Patrol";
	case EMISSIONTYPE::SWEEP:          return "Sweep";
	case EMISSIONTYPE::INTERCEPT:      return "Intercept";
	case EMISSIONTYPE::AIR_PATROL:     return "Airborne Patrol";
	case EMISSIONTYPE::AIR_SWEEP:      return "Airborne Sweep";
	case EMISSIONTYPE::AIR_INTERCEPT:  return "Airborne Intercept";
	case EMISSIONTYPE::STRIKE:         return "Strike";
	case EMISSIONTYPE::ASSAULT:        return "Assault";
	case EMISSIONTYPE::DEFEND:         return "Defend";
	case EMISSIONTYPE::ESCORT:         return "Escort";
	case EMISSIONTYPE::ESCORT_FREIGHT: return "Freight Escort";
	case EMISSIONTYPE::ESCORT_SHUTTLE: return "Shuttle Escort";
	case EMISSIONTYPE::ESCORT_STRIKE:  return "Strike Escort";
	case EMISSIONTYPE::INTEL:          return "Intel";
	case EMISSIONTYPE::SCOUT:          return "Scout";
	case EMISSIONTYPE::RECON:          return "Recon";
	case EMISSIONTYPE::BLOCKADE:       return "Blockade";
	case EMISSIONTYPE::FLEET:          return "Fleet";
	case EMISSIONTYPE::BOMBARDMENT:    return "Attack";
	case EMISSIONTYPE::FLIGHT_OPS:     return "Flight Ops";
	case EMISSIONTYPE::TRANSPORT:      return "Transport";
	case EMISSIONTYPE::CARGO:          return "Cargo";
	case EMISSIONTYPE::TRAINING:       return "Training";

	case EMISSIONTYPE::OTHER:
	default:
		return "Misc";
	}
}
// +--------------------------------------------------------------------+

Text
Mission::Serialize(const char* player_elem, int player_index)
{
	Text s = "MISSION\n\nname:   \"";
	s += SafeString(GetName());

	if (desc.length()) {
		s += "\"\ndesc:   \"";
		s += SafeString(desc);
	}

	s += "\"\ntype:   \"";
	s += SafeString(GetTypeName());

	ListIter<StarSystem> sys_iter = system_list;
	while (++sys_iter) {
		StarSystem* sys = sys_iter.value();

		if (sys != star_system) {
			s += "\"\nsystem: \"";
			s += sys->GetName();
		}
	}

	s += "\"\nsystem: \"";
	if (GetStarSystem())
		s += SafeString(GetStarSystem()->GetName());
	else
		s += "null";

	ListIter<MissionElement> iter = GetElements();

	Sim* sim = Sim::GetSim();
	if (sim && sim->GetElements().size() > 0)
		iter = sim->GetMissionElements();

	s += "\"\n";

	bool region_set = false;
	if (player_elem && *player_elem) {
		// set the mission region to that of the active player
		while (++iter) {
			MissionElement* e = iter.value();
			if (e->GetName() == player_elem) {
				char buf[32];
				sprintf_s(buf, "team: %d\n", e->GetIFF());
				s += buf;

				s += "region: \"";
				s += SafeString(e->GetRegion());
				s += "\"\n\n";

				region_set = true;
				break;
			}
		}

		iter.reset();
	}

	if (!region_set) {
		s += "region: \"";
		s += SafeString(GetRegion());
		s += "\"\n\n";
	}

	if (GetObjective() && *GetObjective()) {
		s += "objective: \"";
		s += SafeString(GetObjective());
		s += "\"\n\n";
	}

	if (GetSituation() && *GetSituation()) {
		s += "sitrep: \"";
		s += SafeString(GetSituation());
		s += "\"\n\n";
	}

	char buffer[256];
	FormatTime(buffer, GetStart());

	s += "start: \"";
	s += buffer;
	s += "\"\n\n";
	s += "degrees: true\n\n";

	while (++iter) {
		MissionElement* elem = iter.value();

		s += "element: {\n";
		s += "   name:      \"";
		s += SafeString(elem->GetName());
		s += "\"\n";

		if (elem->GetPath().length()) {
			s += "   path:      \"";
			s += SafeString(elem->GetPath());
			s += "\"\n";
		}

		if (elem->GetShipDesign()) {
			s += "   design:    \"";
			s += TCHAR_TO_ANSI(*elem->GetShipDesign()->ShipName);
			s += "\"\n";
		}

		if (elem->GetSkin()) {
			s += "   skin:      \"";
			s += SafeString(elem->GetSkin()->Name());
			s += "\"\n";
		}

		if (elem->GetSquadron().length()) {
			s += "   squadron:  \"";
			s += SafeString(elem->GetSquadron());
			s += "\"\n";
		}

		if (elem->GetCarrier().length()) {
			s += "   carrier:   \"";
			s += SafeString(elem->GetCarrier());
			s += "\"\n";
		}

		if (elem->GetCommander().length()) {
			s += "   commander: \"";
			s += SafeString(elem->GetCommander());
			s += "\"\n";
		}

		s += "   mission:   \"";
		s += elem->GetRoleName();
		s += "\"\n\n";

		if (elem->IntelLevel()) {
			s += "   intel:     \"";
			s += Intel::NameFromIntel(elem->IntelLevel());
			s += "\"\n";
		}

		sprintf_s(buffer, "   count:     %d\n", elem->Count());
		s += buffer;

		if (elem->MaintCount()) {
			sprintf_s(buffer, "   maint_count: %d\n", elem->MaintCount());
			s += buffer;
		}

		if (elem->DeadCount()) {
			sprintf_s(buffer, "   dead_count:  %d\n", elem->DeadCount());
			s += buffer;
		}

		if (elem->RespawnCount()) {
			sprintf_s(buffer, "   respawn_count:  %d\n", elem->RespawnCount());
			s += buffer;
		}

		if (elem->HoldTime()) {
			sprintf_s(buffer, "   hold_time:  %d\n", elem->HoldTime());
			s += buffer;
		}

		if (elem->GetZoneLock()) {
			sprintf_s(buffer, "   zone_lock:  %d\n", elem->GetZoneLock());
			s += buffer;
		}

		if (elem->IsAlert()) {
			s += "   alert:     true\n";
		}

		if (!elem->IsSquadron()) {
			sprintf_s(buffer, "   command_ai:%d\n", elem->GetCommandAI());
			s += buffer;
		}

		sprintf_s(buffer, "   iff:       %d\n", elem->GetIFF());
		s += buffer;

		if (player_elem) {
			if (elem->GetName() == player_elem) {
				if (player_index < 1)
					player_index = 1;

				sprintf_s(buffer, "   player:    %d\n", player_index);
				s += buffer;
			}
		}

		else {
			if (elem->IsPlayer()) {
				sprintf_s(buffer, "   player:    %d\n", elem->IsPlayer());
				s += buffer;
			}
		}

		if (!elem->IsSquadron()) {
			if (elem->IsPlayable())
				s += "   playable:  true\n";
			else
				s += "   playable:  false\n";
		}

		s += "   region:    \"";
		s += elem->GetRegion();
		s += "\"\n";

		const FVector ElemLoc = elem->GetLocation();
		sprintf_s(buffer, "   loc:       (%.0f, %.0f, %.0f)\n",
			ElemLoc.X, ElemLoc.Y, ElemLoc.Z);
		s += buffer;

		if (elem->GetHeading() != 0) {
			sprintf_s(buffer, "   head:      %d\n", (int)(elem->GetHeading() / DEGREES));
			s += buffer;
		}

		if (elem->GetLoadouts().size()) {
			s += "\n";

			ListIter<MissionLoad> load_iter = elem->GetLoadouts();
			while (++load_iter) {
				MissionLoad* load = load_iter.value();

				sprintf_s(buffer, "   loadout:   { ship: %d, ", load->GetShip());
				s += buffer;

				if (load->GetName().length()) {
					s += "name: \"";
					s += SafeString(load->GetName());
					s += "\" }\n";
				}
				else {
					s += "stations: (";

					for (int i = 0; i < 16; i++) {
						sprintf_s(buffer, "%d", load->GetStation(i));
						s += buffer;

						if (i < 15)
							s += ", ";
					}

					s += ") }\n";
				}
			}
		}

		if (elem->GetObjectives().size()) {
			s += "\n";

			ListIter<Instruction> obj_iter = elem->GetObjectives();
			while (++obj_iter) {
				Instruction* inst = obj_iter.value();

				s += "   objective: { cmd: ";
				s += Instruction::ActionName(inst->GetAction());
				s += ", tgt: \"";
				s += SafeString(inst->GetTargetName());
				s += "\" }\n";
			}
		}

		if (elem->NavList().size()) {
			s += "\n";

			ListIter<Instruction> nav_iter = elem->NavList();
			while (++nav_iter) {
				Instruction* inst = nav_iter.value();

				s += "   navpt:     { cmd: ";
				s += Instruction::ActionName(inst->GetAction());
				s += ", status: ";
				s += Instruction::StatusName(inst->GetStatus());

				if (inst->GetTargetName() && *inst->GetTargetName()) {
					s += ", tgt: \"";
					s += SafeString(inst->GetTargetName());
					s += "\"";
				}

				const FVector NLoc = inst->GetLocation();
				sprintf_s(buffer, ", loc: (%.0f, %.0f, %.0f), speed: %d",
					NLoc.X, NLoc.Y, NLoc.Z,
					inst->GetSpeed());
				s += buffer;

				if (inst->GetRegionName() && *inst->GetRegionName()) {
					s += ", rgn: \"";
					s += inst->GetRegionName();
					s += "\"";
				}

				if (inst->GetHoldTime()) {
					sprintf_s(buffer, ", hold: %d", (int)inst->GetHoldTime());
					s += buffer;
				}

				if (inst->GetFarcast()) {
					s += ", farcast: true";
				}

				if (inst->GetFormation() > INSTRUCTION_FORMATION::DIAMOND) {
					sprintf_s(buffer, ", formation: %d", (int)inst->GetFormation());
					s += buffer;
				}

				if (inst->GetPriority() > Instruction::PRIMARY) {
					sprintf_s(buffer, ", priority: %d", (int)inst->GetPriority());
					s += buffer;
				}

				s += " }\n";
			}
		}

		if (elem->GetInstructions().size()) {
			s += "\n";

			ListIter<Text> i_iter = elem->GetInstructions();
			while (++i_iter) {
				s += "   instr:     \"";
				s += SafeString(*i_iter.value());
				s += "\"\n";
			}
		}

		if (elem->GetShips().size()) {
			ListIter<MissionShip> s_iter = elem->GetShips();
			while (++s_iter) {
				MissionShip* ship = s_iter.value();

				s += "\n   ship: {\n";

				if (ship->GetName().length()) {
					s += "      name:      \"";
					s += SafeString(ship->GetName());
					s += "\"\n";
				}

				if (ship->GetRegNum().length()) {
					s += "      regnum:    \"";
					s += SafeString(ship->GetRegNum());
					s += "\"\n";
				}

				if (ship->GetRegion().length()) {
					s += "      region:    \"";
					s += SafeString(ship->GetRegion());
					s += "\"\n";
				}

				if (fabs(ship->GetLocation().X) < 1e9) {
					sprintf_s(buffer, "      loc:       (%.0f, %.0f, %.0f),\n",
						ship->GetLocation().X,
						ship->GetLocation().Y,
						ship->GetLocation().Z);
					s += buffer;
				}

				if (fabs(ship->GetVelocity().X) < 1e9) {
					sprintf_s(buffer, "      velocity:  (%.1f, %.1f, %.1f),\n",
						ship->GetVelocity().X,
						ship->GetVelocity().Y,
						ship->GetVelocity().Z);
					s += buffer;
				}

				if (ship->GetRespawns() > -1) {
					sprintf_s(buffer, "      respawns:  %d,\n", ship->GetRespawns());
					s += buffer;
				}

				if (ship->GetHeading() > -1e9) {
					sprintf_s(buffer, "      heading:   %d,\n", (int)(ship->GetHeading() / DEGREES));
					s += buffer;
				}

				if (ship->GetIntegrity() > -1) {
					sprintf_s(buffer, "      integrity: %d,\n", (int)ship->GetIntegrity());
					s += buffer;
				}

				if (ship->GetDecoys() > -1) {
					sprintf_s(buffer, "      decoys:    %d,\n", ship->GetDecoys());
					s += buffer;
				}

				if (ship->GetProbes() > -1) {
					sprintf_s(buffer, "      probes:    %d,\n", ship->GetProbes());
					s += buffer;
				}

				if (ship->GetAmmo()[0] > -10) {
					s += "\n      ammo: (";

					for (int i = 0; i < 16; i++) {
						sprintf_s(buffer, "%d", ship->GetAmmo()[i]);
						s += buffer;

						if (i < 15)
							s += ", ";
					}

					s += ")\n";
				}

				if (ship->GetFuel()[0] > -10) {
					s += "\n      fuel: (";

					for (int i = 0; i < 4; i++) {
						sprintf_s(buffer, "%d", ship->GetFuel()[i]);
						s += buffer;

						if (i < 3)
							s += ", ";
					}

					s += ")\n";
				}

				s += "   }\n";
			}
		}

		s += "}\n\n";
	}

	ListIter<MissionEvent> iter2 = GetEvents();
	while (++iter2) {
		MissionEvent* event = iter2.value();

		s += "event: {\n";

		s += "   id:              ";
		sprintf_s(buffer, "%d", event->EventID());
		s += buffer;
		s += ",\n   time:            ";
		sprintf_s(buffer, "%.1f", event->Time());
		s += buffer;
		s += ",\n   delay:           ";
		sprintf_s(buffer, "%.1f", event->Delay());
		s += buffer;
		s += ",\n   event:           ";
		s += event->EventName();
		s += "\n";

		if (event->EventShip().length()) {
			s += "   event_ship:      \"";
			s += SafeString(event->EventShip());
			s += "\"\n";
		}

		if (event->EventSource().length()) {
			s += "   event_source:    \"";
			s += SafeString(event->EventSource());
			s += "\"\n";
		}

		if (event->EventTarget().length()) {
			s += "   event_target:    \"";
			s += SafeString(event->EventTarget());
			s += "\"\n";
		}

		if (event->EventSound().length()) {
			s += "   event_sound:     \"";
			s += SafeString(event->EventSound());
			s += "\"\n";
		}

		if (event->EventMessage().length()) {
			s += "   event_message:   \"";
			s += SafeString(event->EventMessage());
			s += "\"\n";
		}

		if (event->EventParam()) {
			sprintf_s(buffer, "%d", event->EventParam());
			s += "   event_param:     ";
			s += buffer;
			s += "\n";
		}

		if (event->EventChance()) {
			sprintf_s(buffer, "%d", event->EventChance());
			s += "   event_chance:    ";
			s += buffer;
			s += "\n";
		}

		s += "   trigger:         \"";
		s += event->TriggerName();
		s += "\"\n";

		if (event->TriggerShip().length()) {
			s += "   trigger_ship:    \"";
			s += SafeString(event->TriggerShip());
			s += "\"\n";
		}

		if (event->TriggerTarget().length()) {
			s += "   trigger_target:  \"";
			s += SafeString(event->TriggerTarget());
			s += "\"\n";
		}

		Text param_str = event->TriggerParamStr();

		if (param_str.length()) {
			s += "   trigger_param:   ";
			s += param_str;
			s += "\n";
		}

		s += "}\n\n";
	}

	s += "// EOF\n";

	return s;
}

bool Mission::LoadFromCampaignMissionData(const FS_CampaignMission& InMission)
{
	return LoadMissionCommon(InMission, true);
}

bool Mission::LoadFromTemplateMissionData(const FS_TemplateMission& InTemplate)
{
	return LoadMissionCommon(InTemplate, false);
}

// +====================================================================+

static int elem_idkey = 1;

MissionElement::MissionElement()
	: id(elem_idkey++),
	elem_id(0),
	ship_design(nullptr),
	skin(0),
	count(1),
	maint_count(0),
	dead_count(0),
	IFF_code(0),
	player(0),
	command_ai(1),
	alert(false),
	playable(false),
	rogue(false),
	invulnerable(false),
	respawns(0),
	hold_time(0),
	zone_lock(0),
	heading(0),
	mission_role((int)EMISSIONTYPE::OTHER),
	intel(Intel::SECRET),
	combat_group(0),
	combat_unit(0)
{
}

MissionElement::~MissionElement()
{
	ships.destroy();
	objectives.destroy();
	instructions.destroy();
	navlist.destroy();
	loadouts.destroy();
}

Text
MissionElement::GetAbbreviation() const
{
	if (ship_design && !ship_design->Abrv.IsEmpty())
		return TCHAR_TO_ANSI(*ship_design->Abrv);

	return "UNK";
}

Text
MissionElement::GetShipName(int index) const
{
	if (index < 0 || index >= ships.size()) {
		if (count > 1) {
			char sname[256];
			sprintf_s(sname, "%s %d", (const char*)name, index + 1);
			return sname;
		}
		else {
			return name;
		}
	}

	return ships.at(index)->GetName();
}

Text
MissionElement::GetRegistry(int index) const
{
	if (index < 0 || index >= ships.size()) {
		return Text();
	}

	return ships.at(index)->GetRegNum();
}

Text
MissionElement::GetRoleName() const
{
	return Mission::GetRoleName(mission_role);
}

FColor
MissionElement::GetMarkerColor() const
{
	return Ship::IFFColor(IFF_code);
}

bool
MissionElement::IsStatic() const
{
	int design_type = 0;
	if (GetShipDesign())
		design_type = GetShipDesign()->ShipType;

	return design_type >= (int)CLASSIFICATION::STATION;
}

bool
MissionElement::IsGroundUnit() const
{
	int design_type = 0;
	if (GetShipDesign())
		design_type = GetShipDesign()->ShipType;

	return (design_type & (int)CLASSIFICATION::GROUND_UNITS) ? true : false;
}

bool
MissionElement::IsStarship() const
{
	int design_type = 0;
	if (GetShipDesign())
		design_type = GetShipDesign()->ShipType;

	return (design_type & (int)CLASSIFICATION::STARSHIPS) ? true : false;
}

bool
MissionElement::IsDropship() const
{
	int design_type = 0;
	if (GetShipDesign())
		design_type = GetShipDesign()->ShipType;

	return (design_type & (int)CLASSIFICATION::DROPSHIPS) ? true : false;
}

bool
MissionElement::IsCarrier() const
{
	const FShipDesign* d = GetShipDesign();
	if (d && d->FlightDeck.Num() > 0)
		return true;

	return false;
}

bool
MissionElement::IsSquadron() const
{
	if (carrier.length() > 0)
		return true;

	return false;
}

// +--------------------------------------------------------------------+

FVector
MissionElement::GetLocation() const
{
	MissionElement* pThis = (MissionElement*)this;
	// NOTE: RLoc remains a Starshatter core type; we convert Point/Vec3 to FVector at the boundary.
	const FVector p = pThis->rloc.Location();
	return FVector((float)p.X, (float)p.Y, (float)p.Z);
}

void
MissionElement::SetLocation(const FVector& l)
{
	rloc.SetBaseLocation(FVector(l.X, l.Y, l.Z));
	rloc.SetReferenceLoc(0);
	rloc.SetDistance(0);
}

void
MissionElement::SetRLoc(const RLoc& r)
{
	rloc = r;
}

// +----------------------------------------------------------------------+

void
MissionElement::AddNavPoint(Instruction* pt, Instruction* afterPoint)
{
	if (pt && !navlist.contains(pt)) {
		if (afterPoint) {
			int index = navlist.index(afterPoint);

			if (index > -1)
				navlist.insert(pt, index + 1);
			else
				navlist.append(pt);
		}

		else {
			navlist.append(pt);
		}
	}
}

void
MissionElement::DelNavPoint(Instruction* pt)
{
	if (pt)
		delete navlist.remove(pt);
}

void
MissionElement::ClearFlightPlan()
{
	navlist.destroy();
}

// +----------------------------------------------------------------------+

int
MissionElement::GetNavIndex(const Instruction* n)
{
	int index = 0;

	if (navlist.size() > 0) {
		ListIter<Instruction> navpt = navlist;
		while (++navpt) {
			index++;
			if (navpt.value() == n)
				return index;
		}
	}

	return 0;
}

void Mission::InitializeFromInfo(const MissionInfo& Info)
{
	SetIdentity(Info.id);
	SetName(Info.name.data());
	SetDescription(Info.description.data());
	SetSystem(Info.system.data());
	SetRegion(Info.region.data());
	SetObjective(Info.objective.data());
	SetSituation(Info.situation.data());
	SetScriptName(Info.script.data());
	SetStart(Info.start);
	SetType(Info.type);
	SetSource(Info.Source);

	UE_LOG(LogTemp, Warning,
		TEXT("[Mission] InitializeFromInfo: Id=%d Name='%s' Source=%d Scripted=%s"),
		Info.id,
		ANSI_TO_TCHAR(Info.name.data()),
		(int32)GetSource(),
		IsScripted() ? TEXT("true") : TEXT("false"));
}