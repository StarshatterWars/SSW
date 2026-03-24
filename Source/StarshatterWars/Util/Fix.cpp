/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         Fix.cpp
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Fixed point number class with 16 bits of fractional precision
*/

#include "Fix.h"

// +--------------------------------------------------------------------+

const fix    fix::one = fix(1);
const fix    fix::two = fix(2);
const fix    fix::three = fix(3);
const fix    fix::five = fix(5);
const fix    fix::ten = fix(10);