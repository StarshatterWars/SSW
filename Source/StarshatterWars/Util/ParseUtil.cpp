/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024.
	All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         ParseUtil.cpp
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Legacy DEF parsing helpers ported to Unreal-native logging.
	- Replaces legacy Print()/printf-style output with UE_LOG.
	- Keeps original parsing behavior and return semantics.
	- Uses ANSI_TO_TCHAR for const char* file/name inputs.
*/

#include "ParseUtil.h"

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"

// +--------------------------------------------------------------------+
// Helpers
// +--------------------------------------------------------------------+

bool GetDefVec(Vec3& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing VEC3 TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() != 3) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed vector in '%s'"),
				ANSI_TO_TCHAR(file));
		}
		else {
			dst.X = (float)(val->elements()->at(0)->isNumber()->value());
			dst.Y = (float)(val->elements()->at(1)->isNumber()->value());
			dst.Z = (float)(val->elements()->at(2)->isNumber()->value());

			UE_LOG(LogTemp, Log,
				TEXT("%s: [ %f,%f,%f ]"),
				ANSI_TO_TCHAR(def->name()->value()),
				dst.X, dst.Y, dst.Z);
			return true;
		}
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: vector expected in '%s'"),
			ANSI_TO_TCHAR(file));
	}

	return false;
}

bool GetDefRect(Rect& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing RECT TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() != 4) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed rect in '%s'"),
				ANSI_TO_TCHAR(file));
		}
		else {
			dst.x = (int)(val->elements()->at(0)->isNumber()->value());
			dst.y = (int)(val->elements()->at(1)->isNumber()->value());
			dst.w = (int)(val->elements()->at(2)->isNumber()->value());
			dst.h = (int)(val->elements()->at(3)->isNumber()->value());
			return true;
		}
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: rect expected in '%s'"),
			ANSI_TO_TCHAR(file));
	}

	return false;
}

bool GetDefInsets(Insets& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing Insets TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() != 4) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed Insets in '%s'"),
				ANSI_TO_TCHAR(file));
		}
		else {
			dst.left = (WORD)(val->elements()->at(0)->isNumber()->value());
			dst.right = (WORD)(val->elements()->at(1)->isNumber()->value());
			dst.top = (WORD)(val->elements()->at(2)->isNumber()->value());
			dst.bottom = (WORD)(val->elements()->at(3)->isNumber()->value());
			return true;
		}
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: Insets expected in '%s'"),
			ANSI_TO_TCHAR(file));
	}

	return false;
}

// ---------------------------------------------------------------------
// Legacy Color overload (Color type is legacy, NOT FColor)
// ---------------------------------------------------------------------
bool GetDefColor(Color& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing COLOR TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() != 3) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed color in '%s'"),
				ANSI_TO_TCHAR(file));
		}
		else {
			BYTE r, g, b;
			double v0 = (val->elements()->at(0)->isNumber()->value());
			double v1 = (val->elements()->at(1)->isNumber()->value());
			double v2 = (val->elements()->at(2)->isNumber()->value());

			if (v0 >= 0 && v0 <= 1 &&
				v1 >= 0 && v1 <= 1 &&
				v2 >= 0 && v2 <= 1) {

				r = (BYTE)(v0 * 255);
				g = (BYTE)(v1 * 255);
				b = (BYTE)(v2 * 255);
			}
			else {
				r = (BYTE)v0;
				g = (BYTE)v1;
				b = (BYTE)v2;
			}

			dst = Color(r, g, b);
			return true;
		}
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: color expected in '%s'"),
			ANSI_TO_TCHAR(file));
	}

	return false;
}

// ---------------------------------------------------------------------
// Unreal Color overload (FColor)
// ---------------------------------------------------------------------
bool GetDefFColor(FColor& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing COLOR TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() != 3) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed color in '%s'"),
				ANSI_TO_TCHAR(file));
			return false;
		}

		const Term* e0 = val->elements()->at(0);
		const Term* e1 = val->elements()->at(1);
		const Term* e2 = val->elements()->at(2);

		if (!e0 || !e1 || !e2 ||
			!e0->isNumber() || !e1->isNumber() || !e2->isNumber()) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: invalid color values in '%s'"),
				ANSI_TO_TCHAR(file));
			return false;
		}

		double v0 = e0->isNumber()->value();
		double v1 = e1->isNumber()->value();
		double v2 = e2->isNumber()->value();

		uint8 r, g, b;

		// Normalize [0..1] or accept [0..255]
		if (v0 >= 0.0 && v0 <= 1.0 &&
			v1 >= 0.0 && v1 <= 1.0 &&
			v2 >= 0.0 && v2 <= 1.0) {

			r = (uint8)(v0 * 255.0);
			g = (uint8)(v1 * 255.0);
			b = (uint8)(v2 * 255.0);
		}
		else {
			r = (uint8)FMath::Clamp(v0, 0.0, 255.0);
			g = (uint8)FMath::Clamp(v1, 0.0, 255.0);
			b = (uint8)FMath::Clamp(v2, 0.0, 255.0);
		}

		dst = FColor(r, g, b, 255);
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: color expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

// ---------------------------------------------------------------------
// ColorValue overload (legacy float RGBA container)
// ---------------------------------------------------------------------
bool GetDefColor(ColorValue& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing COLOR TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		if (val->elements()->size() < 3 || val->elements()->size() > 4) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: malformed color in '%s'"),
				ANSI_TO_TCHAR(file));
		}
		else {
			double r = (val->elements()->at(0)->isNumber()->value());
			double g = (val->elements()->at(1)->isNumber()->value());
			double b = (val->elements()->at(2)->isNumber()->value());
			double a = 1;

			if (val->elements()->size() == 4)
				a = (val->elements()->at(3)->isNumber()->value());

			dst.Set((float)r, (float)g, (float)b, (float)a);
			return true;
		}
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: color expected in '%s'"),
			ANSI_TO_TCHAR(file));
	}

	return false;
}

// +--------------------------------------------------------------------+
// Arrays
// +--------------------------------------------------------------------+

bool GetDefArray(int* dst, int size, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing ARRAY TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		int nelem = (int)val->elements()->size();
		if (nelem > size) nelem = size;

		for (int i = 0; i < nelem; i++)
			*dst++ = (int)(val->elements()->at(i)->isNumber()->value());

		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: array expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefArray(float* dst, int size, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing ARRAY TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		int nelem = (int)val->elements()->size();
		if (nelem > size) nelem = size;

		for (int i = 0; i < nelem; i++)
			*dst++ = (float)(val->elements()->at(i)->isNumber()->value());

		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: array expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefArray(double* dst, int size, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing ARRAY TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		int nelem = (int)val->elements()->size();
		if (nelem > size) nelem = size;

		for (int i = 0; i < nelem; i++)
			*dst++ = (double)(val->elements()->at(i)->isNumber()->value());

		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: array expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefArray(std::vector<DWORD>& array, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing ARRAY TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		int nelem = (int)val->elements()->size();
		array.clear();
		for (int i = 0; i < nelem; i++)
			array.push_back((DWORD)(val->elements()->at(i)->isNumber()->value()));
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: integer array expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefArray(std::vector<float>& array, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing ARRAY TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermArray* val = def->term()->isArray();
	if (val) {
		int nelem = (int)val->elements()->size();
		array.clear();
		for (int i = 0; i < nelem; i++)
			array.push_back((float)(val->elements()->at(i)->isNumber()->value()));
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: float array expected in '%s'"),
		ANSI_TO_TCHAR(file));

	return false;
}

// +--------------------------------------------------------------------+
// Time / Bool / Text / Number
// +--------------------------------------------------------------------+

bool GetDefTime(int& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing TIME TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermText* tn = def->term()->isText();
	if (tn) {
		int d = 0, h = 0, m = 0, s = 0;

		char buf[64];
		strcpy_s(buf, tn->value());

		if (strchr(buf, '/'))
			sscanf_s(buf, "%d/%d:%d:%d", &d, &h, &m, &s);
		else
			sscanf_s(buf, "%d:%d:%d", &h, &m, &s);

		dst = d * 24 * 60 * 60 +
			h * 60 * 60 +
			m * 60 +
			s;

		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid TIME %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefBool(bool& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing BOOL TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermBool* tn = def->term()->isBool();
	if (tn) {
		dst = tn->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid BOOL %s in '%s' (dumping term)"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	// Keep legacy debug dump (writes to legacy output stream, not UE log):
	def->term()->print(10);

	return false;
}

bool GetDefText(Text& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing TEXT TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermText* tn = def->term()->isText();
	if (tn) {
		dst = tn->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid TEXT %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefText(char* dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing TEXT TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermText* tn = def->term()->isText();
	if (tn) {
		strcpy(dst, tn->value()); // keep legacy behavior
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid TEXT %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefNumber(int& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing NUMBER TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermNumber* tr = def->term()->isNumber();
	if (tr) {
		dst = (int)tr->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid NUMBER %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefNumber(DWORD& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing NUMBER TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermNumber* tr = def->term()->isNumber();
	if (tr) {
		dst = (DWORD)tr->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid NUMBER %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefNumber(float& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing NUMBER TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermNumber* tr = def->term()->isNumber();
	if (tr) {
		dst = (float)tr->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid NUMBER %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}

bool GetDefNumber(double& dst, TermDef* def, const char* file)
{
	if (!def || !def->term()) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: missing NUMBER TermDef in '%s'"),
			ANSI_TO_TCHAR(file));
		return false;
	}

	TermNumber* tr = def->term()->isNumber();
	if (tr) {
		dst = (double)tr->value();
		return true;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: invalid NUMBER %s in '%s'"),
		ANSI_TO_TCHAR(def->name()->value().data()),
		ANSI_TO_TCHAR(file));

	return false;
}
