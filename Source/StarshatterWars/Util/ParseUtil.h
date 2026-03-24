/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         ParseUtil.h
	AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#include <vector>

#include "Geometry.h"
#include "Color.h"

#include "Text.h"
#include "Parser.h"
#include "Reader.h"
#include "Token.h"

#include "Math/Vector.h"
#include "Math/Color.h"
/**
 * 
 */

// +--------------------------------------------------------------------+

bool GetDefBool(bool& dst, TermDef* def, const char* file);
bool GetDefText(Text& dst, TermDef* def, const char* file);
bool GetDefText(char* dst, TermDef* def, const char* file);
bool GetDefNumber(int& dst, TermDef* def, const char* file);
bool GetDefNumber(DWORD& dst, TermDef* def, const char* file);
bool GetDefNumber(float& dst, TermDef* def, const char* file);
bool GetDefNumber(double& dst, TermDef* def, const char* file);
bool GetDefVec(Vec3& dst, TermDef* def, const char* file);

bool GetDefColor(Color& dst, TermDef* def, const char* file);
bool GetDefColor(ColorValue& dst, TermDef* def, const char* file);
bool GetDefFColor(FColor& dst, TermDef* def, const char* file);

bool GetDefRect(Rect& dst, TermDef* def, const char* file);
bool GetDefInsets(Insets& dst, TermDef* def, const char* file);
bool GetDefTime(int& dst, TermDef* def, const char* file);

bool GetDefArray(int* dst, int size, TermDef* def, const char* file);
bool GetDefArray(float* dst, int size, TermDef* def, const char* file);
bool GetDefArray(double* dst, int size, TermDef* def, const char* file);
bool GetDefArray(std::vector<DWORD>& array, TermDef* def, const char* file);
bool GetDefArray(std::vector<float>& array, TermDef* def, const char* file);

// +--------------------------------------------------------------------+



