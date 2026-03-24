/* Project Starshatter Wars
   Fractal Dev Games
   Copyright(C) 2024. All Rights Reserved.

   SUBSYSTEM:    Foundation
   FILE :        Term.cpp
   AUTHOR :      Carlos Bott
*/

#include "Term.h"

void Print(const TCHAR* Fmt, ...);

// +-------------------------------------------------------------------+

Term*
error(char* s1, char* s2)
{
	FString Msg(TEXT("ERROR: "));

	if (s1)
	{
		Msg += ANSI_TO_TCHAR(s1);
	}

	if (s2)
	{
		Msg += ANSI_TO_TCHAR(s2);
	}

	UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
	return 0;
}

// +-------------------------------------------------------------------+

void TermBool::print(int level)
{
	if (level > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), val ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("..."));
	}
}

void TermNumber::print(int level)
{
	if (level > 0)
	{
		// %g expects a floating type; keep it explicit
		UE_LOG(LogTemp, Log, TEXT("%g"), static_cast<double>(val));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("..."));
	}
}

void TermText::print(int level)
{
	if (level > 0)
	{
		// val.data() is (likely) const char*. Convert to TCHAR*.
		UE_LOG(LogTemp, Log, TEXT("\"%s\""), ANSI_TO_TCHAR(val.data()));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("..."));
	}
}

// +-------------------------------------------------------------------+

TermArray::TermArray(TermList* elist)
	: elems(elist)
{
}

TermArray::~TermArray()
{
	if (elems) elems->destroy();
	delete elems;
	elems = nullptr;
}

void
TermArray::print(int level)
{
	if (level > 1) {
		Print(TEXT("("));

		if (elems) {
			const int32 Count = elems->size();
			for (int32 Index = 0; Index < Count; Index++) {
				Term* Elem = elems->at(Index);
				if (Elem) {
					Elem->print(level - 1);
				}
				else {
					Print(TEXT("null"));
				}

				if (Index < Count - 1) {
					Print(TEXT(", "));
				}
			}
		}

		Print(TEXT(") "));
	}
	else {
		Print(TEXT("(...) "));
	}
}

// +-------------------------------------------------------------------+

TermStruct::TermStruct(TermList* elist)
	: elems(elist)
{
}

TermStruct::~TermStruct()
{
	if (elems) elems->destroy();
	delete elems;
	elems = nullptr;
}

void
TermStruct::print(int level)
{
	if (level > 1) {
		Print(TEXT("{"));

		if (elems) {
			const int32 Count = elems->size();
			for (int32 Index = 0; Index < Count; Index++) {
				Term* Elem = elems->at(Index);
				if (Elem) {
					Elem->print(level - 1);
				}
				else {
					Print(TEXT("null"));
				}

				if (Index < Count - 1) {
					Print(TEXT(", "));
				}
			}
		}

		Print(TEXT("} "));
	}
	else {
		Print(TEXT("{...} "));
	}
}

// +-------------------------------------------------------------------+

TermDef::~TermDef()
{
	delete mname;
	mname = nullptr;

	delete mval;
	mval = nullptr;
}

void
TermDef::print(int level)
{
	if (level >= 0) {
		if (mname) mname->print(level);
		else       Print(TEXT("null"));

		Print(TEXT(": "));

		if (mval)  mval->print(level - 1);
		else       Print(TEXT("null"));
	}
	else {
		Print(TEXT("..."));
	}
}

// +-------------------------------------------------------------------+
