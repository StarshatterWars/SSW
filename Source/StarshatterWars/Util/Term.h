/* Project Starshatter Wars
   Fractal Dev Games
   Copyright(C) 2024. All Rights Reserved.

   SUBSYSTEM:    Foundation
   FILE :        Term.h
   AUTHOR : Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#include "Text.h"
#include "List.h"

// +-------------------------------------------------------------------+

class Term;
class TermBool;
class TermNumber;
class TermText;
class TermArray;
class TermDef;
class TermStruct;

// +-------------------------------------------------------------------+

class STARSHATTERWARS_API Term
{
public:
	static const char* TYPENAME() { return "Term"; }

	Term() {}
	virtual ~Term() {}

	virtual int operator==(const Term& rhs) const { return 0; }

	virtual void print(int level = 10) {}

	// conversion tests (non-const legacy API):
	virtual Term* touch() { return this; }
	virtual TermBool* isBool() { return 0; }
	virtual TermNumber* isNumber() { return 0; }
	virtual TermText* isText() { return 0; }
	virtual TermArray* isArray() { return 0; }
	virtual TermDef* isDef() { return 0; }
	virtual TermStruct* isStruct() { return 0; }

	// conversion tests (const-correct overloads for UE / modern C++):
	virtual const Term* touch() const { return this; }
	virtual const TermBool* isBool() const { return 0; }
	virtual const TermNumber* isNumber() const { return 0; }
	virtual const TermText* isText() const { return 0; }
	virtual const TermArray* isArray() const { return 0; }
	virtual const TermDef* isDef() const { return 0; }
	virtual const TermStruct* isStruct() const { return 0; }
};

Term* error(char*, char* = 0);

// +-------------------------------------------------------------------+

typedef List<Term>      TermList;
typedef ListIter<Term>  TermListIter;

// +-------------------------------------------------------------------+

class TermBool : public Term
{
public:
	static const char* TYPENAME() { return "TermBool"; }

	TermBool(bool v) : val(v) {}

	virtual void      print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermBool* isBool() override { return this; }
	virtual const TermBool* isBool() const override { return this; }

	bool value() const { return val; }

private:
	bool val;
};

// +-------------------------------------------------------------------+

class TermNumber : public Term
{
public:
	static const char* TYPENAME() { return "TermNumber"; }

	TermNumber(double v) : val(v) {}

	virtual void        print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermNumber* isNumber() override { return this; }
	virtual const TermNumber* isNumber() const override { return this; }

	double value() const { return val; }

private:
	double val;
};

// +-------------------------------------------------------------------+

class TermText : public Term
{
public:
	static const char* TYPENAME() { return "TermText"; }

	TermText(const Text& v) : val(v) {}

	virtual void      print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermText* isText() override { return this; }
	virtual const TermText* isText() const override { return this; }

	Text value() const { return val; }

private:
	Text val;
};

// +-------------------------------------------------------------------+

class TermArray : public Term
{
public:
	static const char* TYPENAME() { return "TermArray"; }

	TermArray(TermList* elist);
	virtual ~TermArray();

	virtual void print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermArray* isArray() override { return this; }
	virtual const TermArray* isArray() const override { return this; }

	TermList* elements() { return elems; }
	const TermList* elements() const { return elems; }

private:
	TermList* elems;
};

// +-------------------------------------------------------------------+

class TermStruct : public Term
{
public:
	static const char* TYPENAME() { return "TermStruct"; }

	TermStruct(TermList* elist);
	virtual ~TermStruct();

	virtual void print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermStruct* isStruct() override { return this; }
	virtual const TermStruct* isStruct() const override { return this; }

	TermList* elements() { return elems; }
	const TermList* elements() const { return elems; }

private:
	TermList* elems;
};

// +-------------------------------------------------------------------+

class TermDef : public Term
{
public:
	static const char* TYPENAME() { return "TermDef"; }

	TermDef(TermText* n, Term* v) : mname(n), mval(v) {}
	virtual ~TermDef();

	virtual void print(int level = 10) override;

	// Non-const + const overloads:
	virtual TermDef* isDef() override { return this; }
	virtual const TermDef* isDef() const override { return this; }

	virtual TermText* name() { return mname; }
	virtual const TermText* name() const { return mname; }

	virtual Term* term() { return mval; }
	virtual const Term* term() const { return mval; }

private:
	TermText* mname;
	Term* mval;
};
