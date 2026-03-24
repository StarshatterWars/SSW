// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#include "CombatRoster.h"

CombatRoster* CombatRoster::Instance = nullptr;

CombatRoster* CombatRoster::GetInstance()
{
	if (!Instance)
	{
		Initialize();
	}
	return Instance;
}

void CombatRoster::Initialize()
{
	if (!Instance)
	{
		Instance = new CombatRoster();
	}
}

void CombatRoster::Close()
{
	if (Instance)
	{
		Instance->Clear();
		delete Instance;
		Instance = nullptr;
	}
}

void CombatRoster::Clear()
{
	ForcesByName.Reset();
	Forces.Reset();
}

FString CombatRoster::NormalizeKey(const FString& In)
{
	// Case-insensitive match: normalize to lower, trimmed.
	FString Key = In;
	Key.TrimStartAndEndInline();
	Key.ToLowerInline();
	return Key;
}

void CombatRoster::RegisterForce(const FString& ForceName, CombatGroup* Force)
{
	if (!Force)
	{
		return;
	}

	const FString Key = NormalizeKey(ForceName);
	if (Key.IsEmpty())
	{
		return;
	}

	// If already registered, update pointer (and maintain ordered list sensibly)
	CombatGroup** Existing = ForcesByName.Find(Key);
	if (Existing)
	{
		*Existing = Force;

		// Update ordered list entry if present; otherwise append.
		const int32 Index = Forces.IndexOfByKey(*Existing);
		if (Index >= 0)
		{
			Forces[Index] = Force;
		}
		else
		{
			Forces.Add(Force);
		}
		return;
	}

	ForcesByName.Add(Key, Force);
	Forces.Add(Force);
}

void CombatRoster::UnregisterForce(const FString& ForceName)
{
	const FString Key = NormalizeKey(ForceName);
	if (Key.IsEmpty())
	{
		return;
	}

	CombatGroup* const* Found = ForcesByName.Find(Key);
	if (!Found)
	{
		return;
	}

	CombatGroup* ForcePtr = *Found;
	ForcesByName.Remove(Key);

	// Remove from ordered list (first match).
	Forces.RemoveSingle(ForcePtr);
}

CombatGroup* CombatRoster::GetForce(const FString& ForceName) const
{
	const FString Key = NormalizeKey(ForceName);
	if (Key.IsEmpty())
	{
		return nullptr;
	}

	if (CombatGroup* const* Found = ForcesByName.Find(Key))
	{
		return *Found;
	}

	return nullptr;
}

