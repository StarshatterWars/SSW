/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         Text.h
	AUTHOR:       Carlos Bott
*/

#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "CoreMinimal.h"
#include <string.h>

#include "ThreadSync.h"

/**
 * 
 */

class STARSHATTERWARS_API TextRep
{
	friend class Text;

public:
	TextRep();
	~TextRep();

private:
	TextRep(const char* s);
	TextRep(const char* s, int len);
	TextRep(char c, int len);
	TextRep(const TextRep* rep);

	void        addref();
	long        deref();

	void        dohash();

	char* data;
	long        ref;
	int         length;
	unsigned    hash;
	bool        sensitive;

	static ThreadSync sync;
	static TextRep    nullrep;
};

class STARSHATTERWARS_API Text
{
public:
	
	static const char* TYPENAME() { return "Text"; }

	Text(); 
	Text(char c);
	Text(const char* s);
	Text(const char* s, int len);
	Text(char c, int len);
	Text(const Text& s);
	~Text();

	// case sensitivity
	bool  isSensitive()     const;
	void  setSensitive(bool s);

	// comparison
	int compare(const char* s) const;
	int compare(const Text& s) const;

	// assignment
	Text& operator=(const char* s);
	Text& operator=(const Text& s);

	// catenation
	Text& append(char c);
	Text& append(const char* s);
	Text& append(const Text& s);

	Text  operator+(char          c);
	Text  operator+(const char* s);
	Text  operator+(const Text& s);

	Text& operator+=(char         c) { return append(c); }
	Text& operator+=(const char* s) { return append(s); }
	Text& operator+=(const Text& s) { return append(s); }


	// indexing
	char     operator[](int index) const;
	char     operator()(int index) const;
	char& operator[](int index);
	char& operator()(int index);

	Text  operator()(int start, int len) const;

	// access
	int         length() const { return rep->length; }
	unsigned    hash() const { return rep->hash; }

	const char* data() const { return sym; }
	operator const char* () const { return sym; }

	bool        contains(char        c) const;
	bool        contains(const char* s) const;

	bool        containsAnyOf(const char* charSet) const;

	int         indexOf(char         c) const;
	int         indexOf(const char* s) const;

	// mutation
	void        toLower();
	void        toUpper();

	// substring
	Text        substring(int start, int length);
	Text        trim();
	Text        replace(const char* pattern, const char* substitution);

private:
	void        clone();

	const char* sym;
	TextRep* rep;
};

// +-------------------------------------------------------------------+



// +-------------------------------------------------------------------+

inline int Text::compare(const char* s) const
{
	if (rep->sensitive)
		return strcmp(sym, s);
	else
		return _stricmp(sym, s);
}

inline int Text::compare(const Text& s) const
{
	if (rep->sensitive && s.rep->sensitive)
		return strcmp(sym, s.sym);
	else
		return _stricmp(sym, s.sym);
}


// +-------------------------------------------------------------------+

inline int operator==(const Text& l, const Text& r) {
	return (l.length() == r.length()) && (l.compare(r) == 0);
}
inline int operator!=(const Text& l, const Text& r) { return l.compare(r) != 0; }
inline int operator< (const Text& l, const Text& r) { return l.compare(r) < 0; }
inline int operator<=(const Text& l, const Text& r) { return l.compare(r) <= 0; }
inline int operator> (const Text& l, const Text& r) { return l.compare(r) > 0; }
inline int operator>=(const Text& l, const Text& r) { return l.compare(r) >= 0; }

inline int operator==(const char* l, const Text& r) { return r.compare(l) == 0; }
inline int operator!=(const char* l, const Text& r) { return r.compare(l) != 0; }
inline int operator< (const char* l, const Text& r) { return r.compare(l) < 0; }
inline int operator<=(const char* l, const Text& r) { return r.compare(l) <= 0; }
inline int operator> (const char* l, const Text& r) { return r.compare(l) > 0; }
inline int operator>=(const char* l, const Text& r) { return r.compare(l) >= 0; }

inline int operator==(char* l, const Text& r) { return r.compare(l) == 0; }
inline int operator!=(char* l, const Text& r) { return r.compare(l) != 0; }
inline int operator< (char* l, const Text& r) { return r.compare(l) < 0; }
inline int operator<=(char* l, const Text& r) { return r.compare(l) <= 0; }
inline int operator> (char* l, const Text& r) { return r.compare(l) > 0; }
inline int operator>=(char* l, const Text& r) { return r.compare(l) >= 0; }

inline int operator==(const Text& l, const char* r) { return l.compare(r) == 0; }
inline int operator!=(const Text& l, const char* r) { return l.compare(r) != 0; }
inline int operator< (const Text& l, const char* r) { return l.compare(r) < 0; }
inline int operator<=(const Text& l, const char* r) { return l.compare(r) <= 0; }
inline int operator> (const Text& l, const char* r) { return l.compare(r) > 0; }
inline int operator>=(const Text& l, const char* r) { return l.compare(r) >= 0; }

inline int operator==(const Text& l, char* r) { return l.compare(r) == 0; }
inline int operator!=(const Text& l, char* r) { return l.compare(r) != 0; }
inline int operator< (const Text& l, char* r) { return l.compare(r) < 0; }
inline int operator<=(const Text& l, char* r) { return l.compare(r) <= 0; }
inline int operator> (const Text& l, char* r) { return l.compare(r) > 0; }
inline int operator>=(const Text& l, char* r) { return l.compare(r) >= 0; }

inline Text operator+(const char* l, const Text& r) { return Text(l) + r; }
inline Text operator+(char* l, const Text& r) { return Text(l) + r; }

// +-------------------------------------------------------------------+	


