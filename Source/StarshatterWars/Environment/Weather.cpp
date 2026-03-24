/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo / Destroyer Studios LLC
	Copyright © 1997-2007. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Weather.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Manages local weather conditions according to the system stardate
*/

#include "Weather.h"
#include "StarSystem.h"
#include "Game.h"

#include "Math/UnrealMathUtility.h" // FMath::Sin, PI

// +--------------------------------------------------------------------+

Weather::Weather()
{
	state = CLEAR;
	period = 7 * 20 * 3600;
	ceiling = 0;
	visibility = 1;

	for (int i = 0; i < NUM_STATES; i++)
		chances[i] = 0;

	chances[0] = 1;
}

// +--------------------------------------------------------------------+

Weather::~Weather()
{
}

// +--------------------------------------------------------------------+

void
Weather::SetChance(int n, double c)
{
	if (n >= 0 && n < NUM_STATES) {
		if (c > 1 && c <= 100)
			chances[n] = c / 100;

		else if (c < 1)
			chances[n] = c;
	}
}

void
Weather::NormalizeChances()
{
	double total = 0;

	for (int i = 1; i < NUM_STATES; i++)
		total += chances[i];

	if (total <= 1) {
		chances[0] = 1 - total;
	}
	else {
		chances[0] = 0;

		for (int i = 1; i < NUM_STATES; i++)
			chances[i] /= total;
	}

	int    index = 0;
	double level = 0;

	for (int i = 0; i < NUM_STATES; i++) {
		if (chances[i] > 0) {
			level += chances[i];

			active_states[index] = (STATE)i;
			thresholds[index] = level;

			index++;
		}
	}

	while (index < NUM_STATES)
		thresholds[index++] = 10;
}

// +--------------------------------------------------------------------+

void
Weather::Update()
{
	NormalizeChances();

	// NOTE:
	// - StarSystem::Stardate() is assumed to be "days" (as in the original codebase),
	//   and 'period' is in seconds. We preserve the legacy behavior here.
	// - If you later standardize stardate units, adjust this expression accordingly.
	const double stardate = StarSystem::Stardate();
	const double omega = 2.0 * PI / period;

	const double w = (FMath::Sin(stardate * omega) + 1.0) * 0.5;

	state = active_states[0];

	for (int i = 1; i < NUM_STATES; i++) {
		if (w > thresholds[i - 1] && w <= thresholds[i]) {
			state = active_states[i];
			break;
		}
	}

	switch (state) {
	default:
	case CLEAR:
		ceiling = 0;
		visibility = 1.0;
		break;

	case HIGH_CLOUDS:
		ceiling = 0;
		visibility = 0.9;
		break;

	case MODERATE_CLOUDS:
		ceiling = 0;
		visibility = 0.8;
		break;

	case OVERCAST:
		ceiling = 6000;
		visibility = 0.7;
		break;

	case FOG:
		ceiling = 3500;
		visibility = 0.6;
		break;

	case STORM:
		ceiling = 7500;
		visibility = 0.5;
		break;
	}
}

// +--------------------------------------------------------------------+

Text
Weather::Description() const
{
	Text description;

	switch (state) {
	default:
	case CLEAR:
		description = Game::GetText("weather.clear");
		break;

	case HIGH_CLOUDS:
		description = Game::GetText("weather.high-clouds");
		break;

	case MODERATE_CLOUDS:
		description = Game::GetText("weather.partly-cloudy");
		break;

	case OVERCAST:
		description = Game::GetText("weather.overcast");
		break;

	case FOG:
		description = Game::GetText("weather.fog");
		break;

	case STORM:
		description = Game::GetText("weather.storm");
		break;
	}

	return description;
}
