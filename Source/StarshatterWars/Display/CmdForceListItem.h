/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    UI
	FILE:         CmdForceListItem.h
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ListView row object for CmdForceDlg.
	Represents a single visible row in the forces list.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CmdForceListItem.generated.h"

class CombatGroup;
class CombatUnit;

UENUM(BlueprintType)
enum class EForceRowType : uint8
{
	Group	UMETA(DisplayName = "Group"),
	Unit	UMETA(DisplayName = "Unit"),
	Spacer	UMETA(DisplayName = "Spacer")
};

UCLASS(BlueprintType)
class STARSHATTERWARS_API UCmdForceListItem : public UObject
{
	GENERATED_BODY()

public:
	// Fully formatted row text, including any pipe/indent prefix you build in CmdForceDlg
	UPROPERTY(BlueprintReadOnly, Category = "CmdForce")
	FString DisplayText;

	// What kind of row this is
	UPROPERTY(BlueprintReadOnly, Category = "CmdForce")
	EForceRowType RowType = EForceRowType::Spacer;

	// Logical indent level for UI layout if needed
	UPROPERTY(BlueprintReadOnly, Category = "CmdForce")
	int32 IndentLevel = 0;

	// Group-only state
	UPROPERTY(BlueprintReadOnly, Category = "CmdForce")
	bool bExpanded = false;

	UPROPERTY(BlueprintReadOnly, Category = "CmdForce")
	bool bHasChildren = false;

	// Source pointers from legacy runtime objects
	CombatGroup* Group = nullptr;
	CombatUnit* Unit = nullptr;

public:
	// Convenience helpers
	bool IsGroup() const
	{
		return RowType == EForceRowType::Group;
	}

	bool IsUnit() const
	{
		return RowType == EForceRowType::Unit;
	}

	bool IsSpacer() const
	{
		return RowType == EForceRowType::Spacer;
	}

	void InitAsGroup(const FString& InDisplayText, int32 InIndentLevel, CombatGroup* InGroup, bool bInExpanded, bool bInHasChildren)
	{
		DisplayText = InDisplayText;
		RowType = EForceRowType::Group;
		IndentLevel = InIndentLevel;
		Group = InGroup;
		Unit = nullptr;
		bExpanded = bInExpanded;
		bHasChildren = bInHasChildren;
	}

	void InitAsUnit(const FString& InDisplayText, int32 InIndentLevel, CombatUnit* InUnit)
	{
		DisplayText = InDisplayText;
		RowType = EForceRowType::Unit;
		IndentLevel = InIndentLevel;
		Group = nullptr;
		Unit = InUnit;
		bExpanded = false;
		bHasChildren = false;
	}

	void InitAsSpacer()
	{
		DisplayText = TEXT("");
		RowType = EForceRowType::Spacer;
		IndentLevel = 0;
		Group = nullptr;
		Unit = nullptr;
		bExpanded = false;
		bHasChildren = false;
	}
};
