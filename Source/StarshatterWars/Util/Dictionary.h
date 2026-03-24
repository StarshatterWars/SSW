/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         Dictionary.h
	AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#include "Text.h"

/**
 * 
 */

// +-------------------------------------------------------------------+

template <class T> class Dictionary;
template <class T> class DictionaryIter;
template <class T> class DictionaryCell;

// +-------------------------------------------------------------------+


// +------------------------------------------------------------------ - +

template <class T> class Dictionary
{
public:
	Dictionary();
	~Dictionary();

	T& operator[](const Text& key);

	void Insert(const Text& key, const T& val);
	void Remove(const Text& key);

	void Clear();

	int  Size() const { return items; }
	int  IsEmpty() const { return !items; }

	int  Contains(const Text& key)         const;
	T    Find(const Text& key, T defval)   const;

private:
	void           Init();

	int            items;

	typedef DictionaryCell<T>* PTR;
	PTR            table[256];

	friend class DictionaryIter<T>;
};

// +-------------------------------------------------------------------+

template <class T> class DictionaryIter
{
public:
	DictionaryIter(Dictionary<T>& l);
	~DictionaryIter();

	int operator++();       // prefix

	void     reset();
	void     forth();

	Text     key()    const;
	T        value()  const;

	void           attach(Dictionary<T>& l);
	Dictionary<T>& container();

private:
	Dictionary<T>* dict;
	DictionaryCell<T>* here;
	int                  chain;
};

// +-------------------------------------------------------------------+

template <class T>
class DictionaryCell
{
public:
	DictionaryCell(const Text& InKey)
		: key(InKey), value(), next(nullptr)
	{
	}

	DictionaryCell(const Text& InKey, const T& InValue)
		: key(InKey), value(InValue), next(nullptr)
	{
	}

	~DictionaryCell() = default;

	Text key;
	T value;
	DictionaryCell<T>* next = nullptr;
};

#include "Dictionary.inl"

// +-------------------------------------------------------------------+
