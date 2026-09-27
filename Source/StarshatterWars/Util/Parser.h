/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         Parser.h
	AUTHOR:       Carlos Bott
*/

#pragma once
#include "CoreMinimal.h"

#include "text.h"
#include "term.h"

// +-------------------------------------------------------------------+

class Reader;
class Scanner;

/**
 * 
 */
class STARSHATTERWARS_API Parser
{
public:

	Parser(Reader* r = 0);
	~Parser();

	Term* ParseTerm();
	Term* ParseTermBase();
	Term* ParseTermRest(Term* base);
	TermList* ParseTermList(int for_struct);
	TermArray* ParseArray();
	TermStruct* ParseStruct();

private:
	Reader* reader;
	Scanner* lexer;
};






