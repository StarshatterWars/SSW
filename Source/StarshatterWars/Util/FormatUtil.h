/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         FormatUtil.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Text formatting utilities
*/

#pragma once

#include "CoreMinimal.h"
#include "Types.h"
#include "Geometry.h"
#include "Text.h"

/**
 * 
 */

 // +--------------------------------------------------------------------+

void FormatNumber(TArrayView<char> txt, double n);
void FormatNumberExp(TArrayView<char> txt, double n);
void FormatTime(TArrayView<char> txt, double seconds);
void FormatTimeOfDay(TArrayView<char> txt, double seconds);
void FormatDayTime(TArrayView<char> txt, double seconds, bool short_format = false);
void FormatDay(TArrayView<char> txt, double seconds);
FString FormatDayFromString(FString txt);
void FormatPoint(TArrayView<char> txt, const Point& p);
Text FormatTimeString(int utc = 0);

static const char* FormatTimeString(double seconds);

const char* SafeString(const char* s);
const char* SafeQuotes(const char* s);

// scan msg and replace all occurrences of tgt with val
// return new result, leave msg unmodified
Text FormatTextReplace(const char* msg, const char* tgt, const char* val);

// scan msg and replace all C-style \x escape sequences
// with their single-character values, leave orig unmodified
Text FormatTextEscape(const char* msg);

// +--------------------------------------------------------------------+
