// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "text.h"

/**
 * 
 */
class STARSHATTERWARS_API Reader
{
public:
	Reader() { }
	virtual ~Reader() { }

	virtual Text more() = 0;
};

class ConsoleReader : public Reader
{
public:
	virtual Text more();

	void printPrimaryPrompt();
	void fillInputBuffer();

private:
	char  buffer[1000];
	char* p;
};

class FileReader : public Reader
{
public:
	FileReader(const char* fname);
	virtual Text more();

private:
	Text filename;
	int  done;
};

class BlockReader : public Reader
{
public:
	BlockReader(const char* block);
	BlockReader(const char* block, int len);
	virtual Text more();

private:
	char* data;
	int   done;
	int   length;
};
