/*
    Project Starshatter Wars
    Fractal Dev Games
    Copyright (C) 2024-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterGameDataSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation for the global game data loader and registry subsystem.

    This file is a direct port of the legacy GameDataLoader actor logic
    into a GameInstanceSubsystem. It performs no Tick() and has no world
    presence. Loading is triggered explicitly during EGameMode::INIT.
*/

#include "StarshatterGameDataSubsystem.h"

#include "SSWGameInstance.h"
#include "Starshatter.h"
#include "Campaign.h"
#include "Game.h"
#include "Starsystem.h"
#include "Galaxy.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatRoster.h"
#include "CombatAction.h"
#include "CombatEvent.h"
#include "Combatant.h"
#include "Mission.h"
#include "Intel.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "PlayerData.h"

#include "Engine/DataTable.h"
#include "FormattingUtils.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Logging/LogMacros.h"

#include "StarshatterAssetRegistrySubsystem.h"

template<typename TEnum>
static bool FStringToEnum(const FString& InString, TEnum& OutEnum, bool bCaseSensitive = true)
{
    UEnum* Enum = StaticEnum<TEnum>();
    if (!Enum)
        return false;

    for (int32 i = 0; i < Enum->NumEnums(); ++i)
    {
        const FString Name = Enum->GetNameStringByIndex(i);
        if ((bCaseSensitive && Name == InString) ||
            (!bCaseSensitive && Name.Equals(InString, ESearchCase::IgnoreCase)))
        {
            OutEnum = static_cast<TEnum>(Enum->GetValueByIndex(i));
            return true;
        }
    }

    return false;
}

template <typename TEnum>
static FString EnumToDisplayNameString(TEnum EnumValue)
{
    static_assert(TIsEnum<TEnum>::Value, "EnumToDisplayNameString only works with UENUMs.");

    UEnum* EnumPtr = StaticEnum<TEnum>();
    if (!EnumPtr)
        return TEXT("Invalid");

    return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(EnumValue)).ToString();
}

static FIntRect ToFIntRect(const Rect& r)
{
	// Adjust if Rect is (x,y,w,h) instead of (l,t,r,b).
	return FIntRect((int32)r.x, (int32)r.y, (int32)(r.x + r.w), (int32)(r.y + r.h));
}

static FMargin ToFMargin(const Insets& in)
{
	return FMargin((float)in.left, (float)in.top, (float)in.right, (float)in.bottom);
}

static FString NormalizeCombatGroupTypeToken(const FString& InValue)
{
	FString Out = InValue;
	Out = Out.TrimStartAndEnd();
	Out = Out.ToUpper();

	Out.ReplaceInline(TEXT("-"), TEXT("_"));
	Out.ReplaceInline(TEXT(" "), TEXT("_"));
	Out.ReplaceInline(TEXT("/"), TEXT("_"));

	while (Out.Contains(TEXT("__")))
	{
		Out.ReplaceInline(TEXT("__"), TEXT("_"));
	}

	return Out;
}

static bool IsCivilianFamilyGroupType(ECOMBATGROUP_TYPE Type)
{
	switch (Type)
	{
	case ECOMBATGROUP_TYPE::CIVILIAN:
	case ECOMBATGROUP_TYPE::WAR_PRODUCTION:
	case ECOMBATGROUP_TYPE::FACTORY:
	case ECOMBATGROUP_TYPE::REFINERY:
	case ECOMBATGROUP_TYPE::RESOURCE:
	case ECOMBATGROUP_TYPE::INFRASTRUCTURE:
	case ECOMBATGROUP_TYPE::TRANSPORT:
	case ECOMBATGROUP_TYPE::NETWORK:
	case ECOMBATGROUP_TYPE::HABITAT:
	case ECOMBATGROUP_TYPE::STORAGE:
	case ECOMBATGROUP_TYPE::NON_COM:
	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:
		return true;

	default:
		return false;
	}
}

static void DumpGroupTree(CombatGroup* Group, int Depth = 0)
{
	if (!Group) return;

	FString Indent;
	for (int i = 0; i < Depth; ++i)
	{
		Indent += TEXT("  ");
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatRoster] %sGroup %s Type=%d Units=%d Components=%d"),
		*Indent,
		ANSI_TO_TCHAR(Group->GetName()),
		(int32)Group->GetType(),
		Group->GetUnits().size(),
		Group->GetComponents().size());

	ListIter<CombatGroup> CompIter = Group->GetComponents();
	while (++CompIter)
	{
		DumpGroupTree(CompIter.value(), Depth + 1);
	}
}

void UStarshatterGameDataSubsystem::Tick(float DeltaTime)
{
	Galaxy* GalaxyPtr = Galaxy::GetInstance();
	if (GalaxyPtr)
	{
		GalaxyPtr->ExecFrame();
	}

	Starshatter* Stars = Starshatter::GetInstance();
	const bool bInCutscene = Stars ? Stars->InCutscene() : false;

	if (!bInCutscene)
	{
		Campaign* CampaignPtr = Campaign::GetCampaign();
		if (CampaignPtr)
		{
			CampaignPtr->ExecFrame();
		}
	}
}

bool UStarshatterGameDataSubsystem::IsTickable() const
{
	return true;
}

TStatId UStarshatterGameDataSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStarshatterGameDataSubsystem, STATGROUP_Tickables);
}

void BuildRankCache(const UDataTable* RanksTable, TMap<int32, FRankInfo>& OutById)
{
	OutById.Reset();
	if (!RanksTable) return;

	static const FString Ctx(TEXT("BuildRankCache"));
	TArray<FRankInfo*> Rows;
	RanksTable->GetAllRows<FRankInfo>(Ctx, Rows);

	for (const FRankInfo* Row : Rows)
		if (Row && Row->RankId > 0)
			OutById.Add(Row->RankId, *Row);
}

void BuildMedalCache(const UDataTable* MedalsTable, TMap<int32, FMedalInfo>& OutById)
{
	OutById.Reset();
	if (!MedalsTable) return;

	static const FString Ctx(TEXT("BuildMedalCache"));
	TArray<FMedalInfo*> Rows;
	MedalsTable->GetAllRows<FMedalInfo>(Ctx, Rows);

	for (const FMedalInfo* Row : Rows)
		if (Row && Row->MedalId > 0) // or AwardId
			OutById.Add(Row->MedalId, *Row);
}

void UStarshatterGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[GAMEDATA] Initialize: GameInstance is null"));
		return;
	}

	UStarshatterAssetRegistrySubsystem* Assets = GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
	if (!Assets)
	{
		UE_LOG(LogTemp, Error, TEXT("[GAMEDATA] Initialize: AssetRegistry subsystem missing"));
		return;
	}

	// Resolve DataTables via Asset Registry (Project Settings bindings)
	CampaignDataTable = Assets->GetDataTable(TEXT("Data.CampaignTable"), true);
	CampaignActionDataTable = Assets->GetDataTable(TEXT("Data.CampaignActionTable"), true);
	CampaignOOBDataTable = Assets->GetDataTable(TEXT("Data.CampaignOOBTable"), true);
	CombatGroupDataTable = Assets->GetDataTable(TEXT("Data.CombatGroupTable"), true);
	OrderOfBattleDataTable = Assets->GetDataTable(TEXT("Data.OrderOfBattleTable"), true);
	MedalsDataTable = Assets->GetDataTable(TEXT("Data.MedalsTable"), true);
	RanksDataTable = Assets->GetDataTable(TEXT("Data.RanksTable"), true);
	
	ZonesDataTable = Assets->GetDataTable(TEXT("Data.ZonesTable"), true);

	UE_LOG(LogTemp, Log, TEXT("[GAMEDATA] DT Campaign=%s"),
		*GetNameSafe(CampaignDataTable));

	// IMPORTANT:
	// Do NOT EmptyTable() in runtime. DataTables are cooked assets in packaged builds.
	// Clearing/regenerating must be editor-only tooling, not part of game boot.
#if WITH_EDITOR
	if (bClearTables)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GAMEDATA] bClearTables is TRUE (Editor). Runtime table mutation is discouraged."));
		// Leave this disabled to avoid PIE crash / asset mutation issues:
		// if (CampaignDataTable)      CampaignDataTable->EmptyTable();
		// if (CampaignOOBDataTable)   CampaignOOBDataTable->EmptyTable();
		// if (CombatGroupDataTable)   CombatGroupDataTable->EmptyTable();
		// if (OrderOfBattleDataTable) OrderOfBattleDataTable->EmptyTable();
	}
#endif
}

void UStarshatterGameDataSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UStarshatterGameDataSubsystem::SetProjectPath()
{
	ProjectPath = FPaths::ProjectDir();
	ProjectPath.Append(TEXT("GameData/"));

	UE_LOG(LogTemp, Log, TEXT("Setting Game Data Directory %s"), *ProjectPath);
}

FString UStarshatterGameDataSubsystem::GetProjectPath()
{
	return ProjectPath;
}

void UStarshatterGameDataSubsystem::InitializeOrderOfBattleTable()
{
	if (!OrderOfBattleDataTable)
	{
		return;
	}

	OrderOfBattleDataTable->EmptyTable();

	UE_LOG(LogTemp, Log, TEXT("StarshatterGameDataSubsystem::CreateOrderOfBattleTable"));

	// Use LOCAL containers with unique names so we do not hide members:
	TArray<FS_OOBFleet> LocalFleets;
	TArray<FS_OOBCarrier> LocalCarriers;
	TArray<FS_OOBDestroyer> LocalDestroyers;
	TArray<FS_OOBBattle> LocalBattles;
	TArray<FS_OOBWing> LocalWings;

	TArray<FS_OOBFighter> LocalFighters;
	TArray<FS_OOBIntercept> LocalInterceptors;
	TArray<FS_OOBAttack> LocalAttacks;
	TArray<FS_OOBLanding> LocalLandings;

	TArray<FS_OOBBattalion> LocalBattalions;
	TArray<FS_OOBBattery> LocalBatteries;
	TArray<FS_OOBStation> LocalStations;
	TArray<FS_OOBStarbase> LocalStarbases;

	TArray<FS_OOBCivilian> LocalCivilians;
	TArray<FS_OOBMinefield> LocalMinefields;

	FS_OOBForce NewForce;
	FS_OOBForce* ForceRow = nullptr;

	int32 CurrentForceId = -1;
	EEMPIRE_NAME CurrentEmpire = EEMPIRE_NAME::Unknown;
	int32 CurrentIff = -1;
	FName CurrentForceRowName = NAME_None;

	auto FinalizeCurrentForce = [&]()
		{
			if (CurrentForceRowName == NAME_None)
			{
				return;
			}

			// -------------------------------------------------
			// Build battalion children for THIS force
			// -------------------------------------------------
			for (FS_OOBBattalion& Battalion : LocalBattalions)
			{
				Battalion.Battery.Reset();
				Battalion.Station.Reset();
				Battalion.Starbase.Reset();

				for (const FS_OOBBattery& Battery : LocalBatteries)
				{
					if (Battery.ParentId == Battalion.Id &&
						Battery.Empire == Battalion.Empire)
					{
						Battalion.Battery.Add(Battery);
					}
				}

				for (const FS_OOBStation& Station : LocalStations)
				{
					if (Station.ParentId == Battalion.Id &&
						Station.Empire == Battalion.Empire)
					{
						Battalion.Station.Add(Station);
					}
				}

				for (const FS_OOBStarbase& Starbase : LocalStarbases)
				{
					if (Starbase.ParentId == Battalion.Id &&
						Starbase.Empire == Battalion.Empire)
					{
						Battalion.Starbase.Add(Starbase);
					}
				}
			}

			// -------------------------------------------------
			// Build fleet children for THIS force
			// -------------------------------------------------
			for (FS_OOBFleet& Fleet : LocalFleets)
			{
				Fleet.Carrier.Reset();
				Fleet.Destroyer.Reset();
				Fleet.Battle.Reset();
				Fleet.Minefield.Reset();

				for (FS_OOBCarrier& Carrier : LocalCarriers)
				{
					if (Carrier.ParentId != Fleet.Id || Carrier.Empire != Fleet.Empire)
					{
						continue;
					}

					Carrier.Wing.Reset();
					Carrier.Fighter.Reset();
					Carrier.Attack.Reset();
					Carrier.Intercept.Reset();
					Carrier.Landing.Reset();

					for (const FS_OOBFighter& Fighter : LocalFighters)
					{
						if (Fighter.ParentId == Carrier.Id &&
							Fighter.Empire == Carrier.Empire &&
							Fighter.ParentType == ECOMBATGROUP_TYPE::CARRIER_GROUP)
						{
							Carrier.Fighter.Add(Fighter);
						}
					}

					for (const FS_OOBAttack& Attack : LocalAttacks)
					{
						if (Attack.ParentId == Carrier.Id &&
							Attack.Empire == Carrier.Empire &&
							Attack.ParentType == ECOMBATGROUP_TYPE::CARRIER_GROUP)
						{
							Carrier.Attack.Add(Attack);
						}
					}

					for (const FS_OOBIntercept& Intercept : LocalInterceptors)
					{
						if (Intercept.ParentId == Carrier.Id &&
							Intercept.Empire == Carrier.Empire &&
							Intercept.ParentType == ECOMBATGROUP_TYPE::CARRIER_GROUP)
						{
							Carrier.Intercept.Add(Intercept);
						}
					}

					for (const FS_OOBLanding& Landing : LocalLandings)
					{
						if (Landing.ParentId == Carrier.Id &&
							Landing.Empire == Carrier.Empire &&
							Landing.ParentType == ECOMBATGROUP_TYPE::CARRIER_GROUP)
						{
							Carrier.Landing.Add(Landing);
						}
					}

					for (FS_OOBWing& Wing : LocalWings)
					{
						if (Wing.ParentId != Carrier.Id || Wing.Empire != Carrier.Empire)
						{
							continue;
						}

						Wing.Fighter.Reset();
						Wing.Attack.Reset();
						Wing.Intercept.Reset();
						Wing.Landing.Reset();

						for (const FS_OOBFighter& Fighter : LocalFighters)
						{
							if (Fighter.ParentId == Wing.Id &&
								Fighter.Empire == Wing.Empire &&
								Fighter.ParentType == ECOMBATGROUP_TYPE::WING)
							{
								Wing.Fighter.Add(Fighter);
							}
						}

						for (const FS_OOBAttack& Attack : LocalAttacks)
						{
							if (Attack.ParentId == Wing.Id &&
								Attack.Empire == Wing.Empire)
							{
								Wing.Attack.Add(Attack);
							}
						}

						for (const FS_OOBIntercept& Intercept : LocalInterceptors)
						{
							if (Intercept.ParentId == Wing.Id &&
								Intercept.Empire == Wing.Empire)
							{
								Wing.Intercept.Add(Intercept);
							}
						}

						for (const FS_OOBLanding& Landing : LocalLandings)
						{
							if (Landing.ParentId == Wing.Id &&
								Landing.Empire == Wing.Empire)
							{
								Wing.Landing.Add(Landing);
							}
						}

						Carrier.Wing.Add(Wing);
					}

					Fleet.Carrier.Add(Carrier);
				}

				for (const FS_OOBBattle& Battle : LocalBattles)
				{
					if (Battle.ParentId == Fleet.Id &&
						Battle.Empire == Fleet.Empire)
					{
						Fleet.Battle.Add(Battle);
					}
				}

				for (const FS_OOBDestroyer& Destroyer : LocalDestroyers)
				{
					if (Destroyer.ParentId == Fleet.Id &&
						Destroyer.Empire == Fleet.Empire)
					{
						Fleet.Destroyer.Add(Destroyer);
					}
				}

				for (const FS_OOBMinefield& Minefield : LocalMinefields)
				{
					if (Minefield.ParentId == Fleet.Id &&
						Minefield.Empire == Fleet.Empire)
					{
						Fleet.Minefield.Add(Minefield);
					}
				}
			}

			// -------------------------------------------------
			// Write fully-built data into the force row
			// -------------------------------------------------
			ForceRow = OrderOfBattleDataTable->FindRow<FS_OOBForce>(
				CurrentForceRowName,
				TEXT("CreateOrderOfBattleTable"));

			if (!ForceRow)
			{
				return;
			}

			ForceRow->Fleet = LocalFleets;
			ForceRow->Battalion = LocalBattalions;
			ForceRow->Civilian = LocalCivilians;

			UE_LOG(LogTemp, Log,
				TEXT("[OrderOfBattle] Finalized OOB Force '%s': Fleets=%d Battalions=%d Civilians=%d"),
				*CurrentForceRowName.ToString(),
				LocalFleets.Num(),
				LocalBattalions.Num(),
				LocalCivilians.Num());
		};

	// ---------------------------------------------
	// Pass 1: Walk roster and build flat collections
	// ---------------------------------------------
	for (const FS_CombatGroup& Item : CombatRosterData)
	{
		if (Item.Type == ECOMBATGROUP_TYPE::FORCE)
		{
			// Finalize previous force before resetting local state
			FinalizeCurrentForce();

			// Reset all per-force aggregates
			LocalFleets.Empty();
			LocalCarriers.Empty();
			LocalDestroyers.Empty();
			LocalBattles.Empty();
			LocalWings.Empty();

			LocalFighters.Empty();
			LocalInterceptors.Empty();
			LocalAttacks.Empty();
			LocalLandings.Empty();

			LocalBattalions.Empty();
			LocalBatteries.Empty();
			LocalStations.Empty();
			LocalStarbases.Empty();

			LocalCivilians.Empty();
			LocalMinefields.Empty();

			CurrentForceId = Item.Id;
			CurrentEmpire = Item.EmpireId;
			CurrentIff = Item.Iff;

			NewForce = FS_OOBForce();
			NewForce.Id = Item.Id;
			NewForce.Name = Item.DisplayName;
			NewForce.Iff = Item.Iff;
			NewForce.Location = Item.Region;
			NewForce.Empire = Item.EmpireId;
			NewForce.Intel = Item.Intel;

			CurrentForceRowName = FName(*NewForce.Name);

			if (NewForce.Iff > -1)
			{
				OrderOfBattleDataTable->AddRow(CurrentForceRowName, NewForce);
			}

			continue;
		}

		// Everything below assumes we are currently under a valid FORCE
		if (CurrentForceId < 0)
		{
			continue;
		}

		// ---------------------------------------------
		// Fleet under Force
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::FLEET)
		{
			if (Item.ParentId == CurrentForceId && Item.EmpireId == CurrentEmpire)
			{
				FS_OOBFleet NewFleet;
				NewFleet.Id = Item.Id;
				NewFleet.ParentId = Item.ParentId;
				NewFleet.Name = Item.DisplayName;
				NewFleet.Iff = Item.Iff;
				NewFleet.Location = Item.Region;
				NewFleet.Empire = Item.EmpireId;
				NewFleet.Intel = Item.Intel;

				LocalFleets.Add(NewFleet);
			}

			continue;
		}

		// ---------------------------------------------
		// Battalion under Force
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::BATTALION)
		{
			if (Item.ParentId == CurrentForceId && Item.EmpireId == CurrentEmpire)
			{
				FS_OOBBattalion NewBattalion;
				NewBattalion.Id = Item.Id;
				NewBattalion.ParentId = Item.ParentId;
				NewBattalion.Name = Item.DisplayName;
				NewBattalion.Iff = Item.Iff;
				NewBattalion.Location = Item.Region;
				NewBattalion.Empire = Item.EmpireId;
				NewBattalion.Intel = Item.Intel;

				LocalBattalions.Add(NewBattalion);
			}

			continue;
		}

		// ---------------------------------------------
		// Civilian-family groups
		// ---------------------------------------------
		if (IsCivilianFamilyGroupType(Item.Type))
		{
			if (Item.EmpireId == CurrentEmpire)
			{
				FS_OOBCivilian NewCivilian;
				NewCivilian.Id = Item.Id;
				NewCivilian.ParentId = Item.ParentId;
				NewCivilian.Name = Item.DisplayName;
				NewCivilian.Iff = Item.Iff;
				NewCivilian.Location = Item.Region;
				NewCivilian.Empire = Item.EmpireId;
				NewCivilian.Intel = Item.Intel;
				NewCivilian.Type = Item.Type;
				NewCivilian.ParentType = Item.ParentType;

				LocalCivilians.Add(NewCivilian);
			}

			continue;
		}

		// ---------------------------------------------
		// Station
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::STATION)
		{
			FS_OOBStation NewStation;
			NewStation.Id = Item.Id;
			NewStation.ParentId = Item.ParentId;
			NewStation.Name = Item.DisplayName;
			NewStation.Iff = Item.Iff;
			NewStation.Location = Item.Region;
			NewStation.Empire = Item.EmpireId;
			NewStation.Intel = Item.Intel;

			NewStation.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewStation.Unit.IsValidIndex(UnitIndex))
					break;

				NewStation.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewStation.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewStation.Unit[UnitIndex].Location = Item.Region;
				NewStation.Unit[UnitIndex].ParentId = Item.ParentId;
				NewStation.Unit[UnitIndex].Empire = Item.EmpireId;
				NewStation.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::STATION;
				NewStation.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::STATION;
				NewStation.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalStations.Add(NewStation);
			continue;
		}

		// ---------------------------------------------
		// Starbase
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::STARBASE)
		{
			FS_OOBStarbase NewStarbase;
			NewStarbase.Id = Item.Id;
			NewStarbase.ParentId = Item.ParentId;
			NewStarbase.Name = Item.DisplayName;
			NewStarbase.Iff = Item.Iff;
			NewStarbase.Location = Item.Region;
			NewStarbase.Empire = Item.EmpireId;
			NewStarbase.Intel = Item.Intel;

			NewStarbase.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewStarbase.Unit.IsValidIndex(UnitIndex))
					break;

				NewStarbase.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewStarbase.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewStarbase.Unit[UnitIndex].Location = Item.Region;
				NewStarbase.Unit[UnitIndex].ParentId = Item.ParentId;
				NewStarbase.Unit[UnitIndex].Empire = Item.EmpireId;
				NewStarbase.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::STARBASE;
				NewStarbase.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::STARBASE;
				NewStarbase.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalStarbases.Add(NewStarbase);
			continue;
		}

		// ---------------------------------------------
		// Battery
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::BATTERY)
		{
			FS_OOBBattery NewBattery;
			NewBattery.Id = Item.Id;
			NewBattery.ParentId = Item.ParentId;
			NewBattery.Name = Item.DisplayName;
			NewBattery.Iff = Item.Iff;
			NewBattery.Location = Item.Region;
			NewBattery.Empire = Item.EmpireId;
			NewBattery.Intel = Item.Intel;

			NewBattery.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewBattery.Unit.IsValidIndex(UnitIndex))
					break;

				NewBattery.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewBattery.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewBattery.Unit[UnitIndex].Location = Item.Region;
				NewBattery.Unit[UnitIndex].ParentId = Item.ParentId;
				NewBattery.Unit[UnitIndex].Empire = Item.EmpireId;
				NewBattery.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::BATTERY;
				NewBattery.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::BATTERY;
				NewBattery.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalBatteries.Add(NewBattery);
			continue;
		}

		// ---------------------------------------------
		// Minefield
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::MINEFIELD)
		{
			FS_OOBMinefield NewMinefield;
			NewMinefield.Id = Item.Id;
			NewMinefield.ParentId = Item.ParentId;
			NewMinefield.Name = Item.DisplayName;
			NewMinefield.Iff = Item.Iff;
			NewMinefield.Location = Item.Region;
			NewMinefield.Empire = Item.EmpireId;
			NewMinefield.Intel = Item.Intel;

			NewMinefield.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewMinefield.Unit.IsValidIndex(UnitIndex))
					break;

				NewMinefield.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewMinefield.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewMinefield.Unit[UnitIndex].Location = Item.Region;
				NewMinefield.Unit[UnitIndex].ParentId = Item.ParentId;
				NewMinefield.Unit[UnitIndex].Empire = Item.EmpireId;
				NewMinefield.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::MINE;
				NewMinefield.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::MINEFIELD;
				NewMinefield.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalMinefields.Add(NewMinefield);
			continue;
		}

		// ---------------------------------------------
		// Carrier group
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::CARRIER_GROUP)
		{
			FS_OOBCarrier NewCarrier;
			NewCarrier.Id = Item.Id;
			NewCarrier.ParentId = Item.ParentId;
			NewCarrier.Name = Item.DisplayName;
			NewCarrier.Iff = Item.Iff;
			NewCarrier.Location = Item.Region;
			NewCarrier.Empire = Item.EmpireId;
			NewCarrier.Intel = Item.Intel;

			NewCarrier.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewCarrier.Unit.IsValidIndex(UnitIndex))
					break;

				NewCarrier.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewCarrier.Unit[UnitIndex].Count = 1;
				NewCarrier.Unit[UnitIndex].ParentId = Item.ParentId;
				NewCarrier.Unit[UnitIndex].Regnum = UnitItem.UnitRegnum;
				NewCarrier.Unit[UnitIndex].Empire = Item.EmpireId;
				NewCarrier.Unit[UnitIndex].Location = Item.Region;
				NewCarrier.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::CARRIER_GROUP;
				NewCarrier.Unit[UnitIndex].Design = UnitItem.UnitDesign;

				if (UnitItem.UnitClass == "Cruiser")
					NewCarrier.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CRUISER;
				else if (UnitItem.UnitClass == "Destroyer")
					NewCarrier.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::DESTROYER;
				else if (UnitItem.UnitClass == "Frigate")
					NewCarrier.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::FRIGATE;
				else if (UnitItem.UnitClass == "Carrier")
					NewCarrier.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CARRIER;

				NewCarrier.Unit[UnitIndex].DisplayName =
					UFormattingUtils::GetUnitPrefixFromType(NewCarrier.Unit[UnitIndex].Type) +
					UnitItem.UnitRegnum + " " + UnitItem.UnitName;

				++UnitIndex;
			}

			LocalCarriers.Add(NewCarrier);
			continue;
		}

		// ---------------------------------------------
		// Destroyer squadron
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::DESTROYER_SQUADRON)
		{
			FS_OOBDestroyer NewDestroyer;
			NewDestroyer.Id = Item.Id;
			NewDestroyer.ParentId = Item.ParentId;
			NewDestroyer.Name = Item.DisplayName;
			NewDestroyer.Iff = Item.Iff;
			NewDestroyer.Location = Item.Region;
			NewDestroyer.Empire = Item.EmpireId;
			NewDestroyer.Intel = Item.Intel;

			NewDestroyer.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewDestroyer.Unit.IsValidIndex(UnitIndex))
					break;

				NewDestroyer.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewDestroyer.Unit[UnitIndex].Count = 1;
				NewDestroyer.Unit[UnitIndex].ParentId = Item.ParentId;
				NewDestroyer.Unit[UnitIndex].Empire = Item.EmpireId;
				NewDestroyer.Unit[UnitIndex].Regnum = UnitItem.UnitRegnum;
				NewDestroyer.Unit[UnitIndex].Location = Item.Region;
				NewDestroyer.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
				NewDestroyer.Unit[UnitIndex].Design = UnitItem.UnitDesign;

				if (UnitItem.UnitClass == "Cruiser")
					NewDestroyer.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CRUISER;
				else if (UnitItem.UnitClass == "Destroyer")
					NewDestroyer.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::DESTROYER;
				else if (UnitItem.UnitClass == "Frigate")
					NewDestroyer.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::FRIGATE;
				else if (UnitItem.UnitClass == "Carrier")
					NewDestroyer.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CARRIER;

				NewDestroyer.Unit[UnitIndex].DisplayName =
					UFormattingUtils::GetUnitPrefixFromType(NewDestroyer.Unit[UnitIndex].Type) +
					UnitItem.UnitRegnum + " " + UnitItem.UnitName;

				++UnitIndex;
			}

			LocalDestroyers.Add(NewDestroyer);
			continue;
		}

		// ---------------------------------------------
		// Battle group
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::BATTLE_GROUP)
		{
			FS_OOBBattle NewBattle;
			NewBattle.Id = Item.Id;
			NewBattle.ParentId = Item.ParentId;
			NewBattle.Name = Item.DisplayName;
			NewBattle.Iff = Item.Iff;
			NewBattle.Location = Item.Region;
			NewBattle.Empire = Item.EmpireId;
			NewBattle.Intel = Item.Intel;

			NewBattle.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewBattle.Unit.IsValidIndex(UnitIndex))
					break;

				NewBattle.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewBattle.Unit[UnitIndex].Count = 1;
				NewBattle.Unit[UnitIndex].ParentId = Item.ParentId;
				NewBattle.Unit[UnitIndex].Regnum = UnitItem.UnitRegnum;
				NewBattle.Unit[UnitIndex].Empire = Item.EmpireId;
				NewBattle.Unit[UnitIndex].Location = Item.Region;
				NewBattle.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::BATTLE_GROUP;
				NewBattle.Unit[UnitIndex].Design = UnitItem.UnitDesign;

				if (UnitItem.UnitClass == "Cruiser")
					NewBattle.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CRUISER;
				else if (UnitItem.UnitClass == "Destroyer")
					NewBattle.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::DESTROYER;
				else if (UnitItem.UnitClass == "Frigate")
					NewBattle.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::FRIGATE;
				else if (UnitItem.UnitClass == "Carrier")
					NewBattle.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::CARRIER;

				NewBattle.Unit[UnitIndex].DisplayName =
					UFormattingUtils::GetUnitPrefixFromType(NewBattle.Unit[UnitIndex].Type) +
					UnitItem.UnitRegnum + " " + UnitItem.UnitName;

				++UnitIndex;
			}

			LocalBattles.Add(NewBattle);
			continue;
		}

		// ---------------------------------------------
		// Wing
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::WING)
		{
			FS_OOBWing NewWing;
			NewWing.Id = Item.Id;
			NewWing.ParentId = Item.ParentId;
			NewWing.Name = Item.DisplayName;
			NewWing.Iff = Item.Iff;
			NewWing.Location = Item.Region;
			NewWing.Empire = Item.EmpireId;
			NewWing.Intel = Item.Intel;

			LocalWings.Add(NewWing);
			continue;
		}

		// ---------------------------------------------
		// Fighter squadron
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON)
		{
			FS_OOBFighter NewFighter;
			NewFighter.Id = Item.Id;
			NewFighter.ParentId = Item.ParentId;
			NewFighter.Name = Item.DisplayName;
			NewFighter.Iff = Item.Iff;
			NewFighter.Location = Item.Region;
			NewFighter.ParentType = Item.ParentType;
			NewFighter.Empire = Item.EmpireId;
			NewFighter.Intel = Item.Intel;

			NewFighter.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewFighter.Unit.IsValidIndex(UnitIndex))
					break;

				NewFighter.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewFighter.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewFighter.Unit[UnitIndex].Location = Item.Region;
				NewFighter.Unit[UnitIndex].ParentId = Item.ParentId;
				NewFighter.Unit[UnitIndex].Empire = Item.EmpireId;
				NewFighter.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::FIGHTER;
				NewFighter.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
				NewFighter.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalFighters.Add(NewFighter);
			continue;
		}

		// ---------------------------------------------
		// Intercept squadron
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON)
		{
			FS_OOBIntercept NewIntercept;
			NewIntercept.Id = Item.Id;
			NewIntercept.ParentId = Item.ParentId;
			NewIntercept.Name = Item.DisplayName;
			NewIntercept.Iff = Item.Iff;
			NewIntercept.Location = Item.Region;
			NewIntercept.ParentType = Item.ParentType;
			NewIntercept.Empire = Item.EmpireId;
			NewIntercept.Intel = Item.Intel;

			NewIntercept.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewIntercept.Unit.IsValidIndex(UnitIndex))
					break;

				NewIntercept.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewIntercept.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewIntercept.Unit[UnitIndex].Location = Item.Region;
				NewIntercept.Unit[UnitIndex].ParentId = Item.ParentId;
				NewIntercept.Unit[UnitIndex].Empire = Item.EmpireId;
				NewIntercept.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::INTERCEPT;
				NewIntercept.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
				NewIntercept.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalInterceptors.Add(NewIntercept);
			continue;
		}

		// ---------------------------------------------
		// Attack squadron
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::ATTACK_SQUADRON)
		{
			FS_OOBAttack NewAttack;
			NewAttack.Id = Item.Id;
			NewAttack.ParentId = Item.ParentId;
			NewAttack.Name = Item.DisplayName;
			NewAttack.Iff = Item.Iff;
			NewAttack.Location = Item.Region;
			NewAttack.ParentType = Item.ParentType;
			NewAttack.Empire = Item.EmpireId;
			NewAttack.Intel = Item.Intel;

			NewAttack.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewAttack.Unit.IsValidIndex(UnitIndex))
					break;

				NewAttack.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewAttack.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewAttack.Unit[UnitIndex].Location = Item.Region;
				NewAttack.Unit[UnitIndex].ParentId = Item.ParentId;
				NewAttack.Unit[UnitIndex].Empire = Item.EmpireId;
				NewAttack.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::ATTACK;
				NewAttack.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
				NewAttack.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalAttacks.Add(NewAttack);
			continue;
		}

		// ---------------------------------------------
		// Landing squadron
		// ---------------------------------------------
		if (Item.Type == ECOMBATGROUP_TYPE::LCA_SQUADRON)
		{
			FS_OOBLanding NewLanding;
			NewLanding.Id = Item.Id;
			NewLanding.ParentId = Item.ParentId;
			NewLanding.Name = Item.DisplayName;
			NewLanding.Iff = Item.Iff;
			NewLanding.Location = Item.Region;
			NewLanding.ParentType = Item.ParentType;
			NewLanding.Empire = Item.EmpireId;
			NewLanding.Intel = Item.Intel;

			NewLanding.Unit.SetNum(Item.Unit.Num());

			int32 UnitIndex = 0;
			for (const auto& UnitItem : Item.Unit)
			{
				if (!NewLanding.Unit.IsValidIndex(UnitIndex))
					break;

				NewLanding.Unit[UnitIndex].Name = UnitItem.UnitName;
				NewLanding.Unit[UnitIndex].Count = UnitItem.UnitCount;
				NewLanding.Unit[UnitIndex].Location = Item.Region;
				NewLanding.Unit[UnitIndex].ParentId = Item.ParentId;
				NewLanding.Unit[UnitIndex].Empire = Item.EmpireId;
				NewLanding.Unit[UnitIndex].Type = ECOMBATUNIT_TYPE::LCA;
				NewLanding.Unit[UnitIndex].ParentType = ECOMBATGROUP_TYPE::LCA_SQUADRON;
				NewLanding.Unit[UnitIndex].Design = UnitItem.UnitDesign;
				++UnitIndex;
			}

			LocalLandings.Add(NewLanding);
			continue;
		}
	}

	// Finalize LAST force after the loop
	FinalizeCurrentForce();
}


void UStarshatterGameDataSubsystem::ExportDataToCSV(UDataTable* DataTable, const FString& FileName)
{
	if (!DataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Export failed: DataTable is null."));
		return;
	}

	// You can use none or pretty names — make sure the flag is in scope
	const FString CSVData = DataTable->GetTableAsCSV(EDataTableExportFlags::None);

	const FString SavePath = FPaths::ProjectDir() + FileName;

	if (FFileHelper::SaveStringToFile(CSVData, *SavePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully exported DataTable to: %s"), *SavePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write DataTable CSV to file."));
	}
}

void UStarshatterGameDataSubsystem::GetSSWInstance()
{
	SSWInstance = (USSWGameInstance*)GetGameInstance();
}

void UStarshatterGameDataSubsystem::CacheSSWInstance()
{
	if (SSWInstance)
		return;

	UGameInstance* GI = GetGameInstance();
	if (!GI)
		return;

	SSWInstance = Cast<USSWGameInstance>(GI);
}

void UStarshatterGameDataSubsystem::SetActiveCampaign(const FS_Campaign& Campaign)
{
	ActiveCampaign = Campaign;
}

void UStarshatterGameDataSubsystem::LoadAll(bool bFull)
{
	if (bLoaded)
		return;

	bLoaded = true;

	CacheSSWInstance();
	if (!SSWInstance)
		return;
	
	LoadContentBundle();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStarshatterGameDataSubsystem* GD = GI->GetSubsystem<UStarshatterGameDataSubsystem>())
		{
			//GD->LoadForms();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GameData subsystem is NULL at LoadForms call site"));
		}
	}

	//LoadAwardTables();

	BuildRankCache(RanksDataTable, RankById);
	BuildMedalCache(MedalsDataTable, MedalById);
	BuildMedalCache_ByFlag(MedalsDataTable);
	
	//InitializeCampaignData();
	ReadCampaignData();
	
	//InitializeCombatRoster();
	ReadCombatRosterData();

	ReadCombatants();

	//InitializeOrderOfBattleTable();
	ReadOrderOfBattleData();
	BuildCombatRosterFromOrderOfBattle();	
	
	SSWInstance->StartGameTimers();

	// USystemDesign::Initialize(SystemDesignTable);
}

void UStarshatterGameDataSubsystem::InitializeCampaignData() {
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::InitializeCampaignData()"));

	ProjectPath = FPaths::ProjectContentDir();
	ProjectPath.Append(TEXT("GameData/Campaigns/"));
	FString FileName = ProjectPath;

	for (int i = 1; i < 6; i++) {
		FileName = ProjectPath;
		FileName.Append("0");
		FileName.Append(FString::FormatAsNumber(i));
		FileName.Append("/");
		CampaignPath = FileName;
		FileName.Append("campaign.def");
		LoadCampaignData(TCHAR_TO_ANSI(*FileName), true);
	}
}

void UStarshatterGameDataSubsystem::LoadCampaignData(const char* fs, bool full)
{
	if (!CampaignDataTable)
	{
		return;
	}

	//CampaignDataTable->EmptyTable();

	const FString CampaignDPath = ANSI_TO_TCHAR(fs);

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *CampaignDPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load: %s"), *CampaignDPath);
		return;
	}

	Bytes.Add(0); // null terminate for BlockReader

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term) {
		UE_LOG(LogTemp, Log, TEXT("WARNING: could not parse '%s'"), ANSI_TO_TCHAR(fs));
		return;
	}
	else {
		const FString CampaignPathStr = ANSI_TO_TCHAR(fs);
		UE_LOG(LogTemp, Log, TEXT("Campaign file '%s'"), *CampaignPathStr);
	}

	FS_Campaign NewCampaignData;
	TArray<FString> OrdersArray;
	OrdersArray.Empty();

	CombatantArray.Empty();
	CampaignActionArray.Empty();
	MissionArray.Empty();
	TemplateMissionArray.Empty();
	ScriptedMissionArray.Empty();
	ZoneArray.Empty();

	do {
		delete term;
		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {

				if (def->name()->value() == "name") {
					GetDefText(name, def, filename);
					NewCampaignData.Name = FString(name);
				}
				else if (def->name()->value() == "desc") {
					GetDefText(description, def, filename);
					NewCampaignData.Description = FString(description);
				}
				else if (def->name()->value() == "index") {
					GetDefNumber(CampaignIndex, def, filename);
					NewCampaignData.Index = CampaignIndex;
				}
				else if (def->name()->value() == "situation") {
					GetDefText(situation, def, filename);
					NewCampaignData.Situation = FString(situation);
				}
				else if (def->name()->value() == "system") {
					GetDefText(system, def, filename);
					NewCampaignData.System = FString(system);
				}
				else if (def->name()->value() == "region") {
					GetDefText(region, def, filename);
					NewCampaignData.Region = FString(region);
				}
				else if (def->name()->value() == "image") {
					GetDefText(MainImage, def, filename);
					NewCampaignData.MainImage = FString(MainImage);
				}
				else if (def->name()->value() == "date") {
					GetDefText(start, def, filename);
					NewCampaignData.Start = FString(start);
				}
				else if (def->name()->value() == "orders") {
					GetDefText(orders, def, filename);
					OrdersArray.Add(FString(orders));
					NewCampaignData.Orders = OrdersArray;
				}
				else if (def->name()->value() == "scripted") {
					if (def->term() && def->term()->isBool()) {
						scripted = def->term()->isBool()->value();
						NewCampaignData.bScripted = scripted;
					}
				}
				else if (def->name()->value() == "sequential") {
					if (def->term() && def->term()->isBool()) {
						sequential = def->term()->isBool()->value();
						NewCampaignData.bSequential = sequential;
					}
				}

				else if (def->name()->value() == "available") {
					if (def->term() && def->term()->isBool()) {
						available = def->term()->isBool()->value();
						NewCampaignData.bAvailable = available;
					}
				}

				else if (def->name()->value() == "combatant_groups") {
					GetDefNumber(CombatantSize, def, filename);
					NewCampaignData.CombatantSize = CombatantSize;
				}

				else if (def->name()->value() == "action_groups") {
					GetDefNumber(ActionSize, def, filename);
					NewCampaignData.ActionSize = ActionSize;
				}

				else if (def->name()->value() == "action") {

					TermStruct* ActionTerm = def->term()->isStruct();

					NewCampaignData.ActionSize = ActionTerm->elements()->size();

					if (NewCampaignData.ActionSize > 0)
					{
						ActionId = 0;
						ActionType = "";
						ActionSubtype = 0;
						OppType = -1;
						ActionTeam = 0;
						ActionSource = "";
						ActionLocation = FVector::Zero();

						ActionSystem = "";
						ActionRegion = "";
						ActionFile = "";

						ActionScene = "";
						ActionText = "";

						ActionCount = 1;
						StartBefore = Game::TIME_NEVER;
						StartAfter = 0;
						MinRank = 0;
						MaxRank = 100;
						Delay = 0;
						Probability = 100;

						AssetType = "";
						AssetId = 0;
						TargetType = "";
						TargetId = 0;
						TargetIff = 0;

						AssetKill = "";
						TargetKill = "";

						for (int ActionIdx = 0; ActionIdx < NewCampaignData.ActionSize; ActionIdx++)
						{
							ActionImage = "Empty";
							ActionAudio = "";

							NewCampaignAction.Campaign = NewCampaignData.Name;
							TermDef* pdef = ActionTerm->elements()->at(ActionIdx)->isDef();

							if (pdef->name()->value() == "id") {
								GetDefNumber(ActionId, pdef, filename);
								NewCampaignAction.Id = ActionId;
								UE_LOG(LogTemp, Log, TEXT("action id: '%d'"), ActionId);
							}
							else if (pdef->name()->value() == "type") {
								GetDefText(ActionType, pdef, filename);
								//type = CombatAction::TypeFromName(txt);
								NewCampaignAction.Type = FString(ActionType);
								UE_LOG(LogTemp, Log, TEXT("action type: '%s'"), *FString(ActionType));
							}
							else if (pdef->name()->value() == "subtype") {
								if (pdef->term()->isNumber()) {
									GetDefNumber(ActionSubtype, pdef, filename);
									NewCampaignAction.Subtype = ActionSubtype;
								}
								else if (pdef->term()->isText()) {
									char txt[64];
									GetDefText(txt, pdef, filename);

									if (ActionType == ECombatActionType::MISSION_TEMPLATE) {
										ActionSubtype = Mission::TypeFromName(txt);
									}
									else if (ActionType == ECombatActionType::COMBAT_EVENT) {
										ActionSubtype = (int) CombatEvent::GetTypeFromName(txt);
									}
									if (ActionType == ECombatActionType::INTEL_EVENT) {
										ActionSubtype = Intel::IntelFromName(txt);
									}
									NewCampaignAction.Subtype = ActionSubtype;
								}

							}
							else if (pdef->name()->value() == "opp_type") {
								if (pdef->term()->isNumber()) {
									GetDefNumber(OppType, pdef, filename);
									NewCampaignAction.OppType = OppType;
								}

								else if (pdef->term()->isText()) {
									char txt[64];
									GetDefText(txt, pdef, filename);

									if (ActionType == ECombatActionType::MISSION_TEMPLATE) {
										OppType = Mission::TypeFromName(txt);
									}
								}
							}
							else if (pdef->name()->value() == "source") {
								GetDefText(ActionSource, pdef, filename);
								//source = CombatEvent::SourceFromName(txt);
								NewCampaignAction.Source = FString(ActionSource);
							}
							else if (pdef->name()->value() == "team") {
								GetDefNumber(ActionTeam, pdef, filename);
								NewCampaignAction.Team = ActionTeam;
							}
							else if (pdef->name()->value() == "iff") {
								GetDefNumber(ActionTeam, pdef, filename);
								NewCampaignAction.Iff = ActionTeam;
							}
							else if (pdef->name()->value() == "count") {
								GetDefNumber(ActionCount, pdef, filename);
								NewCampaignAction.Count = ActionCount;
							}
							else if (pdef->name()->value().contains("before")) {
								if (pdef->term()->isNumber()) {
									GetDefNumber(StartBefore, pdef, filename);
									NewCampaignAction.StartBefore = StartBefore;
								}
								else {
									GetDefTime(StartBefore, pdef, filename);
									StartBefore -= Game::ONE_DAY;
									NewCampaignAction.StartBefore = StartBefore;
								}
							}
							else if (pdef->name()->value().contains("after")) {
								if (pdef->term()->isNumber()) {
									GetDefNumber(StartAfter, pdef, filename);
									NewCampaignAction.StartAfter = StartAfter;
								}
								else {
									GetDefTime(StartAfter, pdef, filename);
									StartAfter -= Game::ONE_DAY;
									NewCampaignAction.StartAfter = StartAfter;
								}
							}
							else if (pdef->name()->value() == "min_rank") {
								if (pdef->term()->isNumber()) {
									GetDefNumber(MinRank, pdef, filename);
									NewCampaignAction.MinRank = MinRank;
								}
								else {
									char rank_name[64];
									GetDefText(rank_name, pdef, filename);
									MinRank = PlayerData::RankFromName(rank_name);
									NewCampaignAction.MinRank = MinRank;
								}
							}
							else if (pdef->name()->value() == "max_rank") {
								if (pdef->term()->isNumber()) {
									GetDefNumber(MaxRank, pdef, filename);
									NewCampaignAction.MaxRank = MaxRank;
								}
								else {
									char rank_name[64];
									GetDefText(rank_name, pdef, filename);
									MaxRank = PlayerData::RankFromName(rank_name);
									NewCampaignAction.MaxRank = MaxRank;
								}
							}
							else if (pdef->name()->value() == "delay") {
								GetDefNumber(Delay, pdef, filename);
								NewCampaignAction.Delay = Delay;
							}
							else if (pdef->name()->value() == "probability") {
								GetDefNumber(Probability, pdef, filename);
								NewCampaignAction.Probability = Probability;
							}
							else if (pdef->name()->value() == "asset_type") {
								GetDefText(AssetType, pdef, filename);
								//asset_type = CombatGroup::TypeFromName(type_name);
								NewCampaignAction.AssetType = FString(AssetType);
							}
							else if (pdef->name()->value() == "target_type") {
								GetDefText(TargetType, pdef, filename);
								NewCampaignAction.TargetType = FString(TargetType);
								//target_type = CombatGroup::TypeFromName(type_name);
							}
							else if (pdef->name()->value() == "location" ||
								pdef->name()->value() == "loc") {
								GetDefVec(ActionLocation, pdef, filename);
								NewCampaignAction.Location.X = ActionLocation.X;
								NewCampaignAction.Location.Y = ActionLocation.Y;
								NewCampaignAction.Location.Z = ActionLocation.Z;

							}
							else if (pdef->name()->value() == "system" ||
								pdef->name()->value() == "sys") {
								GetDefText(ActionSystem, pdef, filename);
								NewCampaignAction.System = FString(ActionSystem);
							}
							else if (pdef->name()->value() == "region" ||
								pdef->name()->value() == "rgn" ||
								pdef->name()->value() == "zone") {
								GetDefText(ActionRegion, pdef, filename);
								NewCampaignAction.Region = FString(ActionRegion);
							}
							else if (pdef->name()->value() == "file") {
								GetDefText(ActionFile, pdef, filename);
								NewCampaignAction.Message = FString(ActionFile);
							}
							else if (pdef->name()->value() == "image") {
								GetDefText(ActionImage, pdef, filename);
								NewCampaignAction.Image = FString(ActionImage);
							}
							else if (pdef->name()->value() == "audio") {
								GetDefText(ActionAudio, pdef, filename);
								NewCampaignAction.Audio = FString(ActionAudio);
							}
							else if (pdef->name()->value() == "date") {
								GetDefText(ActionDate, pdef, filename);
								NewCampaignAction.Date = FString(ActionDate);
							}
							else if (pdef->name()->value() == "scene") {
								GetDefText(ActionScene, pdef, filename);
								NewCampaignAction.Scene = FString(ActionScene);
							}
							else if (pdef->name()->value() == "text") {
								GetDefText(ActionText, pdef, filename);
								NewCampaignAction.Title = FString(ActionText);
							}
							else if (pdef->name()->value() == "asset_id") {
								GetDefNumber(AssetId, pdef, filename);
								NewCampaignAction.AssetId = AssetId;
							}
							else if (pdef->name()->value() == "target_id") {
								GetDefNumber(TargetId, pdef, filename);
								NewCampaignAction.TargetId = TargetId;
							}

							else if (pdef->name()->value() == "target_iff") {
								GetDefNumber(TargetIff, pdef, filename);
								NewCampaignAction.TargetIff = TargetIff;
							}
							else if (pdef->name()->value() == "asset_kill") {
								GetDefText(AssetKill, pdef, filename);
								NewCampaignAction.AssetKill = FString(AssetKill);
							}

							else if (pdef->name()->value() == "target_kill") {
								GetDefText(TargetKill, pdef, filename);
								NewCampaignAction.TargetKill = FString(TargetKill);
							}
							else if (pdef->name()->value() == "req") {
								TermStruct* val2 = pdef->term()->isStruct();
								CampaignActionReqArray.Empty();
								Action = 0;

								ActionStatus = ECombatActionStatus::COMPLETE;
								EINTEL_TYPE LocalIntelType = EINTEL_TYPE::KNOWN;

								NotAction = false;

								Combatant1 = "";
								Combatant2 = "";

								comp = 0;
								score = 0;
								intel = 0;
								gtype = 0;
								gid = 0;

								FS_CampaignReq NewCampaignReq;
								ECombatActionStatus AStatus = ECombatActionStatus::UNKNOWN;
								for (int index = 0; index < val2->elements()->size(); index++) {
									TermDef* pdef2 = val2->elements()->at(index)->isDef();

									if (pdef2) {
										if (pdef2->name()->value() == "action") {
											GetDefNumber(Action, pdef2, filename);
											NewCampaignReq.Action = Action;
										}
										else if (pdef2->name()->value() == "status") {
											Text Buf;
											GetDefText(Buf, pdef2, filename);

											ECombatActionStatus AcStatus = ECombatActionStatus::UNKNOWN;
											if (FStringToEnum<ECombatActionStatus>(FString(ANSI_TO_TCHAR(Buf)).ToUpper(), AcStatus, false))
											{
												UE_LOG(LogTemp, Log, TEXT("Converted to enum: %d"), (int32)AcStatus);
											}
											else
											{
												UE_LOG(LogTemp, Warning, TEXT("Invalid enum string: %s"), ANSI_TO_TCHAR(Buf));
											}
											NewCampaignReq.Status = AcStatus;
										}
										else if (pdef2->name()->value() == "not") {
											GetDefBool(NotAction, pdef2, filename);
											NewCampaignReq.NotAction = NotAction;
										}

										else if (pdef2->name()->value() == "c1") {
											GetDefText(Combatant1, pdef2, filename);
											NewCampaignReq.Combatant1 = FString(Combatant1);
											//c1 = GetCombatant(txt);
										}
										else if (pdef2->name()->value() == "c2") {
											GetDefText(Combatant2, pdef2, filename);
											NewCampaignReq.Combatant2 = FString(Combatant2);
											//c2 = GetCombatant(txt);
										}
										else if (pdef2->name()->value() == "comp") {
											char txt[64];
											GetDefText(txt, pdef2, filename);
											//comp = CombatActionReq::CompFromName(txt);
											NewCampaignReq.Comp = comp;

										}
										else if (pdef2->name()->value() == "score") {
											GetDefNumber(score, pdef2, filename);
											NewCampaignReq.Score = score;

										}
										else if (pdef2->name()->value() == "intel")
										{
											Text Intel = "";
											GetDefText(Intel, pdef2, filename);

											if (!FStringToEnum<EINTEL_TYPE>(FString(Intel).ToUpper(), LocalIntelType, false))
												LocalIntelType = EINTEL_TYPE::KNOWN;

											NewCampaignReq.Intel = LocalIntelType;
										}
										else if (pdef2->name()->value() == "group_type") {
											char type_name[64];
											GetDefText(type_name, pdef2, filename);
											gtype = (int)CombatGroup::TypeFromName(type_name);
											NewCampaignReq.GroupType = gtype;

										}
										else if (pdef2->name()->value() == "group_id") {
											GetDefNumber(gid, pdef2, filename);
											NewCampaignReq.GroupId = gid;
										}
									}
								}
								CampaignActionReqArray.Add(NewCampaignReq);
							}
						}
						NewCampaignAction.Requirement = CampaignActionReqArray;
						CampaignActionReqArray.Empty();
						CampaignActionArray.Add(NewCampaignAction);
					} 
					NewCampaignData.Action = CampaignActionArray;
					
					if (CampaignActionDataTable)
					{
						const FString RowCampaignId = NewCampaignAction.Campaign.IsEmpty() ? TEXT("Default") : NewCampaignAction.Campaign;

						const FName RowName(*FString::Printf(TEXT("%s.Action%03d"), *RowCampaignId, NewCampaignAction.Id));

						if (!CampaignActionDataTable->GetRowMap().Contains(RowName))
						{
							CampaignActionDataTable->AddRow(RowName, NewCampaignAction);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("DT_CampaignAction already has row '%s' - skipping duplicate"), *RowName.ToString());
						}
					}
				}

				else if (def->name()->value() == "combatant") {

					TermStruct* CombatantTerm = def->term()->isStruct();

					NewCampaignData.CombatantSize = CombatantTerm->elements()->size();

					if (NewCampaignData.CombatantSize > 0)
					{
						// Add Unit Stuff Here

						CombatantName = "";
						EEMPIRE_NAME EName = EEMPIRE_NAME::Unknown;
						CombatantSize = 0;
						NewCombatUnit.Group.Empty();

						for (int UnitIdx = 0; UnitIdx < NewCampaignData.CombatantSize; UnitIdx++)
						{
							def = CombatantTerm->elements()->at(UnitIdx)->isDef();

							if (def->name()->value() == "name") {
								GetDefText(CombatantName, def, filename);

								if (FStringToEnum<EEMPIRE_NAME>(FString(CombatantName).ToUpper(), EName, false))
								{
									UE_LOG(LogTemp, Log, TEXT("Converted to enum: %d"), static_cast<int32>(EName));
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("Invalid enum string"));
								}

								NewCombatUnit.Name = EName;
							}
							else if (def->name()->value() == "size") {
								GetDefNumber(CombatantSize, def, filename);
								NewCombatUnit.Size = CombatantSize;
							}
							else if (def->name()->value() == "group") {
								//ParseGroup(def->term()->isStruct(), filename);
								TermStruct* GroupTerm = def->term()->isStruct();

								CombatantType = "";
								ECOMBATGROUP_TYPE EType = ECOMBATGROUP_TYPE::NONE;

								CombatantId = 0;

								for (int i = 0; i < GroupTerm->elements()->size(); i++) {

									TermDef* pdef = GroupTerm->elements()->at(i)->isDef();

									if (pdef->name()->value() == ("type"))
									{
										GetDefText(CombatantType, pdef, filename);
										if (FStringToEnum<ECOMBATGROUP_TYPE>(FString(CombatantType).ToUpper(), EType, false))
										{
											UE_LOG(LogTemp, Log, TEXT("Converted to enum: %d"), static_cast<int32>(EType));
										}
										else
										{
											UE_LOG(LogTemp, Warning, TEXT("Invalid enum string"));
										}

										NewGroupUnit.Type = EType;
									}

									else if (pdef->name()->value() == "id") {
										GetDefNumber(CombatantId, pdef, filename);
										NewGroupUnit.Id = CombatantId;
										UE_LOG(LogTemp, Log, TEXT("%s: %d"), *FString(pdef->name()->value()), CombatantId);
									}
									else if (pdef->name()->value() == "req") {

										TermStruct* val2 = pdef->term()->isStruct();
									}
								}
								NewCombatUnit.Group.Add(NewGroupUnit);
							}
						}
						CombatantArray.Add(NewCombatUnit);
					}
					NewCampaignData.Combatant = CombatantArray;
				}
			}
		}

	} while (term);

	LoadZones(CampaignPath, FString(name));
	LoadMissionList(CampaignPath);
	LoadTemplateList(CampaignPath);
	LoadMission(CampaignPath);
	LoadTemplateMission(CampaignPath);
	LoadScriptedMission(CampaignPath);

	NewCampaignData.Zone = ZoneArray;
	NewCampaignData.MissionList = MissionListArray;
	NewCampaignData.TemplateList = TemplateListArray;
	NewCampaignData.Missions = MissionArray;
	NewCampaignData.TemplateMissions = TemplateMissionArray;
	NewCampaignData.ScriptedMissions = ScriptedMissionArray;
	NewCampaignData.bAvailable = true;
	NewCampaignData.bCompleted = false;

	// define our data table struct
	FName RowName = FName(FString(name));
	CampaignData.RowName = RowName;

	CampaignDataTable->AddRow(RowName, NewCampaignData);
	CampaignData = NewCampaignData;
}

void UStarshatterGameDataSubsystem::ReadCombatRosterData()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadCombatRosterData: BEGIN"));

	CombatRosterData.Empty();

	if (!CombatGroupDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameData] ReadCombatRosterData: CombatGroupDataTable is NULL"));
		return;
	}

	static const FString ContextString(TEXT("ReadCombatRosterData"));

	const TArray<FName> RowNames = CombatGroupDataTable->GetRowNames();

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] ReadCombatRosterData: Found %d rows"),
		RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FS_CombatGroup* Row =
			CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, ContextString);

		if (!Row)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] ReadCombatRosterData: Failed to read row '%s'"),
				*RowName.ToString());
			continue;
		}

		FS_CombatGroup Group = *Row;

		// Preserve / normalize display name if needed
		if (Group.DisplayName.IsEmpty())
		{
			Group.DisplayName = RowName.ToString();
		}

		CombatRosterData.Add(Group);

		UE_LOG(LogTemp, Log,
			TEXT("[GameData] CombatGroup Loaded: Row=%s Type=%d Id=%d ParentType=%d ParentId=%d Empire=%d Iff=%d Name=%s"),
			*RowName.ToString(),
			(int32)Group.Type,
			Group.Id,
			(int32)Group.ParentType,
			Group.ParentId,
			(int32)Group.EmpireId,
			Group.Iff,
			*Group.DisplayName);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] ReadCombatRosterData: COMPLETE (%d groups)"),
		CombatRosterData.Num());
}

void UStarshatterGameDataSubsystem::ReadCampaignData()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadCampaignData: BEGIN"));

	AllCampaigns.Empty();
	CampaignMap.Empty();
	CampaignIndexLookup.Empty();

	if (!CampaignDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameData] ReadCampaignData: CampaignDataTable is NULL"));
		return;
	}

	static const FString ContextString(TEXT("ReadCampaignData"));

	TArray<FName> RowNames = CampaignDataTable->GetRowNames();

	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadCampaignData: Found %d rows"), RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FS_Campaign* Row = CampaignDataTable->FindRow<FS_Campaign>(RowName, ContextString);

		if (!Row)
		{
			UE_LOG(LogTemp, Error, TEXT("[GameData] ReadCampaignData: Failed to read row '%s'"),
				*RowName.ToString());
			continue;
		}

		FS_Campaign Campaign = *Row;

		// Preserve the DataTable row name in the runtime copy
		Campaign.RowName = RowName;

		AllCampaigns.Add(Campaign);
		CampaignMap.Add(RowName, Campaign);

		UE_LOG(LogTemp, Warning, TEXT("[GameData] Campaign Loaded: Row=%s Name=%s Available=%d Index=%d"),
			*Campaign.RowName.ToString(),
			*Campaign.Name,
			Campaign.bAvailable ? 1 : 0,
			Campaign.Index);
	}

	// Build 1-based campaign index lookup after array is fully populated
	for (int32 i = 0; i < AllCampaigns.Num(); ++i)
	{
		const int32 CampaignIndex1Based = AllCampaigns[i].Index + 1;

		if (CampaignIndexLookup.Contains(CampaignIndex1Based))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] ReadCampaignData: Duplicate campaign index %d for row %s"),
				CampaignIndex1Based,
				*AllCampaigns[i].RowName.ToString());
			continue;
		}

		CampaignIndexLookup.Add(CampaignIndex1Based, i);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadCampaignData: COMPLETE (%d campaigns loaded)"),
		AllCampaigns.Num());
}

void UStarshatterGameDataSubsystem::ReadOrderOfBattleData()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadOrderOfBattleData: BEGIN"));

	AllForces.Empty();
	ForceMap.Empty();
	ForceIdLookup.Empty();

	if (!OrderOfBattleDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameData] ReadOrderOfBattleData: OrderOfBattleDataTable is NULL"));
		return;
	}

	static const FString ContextString(TEXT("ReadOrderOfBattleData"));

	TArray<FName> RowNames = OrderOfBattleDataTable->GetRowNames();

	UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadOrderOfBattleData: Found %d rows"), RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FS_OOBForce* Row =
			OrderOfBattleDataTable->FindRow<FS_OOBForce>(RowName, ContextString);

		if (!Row)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[GameData] ReadOrderOfBattleData: Failed to read row '%s'"),
				*RowName.ToString());
			continue;
		}

		FS_OOBForce Force = *Row;

		AllForces.Add(Force);
		ForceMap.Add(RowName, Force);

		UE_LOG(LogTemp, Warning,
			TEXT("[GameData] OOB Force Loaded: Row=%s Name=%s Id=%d Fleets=%d Battalions=%d Civilians=%d"),
			*RowName.ToString(),
			*Force.Name,
			Force.Id,
			Force.Fleet.Num(),
			Force.Battalion.Num(),
			Force.Civilian.Num());
	}

	// Build lookup (by Id)
	for (int32 i = 0; i < AllForces.Num(); ++i)
	{
		const int32 Id = AllForces[i].Id;

		if (ForceIdLookup.Contains(Id))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] Duplicate Force Id %d (%s)"),
				Id,
				*AllForces[i].Name);
			continue;
		}

		ForceIdLookup.Add(Id, i);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] ReadOrderOfBattleData: COMPLETE (%d forces loaded)"),
		AllForces.Num());
}

const FS_OOBForce* UStarshatterGameDataSubsystem::GetForceById(int32 ForceId) const
{
	const int32* FoundIndex = ForceIdLookup.Find(ForceId);

	if (!FoundIndex)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameData] GetForceById: Force not found for Id %d"),
			ForceId);
		return nullptr;
	}

	if (!AllForces.IsValidIndex(*FoundIndex))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameData] GetForceById: Invalid index %d"),
			*FoundIndex);
		return nullptr;
	}

	return &AllForces[*FoundIndex];
}

const FS_Campaign* UStarshatterGameDataSubsystem::GetCampaignByIndex1Based(int32 CampaignIndex1Based) const
{
	if (CampaignIndex1Based <= 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameData] GetCampaignByIndex1Based: Invalid campaign index %d"),
			CampaignIndex1Based);
		return nullptr;
	}

	const int32* FoundArrayIndex = CampaignIndexLookup.Find(CampaignIndex1Based);
	if (!FoundArrayIndex)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GameData] GetCampaignByIndex1Based: Campaign not found for index %d"),
			CampaignIndex1Based);
		return nullptr;
	}

	if (!AllCampaigns.IsValidIndex(*FoundArrayIndex))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GameData] GetCampaignByIndex1Based: Lookup returned invalid array index %d for campaign index %d"),
			*FoundArrayIndex,
			CampaignIndex1Based);
		return nullptr;
	}

	return &AllCampaigns[*FoundArrayIndex];
}

void UStarshatterGameDataSubsystem::LoadZones(
	const FString& Path,
	const FString& CampaignId)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadZones()"));

	FString FileName = Path;
	FileName.Append(TEXT("zones.def"));

	UE_LOG(LogTemp, Log, TEXT("Loading Campaign Zone: %s"), *FileName);

	if (!FPaths::FileExists(FileName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Zones file not found: %s"), *FileName);
		return;
	}

	const char* fn = TCHAR_TO_ANSI(*FileName);

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read zones file: %s"), *FileName);
		return;
	}
	Bytes.Add(0);

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term)
		return;

	{
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "ZONES")
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Zone File %s"), *FileName);
			delete term;
			return;
		}
	}

	int32 ZoneIndex = 0;

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def || def->name()->value() != "zone")
			continue;

		if (!def->term() || !def->term()->isStruct())
		{
			UE_LOG(LogTemp, Warning, TEXT("zone struct missing in '%s'"), *FileName);
			continue;
		}

		TermStruct* val = def->term()->isStruct();

		// Reset per-zone
		Text ParsedZoneRegion = "";
		Text ParsedZoneSystem = "";

		FS_CampaignZone NewCampaignZone;

		for (int i = 0; i < val->elements()->size(); i++)
		{
			TermDef* pdef = val->elements()->at(i)->isDef();
			if (!pdef)
				continue;

			const Text& Key = pdef->name()->value();

			if (Key == "region")
			{
				GetDefText(ParsedZoneRegion, pdef, fn);
				NewCampaignZone.Region = FString(ANSI_TO_TCHAR(ParsedZoneRegion.data())).TrimStartAndEnd();
			}
			else if (Key == "system")
			{
				GetDefText(ParsedZoneSystem, pdef, fn);
				NewCampaignZone.System = FString(ANSI_TO_TCHAR(ParsedZoneSystem.data())).TrimStartAndEnd();
			}
		}

		ZoneArray.Add(NewCampaignZone);
		// If you are building DT_Zones here:
		if (ZonesDataTable)
		{
			const FString RowCampaignId = CampaignId.IsEmpty() ? TEXT("Default") : CampaignId;

			const FName RowName(*FString::Printf(TEXT("%s.Zone%03d"), *RowCampaignId, ZoneIndex));

			if (!ZonesDataTable->GetRowMap().Contains(RowName))
			{
				ZonesDataTable->AddRow(RowName, NewCampaignZone);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DT_Zones already has row '%s' - skipping duplicate"), *RowName.ToString());
			}
		}

		ZoneIndex++;
	} while (term);

	delete term; // defensive
}

// +--------------------------------------------------------------------+

void
UStarshatterGameDataSubsystem::LoadMissionList(FString Path)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadMissionList()"));
	MissionListArray.Empty();

	FString FileName = Path;
	FileName.Append(TEXT("Missions.def"));

	UE_LOG(LogTemp, Log, TEXT("Loading Mission List : %s"), *FileName);

	if (!FPaths::FileExists(FileName))
	{
		UE_LOG(LogTemp, Log, TEXT("Mission List does not exist"));
		return;
	}

	// Keep legacy fn for GetDef* helpers:
	const char* fn = TCHAR_TO_ANSI(*FileName);

	// ------------------------------------------------------------
	// REPLACEMENT FOR DataLoader::LoadBuffer
	// ------------------------------------------------------------
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read mission list file: %s"), *FileName);
		return;
	}
	Bytes.Add(0); // null terminate for BlockReader

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term)
	{
		return;
	}
	else
	{
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "MISSIONLIST")
		{
			UE_LOG(LogTemp, Log, TEXT("WARNING: invalid mission list file '%s'"), *FileName);
			delete term;
			return;
		}
	}

	do
	{
		delete term; term = nullptr;
		term = parser.ParseTerm();

		if (term)
		{
			TermDef* def = term->isDef();
			if (def && def->name()->value() == "mission")
			{
				if (!def->term() || !def->term()->isStruct())
				{
					UE_LOG(LogTemp, Log, TEXT("WARNING: mission struct missing in '%s'"), *FileName);
				}
				else
				{
					TermStruct* val = def->term()->isStruct();

					FS_CampaignMissionList NewMissionList;

					int   MissionId = 0;
					Text  MLName;
					Text  Desc;
					Text  Script;
					Text  Sitrep;
					Text  Objective;
					Text  System = "Unknown";
					Text  Region = "Unknown";
					Text  TypeName = "";
					Text  MissionImage = "";
					Text  MissionAudio = "";
					Text  Start = "";
					int   Type = 0;

					ECOMBATGROUP_TYPE LocalGroupType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
					EMISSIONTYPE LocalMissionType = EMISSIONTYPE::PATROL;

					for (int i = 0; i < val->elements()->size(); i++)
					{
						TermDef* pdef = val->elements()->at(i)->isDef();
						if (!pdef) continue;

						if (pdef->name()->value() == "id")
						{
							GetDefNumber(MissionId, pdef, fn);
							NewMissionList.Id = MissionId;
						}
						else if (pdef->name()->value() == "name")
						{
							GetDefText(MLName, pdef, fn);
							NewMissionList.Name = FString(MLName);
						}
						else if (pdef->name()->value() == "desc")
						{
							GetDefText(Desc, pdef, fn);
							NewMissionList.Description = FString(Desc);
						}
						else if (pdef->name()->value() == "start")
						{
							GetDefText(Start, pdef, fn);
							NewMissionList.Start = FString(Start);
						}
						else if (pdef->name()->value() == "system")
						{
							GetDefText(System, pdef, fn);
							NewMissionList.System = FString(System);
						}
						else if (pdef->name()->value() == "region")
						{
							GetDefText(Region, pdef, fn);
							NewMissionList.Region = FString(Region);
						}
						else if (pdef->name()->value() == "objective")
						{
							GetDefText(Objective, pdef, fn);
							NewMissionList.Objective = FString(Objective);
						}
						else if (pdef->name()->value() == "image")
						{
							GetDefText(MissionImage, pdef, fn);
							NewMissionList.MissionImage = FString(MissionImage);
						}
						else if (pdef->name()->value() == "audio")
						{
							GetDefText(MissionAudio, pdef, fn);
							NewMissionList.MissionAudio = FString(MissionAudio);
						}
						else if (pdef->name()->value() == "sitrep")
						{
							GetDefText(Sitrep, pdef, fn);
							FString SitrepText = FString(Sitrep);
							SitrepText = SitrepText.Replace(TEXT("\n"), TEXT("\\n"));
							NewMissionList.Sitrep = SitrepText;
						}
						else if (pdef->name()->value() == "script")
						{
							GetDefText(Script, pdef, fn);
							NewMissionList.Script = FString(Script);
						}
						else if (pdef->name()->value() == "type")
						{
							GetDefText(TypeName, pdef, fn);
							//Type = Mission::TypeFromName(typestr);
							NewMissionList.TypeName = FString(TypeName);
						}

						// Preserve legacy defaults (set repeatedly in your original loop):
						NewMissionList.Available = true;
						NewMissionList.Complete = false;
						NewMissionList.Status = EMISSIONSTATUS::Available;
					}

					MissionListArray.Add(NewMissionList);
				}
			}
		}

	} while (term);

	delete term; // defensive
	// No Loader->ReleaseBuffer(block) needed (Bytes owns memory)
}

// +--------------------------------------------------------------------+

void
UStarshatterGameDataSubsystem::LoadTemplateList(FString Path)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadTemplateList()"));

	TemplateListArray.Empty();

	FString FileName = Path;
	FileName.Append(TEXT("Templates.def"));

	UE_LOG(LogTemp, Log, TEXT("Loading Template List : %s"), *FileName);

	if (!FPaths::FileExists(FileName))
	{
		UE_LOG(LogTemp, Log, TEXT("Template List does not exist"));
		return;
	}

	// Keep legacy fn for GetDef* helpers:
	const char* fn = TCHAR_TO_ANSI(*FileName);

	// ------------------------------------------------------------
	// REPLACEMENT FOR DataLoader::LoadBuffer
	// ------------------------------------------------------------
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read template list file: %s"), *FileName);
		return;
	}
	Bytes.Add(0); // null terminate for BlockReader

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term)
	{
		return;
	}
	else
	{
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "TEMPLATELIST")
		{
			UE_LOG(LogTemp, Log, TEXT("WARNING: invalid template list file '%s'"), *FileName);
			delete term;
			return;
		}
	}

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (term)
		{
			TermDef* def = term->isDef();
			if (def && def->name()->value() == "mission")
			{
				if (!def->term() || !def->term()->isStruct())
				{
					UE_LOG(LogTemp, Log, TEXT("WARNING: mission struct missing in '%s'"), *FileName);
				}
				else
				{
					TermStruct* val = def->term()->isStruct();

					FS_CampaignTemplateList NewTemplateList;

					Text  TLName = "";
					Text  Script = "";
					Text  Region = "";
					int   id = 0;
					int   msn_type = 0;

					int   min_rank = 0;
					int   max_rank = 0;
					int   action_id = 0;
					int   action_status = 0;
					int   exec_once = 0;
					int   start_before = Game::TIME_NEVER;
					int   start_after = 0;

					EMISSIONTYPE MissionType = EMISSIONTYPE::PATROL;
					ECOMBATGROUP_TYPE TemplateGroupType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;

					for (int i = 0; i < val->elements()->size(); i++)
					{
						TermDef* pdef = val->elements()->at(i)->isDef();
						if (!pdef) continue;

						if (pdef->name()->value() == "id")
						{
							GetDefNumber(id, pdef, fn);
							NewTemplateList.Id = id;
						}
						else if (pdef->name()->value() == "name")
						{
							GetDefText(TLName, pdef, fn);
							NewTemplateList.Name = FString(TLName);
						}
						else if (pdef->name()->value() == "script")
						{
							GetDefText(Script, pdef, fn);
							NewTemplateList.Script = FString(Script);
						}
						else if (pdef->name()->value() == "rgn" || pdef->name()->value() == "region")
						{
							GetDefText(Region, pdef, fn);
							NewTemplateList.Region = FString(Region);
						}
						else if (pdef->name()->value() == "type")
						{
							char typestr[64];
							GetDefText(typestr, pdef, fn);

							const FString NormalizedType =
								NormalizeCombatGroupTypeToken(FString(typestr));

							if (!FStringToEnum<EMISSIONTYPE>(NormalizedType, MissionType, false))
							{
								MissionType = EMISSIONTYPE::OTHER;

								UE_LOG(LogTemp, Warning,
									TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
									*FString(typestr),
									*NormalizedType,
									UTF8_TO_TCHAR(fn));
							}

							NewTemplateList.MissionType = MissionType;
						}
						else if (pdef->name()->value() == "group")
						{
							char typestr[64];
							GetDefText(typestr, pdef, fn);

							const FString NormalizedType =
								NormalizeCombatGroupTypeToken(FString(typestr));
							if (!FStringToEnum<ECOMBATGROUP_TYPE>(NormalizedType, TemplateGroupType, false))
							{
								TemplateGroupType = ECOMBATGROUP_TYPE::NONE;

								UE_LOG(LogTemp, Warning,
									TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
									*FString(typestr),
									*NormalizedType,
									UTF8_TO_TCHAR(fn));
							}

							NewTemplateList.GroupType = TemplateGroupType;
						}
						else if (pdef->name()->value() == "min_rank")
						{
							GetDefNumber(min_rank, pdef, fn);
							NewTemplateList.MinRank = min_rank;
						}
						else if (pdef->name()->value() == "max_rank")
						{
							GetDefNumber(max_rank, pdef, fn);
							NewTemplateList.MaxRank = max_rank;
						}
						else if (pdef->name()->value() == "action_id")
						{
							GetDefNumber(action_id, pdef, fn);
							NewTemplateList.ActionId = action_id;
						}
						else if (pdef->name()->value() == "action_status")
						{
							GetDefNumber(action_status, pdef, fn);
							NewTemplateList.ActionStatus = action_status;
						}
						else if (pdef->name()->value() == "exec_once")
						{
							GetDefNumber(exec_once, pdef, fn);
							NewTemplateList.ExecOnce = exec_once;
						}
						else if (pdef->name()->value().contains("before"))
						{
							if (pdef->term() && pdef->term()->isNumber())
							{
								GetDefNumber(start_before, pdef, fn);
								NewTemplateList.StartBefore = start_before;
							}
							else
							{
								GetDefTime(start_before, pdef, fn);
								start_before -= Game::ONE_DAY;
								NewTemplateList.StartBefore = start_before;
							}
						}
						else if (pdef->name()->value().contains("after"))
						{
							if (pdef->term() && pdef->term()->isNumber())
							{
								GetDefNumber(start_after, pdef, fn);
								NewTemplateList.StartAfter = start_after;
							}
							else
							{
								GetDefTime(start_after, pdef, fn);
								start_after -= Game::ONE_DAY;
								NewTemplateList.StartAfter = start_after;
							}
						}
					}

					TemplateListArray.Add(NewTemplateList);
				}
			}
		}

	} while (term);

	delete term; 
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::LoadMission(FString Path)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadMission()"));

	// Use the passed-in Path (not CampaignPath), but keep legacy behavior if you intended CampaignPath:
	const FString ScenesDir = Path / TEXT("Scenes/");

	TArray<FString> Files;
	Files.Empty();

	const FString Pattern = ScenesDir / TEXT("*.def");
	FFileManagerGeneric::Get().FindFiles(Files, *Pattern, true, false);

	for (const FString& LeafName : Files)
	{
		const FString FullPath = ScenesDir / LeafName;

		// Keep legacy ParseMission signature:
		const char* fn = TCHAR_TO_ANSI(*FullPath);
		ParseMission(fn);
	}
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::LoadTemplateMission(FString Name)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadTemplateMission()"));

	// NOTE: Keeping legacy behavior: ignores Name and uses CampaignPath/Templates/.
	const FString TemplatesDir = CampaignPath / TEXT("Templates/");

	TArray<FString> Files;
	Files.Empty();

	const FString Pattern = TemplatesDir / TEXT("*.def");
	FFileManagerGeneric::Get().FindFiles(Files, *Pattern, true, false);

	for (const FString& LeafName : Files)
	{
		const FString FullPath = TemplatesDir / LeafName;

		// Keep legacy ParseMissionTemplate signature:
		const char* fn = TCHAR_TO_ANSI(*FullPath);
		ParseMissionTemplate(fn);
	}
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::LoadScriptedMission(FString Name)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadScriptedMission()"));

	// Legacy behavior: ignore Name, use CampaignPath/Scripts/
	const FString ScriptsDir = CampaignPath / TEXT("Scripts/");

	TArray<FString> Files;
	Files.Empty();

	const FString Pattern = ScriptsDir / TEXT("*.def");
	FFileManagerGeneric::Get().FindFiles(Files, *Pattern, true, false);

	for (const FString& LeafName : Files)
	{
		const FString FullPath = ScriptsDir / LeafName;

		// Preserve legacy parser interface
		const char* fn = TCHAR_TO_ANSI(*FullPath);
		ParseScriptedTemplate(fn);
	}
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseMission(const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMission()"));

	if (!fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseMission called with null/empty filename"));
		return;
	}

	// Use a local name that cannot collide with a class member:
	const FString MissionPath = ANSI_TO_TCHAR(fn);

	MissionElementArray.Empty();
	MissionEventArray.Empty();

	if (!FPaths::FileExists(MissionPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MISSION file not found: %s"), *MissionPath);
		return;
	}

	// Read raw bytes (preserves legacy parser expectations)
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *MissionPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read MISSION file: %s"), *MissionPath);
		return;
	}

	// Null-terminate for BlockReader / legacy parsing
	Bytes.Add(0);

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term)
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING: could not parse MISSION file '%s'"), *MissionPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MISSION file '%s'"), *MissionPath);

	FS_CampaignMission NewMission;

	int     id = 0;

	Text    Region = "";
	Text    Scene = "";
	Text    System = "";
	Text    Subtitles = "";
	Text    Name = "";
	Text    Desc = "";
	Text    TargetName = "";
	Text    WardName = "";
	Text    Objective = "";
	Text    Sitrep = "";
	Text	LocalType = "";
	Text    MissionType = "";

	EMISSIONTYPE Type = EMISSIONTYPE::PATROL;
	int     Team = 0;
	int     Id = 0;
	Text    Start = ""; 
	Text    After = "";

	double  Stardate = 0.0;
	bool    Degrees = false;
	bool    Once = false;

	ECOMBATGROUP_TYPE LocalGroupType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
	EMISSIONTYPE LocalMissionType = EMISSIONTYPE::PATROL;

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def)
			continue;

		const Text& Key = def->name()->value();

		if (Key == "name")
		{
			GetDefText(Name, def, fn);
			NewMission.MissionName = FString(Name);
			UE_LOG(LogTemp, Log, TEXT("mission name '%s'"), *FString(Name));
		}
		else if (Key == "scene")
		{
			GetDefText(Scene, def, fn);
			NewMission.Scene = FString(Scene);
		}
		else if (Key == "desc")
		{
			GetDefText(Desc, def, fn);
			if (Desc.length() > 0 && Desc.length() < 32)
				NewMission.Desc = FString(Desc);
		}
		else if (Key == "type")
		{
			char typestr[64];
			GetDefText(typestr, def, fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(typestr));

			if (!FStringToEnum<EMISSIONTYPE>(NormalizedType, LocalMissionType, false))
			{
				LocalMissionType = EMISSIONTYPE::OTHER;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(typestr),
					*NormalizedType,
					UTF8_TO_TCHAR(fn));
			}

			NewMission.MissionType = LocalMissionType;
		}
		else if (Key == "system")
		{
			GetDefText(System, def, fn);
			NewMission.MissionSystem = FString(System);
		}
		else if (Key == "region")
		{
			GetDefText(Region, def, fn);
			NewMission.MissionRegion = FString(Region);
		}
		else if (Key == "degrees")
		{
			GetDefBool(Degrees, def, fn);
			NewMission.Degrees = Degrees;
		}
		else if (Key == "exec_once")
		{
			GetDefBool(Once, def, fn);
			NewMission.Degrees = Degrees;
		}
		else if (Key == "objective")
		{
			GetDefText(Objective, def, fn);
			if (Objective.length() > 0 && Objective.length() < 32)
				NewMission.Objective = FString(Objective);
		}
		else if (Key == "sitrep")
		{
			GetDefText(Sitrep, def, fn);
			if (Sitrep.length() > 0 && Sitrep.length() < 32)
				NewMission.Sitrep = FString(Sitrep);
		}
		else if (Key == "subtitles")
		{
			GetDefText(Subtitles, def, fn);
			NewMission.Subtitles = FString(Subtitles);
		}
		else if (Key == "start")
		{
			GetDefText(Start, def, fn);
			NewMission.StartTime = FString(Start);
		}
		else if (Key == "after")
		{
			GetDefText(After, def, fn);
			NewMission.StartAfter = FString(After);
		}
		else if (Key == "stardate")
		{
			GetDefNumber(Stardate, def, fn);
			NewMission.Stardate = Stardate;
		}
		else if (Key == "id")
		{
			GetDefNumber(Id, def, fn);
			NewMission.MissionId = Id;
		}
		else if (Key == "team")
		{
			GetDefNumber(Team, def, fn);
			NewMission.Team = Team;
		}
		else if (Key == "target")
		{
			GetDefText(TargetName, def, fn);
			NewMission.TargetName = FString(TargetName);
		}
		else if (Key == "ward")
		{
			GetDefText(WardName, def, fn);
			NewMission.WardName = FString(WardName);
		}
		else if (Key == "group")
		{
			GetDefText(LocalType, def, fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(LocalType));

			if (!FStringToEnum<ECOMBATGROUP_TYPE>(NormalizedType, LocalGroupType, false))
			{
				LocalGroupType = ECOMBATGROUP_TYPE::NONE;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(LocalType),
					*NormalizedType,
					UTF8_TO_TCHAR(fn));
			}

			NewMission.Group = LocalGroupType;
		}
		else if (Key == "event")
		{
			if (def->term() && def->term()->isStruct())
			{
				ParseEvent(def->term()->isStruct(), fn);
				NewMission.Event = MissionEventArray;
			}
		}
		else if (Key == "element" || Key == "ship" || Key == "station")
		{
			if (def->term() && def->term()->isStruct())
			{
				ParseElement(def->term()->isStruct(), fn);
				NewMission.Element = MissionElementArray;
			}
		}

	} while (term);

	if (term)
	{
		delete term;
		term = nullptr;
	}

	MissionArray.Add(NewMission);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseNavpoint(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseNavpoint()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseNavpoint called with null args"));
		return;
	}

	// ---- legacy defaults ----
	int32 Formation = 0;
	int32 Speed = 0;
	int32 Priority = 1;
	int32 Farcast = 0;
	int32 Hold = 0;
	int32 EMCON = 0;

	FVector Location = FVector::ZeroVector;

	Text OrderName = "";
	Text StatusName = "";
	Text RegionName = "";
	Text TargetName = "";
	Text TargetDesc = "";

	FS_MissionInstruction NewInstr;

	// Legacy behavior: rlocs are per-navpoint
	MissionRLocArray.Empty();

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "cmd")
		{
			GetDefText(OrderName, PDef, Fn);
			NewInstr.OrderName = FString(OrderName);
		}
		else if (Key == "status")
		{
			GetDefText(StatusName, PDef, Fn);
			NewInstr.StatusName = FString(StatusName);
		}
		else if (Key == "loc")
		{
			// Use FVector only
			FVector V;
			GetDefVec(V, PDef, Fn);
			NewInstr.Location = V;
		}
		else if (Key == "rloc")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseRLoc(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "rgn")
		{
			GetDefText(RegionName, PDef, Fn);
			NewInstr.OrderRegionName = FString(RegionName);
		}
		else if (Key == "speed")
		{
			GetDefNumber(Speed, PDef, Fn);
			NewInstr.Speed = Speed;
		}
		else if (Key == "formation")
		{
			GetDefNumber(Formation, PDef, Fn);
			NewInstr.Formation = Formation;
		}
		else if (Key == "emcon")
		{
			GetDefNumber(EMCON, PDef, Fn);
			NewInstr.EMCON = EMCON;
		}
		else if (Key == "priority")
		{
			GetDefNumber(Priority, PDef, Fn);
			NewInstr.Priority = Priority;
		}
		else if (Key == "farcast")
		{
			// Legacy allows farcast as bool or number
			if (PDef->term() && PDef->term()->isBool())
			{
				bool b = false;
				GetDefBool(b, PDef, Fn);
				Farcast = b ? 1 : 0;
			}
			else
			{
				GetDefNumber(Farcast, PDef, Fn);
			}

			NewInstr.Farcast = Farcast;
		}
		else if (Key == "tgt")
		{
			GetDefText(TargetName, PDef, Fn);
			NewInstr.TargetName = FString(TargetName);
		}
		else if (Key == "tgt_desc")
		{
			GetDefText(TargetDesc, PDef, Fn);
			NewInstr.TargetDesc = FString(TargetDesc);
		}
		else
		{
			// legacy: hold, hold1, hold2, etc.
			const char* KeyC = Key; // Text ? const char* (Starshatter-style)
			if (KeyC && !strncmp(KeyC, "hold", 4))
			{
				GetDefNumber(Hold, PDef, Fn);
				NewInstr.Hold = Hold;
			}
		}
	}

	// Assign accumulated rlocs once
	NewInstr.RLoc = MissionRLocArray;

	MissionNavpointArray.Add(NewInstr);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseObjective(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseObjective()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseObjective called with null args"));
		return;
	}

	// ---- legacy defaults ----
	int32 Formation = 0;
	int32 Speed = 0;
	int32 Priority = 1;
	int32 Farcast = 0;
	int32 Hold = 0;
	int32 EMCON = 0;

	Text OrderName = "";
	Text StatusName = "";
	Text RegionName = "";
	Text TargetName = "";
	Text TargetDesc = "";

	FS_MissionInstruction NewObj;

	// Legacy behavior: rlocs are per-objective; clear before parsing this objective.
	// NOTE: Assumes ParseRLoc appends into MissionRLocArray (member scratch).
	MissionRLocArray.Empty();

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "cmd")
		{
			GetDefText(OrderName, PDef, Fn);
			NewObj.OrderName = FString(OrderName);
		}
		else if (Key == "status")
		{
			GetDefText(StatusName, PDef, Fn);
			NewObj.StatusName = FString(StatusName);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewObj.Location = V;
		}
		else if (Key == "rloc")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseRLoc(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "rgn")
		{
			GetDefText(RegionName, PDef, Fn);
			NewObj.OrderRegionName = FString(RegionName);
		}
		else if (Key == "speed")
		{
			GetDefNumber(Speed, PDef, Fn);
			NewObj.Speed = Speed;
		}
		else if (Key == "formation")
		{
			GetDefNumber(Formation, PDef, Fn);
			NewObj.Formation = Formation;
		}
		else if (Key == "emcon")
		{
			GetDefNumber(EMCON, PDef, Fn);
			NewObj.EMCON = EMCON;
		}
		else if (Key == "priority")
		{
			GetDefNumber(Priority, PDef, Fn);
			NewObj.Priority = Priority;
		}
		else if (Key == "farcast")
		{
			// Legacy allows farcast as bool or number
			if (PDef->term() && PDef->term()->isBool())
			{
				bool b = false;
				GetDefBool(b, PDef, Fn);
				Farcast = b ? 1 : 0;
			}
			else
			{
				GetDefNumber(Farcast, PDef, Fn);
			}

			NewObj.Farcast = Farcast;
		}
		else if (Key == "tgt")
		{
			GetDefText(TargetName, PDef, Fn);
			NewObj.TargetName = FString(TargetName);
		}
		else if (Key == "tgt_desc")
		{
			GetDefText(TargetDesc, PDef, Fn);
			NewObj.TargetDesc = FString(TargetDesc);
		}
		else
		{
			// legacy: hold, hold1, hold2, etc.
			const char* KeyC = Key; // Text -> const char* (Starshatter style)
			if (KeyC && !strncmp(KeyC, "hold", 4))
			{
				GetDefNumber(Hold, PDef, Fn);
				NewObj.Hold = Hold;
			}
		}
	}

	// Assign accumulated rlocs once
	NewObj.RLoc = MissionRLocArray;

	MissionObjectiveArray.Add(NewObj);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseInstruction(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseInstruction()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseInstruction called with null args"));
		return;
	}

	// ---- legacy defaults ----
	int32 Formation = 0;
	int32 Speed = 0;
	int32 Priority = 1;
	int32 Farcast = 0;
	int32 Hold = 0;
	int32 EMCON = 0;

	Text OrderName = "";
	Text StatusName = "";
	Text RegionName = "";
	Text TargetName = "";
	Text TargetDesc = "";

	FS_MissionInstruction NewInstr;

	// Legacy behavior: rlocs are per-instruction; clear before parsing this instruction.
	// NOTE: Assumes ParseRLoc appends into MissionRLocArray (member scratch).
	MissionRLocArray.Empty();

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "cmd")
		{
			GetDefText(OrderName, PDef, Fn);
			NewInstr.OrderName = FString(OrderName);
		}
		else if (Key == "status")
		{
			GetDefText(StatusName, PDef, Fn);
			NewInstr.StatusName = FString(StatusName);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewInstr.Location = V;
		}
		else if (Key == "rloc")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseRLoc(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "rgn")
		{
			GetDefText(RegionName, PDef, Fn);
			NewInstr.OrderRegionName = FString(RegionName);
		}
		else if (Key == "speed")
		{
			GetDefNumber(Speed, PDef, Fn);
			NewInstr.Speed = Speed;
		}
		else if (Key == "formation")
		{
			GetDefNumber(Formation, PDef, Fn);
			NewInstr.Formation = Formation;
		}
		else if (Key == "emcon")
		{
			GetDefNumber(EMCON, PDef, Fn);
			NewInstr.EMCON = EMCON;
		}
		else if (Key == "priority")
		{
			GetDefNumber(Priority, PDef, Fn);
			NewInstr.Priority = Priority;
		}
		else if (Key == "farcast")
		{
			// Legacy allows farcast as bool or number
			if (PDef->term() && PDef->term()->isBool())
			{
				bool b = false;
				GetDefBool(b, PDef, Fn);
				Farcast = b ? 1 : 0;
			}
			else
			{
				GetDefNumber(Farcast, PDef, Fn);
			}

			NewInstr.Farcast = Farcast;
		}
		else if (Key == "tgt")
		{
			GetDefText(TargetName, PDef, Fn);
			NewInstr.TargetName = FString(TargetName);
		}
		else if (Key == "tgt_desc")
		{
			GetDefText(TargetDesc, PDef, Fn);
			NewInstr.TargetDesc = FString(TargetDesc);
		}
		else
		{
			// legacy: hold, hold1, hold2, etc.
			const char* KeyC = Key; // Text -> const char* (Starshatter style)
			if (KeyC && !strncmp(KeyC, "hold", 4))
			{
				GetDefNumber(Hold, PDef, Fn);
				NewInstr.Hold = Hold;
			}
		}
	}

	// Assign accumulated rlocs once
	NewInstr.RLoc = MissionRLocArray;

	MissionInstructionArray.Add(NewInstr);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseShip(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseShip()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseShip called with null args"));
		return;
	}

	// --- strings ---
	Text ShipName = "";
	Text SkinName = "";
	Text RegNum = "";
	Text Region = "";

	// --- scalars ---
	int32  Respawns = -1;
	double Heading = -1.0e9;
	double Integrity = -1.0;

	// --- arrays (legacy) ---
	int32 Ammo[16];
	int32 Fuel[4];

	for (int32 i = 0; i < 16; ++i) Ammo[i] = -10;
	for (int32 i = 0; i < 4; ++i) Fuel[i] = -10;

	FS_MissionShip NewMissionShip;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)   // <-- renamed from Index
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(ShipName, PDef, Fn);
			NewMissionShip.ShipName = FString(ShipName);
		}
		else if (Key == "skin")
		{
			GetDefText(SkinName, PDef, Fn);
			NewMissionShip.SkinName = FString(SkinName);
		}
		else if (Key == "regnum")
		{
			GetDefText(RegNum, PDef, Fn);
			NewMissionShip.RegNum = FString(RegNum);
		}
		else if (Key == "region")
		{
			GetDefText(Region, PDef, Fn);
			NewMissionShip.Region = FString(Region);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewMissionShip.Location = V;
		}
		else if (Key == "velocity")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewMissionShip.Velocity = V;
		}
		else if (Key == "respawns")
		{
			GetDefNumber(Respawns, PDef, Fn);
			NewMissionShip.Respawns = Respawns;
		}
		else if (Key == "heading")
		{
			GetDefNumber(Heading, PDef, Fn);
			NewMissionShip.Heading = Heading;
		}
		else if (Key == "integrity")
		{
			GetDefNumber(Integrity, PDef, Fn);
			NewMissionShip.Integrity = Integrity;
		}
		else if (Key == "ammo")
		{
			GetDefArray(Ammo, 16, PDef, Fn);

			// If FS_MissionShip::Ammo is TArray<int32>
			NewMissionShip.Ammo.SetNum(16);
			for (int32 i = 0; i < 16; ++i)
			{
				NewMissionShip.Ammo[i] = Ammo[i];
			}
		}
		else if (Key == "fuel")
		{
			GetDefArray(Fuel, 4, PDef, Fn);

			// If FS_MissionShip::Fuel is TArray<int32>
			NewMissionShip.Fuel.SetNum(4);
			for (int32 i = 0; i < 4; ++i)
			{
				NewMissionShip.Fuel[i] = Fuel[i];
			}
		}
	}

	MissionShipArray.Add(NewMissionShip);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseMissionLoadout(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMissionLoadout()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseLoadout called with null args"));
		return;
	}

	int  ship = -1;
	int  stations[16];
	Text LoadoutName;

	for (int i = 0; i < 16; i++)
		stations[i] = -1;

	FS_MissionLoadout NewLoadout;

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "ship")
		{
			GetDefNumber(ship, pdef, fn);
			NewLoadout.Ship = ship;
		}
		else if (Key == "name")
		{
			GetDefText(LoadoutName, pdef, fn);
			NewLoadout.LoadoutName = FString(LoadoutName);
		}
		else if (Key == "stations")
		{
			GetDefArray(stations, 16, pdef, fn);

			// If FS_MissionLoadout::Stations is a TArray<int32>:
			NewLoadout.Stations.SetNum(16);
			for (int index = 0; index < 16; index++)
			{
				NewLoadout.Stations[index] = stations[index];
			}

			// If FS_MissionLoadout::Stations is a fixed C array: int32 Stations[16];
			// for (int index = 0; index < 16; index++)
			// {
			//     NewLoadout.Stations[index] = stations[index];
			// }
		}
	}

	MissionLoadoutArray.Add(NewLoadout);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseEvent(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseEvent()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseEvent called with null args"));
		return;
	}

	// ---- strings ----
	Text EventShip = "";
	Text EventSource = "";
	Text EventTarget = "";
	Text EventMessage = "";
	Text EventSound = "";
	Text TriggerShip = "";
	Text TriggerTarget = "";

	// ---- scalars ----
	int32  EventId = 1;
	int32  EventChance = 0;
	int32  EventDelay = 0;
	double EventTime = 0.0;

	// ---- FVector only ----
	FVector EventPoint = FVector::ZeroVector;

	// ---- rect ----
	Rect EventRect{}; // legacy rect (x,y,w,h)

	// ---- params ----
	int32 EventParam[10];
	int32 TriggerParam[10];
	int32 EventNParams = 0;
	int32 TriggerNParams = 0;

	for (int32 k = 0; k < 10; ++k)
	{
		EventParam[k] = 0;
		TriggerParam[k] = 0;
	}

	MISSIONEVENT_TYPE EventType = MISSIONEVENT_TYPE::MESSAGE;
	MISSIONEVENT_TRIGGER EventTrigger = MISSIONEVENT_TRIGGER::TRIGGER_EVENT;

	FS_MissionEvent NewMissionEvent;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			char typestr[64];
			GetDefText(typestr, PDef, Fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(typestr));

			if (!FStringToEnum<MISSIONEVENT_TYPE>(NormalizedType, EventType, false))
			{
				EventType = MISSIONEVENT_TYPE::MESSAGE;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseEvent] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(typestr),
					*NormalizedType,
					UTF8_TO_TCHAR(Fn));
			}

			NewMissionEvent.EventType = EventType;
		}
		if (Key == "trigger")
		{
			char typestr[64];
			GetDefText(typestr, PDef, Fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(typestr));

			if (!FStringToEnum<MISSIONEVENT_TRIGGER>(NormalizedType, EventTrigger, false))
			{
				EventTrigger = MISSIONEVENT_TRIGGER::TRIGGER_EVENT;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseEvent] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(typestr),
					*NormalizedType,
					UTF8_TO_TCHAR(Fn));
			}

			NewMissionEvent.EventTrigger = EventTrigger;
		}
		else if (Key == "id")
		{
			GetDefNumber(EventId, PDef, Fn);
			NewMissionEvent.EventId = EventId;
		}
		else if (Key == "time")
		{
			GetDefNumber(EventTime, PDef, Fn);
			NewMissionEvent.EventTime = EventTime;
		}
		else if (Key == "delay")
		{
			GetDefNumber(EventDelay, PDef, Fn);
			NewMissionEvent.EventDelay = EventDelay;
		}
		else if (Key == "event_param" || Key == "param" || Key == "color")
		{
			if (PDef->term() && PDef->term()->isNumber())
			{
				GetDefNumber(EventParam[0], PDef, Fn);
				EventNParams = 1;

				NewMissionEvent.EventParam[0] = EventParam[0];
				NewMissionEvent.EventNParams = EventNParams;
			}
			else if (PDef->term() && PDef->term()->isArray())
			{
				std::vector<float> PList;
				GetDefArray(PList, PDef, Fn);

				EventNParams = 0;
				for (int32 idx = 0; idx < 10 && idx < (int32)PList.size(); ++idx)
				{
					EventParam[idx] = (int32)PList[idx];
					NewMissionEvent.EventParam[idx] = EventParam[idx];
					EventNParams = idx + 1;
				}

				NewMissionEvent.EventNParams = EventNParams;
			}
		}
		else if (Key == "trigger_param")
		{
			if (PDef->term() && PDef->term()->isNumber())
			{
				GetDefNumber(TriggerParam[0], PDef, Fn);
				TriggerNParams = 1;

				NewMissionEvent.TriggerParam[0] = TriggerParam[0];
				NewMissionEvent.TriggerNParams = TriggerNParams; // correct
			}
			else if (PDef->term() && PDef->term()->isArray())
			{
				std::vector<float> PList;
				GetDefArray(PList, PDef, Fn);

				TriggerNParams = 0;
				for (int32 ti = 0; ti < 10 && ti < (int32)PList.size(); ++ti)
				{
					TriggerParam[ti] = (int32)PList[ti];
					NewMissionEvent.TriggerParam[ti] = TriggerParam[ti];
					TriggerNParams = ti + 1;
				}

				NewMissionEvent.TriggerNParams = TriggerNParams;
			}
		}
		else if (Key == "event_ship" || Key == "ship")
		{
			GetDefText(EventShip, PDef, Fn);
			NewMissionEvent.EventShip = FString(EventShip);
		}
		else if (Key == "event_source" || Key == "source" || Key == "font")
		{
			GetDefText(EventSource, PDef, Fn);
			NewMissionEvent.EventSource = FString(EventSource);
		}
		else if (Key == "event_target" || Key == "target" || Key == "image")
		{
			GetDefText(EventTarget, PDef, Fn);
			NewMissionEvent.EventTarget = FString(EventTarget);
		}
		else if (Key == "event_message" || Key == "message")
		{
			GetDefText(EventMessage, PDef, Fn);
			NewMissionEvent.EventMessage = FString(EventMessage);
		}
		else if (Key == "event_chance" || Key == "chance")
		{
			GetDefNumber(EventChance, PDef, Fn);
			NewMissionEvent.EventChance = EventChance;
		}
		else if (Key == "event_sound" || Key == "sound")
		{
			GetDefText(EventSound, PDef, Fn);
			NewMissionEvent.EventSound = FString(EventSound);
		}
		else if (Key == "loc" || Key == "vec" || Key == "fade")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewMissionEvent.EventPoint = V;
		}
		else if (Key == "rect")
		{
			GetDefRect(EventRect, PDef, Fn);

			// Store rect as XYWH (x,y,w,h) in FVector4:
			NewMissionEvent.EventRect = FVector4(
				(float)EventRect.x,
				(float)EventRect.y,
				(float)EventRect.w,
				(float)EventRect.h
			);

			// Alternative if your FS_MissionEvent uses FIntRect:
			// NewMissionEvent.EventRect = FIntRect(EventRect.x, EventRect.y, EventRect.x + EventRect.w, EventRect.y + EventRect.h);
		}
		else if (Key == "trigger_ship")
		{
			GetDefText(TriggerShip, PDef, Fn);
			NewMissionEvent.TriggerShip = FString(TriggerShip);
		}
		else if (Key == "trigger_target")
		{
			GetDefText(TriggerTarget, PDef, Fn);
			NewMissionEvent.TriggerTarget = FString(TriggerTarget);
		}
	}

	MissionEventArray.Add(NewMissionEvent);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseElement(TermStruct* Eval, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseElement()"));

	if (!Eval || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseElement called with null args"));
		return;
	}

	// ---- strings ----
	Text ElementName = "";
	Text Carrier = "";
	Text Commander = "";
	Text Squadron = "";
	Text Path = "";
	Text Design = "";
	Text SkinName = "";
	Text RegionName = "";
	Text Instr = "";
	Text ElementIntel = "";

	// ---- FVector only ----
	FVector ElementLoc = FVector::ZeroVector;

	// ---- ints/bools ----
	int32 Deck = 1;  // (not used below but preserved if you add it later)
	int32 IFFCode = 0;
	int32 Count = 1;
	int32 MaintCount = 0;
	int32 DeadCount = 0;
	int32 Player = 0;
	int32 CommandAI = 0;
	int32 Respawns = 0;
	int32 HoldTime = 0;
	int32 ZoneLock = 0;
	int32 Heading = 0;

	bool bAlert = false;
	bool bPlayable = false;
	bool bRogue = false;
	bool bInvulnerable = false;

	EINTEL_TYPE LocalIntelType = EINTEL_TYPE::KNOWN;
	EMISSIONTYPE LocalMissionType = EMISSIONTYPE::PATROL;

	// Scratch arrays used by nested parsers:
	MissionLoadoutArray.Empty();
	MissionRLocArray.Empty();
	MissionInstructionArray.Empty();
	MissionNavpointArray.Empty();
	MissionShipArray.Empty();

	FS_MissionElement NewMissionElement;

	const int32 ElemCount = (int32)Eval->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Eval->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(ElementName, PDef, Fn);
			NewMissionElement.Name = FString(ElementName);
		}
		else if (Key == "carrier")
		{
			GetDefText(Carrier, PDef, Fn);
			NewMissionElement.Carrier = FString(Carrier);
		}
		else if (Key == "commander")
		{
			GetDefText(Commander, PDef, Fn);
			NewMissionElement.Commander = FString(Commander);
		}
		else if (Key == "squadron")
		{
			GetDefText(Squadron, PDef, Fn);
			NewMissionElement.Squadron = FString(Squadron);
		}
		else if (Key == "path")
		{
			GetDefText(Path, PDef, Fn);
			NewMissionElement.Path = FString(Path);
		}
		else if (Key == "design")
		{
			GetDefText(Design, PDef, Fn);
			NewMissionElement.Design = FString(Design);
		}
		else if (Key == "skin")
		{
			GetDefText(SkinName, PDef, Fn);
			NewMissionElement.SkinName = FString(SkinName);
		}
		else if (Key == "mission")
		{
			Text RoleName = "";
			GetDefText(RoleName, PDef, Fn);

			if (!FStringToEnum<EMISSIONTYPE>(FString(RoleName).ToUpper(), LocalMissionType, false))
				LocalMissionType = EMISSIONTYPE::PATROL;

			NewMissionElement.RoleName = LocalMissionType;
		}
		else if (Key == "intel")
		{
			Text Intel = "";
			GetDefText(Intel, PDef, Fn);

			if (!FStringToEnum<EINTEL_TYPE>(FString(Intel).ToUpper(), LocalIntelType, false))
				LocalIntelType = EINTEL_TYPE::KNOWN;

			NewMissionElement.Intel = LocalIntelType;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			NewMissionElement.Location = V;
		}
		else if (Key == "rloc")
		{
			// FIX: rloc belongs to ParseRLoc, not ParseLoadout
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseRLoc(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "head")
		{
			GetDefNumber(Heading, PDef, Fn);
			NewMissionElement.Heading = Heading;
		}
		else if (Key == "region" || Key == "rgn")
		{
			GetDefText(RegionName, PDef, Fn);
			NewMissionElement.RegionName = FString(RegionName);
		}
		else if (Key == "iff")
		{
			GetDefNumber(IFFCode, PDef, Fn);
			NewMissionElement.IFFCode = IFFCode;
		}
		else if (Key == "count")
		{
			GetDefNumber(Count, PDef, Fn);
			NewMissionElement.Count = Count;
		}
		else if (Key == "maint_count")
		{
			GetDefNumber(MaintCount, PDef, Fn);
			NewMissionElement.MaintCount = MaintCount;
		}
		else if (Key == "dead_count")
		{
			GetDefNumber(DeadCount, PDef, Fn);
			NewMissionElement.DeadCount = DeadCount;
		}
		else if (Key == "player")
		{
			GetDefNumber(Player, PDef, Fn);
			NewMissionElement.Player = Player;
		}
		else if (Key == "alert")
		{
			GetDefBool(bAlert, PDef, Fn);
			NewMissionElement.Alert = bAlert;
		}
		else if (Key == "playable")
		{
			GetDefBool(bPlayable, PDef, Fn);
			NewMissionElement.Playable = bPlayable;
		}
		else if (Key == "rogue")
		{
			GetDefBool(bRogue, PDef, Fn);
			NewMissionElement.Rogue = bRogue;
		}
		else if (Key == "invulnerable")
		{
			GetDefBool(bInvulnerable, PDef, Fn);
			NewMissionElement.Invulnerable = bInvulnerable;
		}
		else if (Key == "command_ai")
		{
			GetDefNumber(CommandAI, PDef, Fn);
			NewMissionElement.CommandAI = CommandAI;
		}
		else if (Key == "respawn")
		{
			GetDefNumber(Respawns, PDef, Fn);
			NewMissionElement.Respawns = Respawns;
		}
		else if (Key == "hold")
		{
			GetDefNumber(HoldTime, PDef, Fn);
			NewMissionElement.HoldTime = HoldTime;
		}
		else if (Key == "zone")
		{
			GetDefNumber(ZoneLock, PDef, Fn);
			NewMissionElement.ZoneLock = ZoneLock;
		}
		else if (Key == "objective")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("Mission error - No objective in '%s'"), *FString(Fn));
			}
			else
			{
				// Objective is an instruction-style struct in your pipeline:
				ParseInstruction(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "instr")
		{
			GetDefText(Instr, PDef, Fn);
			NewMissionElement.Instr = FString(Instr);
		}
		else if (Key == "ship")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("Mission error - no ship in '%s'"), *FString(Fn));
			}
			else
			{
				ParseShip(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "order" || Key == "navpt")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("Mission error - no navpt in '%s'"), *FString(Fn));
			}
			else
			{
				ParseNavpoint(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "loadout")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("Mission error - no loadout in '%s'"), *FString(Fn));
			}
			else
			{
				ParseMissionLoadout(PDef->term()->isStruct(), Fn);
			}
		}
	}

	// Assign accumulated nested arrays once (supports multiple blocks cleanly)
	NewMissionElement.Loadout = MissionLoadoutArray;
	NewMissionElement.RLoc = MissionRLocArray;
	NewMissionElement.Instruction = MissionInstructionArray;
	NewMissionElement.Navpoint = MissionNavpointArray;
	NewMissionElement.Ship = MissionShipArray;

	MissionElementArray.Add(NewMissionElement);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseScriptedTemplate(const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseScriptedTemplate()"));

	if (!fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseScriptedTemplate called with null/empty filename"));
		return;
	}

	// Don't name this FilePath (it collides with your class member):
	const FString ScriptPath = ANSI_TO_TCHAR(fn);

	if (!FPaths::FileExists(ScriptPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Scripted template file not found: %s"), *ScriptPath);
		return;
	}

	// ---- UE-native raw bytes load (preserves legacy parser expectations) ----
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *ScriptPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read scripted template file: %s"), *ScriptPath);
		return;
	}

	// Null-terminate for BlockReader/legacy char* parsing:
	Bytes.Add(0);

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	// Reset scratch arrays for this template parse:
	MissionElementArray.Empty();
	MissionEventArray.Empty();
	MissionCallsignArray.Empty();
	MissionOptionalArray.Empty();
	MissionAliasArray.Empty();

	if (!term)
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING: could not parse '%s'"), *ScriptPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MISSIONTEMPLATE file '%s'"), *ScriptPath);

	// ----------------------------
	// Template-level locals
	// ----------------------------
	Text  TargetName = "";
	Text  WardName = "";
	Text  MissionName = "";
	Text  MissionSystem = "";
	Text  MissionRegion = "";
	Text  TemplateObjective = "";
	Text  TemplateSitrep = "";
	Text  TemplateStart = "";
	Text  TemplateAfter = "";

	int   TemplateType = 0;
	int   MissionId = 0;
	int   TemplateTeam = 0;
	bool  TemplateDegrees = false;
	bool  TemplateOnce = false;

	EMISSIONTYPE LocalMissionType = EMISSIONTYPE::PATROL;

	FS_TemplateMission NewTemplateMission;

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def)
			continue;

		const Text& Key = def->name()->value();

		if (Key == "name")
		{
			GetDefText(MissionName, def, fn);
			NewTemplateMission.MissionName = FString(MissionName);
		}
		else if (Key == "type")
		{
			char typestr[64];
			GetDefText(typestr, def, fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(typestr));

			if (!FStringToEnum<EMISSIONTYPE>(NormalizedType, LocalMissionType, false))
			{
				LocalMissionType = EMISSIONTYPE::OTHER;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(typestr),
					*NormalizedType,
					UTF8_TO_TCHAR(fn));
			}

			NewTemplateMission.TemplateType = LocalMissionType;
		}
		else if (Key == "system")
		{
			GetDefText(MissionSystem, def, fn);
			NewTemplateMission.MissionSystem = FString(MissionSystem);
		}
		else if (Key == "degrees")
		{
			GetDefBool(TemplateDegrees, def, fn);
			NewTemplateMission.TemplateDegrees = TemplateDegrees;
		}
		else if (Key == "region")
		{
			GetDefText(MissionRegion, def, fn);
			NewTemplateMission.MissionRegion = FString(MissionRegion);
		}
		else if (Key == "objective")
		{
			GetDefText(TemplateObjective, def, fn);
			NewTemplateMission.TemplateObjective = FString(TemplateObjective);
		}
		else if (Key == "sitrep")
		{
			GetDefText(TemplateSitrep, def, fn);
			NewTemplateMission.TemplateSitrep = FString(TemplateSitrep);
		}
		else if (Key == "id")
		{
			GetDefNumber(MissionId, def, fn);
			NewTemplateMission.MissionId = MissionId;
		}
		else if (Key == "start")
		{
			GetDefText(TemplateStart, def, fn);
			NewTemplateMission.TemplateStart = FString(TemplateStart);
		}
		else if (Key == "team")
		{
			GetDefNumber(TemplateTeam, def, fn);
			NewTemplateMission.TemplateTeam = TemplateTeam;
		}
		else if (Key == "target")
		{
			GetDefText(TargetName, def, fn);
			NewTemplateMission.TargetName = FString(TargetName);
		}
		else if (Key == "ward")
		{
			GetDefText(WardName, def, fn);
			NewTemplateMission.WardName = FString(WardName);
		}
		else if (Key == "alias")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: alias struct missing in '%s'"), *ScriptPath);
			}
			else
			{
				ParseAlias(def->term()->isStruct(), fn);
				NewTemplateMission.Alias = MissionAliasArray;
			}
		}
		else if (Key == "callsign")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: callsign struct missing in '%s'"), *ScriptPath);
			}
			else
			{
				ParseCallsign(def->term()->isStruct(), fn);
				NewTemplateMission.Callsign = MissionCallsignArray;
			}
		}
		else if (Key == "optional")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: optional struct missing in '%s'"), *ScriptPath);
			}
			else
			{
				ParseOptional(def->term()->isStruct(), fn);
				NewTemplateMission.Optional = MissionOptionalArray;
			}
		}
		else if (Key == "element")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: element struct missing in '%s'"), *ScriptPath);
			}
			else
			{
				ParseElement(def->term()->isStruct(), fn);
				NewTemplateMission.Element = MissionElementArray;
			}
		}
		else if (Key == "event")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: event struct missing in '%s'"), *ScriptPath);
			}
			else
			{
				ParseEvent(def->term()->isStruct(), fn);
				NewTemplateMission.Event = MissionEventArray;
			}
		}

	} while (term);

	if (term)
	{
		delete term;
		term = nullptr;
	}

	ScriptedMissionArray.Add(NewTemplateMission);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseMissionTemplate(const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMissionTemplate()"));

	if (!fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseMissionTemplate called with null/empty filename"));
		return;
	}

	// Don't name this FilePath (it collides with your class member):
	const FString TemplatePath = ANSI_TO_TCHAR(fn);

	if (!FPaths::FileExists(TemplatePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mission template file not found: %s"), *TemplatePath);
		return;
	}

	// ---------------------------------------------------------
	// UE-native raw bytes load (preserves legacy parser behavior)
	// ---------------------------------------------------------
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *TemplatePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read mission template file: %s"), *TemplatePath);
		return;
	}

	// Null-terminate for BlockReader/legacy char* parser:
	Bytes.Add(0);

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	// Reset scratch arrays for this template parse:
	MissionElementArray.Empty();
	MissionEventArray.Empty();
	MissionCallsignArray.Empty();
	MissionOptionalArray.Empty();
	MissionAliasArray.Empty();

	if (!term)
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING: could not parse '%s'"), *TemplatePath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MISSIONTEMPLATE file '%s'"), *TemplatePath);

	Text TargetName = "";
	Text WardName = "";
	Text MissionName = "";
	Text MissionSystem = "";
	Text MissionRegion = "";
	Text TemplateObjective = "";
	Text TemplateSitrep = "";
	Text TemplateStart = "";
	Text TemplateAfter = "";
	Text LocalGroup = "";

	int  TemplateType = 0;
	int  TemplateTeam = 0;
	int  MissionId = 0;
	bool TemplateDegrees = false;
	bool TemplateOnce = false;

	ECOMBATGROUP_TYPE TemplateGroupType = ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
	EMISSIONTYPE LocalMissionType = EMISSIONTYPE::PATROL;

	FS_TemplateMission NewTemplateMission;

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def)
			continue;

		const Text& Key = def->name()->value();

		if (Key == "name")
		{
			GetDefText(MissionName, def, fn);
			NewTemplateMission.MissionName = FString(MissionName);
		}
		else if (Key == "type")
		{
			char typestr[64];
			GetDefText(typestr, def, fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(typestr));

			if (!FStringToEnum<EMISSIONTYPE>(NormalizedType, LocalMissionType, false))
			{
				LocalMissionType = EMISSIONTYPE::OTHER;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(typestr),
					*NormalizedType,
					UTF8_TO_TCHAR(fn));
			}

			NewTemplateMission.TemplateType = LocalMissionType;
		}
		else if (Key == "system")
		{
			GetDefText(MissionSystem, def, fn);
			NewTemplateMission.MissionSystem = FString(MissionSystem);
		}
		else if (Key == "degrees")
		{
			GetDefBool(TemplateDegrees, def, fn);
			NewTemplateMission.TemplateDegrees = TemplateDegrees;
		}
		else if (Key == "exec_once")
		{
			GetDefBool(TemplateOnce, def, fn);
			NewTemplateMission.TemplateOnce = TemplateOnce;
		}
		else if (Key == "region")
		{
			GetDefText(MissionRegion, def, fn);
			NewTemplateMission.MissionRegion = FString(MissionRegion);
		}
		else if (Key == "objective")
		{
			GetDefText(TemplateObjective, def, fn);
			NewTemplateMission.TemplateObjective = FString(TemplateObjective);
		}
		else if (Key == "sitrep")
		{
			GetDefText(TemplateSitrep, def, fn);
			NewTemplateMission.TemplateSitrep = FString(TemplateSitrep);
		}
		else if (Key == "id")
		{
			GetDefNumber(MissionId, def, fn);
			NewTemplateMission.MissionId = MissionId;
		}
		else if (Key == "start")
		{
			GetDefText(TemplateStart, def, fn);
			NewTemplateMission.TemplateStart = FString(TemplateStart);
		}
		else if (Key == "after")
		{
			GetDefText(TemplateAfter, def, fn);
			NewTemplateMission.TemplateAfter =  FString(TemplateAfter);
		}
		else if (Key == "team")
		{
			GetDefNumber(TemplateTeam, def, fn);
			NewTemplateMission.TemplateTeam = TemplateTeam;
		}
		else if (Key == "target")
		{
			GetDefText(TargetName, def, fn);
			NewTemplateMission.TargetName = FString(TargetName);
		}
		else if (Key == "ward")
		{
			GetDefText(WardName, def, fn);
			NewTemplateMission.WardName = FString(WardName);
		}
		else if (Key == "group")
		{
			GetDefText(LocalGroup, def, fn);

			const FString NormalizedType =
				NormalizeCombatGroupTypeToken(FString(LocalGroup));
			if (!FStringToEnum<ECOMBATGROUP_TYPE>(NormalizedType, TemplateGroupType, false))
			{
				TemplateGroupType = ECOMBATGROUP_TYPE::NONE;

				UE_LOG(LogTemp, Warning,
					TEXT("[ParseMission] unknown type '%s' normalized to '%s' in '%s'"),
					*FString(LocalGroup),
					*NormalizedType,
					UTF8_TO_TCHAR(fn));
			}

			NewTemplateMission.TemplateGroup = TemplateGroupType;
		}
		else if (Key == "alias")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: alias struct missing in '%s'"), *TemplatePath);
			}
			else
			{
				ParseAlias(def->term()->isStruct(), fn);
				NewTemplateMission.Alias = MissionAliasArray;
			}
		}
		else if (Key == "callsign")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: callsign struct missing in '%s'"), *TemplatePath);
			}
			else
			{
				ParseCallsign(def->term()->isStruct(), fn);
				NewTemplateMission.Callsign = MissionCallsignArray;
			}
		}
		else if (Key == "optional")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: optional struct missing in '%s'"), *TemplatePath);
			}
			else
			{
				ParseOptional(def->term()->isStruct(), fn);
				NewTemplateMission.Optional = MissionOptionalArray;
			}
		}
		else if (Key == "element")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: element struct missing in '%s'"), *TemplatePath);
			}
			else
			{
				ParseElement(def->term()->isStruct(), fn);
				NewTemplateMission.Element = MissionElementArray;
			}
		}
		else if (Key == "event")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: event struct missing in '%s'"), *TemplatePath);
			}
			else
			{
				ParseEvent(def->term()->isStruct(), fn);
				NewTemplateMission.Event = MissionEventArray;
			}
		}

	} while (term);

	if (term)
	{
		delete term;
		term = nullptr;
	}

	TemplateMissionArray.Add(NewTemplateMission);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseAlias(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseAlias()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseAlias called with null args"));
		return;
	}

	// ---- strings ----
	Text AliasName = "";
	Text Design = "";
	Text Code = "";
	Text ElementName = "";
	Text Mission = "";

	// ---- scalars ----
	int32 Iff = -1;
	int32 Player = 0;

	// ---- FVector only ----
	bool    bUseLocation = false;
	FVector Location = FVector::ZeroVector;

	// Scratch arrays (nested parsers append into these):
	MissionRLocArray.Empty();
	MissionNavpointArray.Empty();
	MissionObjectiveArray.Empty();

	FS_MissionAlias NewMissionAlias;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 i = 0; i < ElemCount; ++i)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(AliasName, PDef, Fn);
			NewMissionAlias.AliasName = FString(AliasName);
		}
		else if (Key == "elem")
		{
			GetDefText(ElementName, PDef, Fn);
			NewMissionAlias.ElementName = FString(ElementName);
		}
		else if (Key == "code")
		{
			GetDefText(Code, PDef, Fn);
			NewMissionAlias.Code = FString(Code);
		}
		else if (Key == "design")
		{
			GetDefText(Design, PDef, Fn);
			NewMissionAlias.Design = FString(Design);
		}
		else if (Key == "mission")
		{
			GetDefText(Mission, PDef, Fn);
			NewMissionAlias.Mission = FString(Mission);
		}
		else if (Key == "iff")
		{
			GetDefNumber(Iff, PDef, Fn);
			NewMissionAlias.Iff = Iff;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);

			Location = V;
			bUseLocation = true;
		}
		else if (Key == "rloc")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseRLoc(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "player")
		{
			GetDefNumber(Player, PDef, Fn);

			// Always record Player
			NewMissionAlias.Player = Player;

			// Legacy behavior: if player is set and code is missing, force "player"
			if (Player && !Code.length())
			{
				Code = "player";
				NewMissionAlias.Code = FString(Code);
			}
		}
		else if (Key == "navpt")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseNavpoint(PDef->term()->isStruct(), Fn);
			}
		}
		else if (Key == "objective")
		{
			if (PDef->term() && PDef->term()->isStruct())
			{
				ParseObjective(PDef->term()->isStruct(), Fn);
			}
		}
	}

	// Assign accumulated nested arrays once
	NewMissionAlias.Location = Location;
	NewMissionAlias.UseLocation = bUseLocation;
	NewMissionAlias.RLoc = MissionRLocArray;
	NewMissionAlias.Navpoint = MissionNavpointArray;
	NewMissionAlias.Objective = MissionObjectiveArray;

	MissionAliasArray.Add(NewMissionAlias);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseRLoc(TermStruct* RVal, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseRLoc()"));

	if (!RVal || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseRLoc called with null args"));
		return;
	}

	// ---- FVector only ----
	FVector BaseLocation = FVector::ZeroVector;

	// ---- strings ----
	Text Reference = "";

	// ---- doubles ----
	double Dex = 0.0;
	double DexVar = 5e3;
	double Az = 0.0;
	double AzVar = PI;
	double El = 0.0;
	double ElVar = 0.1;

	FS_RLoc NewRLocElement;

	const int32 ElemCount = (int32)RVal->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)   // <-- FIX
	{
		TermDef* RDef = RVal->elements()->at(ElemIdx)->isDef();
		if (!RDef)
			continue;

		const Text& Key = RDef->name()->value();

		if (Key == "dex")
		{
			GetDefNumber(Dex, RDef, Fn);
			NewRLocElement.Dex = Dex;
		}
		else if (Key == "dex_var")
		{
			GetDefNumber(DexVar, RDef, Fn);
			NewRLocElement.DexVar = DexVar;
		}
		else if (Key == "az")
		{
			GetDefNumber(Az, RDef, Fn);
			NewRLocElement.Azimuth = Az;
		}
		else if (Key == "az_var")
		{
			GetDefNumber(AzVar, RDef, Fn);
			NewRLocElement.AzimuthVar = AzVar;
		}
		else if (Key == "el")
		{
			GetDefNumber(El, RDef, Fn);
			NewRLocElement.Elevation = El;
		}
		else if (Key == "el_var")
		{
			GetDefNumber(ElVar, RDef, Fn);
			NewRLocElement.ElevationVar = ElVar;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, RDef, Fn);

			BaseLocation = V;
			NewRLocElement.BaseLocation = BaseLocation;
		}
		else if (Key == "ref")
		{
			GetDefText(Reference, RDef, Fn);
			NewRLocElement.Reference = FString(Reference);
		}
	}

	MissionRLocArray.Add(NewRLocElement);
}


// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseCallsign(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseCallsign()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseCallsign called with null args"));
		return;
	}

	// Legacy-safe initialization
	Text CallsignName = "";
	int  CallsignIff = -1;

	FS_MissionCallsign NewMissionCallsign;

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(CallsignName, pdef, fn);
			NewMissionCallsign.Callsign = FString(CallsignName);
		}
		else if (Key == "iff")
		{
			GetDefNumber(CallsignIff, pdef, fn);
			NewMissionCallsign.Iff = CallsignIff;
		}
	}

	MissionCallsignArray.Add(NewMissionCallsign);
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::ParseOptional(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseOptional()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseOptional called with invalid args"));
		return;
	}

	int Min = 0;
	int Max = 1000;

	// Reset scratch arrays used by nested parsing
	MissionElementArray.Empty();

	FS_MissionOptional NewOptional;
	NewOptional.Min = Min;
	NewOptional.Max = Max;

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "min")
		{
			GetDefNumber(Min, pdef, fn);
			NewOptional.Min = Min;
		}
		else if (Key == "max")
		{
			GetDefNumber(Max, pdef, fn);
			NewOptional.Max = Max;
		}
		else if (Key == "element")
		{
			if (pdef->term() && pdef->term()->isStruct())
			{
				// Each call appends to MissionElementArray
				ParseElement(pdef->term()->isStruct(), fn);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Optional element missing struct in '%s'"),
					*FString(fn));
			}
		}
	}

	// Finalize
	NewOptional.Element = MissionElementArray;
	NewOptional.Total = MissionElementArray.Num();

	MissionOptionalArray.Add(NewOptional);
}



// +--------------------------------------------------------------------+

// ------------------------------------------------------------
// FIXED: LoadGalaxyMap
// - UE-native file load (no DataLoader / no ReleaseBuffer)
// - Correct per-system reset (system scratch arrays cleared once per system)
// - Removes undefined fn/filename usage; uses a single Fn derived from FileName
// - Uses your ParseStarMap -> ParsePlanetMap -> ParseMoonMap chain
// ------------------------------------------------------------


// +-------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::InitializeCombatRoster()
{
	//CombatGroupDataTable->EmptyTable();
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::InitializeCombatRoster()"));

	// Content/GameData/Campaigns/
	ProjectPath = FPaths::ProjectContentDir();
	ProjectPath /= TEXT("GameData/Campaigns/");

	TArray<FString> Files;
	Files.Empty();

	// FindFiles wants the full wildcard path:
	const FString Wildcard = ProjectPath / TEXT("*.def");

	IFileManager::Get().FindFiles(Files, *Wildcard, /*Files=*/true, /*Directories=*/false);

	for (const FString& File : Files)
	{
		const FString FullPath = ProjectPath / File;

		// LoadOrderOfBattle expects const char* (legacy).
		// Convert for the call only (do NOT store the pointer).
		const FTCHARToUTF8 Utf8Path(*FullPath);
		LoadCombatRoster(Utf8Path.Get(), -1);
	}
}

void UStarshatterGameDataSubsystem::LoadCombatRoster(const char* InFilename, int32 Team)
{
	if (!CombatGroupDataTable)
	{
		return;
	}

	CombatGroupDataTable->EmptyTable();

	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadCombatRoster"));

	if (!InFilename || !*InFilename)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] null/empty filename"));
		return;
	}

	const FString OobFilePath = ANSI_TO_TCHAR(InFilename);

	UE_LOG(LogTemp, Log, TEXT("[LoadCombatRoster] Loading Data: %s"), *OobFilePath);

	if (!FPaths::FileExists(OobFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] file not found: %s"), *OobFilePath);
		return;
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *OobFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LoadCombatRoster] failed to read: %s"), *OobFilePath);
		return;
	}

	Bytes.Add(0);

	const FTCHARToUTF8 Utf8Path(*OobFilePath);
	const char* fn = Utf8Path.Get();

	Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* TermPtr = ParserObj.ParseTerm();

	if (!TermPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] could not parse: %s"), *OobFilePath);
		return; 
	}

	{
		TermText* FileType = TermPtr->isText();
		if (!FileType || FileType->value() != "ORDER_OF_BATTLE")
		{
			UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] Invalid Order of Battle File: %s"), *OobFilePath);
			delete TermPtr;
			return;
		}
	}

	delete TermPtr;
	TermPtr = nullptr;

	while ((TermPtr = ParserObj.ParseTerm()) != nullptr)
	{
		TermDef* Def = TermPtr->isDef();
		if (!Def || Def->name()->value() != "group")
		{
			delete TermPtr;
			TermPtr = nullptr;
			continue;
		}

		if (!Def->term() || !Def->term()->isStruct())
		{
			UE_LOG(LogTemp, Warning, TEXT("WARNING: group struct missing in '%s'"), *OobFilePath);
			delete TermPtr;
			TermPtr = nullptr;
			continue;
		}

		TermStruct* GroupStruct = Def->term()->isStruct();

		FS_CombatGroup NewCombatGroup;
		NewCombatUnitArray.Empty();

		Text LocalName = "";
		Text LocalType = "";
		Text LocalRegion = "";
		Text LocalSystem = "";
		Text LocalParentType = "";

		EINTEL_TYPE LocalIntelType = EINTEL_TYPE::KNOWN;
		ECOMBATGROUP_TYPE LocalGroupType = ECOMBATGROUP_TYPE::NONE;
		ECOMBATGROUP_TYPE LocalParentGroupType = ECOMBATGROUP_TYPE::NONE;

		int32 LocalParentId = 0;
		int32 LocalEmpireId = 0;
		int32 LocalIff = -1;
		int32 LocalUnitIndex = 0;

		Vec3 LocalLoc(1.0e9f, 0.0f, 0.0f);

		const int32 GroupElemCount = (int32)GroupStruct->elements()->size();
		for (int32 FieldIdx = 0; FieldIdx < GroupElemCount; ++FieldIdx)
		{
			TermDef* PDef = GroupStruct->elements()->at(FieldIdx)->isDef();
			if (!PDef)
				continue;

			const Text& Key = PDef->name()->value();

			if (Key == "name")
			{
				GetDefText(LocalName, PDef, fn);
				NewCombatGroup.Name = FString(LocalName);
				UE_LOG(LogTemp, Warning,
					TEXT("[LoadCombatRoster] name parsed: '%s'"),
					*FString(LocalName));
			}
			else if (Key == "intel")
			{
				Text Intel = "";
				GetDefText(Intel, PDef, fn);

				if (!FStringToEnum<EINTEL_TYPE>(FString(Intel).ToUpper(), LocalIntelType, false))
					LocalIntelType = EINTEL_TYPE::KNOWN;

				NewCombatGroup.Intel = LocalIntelType;
			}
			else if (Key == "region")
			{
				GetDefText(LocalRegion, PDef, fn);
				NewCombatGroup.Region = FString(LocalRegion);
			}
			else if (Key == "system")
			{
				GetDefText(LocalSystem, PDef, fn);
				NewCombatGroup.System = FString(LocalSystem);
			}
			else if (Key == "loc")
			{
				GetDefVec(LocalLoc, PDef, fn);
				NewCombatGroup.Location = FVector(LocalLoc.X, LocalLoc.Y, LocalLoc.Z);
			}
			else if (Key == "parent_type")
			{
				GetDefText(LocalParentType, PDef, fn);

				const FString NormalizedParentType =
					NormalizeCombatGroupTypeToken(FString(LocalParentType));

				if (!FStringToEnum<ECOMBATGROUP_TYPE>(NormalizedParentType, LocalParentGroupType, false))
				{
					LocalParentGroupType = ECOMBATGROUP_TYPE::NONE;

					UE_LOG(LogTemp, Warning,
						TEXT("[LoadCombatRoster] unknown parent_type '%s' normalized to '%s' in '%s'"),
						*FString(LocalParentType),
						*NormalizedParentType,
						*OobFilePath);
				}

				NewCombatGroup.ParentType = LocalParentGroupType;
			}
			else if (Key == "parent_id")
			{
				GetDefNumber(LocalParentId, PDef, fn);
				NewCombatGroup.ParentId = LocalParentId;
			}
			else if (Key == "empire_id")
			{
				GetDefNumber(LocalEmpireId, PDef, fn);
				NewCombatGroup.EmpireId = UFormattingUtils::GetEmpireTypeFromIndex(LocalEmpireId);
			}
			else if (Key == "iff")
			{
				GetDefNumber(LocalIff, PDef, fn);
				NewCombatGroup.Iff = LocalIff;
			}
			else if (Key == "id")
			{
				int32 LocalId = 0;
				GetDefNumber(LocalId, PDef, fn);
				NewCombatGroup.Id = LocalId;
			}
			else if (Key == "unit_index")
			{
				GetDefNumber(LocalUnitIndex, PDef, fn);
				NewCombatGroup.UnitIndex = LocalUnitIndex;
			}
			else if (Key == "type")
			{
				GetDefText(LocalType, PDef, fn);

				const FString NormalizedType =
					NormalizeCombatGroupTypeToken(FString(LocalType));

				if (!FStringToEnum<ECOMBATGROUP_TYPE>(NormalizedType, LocalGroupType, false))
				{
					LocalGroupType = ECOMBATGROUP_TYPE::NONE;

					UE_LOG(LogTemp, Warning,
						TEXT("[LoadCombatRoster] unknown type '%s' normalized to '%s' in '%s'"),
						*FString(LocalType),
						*NormalizedType,
						*OobFilePath);
				}

				NewCombatGroup.Type = LocalGroupType;
			}
			else if (Key == "unit")
			{
				TermStruct* UnitStruct = (PDef->term() ? PDef->term()->isStruct() : nullptr);
				if (!UnitStruct)
					continue;

				FS_CombatGroupUnit NewUnit;

				Text LocalUnitName = "";
				Text LocalUnitRegnum = "";
				Text LocalUnitRegion = "";
				Text LocalUnitClass = "";
				Text LocalUnitDesign = "";
				Text LocalUnitSkin = "";

				int32 LocalUnitCount = 1;
				int32 LocalUnitDamage = 0;
				int32 LocalUnitDead = 0;
				int32 LocalUnitHeading = 0;
				Vec3  LocalUnitLoc(1.0e9f, 0.0f, 0.0f);

				const int32 UnitElemCount = (int32)UnitStruct->elements()->size();
				for (int32 UnitFieldIdx = 0; UnitFieldIdx < UnitElemCount; ++UnitFieldIdx)
				{
					TermDef* UDef = UnitStruct->elements()->at(UnitFieldIdx)->isDef();
					if (!UDef)
						continue;

					const Text& UKey = UDef->name()->value();

					if (UKey == "name")
					{
						GetDefText(LocalUnitName, UDef, fn);
						NewUnit.UnitName = FString(LocalUnitName);
					}
					else if (UKey == "regnum")
					{
						GetDefText(LocalUnitRegnum, UDef, fn);
					}
					else if (UKey == "region")
					{
						GetDefText(LocalUnitRegion, UDef, fn);
					}
					else if (UKey == "loc")
					{
						GetDefVec(LocalUnitLoc, UDef, fn);
					}
					else if (UKey == "type")
					{
						GetDefText(LocalUnitClass, UDef, fn);
					}
					else if (UKey == "design")
					{
						GetDefText(LocalUnitDesign, UDef, fn);
					}
					else if (UKey == "skin")
					{
						GetDefText(LocalUnitSkin, UDef, fn);
					}
					else if (UKey == "count")
					{
						GetDefNumber(LocalUnitCount, UDef, fn);
					}
					else if (UKey == "dead_count")
					{
						GetDefNumber(LocalUnitDead, UDef, fn);
					}
					else if (UKey == "damage")
					{
						GetDefNumber(LocalUnitDamage, UDef, fn);
					}
					else if (UKey == "heading")
					{
						GetDefNumber(LocalUnitHeading, UDef, fn);
					}
				}

				NewUnit.UnitRegnum = FString(LocalUnitRegnum);
				NewUnit.UnitRegion = FString(LocalUnitRegion);
				NewUnit.UnitLoc = FVector(LocalUnitLoc.X, LocalUnitLoc.Y, LocalUnitLoc.Z);
				NewUnit.UnitClass = FString(LocalUnitClass);
				NewUnit.UnitDesign = FString(LocalUnitDesign);
				NewUnit.UnitSkin = FString(LocalUnitSkin);
				NewUnit.UnitCount = LocalUnitCount;
				NewUnit.UnitDead = LocalUnitDead;
				NewUnit.UnitDamage = LocalUnitDamage;
				NewUnit.UnitHeading = LocalUnitHeading;

				NewCombatUnitArray.Add(NewUnit);
				NewCombatGroup.Unit = NewCombatUnitArray;
			}
		}

		const bool bPassTeam = (Team < 0) || (NewCombatGroup.Iff == Team);

		if (NewCombatGroup.Iff > -1 && bPassTeam)
		{
			const FName RowName(*(
				UFormattingUtils::GetOrdinal(NewCombatGroup.Id) + TEXT(" ") +
				FString(UFormattingUtils::GetGroupTypeDisplayName(NewCombatGroup.Type)) +
				TEXT(" [") + NewCombatGroup.Name + TEXT("]")
				)); 

			NewCombatGroup.DisplayName = RowName.ToString();
			CombatGroupDataTable->AddRow(RowName, NewCombatGroup);
		}

		CombatGroupData = NewCombatGroup;

		delete TermPtr;
		TermPtr = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[LoadCombatRoster] complete: %s"), *OobFilePath);
}

// +--------------------------------------------------------------------+

CombatGroup*
UStarshatterGameDataSubsystem::CloneOver(CombatGroup* force, CombatGroup* clone, CombatGroup* group)
{
	CombatGroup* orig_parent = group->GetParent();

	if (orig_parent) {
		CombatGroup* clone_parent = clone->FindGroup(orig_parent->GetType(), orig_parent->GetID());

		if (!clone_parent)
			clone_parent = CloneOver(force, clone, orig_parent);

		CombatGroup* new_clone = clone->FindGroup(group->GetType(), group->GetID());

		if (!new_clone) {
			new_clone = group->Clone(false);
			clone_parent->AddComponent(new_clone);
		}

		return new_clone;
	}
	else {
		return clone;
	}
}

// +--------------------------------------------------------------------+

void
UStarshatterGameDataSubsystem::SetCampaignStatus(ECampaignStatus s)
{
	CampaignStatus = s;

	// record the win in player profile:
	if (CampaignStatus == ECampaignStatus::SUCCESS) {
		PlayerData* player = PlayerData::GetCurrentPlayer();

		if (player)
			player->SetCampaignComplete(campaign_id);
	}

	if (CampaignStatus > ECampaignStatus::ACTIVE) {
		UE_LOG(LogTemp, Log, TEXT("Campaign::SetStatus() destroying mission list at campaign end"));
		missions.destroy();
	}
}

double
UStarshatterGameDataSubsystem::Stardate()
{
	return StarSystem::GetStardate();
}

Combatant*
UStarshatterGameDataSubsystem::GetCombatant(const char* cname)
{
	ListIter<Combatant> iter = combatants;
	while (++iter) {
		Combatant* c = iter.value();
		if (!strcmp(c->GetName(), cname))
			return c;
	}
	return nullptr;
}

// +--------------------------------------------------------------------+

Text
UStarshatterGameDataSubsystem::GetContentBundleText(const char* key) const
{
	return ContentValues.Find(key, Text(key));
}

// +--------------------------------------------------------------------+

void UStarshatterGameDataSubsystem::LoadContentBundle()
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadContentBundle()"));

	const FString ContentFilePath =
		FPaths::ProjectContentDir() / TEXT("GameData/Content/content.txt");

	UE_LOG(LogTemp, Log, TEXT("Loading Content Bundle: %s"), *ContentFilePath);

	if (!FPaths::FileExists(ContentFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Content bundle not found: %s"), *ContentFilePath);
		return;
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *ContentFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read content bundle: %s"), *ContentFilePath);
		return;
	}

	Bytes.Add(0); // null terminate

	const char* buffer = reinterpret_cast<const char*>(Bytes.GetData());
	if (!buffer || !*buffer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Content bundle empty: %s"), *ContentFilePath);
		return;
	}

	// Clear previous content (Dictionary<Text> does NOT have Destroy() in your build)
	// Use the legacy Dictionary API:
	ContentValues.Clear();      // <-- if your Dictionary uses destroy(), change to: ContentValues.destroy();

	Text key;
	Text val;

	const char* p = buffer;
	int s = 0; // 0=key, 1=after '=', waiting for first value char, 2=value, -1=comment
	key = "";
	val = "";

	int32 InsertedCount = 0;

	auto FlushPair = [&]()
		{
			if (key != "" && val != "")
			{
				Text k = Text(key).trim();
				Text v = Text(val).trim();

				ContentValues.Insert(k, v);
				++InsertedCount;

				const FString KStr(ANSI_TO_TCHAR(k.data()));
				const FString VStr(ANSI_TO_TCHAR(v.data()));
				UE_LOG(LogTemp, Log, TEXT("Inserted- %s: %s"), *KStr, *VStr);
			}

			key = "";
			val = "";
			s = 0;
		};

	while (*p)
	{
		if (*p == '\n' || *p == '\r')
		{
			// newline ends a line (and ends comment mode)
			if (s != -1)
			{
				FlushPair();
			}
			else
			{
				// comment line ended
				s = 0;
				key = "";
				val = "";
			}
		}
		else if (s == -1)
		{
			// comment: ignore until newline
		}
		else if (*p == '=')
		{
			s = 1;
		}
		else if (s == 0)
		{
			// key parse (legacy behavior)
			if (!key[0])
			{
				if (*p == '#')
				{
					s = -1; // comment
				}
				else if (!isspace((unsigned char)*p))
				{
					key.append(*p);
				}
			}
			else
			{
				key.append(*p);
			}
		}
		else if (s == 1)
		{
			// first non-space begins value
			if (!isspace((unsigned char)*p))
			{
				s = 2;
				val.append(*p);
			}
		}
		else if (s == 2)
		{
			val.append(*p);
		}

		++p;
	}

	// Handle EOF without trailing newline
	if (s != -1)
	{
		FlushPair();
	}

	UE_LOG(LogTemp, Log, TEXT("Content bundle loaded. Entries=%d"), InsertedCount);
}

// +--------------------------------------------------------------------+


void UStarshatterGameDataSubsystem::LoadAwardTables()
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadAwardTables()"));

	// ------------------------------------------------------------
	// Load-once guard (prevents second-run DT mutation crashes)
	// ------------------------------------------------------------
	if (bAwardTablesLoaded)
	{
		UE_LOG(LogTemp, Log, TEXT("LoadAwardTables: already loaded; skipping."));
		return;
	}
	bAwardTablesLoaded = true;

	// ------------------------------------------------------------
	// Resolve file path
	// ------------------------------------------------------------
	FString AwardsPath = FPaths::ProjectContentDir();
	AwardsPath /= TEXT("GameData/Awards/awards.def");

	UE_LOG(LogTemp, Log, TEXT("Loading Awards file: %s"), *AwardsPath);

	if (!FPaths::FileExists(AwardsPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadAwardTables: file not found: %s"), *AwardsPath);
		return;
	}

	if (!RanksDataTable || !MedalsDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadAwardTables: RanksDataTable or MedalsDataTable is null."));
		return;
	}

	// Optional safety: verify row structs are correct
	if (RanksDataTable->GetRowStruct() != FRankInfo::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("LoadAwardTables: RanksDataTable RowStruct mismatch (expected FRankInfo)."));
		return;
	}

	if (MedalsDataTable->GetRowStruct() != FMedalInfo::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("LoadAwardTables: MedalsDataTable RowStruct mismatch (expected FMedalInfo)."));
		return;
	}

#if !WITH_EDITOR
	// Strong recommendation: do not mutate asset DataTables at runtime in packaged builds.
	UE_LOG(LogTemp, Warning, TEXT("LoadAwardTables: DataTable mutation disabled outside editor builds."));
	return;
#endif

	// ------------------------------------------------------------
	// Optional: full rebuild (clear DTs once)
	// ------------------------------------------------------------
	auto ClearDataTable = [](UDataTable* DT)
		{
			if (!DT) return;
			const TArray<FName> Names = DT->GetRowNames();
			for (const FName& RN : Names)
				DT->RemoveRow(RN);
		};

	// If you want a clean rebuild each editor run, uncomment:
	// ClearDataTable(RanksDataTable);
	// ClearDataTable(MedalsDataTable);

	// ------------------------------------------------------------
	// Load file bytes (UE-native)
	// ------------------------------------------------------------
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *AwardsPath))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadAwardTables: failed to read: %s"), *AwardsPath);
		return;
	}
	Bytes.Add(0); // null terminate

	const FTCHARToUTF8 Utf8Path(*AwardsPath);
	const char* FileNameAnsi = Utf8Path.Get();

	Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* TermPtr = ParserObj.ParseTerm();
	if (!TermPtr)
		return;

	// ------------------------------------------------------------
	// Header validation
	// ------------------------------------------------------------
	TermText* FileType = TermPtr->isText();
	if (!FileType || FileType->value() != "AWARDS")
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Awards file header: %s"), *AwardsPath);
		delete TermPtr;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Parsing awards.def into Rank + Medal tables..."));

	int32 RankCount = 0;
	int32 MedalCount = 0;

	// ------------------------------------------------------------
	// Parse entries
	// ------------------------------------------------------------
	while (true)
	{
		delete TermPtr;
		TermPtr = ParserObj.ParseTerm();
		if (!TermPtr)
			break;

		TermDef* Def = TermPtr->isDef();
		if (!Def || Def->name()->value() != "award")
			continue;

		TermStruct* Val = (Def->term() ? Def->term()->isStruct() : nullptr);
		if (!Val)
		{
			UE_LOG(LogTemp, Warning, TEXT("WARNING: award structure missing in '%s'"), *AwardsPath);
			continue;
		}

		// Scratch
		Text   LocalType = "";
		Text   LocalName = "";
		Text   LocalAbrv = "";
		Text   LocalDesc = "";
		Text   LocalAwardText = "";

		Text   LocalGrantSound = "";
		Text   LocalSmall = "";
		Text   LocalLarge = "";

		int32  LocalId = 0;
		int32  LocalGrant = 0;
		int32  LocalTotalPoints = 0;

		int32  LocalCampaignId = 0;
		bool   bLocalCampaignComplete = false;
		bool   bLocalDynamicCampaign = false;
		bool   bLocalCeremony = true;

		int32  LocalMinShipClass = 0;
		int32  LocalMaxShipClass = 0;

		for (int32 i = 0; i < (int32)Val->elements()->size(); ++i)
		{
			TermDef* PDef = Val->elements()->at(i)->isDef();
			if (!PDef)
				continue;

			const Text& Key = PDef->name()->value();

			if (Key == "type") { GetDefText(LocalType, PDef, FileNameAnsi); }
			else if (Key == "id") { GetDefNumber(LocalId, PDef, FileNameAnsi); }
			else if (Key == "name") { GetDefText(LocalName, PDef, FileNameAnsi); }
			else if (Key == "abrv") { GetDefText(LocalAbrv, PDef, FileNameAnsi); }
			else if (Key == "desc") { GetDefText(LocalDesc, PDef, FileNameAnsi); }
			else if (Key == "award") { GetDefText(LocalAwardText, PDef, FileNameAnsi); }

			else if (Key == "award_sound") { GetDefText(LocalGrantSound, PDef, FileNameAnsi); }

			else if (Key == "small") { GetDefText(LocalSmall, PDef, FileNameAnsi); }
			else if (Key == "large") { GetDefText(LocalLarge, PDef, FileNameAnsi); }

			else if (Key == "grant") { GetDefNumber(LocalGrant, PDef, FileNameAnsi); }
			else if (Key == "total_points") { GetDefNumber(LocalTotalPoints, PDef, FileNameAnsi); }

			else if (Key == "campaign_id") { GetDefNumber(LocalCampaignId, PDef, FileNameAnsi); }
			else if (Key == "campaign_complete") { GetDefBool(bLocalCampaignComplete, PDef, FileNameAnsi); }
			else if (Key == "dynamic_campaign") { GetDefBool(bLocalDynamicCampaign, PDef, FileNameAnsi); }
			else if (Key == "ceremony") { GetDefBool(bLocalCeremony, PDef, FileNameAnsi); }

			else if (Key == "min_ship_class")
			{
				Text ClassName;
				GetDefText(ClassName, PDef, FileNameAnsi);
				LocalMinShipClass = Ship::ClassForName(ClassName);
			}
			else if (Key == "max_ship_class")
			{
				Text ClassName;
				GetDefText(ClassName, PDef, FileNameAnsi);
				LocalMaxShipClass = Ship::ClassForName(ClassName);
			}
		}

		LocalType.setSensitive(false);
		LocalName.setSensitive(false);

		FString TypeStr = FString(LocalType).ToLower();

		FString NameStr = FString(LocalName);
		NameStr.ReplaceInline(TEXT("\r"), TEXT(""));
		NameStr.ReplaceInline(TEXT("\n"), TEXT(""));
		NameStr = NameStr.TrimStartAndEnd();

		if (NameStr.IsEmpty())
			continue;

		const FName RowName(*NameStr);

		if (TypeStr == TEXT("rank"))
		{
			FRankInfo RankRow;
			RankRow.RankId = LocalId;
			RankRow.Name = NameStr;
			RankRow.Abrv = FString(LocalAbrv);
			RankRow.Desc = FString(LocalDesc);
			RankRow.AwardText = FString(LocalAwardText);

			RankRow.SmallImageId = FString(LocalSmall);
			RankRow.LargeImageId = FString(LocalLarge);
			RankRow.GrantSoundId = FString(LocalGrantSound);

			RankRow.GrantedShipClasses = LocalGrant;
			RankRow.TotalPoints = LocalTotalPoints;

			if (FRankInfo* Existing = RanksDataTable->FindRow<FRankInfo>(RowName, TEXT("LoadAwardTables"), false))
			{
				*Existing = RankRow;
			}
			else
			{
				RanksDataTable->AddRow(RowName, RankRow);
			}
			++RankCount;
			continue;
		}

		if (TypeStr == TEXT("medal"))
		{
			FMedalInfo MedalRow;
			MedalRow.MedalId = LocalId;
			MedalRow.Name = NameStr;
			MedalRow.Desc = FString(LocalDesc);
			MedalRow.AwardText = FString(LocalAwardText);
			MedalRow.SmallImageId = FString(LocalSmall);
			MedalRow.LargeImageId = FString(LocalLarge);
			MedalRow.GrantSoundId = FString(LocalGrantSound);

			MedalRow.CampaignId = LocalCampaignId;
			MedalRow.bCampaignCompleteRequired = bLocalCampaignComplete;
			MedalRow.bDynamicCampaignOnly = bLocalDynamicCampaign;
			MedalRow.bCeremony = bLocalCeremony;

			// If your struct has these fields, set them; otherwise remove:
			// MedalRow.MinShipClass = LocalMinShipClass;
			// MedalRow.MaxShipClass = LocalMaxShipClass;

			if (FMedalInfo* Existing = MedalsDataTable->FindRow<FMedalInfo>(RowName, TEXT("LoadAwardTables"), false))
			{
				*Existing = MedalRow;   // overwrite row data in place
			}
			else
			{
				MedalsDataTable->AddRow(RowName, MedalRow);
			}
			++MedalCount;
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("LoadAwardTables: unknown award type '%s' for '%s'"),
			*TypeStr, *NameStr);
	}

	if (TermPtr)
	{
		delete TermPtr;
		TermPtr = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("LoadAwardTables: complete. Ranks=%d Medals=%d"),
		RankCount, MedalCount);
}

bool UStarshatterGameDataSubsystem::FillRankInfoFromTable(const UDataTable* RanksTable, int32 RankId, FRankInfo& OutRank)
{
	if (!RanksTable || RankId <= 0)
		return false;

	static const FString Ctx(TEXT("FillRankInfoFromTable"));

	TArray<FRankInfo*> Rows;
	RanksTable->GetAllRows<FRankInfo>(Ctx, Rows);

	for (const FRankInfo* Row : Rows)
	{
		if (!Row)
			continue;

		if (Row->RankId == RankId)
		{
			OutRank = *Row;   // COPY the whole struct (including your soft refs)
			return true;
		}
	}

	return false;
}

bool UStarshatterGameDataSubsystem::FillMedalInfoFromTable(const UDataTable* MedalsTable, int32 MedalId, FMedalInfo& OutMedal)
{
	if (!MedalsTable || MedalId <= 0)
		return false;

	static const FString Ctx(TEXT("FillMedalInfoFromTable"));

	TArray<FMedalInfo*> Rows;
	MedalsTable->GetAllRows<FMedalInfo>(Ctx, Rows);

	for (const FMedalInfo* Row : Rows)
	{
		if (!Row)
			continue;

		if (Row->MedalId == MedalId) // <- change to AwardId if that’s what your struct uses
		{
			OutMedal = *Row;
			return true;
		}
	}

	return false;
}

bool UStarshatterGameDataSubsystem::GetRankInfo(int32 RankId, FRankInfo& Out) const
{
	if (const FRankInfo* Found = RankById.Find(RankId))
	{
		Out = *Found;
		return true;
	}
	return false;
}

bool UStarshatterGameDataSubsystem::GetMedalInfo(int32 MedalId, FMedalInfo& Out) const
{
	if (const FMedalInfo* Found = MedalById.Find(MedalId))
	{
		Out = *Found;
		return true;
	}
	return false;
}


bool UStarshatterGameDataSubsystem::GetMedalInfoByFlag(uint32 MedalFlag, FMedalInfo& OutRow) const
{
	if (const FMedalInfo* Found = MedalByFlag.Find(MedalFlag))
	{
		OutRow = *Found;
		return true;
	}
	return false;
}

// Key is MedalFlag (1,2,4,8...).
void UStarshatterGameDataSubsystem::BuildMedalCache_ByFlag(const UDataTable* MedalsTable)
{
	MedalByFlag.Reset();

	if (!MedalsTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameData] BuildMedalCache_ByFlag: MedalsTable is NULL"));
		return;
	}

	static const FString Ctx(TEXT("BuildMedalCache_ByFlag"));
	TArray<FMedalInfo*> Rows;
	MedalsTable->GetAllRows<FMedalInfo>(Ctx, Rows);

	UE_LOG(LogTemp, Warning, TEXT("[GameData] Medal rows found: %d"), Rows.Num());

	for (const FMedalInfo* Row : Rows)
	{
		if (!Row)
			continue;

		const uint32 Flag = (uint32)Row->MedalId;

		if (Flag == 0)
			continue;

		// Must be power-of-two (bit flag)
		if ((Flag & (Flag - 1)) != 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] MedalId %u is not power-of-two; skipping."),
				Flag);
			continue;
		}

		MedalByFlag.Add(Flag, *Row);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] MedalByFlag cache built: %d entries"),
		MedalByFlag.Num());
}

FString UStarshatterGameDataSubsystem::GetEmpireRosterName(EEMPIRE_NAME Empire) const
{
	const UEnum* EnumObj = StaticEnum<EEMPIRE_NAME>();
	if (!EnumObj)
	{
		return TEXT("");
	}

	return EnumObj->GetDisplayNameTextByValue((int64)Empire).ToString();
}

void UStarshatterGameDataSubsystem::BuildCombatRosterFromOrderOfBattle()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameData] BuildCombatRosterFromOrderOfBattle: BEGIN"));

	if (AllForces.Num() == 0)
	{
		ReadOrderOfBattleData();
	}

	CombatRoster::Initialize();

	CombatRoster* Roster = CombatRoster::GetInstance();
	if (!Roster)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameData] BuildCombatRosterFromOrderOfBattle: CombatRoster init failed"));
		return;
	}

	Roster->Clear();

	for (const FS_OOBForce& ForceRow : AllForces)
	{
		CombatGroup* ForceTree = BuildCombatForceTree(ForceRow);
		if (!ForceTree)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] Failed to build force tree for '%s'"),
				*ForceRow.Name);
			continue;
		}

		const FString RosterName = GetEmpireRosterName(ForceRow.Empire);
		if (RosterName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameData] Empty roster name for force '%s'"),
				*ForceRow.Name);
			delete ForceTree;
			continue;
		}

		Roster->RegisterForce(RosterName, ForceTree);

		UE_LOG(LogTemp, Warning,
			TEXT("[GameData] Registered CombatRoster force '%s' from OOB force '%s'"),
			*RosterName,
			*ForceRow.Name);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] BuildCombatRosterFromOrderOfBattle: COMPLETE (%d registered forces)"),
		Roster->GetForces().Num());
}

CombatGroup* UStarshatterGameDataSubsystem::BuildCombatForceTree(const FS_OOBForce& ForceRow)
{
	CombatGroup* ForceGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::FORCE,
		ForceRow.Id,
		TCHAR_TO_ANSI(*ForceRow.Name),
		ForceRow.Iff,
		Intel::KNOWN,
		nullptr);

	if (!ForceGroup)
	{
		return nullptr;
	}

	ForceGroup->SetRegion(TCHAR_TO_ANSI(*ForceRow.Location));

	for (const FS_OOBFleet& FleetRow : ForceRow.Fleet)
	{
		AddFleetToForce(ForceGroup, FleetRow);
	}

	for (const FS_OOBBattalion& BattalionRow : ForceRow.Battalion)
	{
		AddBattalionToForce(ForceGroup, BattalionRow);
	}

	for (const FS_OOBCivilian& CivilianRow : ForceRow.Civilian)
	{
		AddCivilianToForce(ForceGroup, CivilianRow);
	}

	return ForceGroup;
}

void UStarshatterGameDataSubsystem::AddFleetToForce(CombatGroup* ForceGroup, const FS_OOBFleet& FleetRow)
{
	if (!ForceGroup)
	{
		return;
	}

	CombatGroup* FleetGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::FLEET,
		FleetRow.Id,
		TCHAR_TO_ANSI(*FleetRow.Name),
		FleetRow.Iff,
		Intel::KNOWN,
		ForceGroup);

	if (!FleetGroup)
	{
		return;
	}

	FleetGroup->SetRegion(TCHAR_TO_ANSI(*FleetRow.Location));

	for (const FS_OOBCarrier& CarrierRow : FleetRow.Carrier)
	{
		AddCarrierGroupToFleet(FleetGroup, CarrierRow);
	}

	for (const FS_OOBDestroyer& DestroyerRow : FleetRow.Destroyer)
	{
		AddDestroyerSquadronToFleet(FleetGroup, DestroyerRow);
	}

	for (const FS_OOBBattle& BattleRow : FleetRow.Battle)
	{
		AddBattleGroupToFleet(FleetGroup, BattleRow);
	}

	for (const FS_OOBMinefield& MinefieldRow : FleetRow.Minefield)
	{
		AddMinefieldToFleet(FleetGroup, MinefieldRow);
	}
}

void UStarshatterGameDataSubsystem::AddCarrierGroupToFleet(CombatGroup* FleetGroup, const FS_OOBCarrier& CarrierRow)
{
	if (!FleetGroup)
	{
		return;
	}

	CombatGroup* CarrierGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::CARRIER_GROUP,
		CarrierRow.Id,
		TCHAR_TO_ANSI(*CarrierRow.Name),
		CarrierRow.Iff,
		Intel::KNOWN,
		FleetGroup);

	if (!CarrierGroup)
	{
		return;
	}

	CarrierGroup->SetRegion(TCHAR_TO_ANSI(*CarrierRow.Location));

	AddUnitsToCombatGroup(CarrierGroup, CarrierRow.Unit);

	for (const FS_OOBWing& WingRow : CarrierRow.Wing)
	{
		AddWingToCarrier(CarrierGroup, WingRow);
	}

	for (const FS_OOBFighter& Row : CarrierRow.Fighter)
	{
		AddFighterSquadronToWing(CarrierGroup, Row);
	}

	for (const FS_OOBIntercept& Row : CarrierRow.Intercept)
	{
		AddInterceptSquadronToWing(CarrierGroup, Row);
	}

	for (const FS_OOBAttack& Row : CarrierRow.Attack)
	{
		AddAttackSquadronToWing(CarrierGroup, Row);
	}

	for (const FS_OOBLanding& Row : CarrierRow.Landing)
	{
		AddLandingSquadronToWing(CarrierGroup, Row);
	}
}

void UStarshatterGameDataSubsystem::AddDestroyerSquadronToFleet(
	CombatGroup* FleetGroup,
	const FS_OOBDestroyer& DestroyerRow)
{
	if (!FleetGroup)
	{
		return;
	}

	CombatGroup* DestroyerGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::DESTROYER_SQUADRON,
		DestroyerRow.Id,
		TCHAR_TO_ANSI(*DestroyerRow.Name),
		DestroyerRow.Iff,
		Intel::KNOWN,
		FleetGroup);

	if (!DestroyerGroup)
	{
		return;
	}

	DestroyerGroup->SetRegion(TCHAR_TO_ANSI(*DestroyerRow.Location));
	AddUnitsToCombatGroup(DestroyerGroup, DestroyerRow.Unit);
}

void UStarshatterGameDataSubsystem::AddBattleGroupToFleet(
	CombatGroup* FleetGroup,
	const FS_OOBBattle& BattleRow)
{
	if (!FleetGroup)
	{
		return;
	}

	CombatGroup* BattleGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::BATTLE_GROUP,
		BattleRow.Id,
		TCHAR_TO_ANSI(*BattleRow.Name),
		BattleRow.Iff,
		Intel::KNOWN,
		FleetGroup);

	if (!BattleGroup)
	{
		return;
	}

	BattleGroup->SetRegion(TCHAR_TO_ANSI(*BattleRow.Location));
	AddUnitsToCombatGroup(BattleGroup, BattleRow.Unit);
}

void UStarshatterGameDataSubsystem::AddBattalionToForce(
	CombatGroup* ForceGroup,
	const FS_OOBBattalion& BattalionRow)
{
	if (!ForceGroup)
	{
		return;
	}

	CombatGroup* BattalionGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::BATTALION,
		BattalionRow.Id,
		TCHAR_TO_ANSI(*BattalionRow.Name),
		BattalionRow.Iff,
		Intel::KNOWN,
		ForceGroup);

	if (!BattalionGroup)
	{
		return;
	}

	BattalionGroup->SetRegion(TCHAR_TO_ANSI(*BattalionRow.Location));

	for (const FS_OOBBattery& BatteryRow : BattalionRow.Battery)
	{
		AddBatteryToBattalion(BattalionGroup, BatteryRow);
	}

	for (const FS_OOBStation& StationRow : BattalionRow.Station)
	{
		AddStationToBattalion(BattalionGroup, StationRow);
	}

	for (const FS_OOBStarbase& StarbaseRow : BattalionRow.Starbase)
	{
		AddStarbaseToBattalion(BattalionGroup, StarbaseRow);
	}
}

void UStarshatterGameDataSubsystem::AddCivilianToForce(
	CombatGroup* ForceGroup,
	const FS_OOBCivilian& CivilianRow)
{
	if (!ForceGroup)
	{
		return;
	}

	CombatGroup* CivilianGroup = new CombatGroup(
		CivilianRow.Type,
		CivilianRow.Id,
		TCHAR_TO_ANSI(*CivilianRow.Name),
		CivilianRow.Iff,
		Intel::KNOWN,
		ForceGroup);

	if (!CivilianGroup)
	{
		return;
	}

	CivilianGroup->SetRegion(TCHAR_TO_ANSI(*CivilianRow.Location));
}

void UStarshatterGameDataSubsystem::AddWingToCarrier(
	CombatGroup* CarrierGroup,
	const FS_OOBWing& WingRow)
{
	if (!CarrierGroup)
	{
		return;
	}

	CombatGroup* WingGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::WING,
		WingRow.Id,
		TCHAR_TO_ANSI(*WingRow.Name),
		WingRow.Iff,
		Intel::KNOWN,
		CarrierGroup);

	if (!WingGroup)
	{
		return;
	}

	WingGroup->SetRegion(TCHAR_TO_ANSI(*WingRow.Location));

	for (const FS_OOBIntercept& InterceptRow : WingRow.Intercept)
	{
		AddInterceptSquadronToWing(WingGroup, InterceptRow);
	}

	for (const FS_OOBAttack& AttackRow : WingRow.Attack)
	{
		AddAttackSquadronToWing(WingGroup, AttackRow);
	}

	for (const FS_OOBFighter& FighterRow : WingRow.Fighter)
	{
		AddFighterSquadronToWing(WingGroup, FighterRow);
	}

	for (const FS_OOBLanding& LandingRow : WingRow.Landing)
	{
		AddLandingSquadronToWing(WingGroup, LandingRow);
	}
}

void UStarshatterGameDataSubsystem::AddInterceptSquadronToWing(
	CombatGroup* WingGroup,
	const FS_OOBIntercept& Row)
{
	if (!WingGroup)
	{
		return;
	}

	CombatGroup* SquadronGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		WingGroup);

	if (!SquadronGroup)
	{
		return;
	}

	SquadronGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(SquadronGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddAttackSquadronToWing(
	CombatGroup* WingGroup,
	const FS_OOBAttack& Row)
{
	if (!WingGroup)
	{
		return;
	}

	CombatGroup* SquadronGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::ATTACK_SQUADRON,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		WingGroup);

	if (!SquadronGroup)
	{
		return;
	}

	SquadronGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(SquadronGroup, Row.Unit);
}


void UStarshatterGameDataSubsystem::AddFighterSquadronToWing(
	CombatGroup* WingGroup,
	const FS_OOBFighter& Row)
{
	if (!WingGroup)
	{
		return;
	}

	CombatGroup* SquadronGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::FIGHTER_SQUADRON,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		WingGroup);

	if (!SquadronGroup)
	{
		return;
	}

	SquadronGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(SquadronGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddLandingSquadronToWing(
	CombatGroup* WingGroup,
	const FS_OOBLanding& Row)
{
	if (!WingGroup)
	{
		return;
	}

	CombatGroup* SquadronGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::LCA_SQUADRON,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		WingGroup);

	if (!SquadronGroup)
	{
		return;
	}

	SquadronGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(SquadronGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddBatteryToBattalion(
	CombatGroup* BattalionGroup,
	const FS_OOBBattery& Row)
{
	if (!BattalionGroup)
	{
		return;
	}

	CombatGroup* BatteryGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::BATTERY,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		BattalionGroup);

	if (!BatteryGroup)
	{
		return;
	}

	BatteryGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(BatteryGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddStationToBattalion(
	CombatGroup* BattalionGroup,
	const FS_OOBStation& Row)
{
	if (!BattalionGroup)
	{
		return;
	}

	CombatGroup* StationGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::STATION,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		BattalionGroup);

	if (!StationGroup)
	{
		return;
	}

	StationGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(StationGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddStarbaseToBattalion(
	CombatGroup* BattalionGroup,
	const FS_OOBStarbase& Row)
{
	if (!BattalionGroup)
	{
		return;
	}

	CombatGroup* StarbaseGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::STARBASE,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		BattalionGroup);

	if (!StarbaseGroup)
	{
		return;
	}

	StarbaseGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(StarbaseGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddMinefieldToFleet(
	CombatGroup* FleetGroup,
	const FS_OOBMinefield& Row)
{
	if (!FleetGroup)
	{
		return;
	}

	CombatGroup* MinefieldGroup = new CombatGroup(
		ECOMBATGROUP_TYPE::MINEFIELD,
		Row.Id,
		TCHAR_TO_ANSI(*Row.Name),
		Row.Iff,
		Intel::KNOWN,
		FleetGroup);

	if (!MinefieldGroup)
	{
		return;
	}

	MinefieldGroup->SetRegion(TCHAR_TO_ANSI(*Row.Location));
	AddUnitsToCombatGroup(MinefieldGroup, Row.Unit);
}

void UStarshatterGameDataSubsystem::AddUnitsToCombatGroup(
	CombatGroup* Parent,
	const TArray<FS_OOBUnit>& Units)
{
	if (!Parent)
	{
		return;
	}

	for (const FS_OOBUnit& UnitRow : Units)
	{
		int32 LocalUnitClass = 0;

		switch (UnitRow.Type)
		{
		case ECOMBATUNIT_TYPE::FIGHTER:
			LocalUnitClass = (int32)CLASSIFICATION::FIGHTER;
			break;
		case ECOMBATUNIT_TYPE::ATTACK:
			LocalUnitClass = (int32)CLASSIFICATION::ATTACK;
			break;
		case ECOMBATUNIT_TYPE::INTERCEPT:
			LocalUnitClass = (int32)CLASSIFICATION::FIGHTER;
			break;
		case ECOMBATUNIT_TYPE::LCA:
			LocalUnitClass = (int32)CLASSIFICATION::LCA;
			break;
		case ECOMBATUNIT_TYPE::CRUISER:
			LocalUnitClass = (int32)CLASSIFICATION::CRUISER;
			break;
		case ECOMBATUNIT_TYPE::DESTROYER:
			LocalUnitClass = (int32)CLASSIFICATION::DESTROYER;
			break;
		case ECOMBATUNIT_TYPE::FRIGATE:
			LocalUnitClass = (int32)CLASSIFICATION::FRIGATE;
			break;
		case ECOMBATUNIT_TYPE::CARRIER:
			LocalUnitClass = (int32)CLASSIFICATION::CARRIER;
			break;
		case ECOMBATUNIT_TYPE::MINE:
			LocalUnitClass = (int32)CLASSIFICATION::MINE;
			break;
		case ECOMBATUNIT_TYPE::BATTERY:
			LocalUnitClass = (int32)CLASSIFICATION::SAM;
			break;
		case ECOMBATUNIT_TYPE::STATION:
			LocalUnitClass = (int32)CLASSIFICATION::STATION;
			break;
		case ECOMBATUNIT_TYPE::STARBASE:
			LocalUnitClass = (int32)CLASSIFICATION::STARBASE;
			break;
		default:
			LocalUnitClass = 0;
			break;
		}

		CombatUnit* NewUnit = new CombatUnit(
			TCHAR_TO_ANSI(*UnitRow.Name),
			TCHAR_TO_ANSI(*UnitRow.Regnum),
			LocalUnitClass,
			TCHAR_TO_ANSI(*UnitRow.Design),
			UnitRow.Count > 0 ? UnitRow.Count : 1,
			Parent->GetIFF());

		if (!NewUnit)
		{
			continue;
		}

		NewUnit->SetCombatGroup(Parent);
		NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.Location));
		Parent->GetUnits().append(NewUnit);
	}

	if (Parent->GetUnits().size() > 0)
	{
		Parent->GetUnits()[0]->SetLeader(true);
	}
}

void UStarshatterGameDataSubsystem::AddUnitsToCombatGroup(
	CombatGroup* Parent,
	const TArray<FS_OOBFighterUnit>& Units)
{
	if (!Parent)
	{
		return;
	}

	for (const FS_OOBFighterUnit& UnitRow : Units)
	{
		int32 LocalUnitClass = (int32)CLASSIFICATION::FIGHTER;

		if (Parent->GetType() == ECOMBATGROUP_TYPE::ATTACK_SQUADRON)
		{
			LocalUnitClass = (int32)CLASSIFICATION::ATTACK;
		}
		else if (Parent->GetType() == ECOMBATGROUP_TYPE::LCA_SQUADRON)
		{
			LocalUnitClass = (int32)CLASSIFICATION::LCA;
		}

		CombatUnit* NewUnit = new CombatUnit(
			TCHAR_TO_ANSI(*UnitRow.Name),
			"",
			LocalUnitClass,
			TCHAR_TO_ANSI(*UnitRow.Design),
			UnitRow.Count > 0 ? UnitRow.Count : 1,
			Parent->GetIFF());

		if (!NewUnit)
		{
			continue;
		}

		NewUnit->SetCombatGroup(Parent);
		NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.Location));
		Parent->GetUnits().append(NewUnit);
	}

	if (Parent->GetUnits().size() > 0)
	{
		Parent->GetUnits()[0]->SetLeader(true);
	}
}

void UStarshatterGameDataSubsystem::AddUnitsToCombatGroup(
	CombatGroup* Parent,
	const TArray<FS_OOBMinefieldUnit>& Units)
{
	if (!Parent)
	{
		return;
	}

	for (const FS_OOBMinefieldUnit& UnitRow : Units)
	{
		CombatUnit* NewUnit = new CombatUnit(
			TCHAR_TO_ANSI(*UnitRow.Name),
			"",
			(int)CLASSIFICATION::MINE,
			TCHAR_TO_ANSI(*UnitRow.Design),
			UnitRow.Count > 0 ? UnitRow.Count : 1,
			Parent->GetIFF());

		if (!NewUnit)
		{
			continue;
		}

		NewUnit->SetCombatGroup(Parent);
		NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.Location));
		Parent->GetUnits().append(NewUnit);
	}

	if (Parent->GetUnits().size() > 0)
	{
		Parent->GetUnits()[0]->SetLeader(true);
	}
}

void UStarshatterGameDataSubsystem::AddUnitsToCombatGroup(
	CombatGroup* Parent,
	const TArray<FS_OOBStationUnit>& Units)
{
	if (!Parent)
	{
		return;
	}

	for (const FS_OOBStationUnit& UnitRow : Units)
	{
		CombatUnit* NewUnit = new CombatUnit(
			TCHAR_TO_ANSI(*UnitRow.Name),
			"",
			(int)CLASSIFICATION::STATION,
			TCHAR_TO_ANSI(*UnitRow.Design),
			UnitRow.Count > 0 ? UnitRow.Count : 1,
			Parent->GetIFF());

		if (!NewUnit)
		{
			continue;
		}

		NewUnit->SetCombatGroup(Parent);
		NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.Location));
		Parent->GetUnits().append(NewUnit);
	}

	if (Parent->GetUnits().size() > 0)
	{
		Parent->GetUnits()[0]->SetLeader(true);
	}
}
void UStarshatterGameDataSubsystem::AddUnitsToCombatGroup(
	CombatGroup* Parent,
	const TArray<FS_OOBStarbaseUnit>& Units)
{
	if (!Parent)
	{
		return;
	}

	for (const FS_OOBStarbaseUnit& UnitRow : Units)
	{
		CombatUnit* NewUnit = new CombatUnit(
			TCHAR_TO_ANSI(*UnitRow.Name),
			"",
			(int)CLASSIFICATION::STARBASE,
			TCHAR_TO_ANSI(*UnitRow.Design),
			UnitRow.Count > 0 ? UnitRow.Count : 1,
			Parent->GetIFF());

		if (!NewUnit)
		{
			continue;
		}

		NewUnit->SetCombatGroup(Parent);
		NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.Location));
		Parent->GetUnits().append(NewUnit);
	}

	if (Parent->GetUnits().size() > 0)
	{
		Parent->GetUnits()[0]->SetLeader(true);
	}
}

CombatGroup* UStarshatterGameDataSubsystem::BuildCombatForceFromRows(
	const TArray<FS_CombatGroup>& Rows,
	EEMPIRE_NAME Empire,
	Combatant* CombatantOwner)
{
	TMap<int32, CombatGroup*> RuntimeGroupById;
	TMap<int32, const FS_CombatGroup*> RuntimeRowById;

	CombatGroup* RootForce = nullptr;

	// Pass 1: create groups
	for (const FS_CombatGroup& Row : Rows)
	{
		if (Row.EmpireId != Empire)
		{
			continue;
		}

		CombatGroup* NewGroup = new CombatGroup(
			Row.Type,
			Row.Id,
			TCHAR_TO_ANSI(*Row.Name),
			Row.Iff,
			static_cast<int>(Row.Intel),
			nullptr);

		if (!NewGroup)
		{
			continue;
		}

		NewGroup->SetRegion(TCHAR_TO_ANSI(*Row.Region));
		NewGroup->SetLocation(Row.Location);
		NewGroup->SetUnitIndex(Row.UnitIndex);
		NewGroup->SetCombatant(CombatantOwner);

		RuntimeGroupById.Add(Row.Id, NewGroup);
		RuntimeRowById.Add(Row.Id, &Row);

		if (Row.Type == ECOMBATGROUP_TYPE::FORCE &&
			Row.ParentType == ECOMBATGROUP_TYPE::NONE &&
			Row.ParentId == 0)
		{
			RootForce = NewGroup;
		}
	}

	// Pass 2: link hierarchy
	for (const TPair<int32, CombatGroup*>& Pair : RuntimeGroupById)
	{
		const int32 RuntimeGroupKey = Pair.Key;
		CombatGroup* RuntimeGroup = Pair.Value;
		const FS_CombatGroup* SourceRow = RuntimeRowById.FindRef(RuntimeGroupKey);

		if (!SourceRow)
		{
			continue;
		}

		if (SourceRow->ParentType == ECOMBATGROUP_TYPE::NONE || SourceRow->ParentId == 0)
		{
			continue;
		}

		CombatGroup** ParentPtr = RuntimeGroupById.Find(SourceRow->ParentId);
		if (!ParentPtr || !(*ParentPtr))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[CombatRoster] Missing parent for group Id=%d ParentId=%d Name=%s"),
				SourceRow->Id, SourceRow->ParentId, *SourceRow->Name);
			continue;
		}

		(*ParentPtr)->AddComponent(RuntimeGroup);
	}

	// Pass 3: build units
	for (const TPair<int32, CombatGroup*>& Pair : RuntimeGroupById)
	{
		const int32 RuntimeGroupKey = Pair.Key;
		CombatGroup* RuntimeGroup = Pair.Value;
		const FS_CombatGroup* SourceRow = RuntimeRowById.FindRef(RuntimeGroupKey);

		if (!SourceRow)
		{
			continue;
		}

		bool bFirstUnit = true;

		for (const FS_CombatGroupUnit& UnitRow : SourceRow->Unit)
		{
			const int32 ResolvedUnitClass =
				ShipDesign::ClassForName(TCHAR_TO_ANSI(*UnitRow.UnitClass));

			if (!ShipDesign::CheckName(TCHAR_TO_ANSI(*UnitRow.UnitDesign)))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[CombatRoster] Invalid design '%s' for unit '%s' in group '%s'"),
					*UnitRow.UnitDesign, *UnitRow.UnitName, *SourceRow->Name);
				continue;
			}

			CombatUnit* NewUnit = new CombatUnit(
				TCHAR_TO_ANSI(*UnitRow.UnitName),
				TCHAR_TO_ANSI(*UnitRow.UnitRegnum),
				ResolvedUnitClass,
				TCHAR_TO_ANSI(*UnitRow.UnitDesign),
				FMath::Max(UnitRow.UnitCount, 1),
				SourceRow->Iff);

			if (!NewUnit)
			{
				continue;
			}

			NewUnit->SetCombatGroup(RuntimeGroup);
			NewUnit->SetSkin(TCHAR_TO_ANSI(*UnitRow.UnitSkin));

			if (!UnitRow.UnitRegion.IsEmpty())
			{
				NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.UnitRegion));
				NewUnit->MoveTo(UnitRow.UnitLoc);
			}
			else
			{
				NewUnit->SetRegion(TCHAR_TO_ANSI(*SourceRow->Region));
				NewUnit->MoveTo(SourceRow->Location);
			}

			NewUnit->Kill(UnitRow.UnitDead);
			NewUnit->SetSustainedDamage(UnitRow.UnitDamage);
			NewUnit->SetHeading(UnitRow.UnitHeading * DEGREES);

			if (bFirstUnit)
			{
				NewUnit->SetLeader(true);
				bFirstUnit = false;
			}

			RuntimeGroup->GetUnits().append(NewUnit);
		}
	}

	// Pass 4: carrier linking for fighter/attack groups
	for (const TPair<int32, CombatGroup*>& Pair : RuntimeGroupById)
	{
		CombatGroup* RuntimeGroup = Pair.Value;
		if (!RuntimeGroup)
		{
			continue;
		}

		const ECOMBATGROUP_TYPE RuntimeGroupType = RuntimeGroup->GetType();
		const bool bNeedsCarrier =
			RuntimeGroupType == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON ||
			RuntimeGroupType == ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON ||
			RuntimeGroupType == ECOMBATGROUP_TYPE::ATTACK_SQUADRON ||
			RuntimeGroupType == ECOMBATGROUP_TYPE::LCA_SQUADRON;

		if (!bNeedsCarrier)
		{
			continue;
		}

		CombatGroup* ParentGroup = RuntimeGroup->GetParent();
		CombatUnit* CarrierUnit = nullptr;

		while (ParentGroup && !CarrierUnit)
		{
			if (ParentGroup->GetUnits().size() > 0 &&
				ParentGroup->GetUnits()[0]->Type() == (int)CLASSIFICATION::CARRIER)
			{
				CarrierUnit = ParentGroup->GetUnits()[0];
			}

			ParentGroup = ParentGroup->GetParent();
		}

		if (CarrierUnit)
		{
			ListIter<CombatUnit> UnitIter = RuntimeGroup->GetUnits();
			while (++UnitIter)
			{
				CombatUnit* RuntimeUnit = UnitIter.value();
				if (!RuntimeUnit)
				{
					continue;
				}

				RuntimeUnit->SetCarrier(CarrierUnit);
				RuntimeUnit->SetRegion(CarrierUnit->GetRegion());
			}
		}
	}

	if (RootForce)
	{
		RootForce->CalcValue();
	}

	return RootForce;
}

TMap<FName, CombatGroup*> UStarshatterGameDataSubsystem::BuildGroupMapFromDataTable()
{
	TMap<FName, CombatGroup*> GroupByRowName;

	if (!CombatGroupDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatRoster] CombatGroupDataTable is NULL"));
		return GroupByRowName;
	}

	const TArray<FName> RowNames = CombatGroupDataTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		FS_CombatGroup* Row = CombatGroupDataTable->FindRow<FS_CombatGroup>(
			RowName,
			TEXT("BuildGroupMapFromDataTable"));

		if (!Row)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[CombatRoster] Missing row for %s"),
				*RowName.ToString());
			continue;
		}

		// Create runtime group
		CombatGroup* NewGroup = new CombatGroup(
			Row->Type,
			Row->Id,
			TCHAR_TO_ANSI(*Row->Name),
			Row->Iff,
			static_cast<int>(Row->Intel),
			nullptr);

		if (!NewGroup)
		{
			continue;
		}

		// Assign region safely (public API)
		NewGroup->AssignRegion(TCHAR_TO_ANSI(*Row->Region));

		// Assign location safely
		FVector GroupLoc = Row->Location;
		NewGroup->MoveTo(GroupLoc);

		// Store using RowName (guaranteed unique)
		GroupByRowName.Add(RowName, NewGroup);

		//UE_LOG(LogTemp, Log,
		//	TEXT("[CombatRoster] Created Group Row=%s Empire=%d Type=%d Id=%d Name=%s"),
		//	*RowName.ToString(),
		//	(int32)Row->EmpireId,
		//	(int32)Row->Type,
		//	Row->Id,
		//	*Row->Name);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatRoster] Total Groups Created: %d"),
		GroupByRowName.Num());

	return GroupByRowName;
}

void UStarshatterGameDataSubsystem::LinkGroupHierarchy(
	const TArray<FName>& RowNames,
	TMap<FName, CombatGroup*>& GroupByRowName)
{
	TMultiMap<FCombatGroupKey, FName> Lookup;

	// Build lookup (non-unique)
	for (const FName& RowName : RowNames)
	{
		FS_CombatGroup* Row =
			CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, TEXT(""));
		if (!Row) continue;

		Lookup.Add(
			FCombatGroupKey(Row->EmpireId, Row->Type, Row->Id),
			RowName);
	}

	// Helper: find empire root FORCE
	auto FindEmpireRootForce = [&](EEMPIRE_NAME Empire) -> CombatGroup*
		{
			for (const auto& Pair : GroupByRowName)
			{
				FS_CombatGroup* Row =
					CombatGroupDataTable->FindRow<FS_CombatGroup>(Pair.Key, TEXT(""));
				if (!Row) continue;

				if (Row->EmpireId == Empire &&
					Row->Type == ECOMBATGROUP_TYPE::FORCE &&
					Row->ParentType == ECOMBATGROUP_TYPE::NONE)
				{
					return Pair.Value;
				}
			}
			return nullptr;
		};

	// Link hierarchy
	for (const FName& RowName : RowNames)
	{
		FS_CombatGroup* Row =
			CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, TEXT(""));
		if (!Row) continue;

		if (Row->ParentType == ECOMBATGROUP_TYPE::NONE || Row->ParentId <= 0)
		{
			continue; // root-level group
		}

		CombatGroup* Child = GroupByRowName.FindRef(RowName);
		if (!Child) continue;

		// ---- NORMAL LOOKUP ----
		FCombatGroupKey ParentKey(Row->EmpireId, Row->ParentType, Row->ParentId);

		TArray<FName> Candidates;
		Lookup.MultiFind(ParentKey, Candidates);

		CombatGroup* Parent = nullptr;

		if (Candidates.Num() > 0)
		{
			Parent = GroupByRowName.FindRef(Candidates[0]);
		}

		// ---- FALLBACK: ParentId == 1 (civilian / infrastructure root) ----
		if (!Parent && Row->ParentId == 1)
		{
			Parent = FindEmpireRootForce(Row->EmpireId);

			if (Parent)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[CombatRoster] Fallback attach %s -> ROOT FORCE (%s)"),
					*Row->Name,
					ANSI_TO_TCHAR(Parent->GetName()));
			}
		}

		// ---- FINAL CHECK ----
		if (!Parent)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[CombatRoster] No parent for %s | Empire=%d Type=%d Id=%d ParentType=%d ParentId=%d"),
				*Row->Name,
				(int32)Row->EmpireId,
				(int32)Row->Type,
				Row->Id,
				(int32)Row->ParentType,
				Row->ParentId);
			continue;
		}

		// ---- LINK ----
		Parent->AddComponent(Child);

		UE_LOG(LogTemp, Log,
			TEXT("[CombatRoster] Linked %s -> %s"),
			*Row->Name,
			ANSI_TO_TCHAR(Parent->GetName()));
	}
}

void UStarshatterGameDataSubsystem::BuildCombatRosterFromDataTables()
{
	UE_LOG(LogTemp, Warning, TEXT("[GameData] BuildCombatRosterFromDataTables: BEGIN"));

	if (CombatRosterData.Num() == 0)
	{
		ReadCombatRosterData();
	}

	if (CombatantData.Num() == 0)
	{
		ReadCombatants();
	}

	//const TArray<FName> RowNames = CombatGroupDataTable->GetRowNames();
	const TArray<FName> RowNames = GetCampaignGroupRowNames();
	TMap<FName, CombatGroup*> GroupByRowName = BuildGroupMapFromDataTable();

	LinkGroupHierarchy(RowNames, GroupByRowName);
	BuildUnitsForGroups(RowNames, GroupByRowName);
	BuildCombatantsFromDataTables(RowNames, GroupByRowName);

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] BuildCombatRosterFromDataTables: COMPLETE (%d groups)"),
		GroupByRowName.Num());

	Campaign* CampaignPtr = Campaign::GetCampaign();
	if (!CampaignPtr)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatRoster] Campaign is NULL after build"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatRoster] After build: combatants=%d"),
		CampaignPtr->GetCombatants().size());

	ListIter<Combatant> It = CampaignPtr->GetCombatants();
	while (++It)
	{
		Combatant* C = It.value();
		if (!C) continue;

		CombatGroup* Root = C->GetForce();
		UE_LOG(LogTemp, Warning,
			TEXT("[CombatRoster] Combatant=%s Root=%s"),
			ANSI_TO_TCHAR(C->GetName()),
			Root ? ANSI_TO_TCHAR(Root->GetName()) : TEXT("NULL"));
	}
}

void UStarshatterGameDataSubsystem::BuildUnitsForGroups(
	const TArray<FName>& RowNames,
	TMap<FName, CombatGroup*>& GroupByRowName)
{
	for (const FName& RowName : RowNames)
	{
		FS_CombatGroup* Row =
			CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, TEXT(""));
		if (!Row) continue;

		CombatGroup* Group = GroupByRowName.FindRef(RowName);
		if (!Group) continue;

		bool bFirstUnit = true;

		for (const FS_CombatGroupUnit& UnitRow : Row->Unit)
		{
			int32 LocalUnitClass =
				ShipDesign::ClassForName(TCHAR_TO_ANSI(*UnitRow.UnitClass));

			if (!ShipDesign::CheckName(TCHAR_TO_ANSI(*UnitRow.UnitDesign)))
				continue;

			CombatUnit* NewUnit = new CombatUnit(
				TCHAR_TO_ANSI(*UnitRow.UnitName),
				TCHAR_TO_ANSI(*UnitRow.UnitRegnum),
				LocalUnitClass,
				TCHAR_TO_ANSI(*UnitRow.UnitDesign),
				FMath::Max(UnitRow.UnitCount, 1),
				Row->Iff);

			if (!NewUnit) continue;

			NewUnit->SetCombatGroup(Group);
			NewUnit->SetSkin(TCHAR_TO_ANSI(*UnitRow.UnitSkin));

			if (!UnitRow.UnitRegion.IsEmpty())
			{
				NewUnit->SetRegion(TCHAR_TO_ANSI(*UnitRow.UnitRegion));
				NewUnit->MoveTo(UnitRow.UnitLoc);
			}
			else
			{
				NewUnit->SetRegion(TCHAR_TO_ANSI(*Row->Region));
				NewUnit->MoveTo(Row->Location);
			}

			NewUnit->Kill(UnitRow.UnitDead);
			NewUnit->SetSustainedDamage(UnitRow.UnitDamage);
			NewUnit->SetHeading(UnitRow.UnitHeading * DEGREES);

			if (bFirstUnit)
			{
				NewUnit->SetLeader(true);
				bFirstUnit = false;
			}

			Group->GetUnits().append(NewUnit);
		}
	}
}

void UStarshatterGameDataSubsystem::BuildCombatantsFromDataTables(
	const TArray<FName>& RowNames,
	const TMap<FName, CombatGroup*>& GroupByRowName)
{
	Campaign* CampaignPtr = Campaign::GetCampaign();
	if (!CampaignPtr)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatRoster] Campaign NULL"));
		return;
	}

	for (const FS_Combatant& Row : CombatantData)
	{
		if (Row.Group.Num() == 0)
			continue;

		const FS_CombatantGroup& RootRef = Row.Group[0];

		CombatGroup* RootForce = nullptr;

		for (const auto& Pair : GroupByRowName)
		{
			FS_CombatGroup* GroupRow =
				CombatGroupDataTable->FindRow<FS_CombatGroup>(Pair.Key, TEXT(""));

			if (!GroupRow) continue;

			if (GroupRow->EmpireId == Row.Name &&
				GroupRow->Type == RootRef.Type &&
				GroupRow->Id == RootRef.Id)
			{
				RootForce = Pair.Value;
				break;
			}
		}

		if (!RootForce)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[CombatRoster] Missing root for combatant %d"),
				(int32)Row.Name);
			continue;
		}

		FString NameStr = UEnum::GetValueAsString(Row.Name);
		FTCHARToUTF8 Utf8(*NameStr);

		Combatant* NewCombatant =
			new Combatant(Utf8.Get(), RootForce);

		CampaignPtr->GetCombatants().append(NewCombatant);

		UE_LOG(LogTemp, Log,
			TEXT("[CombatRoster] Created Combatant %s"),
			*NameStr);
	}
}

void UStarshatterGameDataSubsystem::ReadCombatants()
{
	CombatantData.Empty();

	if (CampaignDataArray.Num() == 0)
	{
		ReadCampaignData();
	}

	if (CampaignIndex < 0 || !CampaignDataArray.IsValidIndex(CampaignIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameData] ReadCombatants: invalid CampaignIndex=%d"), CampaignIndex);
		return;
	}

	const FS_Campaign& CampaignRow = CampaignDataArray[CampaignIndex];
	CombatantData = CampaignRow.Combatant;

	UE_LOG(LogTemp, Warning,
		TEXT("[GameData] ReadCombatants: loaded %d combatants from campaign '%s'"),
		CombatantData.Num(),
		*CampaignRow.Name);
}

void UStarshatterGameDataSubsystem::ValidateCombatRosterRuntime()
{
	Campaign* CampaignPtr = Campaign::GetCampaign();
	if (!CampaignPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatRoster] Validate: Campaign is NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatRoster] Validate: combatants=%d"),
		CampaignPtr->GetCombatants().size());

	ListIter<Combatant> CombatantIter = CampaignPtr->GetCombatants();
	while (++CombatantIter)
	{
		Combatant* C = CombatantIter.value();
		if (!C)
		{
			continue;
		}

		CombatGroup* Force = C->GetForce();

		UE_LOG(LogTemp, Warning,
			TEXT("[CombatRoster] Combatant=%s Force=%s"),
			ANSI_TO_TCHAR(C->GetName()),
			Force ? ANSI_TO_TCHAR(Force->GetName()) : TEXT("NULL"));

		if (Force)
		{
			DumpGroupTree(Force, 0);
		}
	}
}

TArray<FName> UStarshatterGameDataSubsystem::GetCampaignEntryRowNames() const
{
	TArray<FName> EntryRows;

	if (!CombatGroupDataTable)
	{
		return EntryRows;
	}

	const TArray<FName> AllRowNames = CombatGroupDataTable->GetRowNames();

	for (const FS_Combatant& CombatantRow : CombatantData)
	{
		for (const FS_CombatantGroup& GroupRef : CombatantRow.Group)
		{
			for (const FName& RowName : AllRowNames)
			{
				const FS_CombatGroup* GroupRow =
					CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, TEXT(""));

				if (!GroupRow)
				{
					continue;
				}

				if (GroupRow->EmpireId == CombatantRow.Name &&
					GroupRow->Type == GroupRef.Type &&
					GroupRow->Id == GroupRef.Id)
				{
					EntryRows.AddUnique(RowName);
				}
			}
		}
	}

	return EntryRows;
}

void UStarshatterGameDataSubsystem::CollectChildRowsRecursive(
	const FName& ParentRowName,
	TSet<FName>& OutRows) const
{
	if (!CombatGroupDataTable)
	{
		return;
	}

	if (OutRows.Contains(ParentRowName))
	{
		return;
	}

	const FS_CombatGroup* ParentRow =
		CombatGroupDataTable->FindRow<FS_CombatGroup>(ParentRowName, TEXT(""));

	if (!ParentRow)
	{
		return;
	}

	OutRows.Add(ParentRowName);

	const TArray<FName> AllRowNames = CombatGroupDataTable->GetRowNames();

	for (const FName& ChildRowName : AllRowNames)
	{
		const FS_CombatGroup* ChildRow =
			CombatGroupDataTable->FindRow<FS_CombatGroup>(ChildRowName, TEXT(""));

		if (!ChildRow)
		{
			continue;
		}

		if (ChildRow->EmpireId == ParentRow->EmpireId &&
			ChildRow->ParentType == ParentRow->Type &&
			ChildRow->ParentId == ParentRow->Id)
		{
			CollectChildRowsRecursive(ChildRowName, OutRows);
		}
	}
}

TArray<FName> UStarshatterGameDataSubsystem::GetCampaignGroupRowNames() const
{
	TSet<FName> AllowedRows;
	const TArray<FName> EntryRows = GetCampaignEntryRowNames();

	for (const FName& EntryRowName : EntryRows)
	{
		CollectChildRowsRecursive(EntryRowName, AllowedRows);
	}

	return AllowedRows.Array();
}
