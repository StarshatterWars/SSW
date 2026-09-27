// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "Intel.h"
#include "Game.h"
#include "DataLoader.h"

// +--------------------------------------------------------------------+

EIntel
Intel::GetIntelFromName(const char* type_name)
{
	if (!type_name || !type_name[0])
	{
		return EIntel::UNKNOWN;
	}

	for (uint8 i = (uint8)EIntel::RESERVE;
		i <= (uint8)EIntel::ACTIVE;
		i++)
	{
		const EIntel Type =
			(EIntel)i;

		const UEnum* EnumPtr =
			StaticEnum<EIntel>();

		if (!EnumPtr)
		{
			break;
		}

		const FString DisplayName =
			EnumPtr->GetDisplayNameTextByValue(i).ToString();

		if (!_stricmp(
			type_name,
			TCHAR_TO_ANSI(*DisplayName)))
		{
			return Type;
		}

		const FString InternalName =
			EnumPtr->GetNameStringByValue(i);

		if (!_stricmp(
			type_name,
			TCHAR_TO_ANSI(*InternalName)))
		{
			return Type;
		}
	}

	return EIntel::UNKNOWN;
}

const char*
Intel::GetNameFromIntel(EIntel IntelType)
{
	const UEnum* EnumPtr =
		StaticEnum<EIntel>();

	if (!EnumPtr)
	{
		return "Unknown";
	}

	const int64 Value =
		(int64)IntelType;

	if (!EnumPtr->IsValidEnumValue(Value))
	{
		return "Unknown";
	}

	static FString CachedName;

	CachedName =
		EnumPtr->GetDisplayNameTextByValue(Value).ToString();

	return TCHAR_TO_ANSI(*CachedName);
}
