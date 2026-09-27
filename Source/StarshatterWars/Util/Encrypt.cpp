/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Encrypt.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simple Encryption / Decryption class
*/

#include "Encrypt.h"
#include "GameStructs.h"

#include "HAL/PlatformMemory.h"   // FMemory::Memzero / Memcpy
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterEncrypt, Log, All);

// +--------------------------------------------------------------------+

static uint32 k[4] = {
	0x3B398E26u,
	0x40C29501u,
	0x614D7630u,
	0x7F59409Au
};

static void encypher(uint32* v)
{
	uint32 y = v[0];
	uint32 z = v[1];
	uint32 sum = 0;
	const uint32 delta = 0x9e3779b9u; // key schedule constant
	uint32 n = 32;                    // num iterations

	while (n-- > 0) {
		sum += delta;
		y += ((z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
		z += ((y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
	}

	v[0] = y;
	v[1] = z;
}

static void decypher(uint32* v)
{
	uint32 y = v[0];
	uint32 z = v[1];
	const uint32 delta = 0x9e3779b9u; // key schedule constant
	uint32 sum = (delta << 5);
	uint32 n = 32;

	while (n-- > 0) {
		z -= ((y << 4) + k[2]) ^ (y + sum) ^ ((y >> 5) + k[3]);
		y -= ((z << 4) + k[0]) ^ (z + sum) ^ ((z >> 5) + k[1]);
		sum -= delta;
	}

	v[0] = y;
	v[1] = z;
}

// +-------------------------------------------------------------------+

Text
Encryption::Encrypt(Text block)
{
	int len = block.length();
	if (len < 1)
		return Text();

	// pad to eight byte chunks:
	if (len & 0x7) {
		len = (len / 8) * 8 + 8;
	}

	uint8* work = new uint8[len];
	FMemory::Memzero(work, len);
	FMemory::Memcpy(work, block.data(), block.length());

	uint32* v = reinterpret_cast<uint32*>(work);
	for (int i = 0; i < len / 8; i++) {
		encypher(v);
		v += 2;
	}

	Text cypher(reinterpret_cast<const char*>(work), len);
	delete[] work;
	return cypher;
}

// +-------------------------------------------------------------------+

Text
Encryption::Decrypt(Text block)
{
	const int len = block.length();

	if (len & 0x7) {
		UE_LOG(LogStarshatterEncrypt, Warning,
			TEXT("WARNING: attempt to decrypt odd length block (len=%d)"),
			len);
		return Text();
	}

	uint8* work = new uint8[len];
	FMemory::Memcpy(work, block.data(), len);

	uint32* v = reinterpret_cast<uint32*>(work);
	for (int i = 0; i < len / 8; i++) {
		decypher(v);
		v += 2;
	}

	Text clear(reinterpret_cast<const char*>(work), len);
	delete[] work;
	return clear;
}

// +-------------------------------------------------------------------+

static const char* codes = "abcdefghijklmnop";

Text
Encryption::Encode(Text block)
{
	const int len = block.length() * 2;
	char* work = new char[len + 1];

	for (int i = 0; i < block.length(); i++) {
		const uint8 b = static_cast<uint8>(block.data()[i]);
		work[2 * i] = codes[(b >> 4) & 0xf];
		work[2 * i + 1] = codes[(b) & 0xf];
	}

	work[len] = 0;

	Text code(work, len);
	delete[] work;
	return code;
}

// +-------------------------------------------------------------------+

Text
Encryption::Decode(Text block)
{
	const int len = block.length() / 2;
	char* work = new char[len + 1];

	for (int i = 0; i < len; i++) {
		const char u = block[2 * i];
		const char l = block[2 * i + 1];

		work[i] = static_cast<char>(((u - codes[0]) << 4) | (l - codes[0]));
	}

	work[len] = 0;

	Text clear(work, len);
	delete[] work;
	return clear;
}
