/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Encrypt.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simple Encryption / Decryption class
*/

#pragma once

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

#include "Types.h"
#include "Text.h"

// +-------------------------------------------------------------------+

class Encryption
{
public:
	// private key encryption / decryption of
	// arbitrary blocks of data
	static Text Encrypt(Text block);
	static Text Decrypt(Text block);

	// encode / decode binary blocks into
	// ascii strings for use in text files
	static Text Encode(Text block);
	static Text Decode(Text block);
};
