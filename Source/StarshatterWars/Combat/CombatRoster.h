// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

// Forward declare your UE-side CombatGroup class
class CombatGroup;

/**
 * UE port of Starshatter CombatRoster.
 *
 * Starshatter role:
 * - Global registry of "forces" (top-level CombatGroup trees) keyed by name.
 * - Used by Campaign when cloning combatants from the master order of battle.
 *
 * This version is intentionally minimal and compile-safe:
 * - No file parsing.
 * - No reliance on CombatGroup internals (no Name() access).
 * - You register forces explicitly from your DataTable / bootstrap code.
 */
class CombatRoster
{
public:
	static CombatRoster* GetInstance();
	static void Initialize();
	static void Close();

	// Clears all forces
	void Clear();

	// Register a force under a name (case-insensitive keying).
	// This avoids depending on CombatGroup exposing a public Name() accessor.
	void RegisterForce(const FString& ForceName, CombatGroup* Force);

	// Remove a force by name (case-insensitive).
	void UnregisterForce(const FString& ForceName);

	// Lookup a force by name (case-insensitive).
	CombatGroup* GetForce(const FString& ForceName) const;

	// Optional: access to all registered forces (in insertion order).
	const TArray<CombatGroup*>& GetForces() const { return Forces; }

private:
	CombatRoster() = default;
	~CombatRoster() = default;

	// Normalize a key for case-insensitive lookups.
	static FString NormalizeKey(const FString& In);

private:
	static CombatRoster* Instance;

	// Primary registry:
	// - map is keyed on a normalized (lowercase) name.
	TMap<FString, CombatGroup*> ForcesByName;

	// Optional ordered list (useful for debug, UI, iteration).
	TArray<CombatGroup*> Forces;
};

