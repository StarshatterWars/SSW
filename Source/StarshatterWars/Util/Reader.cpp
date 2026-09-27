/*  Project Starshatter Wars
    Fractal Dev Games
    Copyright (C) 2024. All Rights Reserved.

    SUBSYSTEM:    Foundation
    FILE:         Reader.cpp
    AUTHOR:       Carlos Bott
*/



#include "Reader.h"

#include "reader.h"
#include <stdio.h>
#include <fstream>
#include <ctype.h>
#include "CoreMinimal.h"

FILE* ErrLog = 0;
int      ErrLine = 0;
char     ErrBuf[1024];

void Print(const TCHAR* Fmt, ...)
{
    TCHAR Buffer[2048];

    va_list Args;
    va_start(Args, Fmt);

    FCString::GetVarArgs(
        Buffer,
        UE_ARRAY_COUNT(Buffer),
        Fmt,
        Args
    );

    va_end(Args);

    UE_LOG(LogTemp, Log, TEXT("%s"), Buffer);
}

bool ProfileGameLoop(void)
{
    return false;
}

// +-------------------------------------------------------------------+

Text
ConsoleReader::more()
{
    // loop until the user types something
    do {
        printPrimaryPrompt();
        fillInputBuffer();
    } while (!*p);

    return Text(p);
}

void
ConsoleReader::printPrimaryPrompt()
{
    printf("- ");
}

void
ConsoleReader::fillInputBuffer()
{
    fgets(buffer, 980, stdin);
    p = buffer;
    while (isspace(*p)) p++;
}

// +-------------------------------------------------------------------+

FileReader::FileReader(const char* fname)
    : filename(fname), done(0)
{ }

Text
FileReader::more()
{
    if (done) return Text();

    std::fstream fin(filename, std::fstream::in);

    if (!fin) {
        UE_LOG(LogTemp, Error,
            TEXT("ERROR(Parse): Could not open file '%s'"),
            ANSI_TO_TCHAR(filename.data()));

        return Text();
    }

    Text result;
    char buf[1000], newline;

    while (fin.get(buf, 1000)) {
        result.append(buf);
        fin.get(newline);
        result.append(newline);
    }

    done = 1;
    return result;
}

// +-------------------------------------------------------------------+

BlockReader::BlockReader(const char* block)
    : data((char*)block), done(0), length(0)
{ }

BlockReader::BlockReader(const char* block, int len)
    : data((char*)block), done(0), length(len)
{ }

Text
BlockReader::more()
{
    if (done) return Text();

    if (length) {
        Text result(data, length);
        done = 1;
        return result;
    }
    else if (data) {
        Text result(data);
        done = 1;
        return result;
    }

    done = 1;
    return Text();
}



