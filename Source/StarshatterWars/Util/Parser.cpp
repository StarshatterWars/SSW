/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation	
	FILE:         Parser.cpp	
	AUTHOR:       Carlos Bott
*/


#include "Parser.h"
#include "reader.h"
#include "token.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>


enum KEYS { KEY_TRUE, KEY_FALSE, KEY_DEF, KEY_MINUS };

void Print(const TCHAR* Fmt, ...);

static int dump_tokens = 0;

// +-------------------------------------------------------------------+

Term* error(char* msg, const Token& token)
{
    static char buf[1024];
    sprintf_s(buf, " near '%s' in line %d.", (const char*)token.symbol(), token.line());

    return error(msg, buf);
}

// +-------------------------------------------------------------------+

Parser::Parser(Reader* r)
{
    reader = r ? r : new ConsoleReader;
    lexer = new Scanner(reader);

    Token::addKey("true", KEY_TRUE);
    Token::addKey("false", KEY_FALSE);
    Token::addKey(":", KEY_DEF);
    Token::addKey("-", KEY_MINUS);
}

Parser::~Parser()
{
    delete lexer;
    delete reader;
    //Token::close();
}

Term*
Parser::ParseTerm()
{
    Term* t = ParseTermBase();
    if (t == 0) return t;

    Term* t2 = ParseTermRest(t);

    return t2;
}

Term*
Parser::ParseTermRest(Term* base)
{
    Token    t = lexer->Get();

    switch (t.type()) {
    default:
        lexer->PutBack();
        return base;

    case Token::StringLiteral: {
        // concatenate adjacent string literal tokens:
        TermText* text = base->isText();
        if (text) {
            TermText* base2 = new TermText(text->value() + t.symbol()(1, t.symbol().length() - 2));
            delete base;
            return ParseTermRest(base2);
        }
        else {
            lexer->PutBack();
        }
    }
    break;

    case Token::Keyword:
        switch (t.key()) {
        case KEY_DEF:
            if (base->isText())
                return new TermDef(base->isText(), ParseTerm());
            else {
                UE_LOG(LogTemp, Log, TEXT("(Parse) illegal lhs in def"));
            }
 
        default:
            lexer->PutBack();
            return base;
        }
        break;
    }

    return base;
}

static int xtol(const char* p)
{
    int n = 0;

    while (*p) {
        char digit = *p++;
        n *= 16;

        if (digit >= '0' && digit <= '9')
            n += digit - '0';

        else if (digit >= 'a' && digit <= 'f')
            n += digit - 'a' + 10;

        else if (digit >= 'A' && digit <= 'F')
            n += digit - 'A' + 10;
    }

    return n;
}

Term*
Parser::ParseTermBase()
{
    Token    t = lexer->Get();
    int      n = 0;
    double   d = 0.0;

    switch (t.type()) {
    case Token::IntLiteral: {
        if (dump_tokens)
            Print(TEXT("%s"), t.symbol().data());

        char nstr[256], * p = nstr;
        for (int i = 0; i < (int)t.symbol().length(); i++)
            if (t.symbol()[i] != '_')
                *p++ = t.symbol()[i];
        *p++ = '\0';

        // handle hex notation:
        if (nstr[1] == 'x')
            n = xtol(nstr + 2);

        else
            n = atol(nstr);

        return new TermNumber(n);
    }

    case Token::FloatLiteral: {
        if (dump_tokens)
            Print(TEXT("%s"), t.symbol().data());

        char nstr[256], * p = nstr;
        for (int i = 0; i < (int)t.symbol().length(); i++)
            if (t.symbol()[i] != '_')
                *p++ = t.symbol()[i];
        *p++ = '\0';

        d = atof(nstr);
        return new TermNumber(d);
    }

    case Token::StringLiteral:
        if (dump_tokens)
            Print(TEXT("%s"), t.symbol().data());

        return new TermText(t.symbol()(1, t.symbol().length() - 2));

    case Token::AlphaIdent:
        if (dump_tokens)
            Print(TEXT("%s"), t.symbol().data());

        return new TermText(t.symbol());

    case Token::Keyword:
        if (dump_tokens)
            Print(TEXT("%s"), t.symbol().data());

        switch (t.key()) {
        case KEY_FALSE:   return new TermBool(0);
        case KEY_TRUE:    return new TermBool(1);

        case KEY_MINUS: {
            Token next = lexer->Get();
            if (next.type() == Token::IntLiteral) {
                if (dump_tokens)
                    Print(TEXT("%s"), next.symbol().data());

                char nstr[256], * p = nstr;
                for (int i = 0; i < (int)next.symbol().length(); i++)
                    if (next.symbol()[i] != '_')
                        *p++ = next.symbol()[i];
                *p++ = '\0';

                n = -1 * atol(nstr);
                return new TermNumber(n);
            }
            else if (next.type() == Token::FloatLiteral) {
                if (dump_tokens)
                    Print(TEXT("%s"), next.symbol().data());

                char nstr[256], * p = nstr;
                for (int i = 0; i < (int)next.symbol().length(); i++)
                    if (next.symbol()[i] != '_')
                        *p++ = next.symbol()[i];
                *p++ = '\0';

                d = -1.0 * atof(nstr);
                return new TermNumber(d);
            }
            else {
                lexer->PutBack();
                UE_LOG(LogTemp, Log, TEXT("(Parse) illegal token '-': number expected"));
                return 0;
            }
        }
        break;

        default:
            lexer->PutBack();
            return 0;
        }

    case Token::LParen:  return ParseArray();

    case Token::LBrace:  return ParseStruct();

    case Token::CharLiteral:
        UE_LOG(LogTemp, Log, TEXT("(Parse) illegal token"));

    default:
        lexer->PutBack();
        return 0;
    }
}

TermArray*
Parser::ParseArray()
{
    TermList* elems = ParseTermList(0);
    Token       end = lexer->Get();

    if (end.type() != Token::RParen) {
        UE_LOG(LogTemp, Log, TEXT("(Parse) illegal token"));
        return 0;
    }

    return new TermArray(elems);
}

TermStruct*
Parser::ParseStruct()
{
    TermList* elems = ParseTermList(1);
    Token       end = lexer->Get();

    if (end.type() != Token::RBrace) {
        UE_LOG(LogTemp, Log, TEXT("(Parse) '}' missing in struct"));
        return 0;
    }
     

    return new TermStruct(elems);
}

TermList*
Parser::ParseTermList(int for_struct)
{
    TermList* tlist = new TermList;

    Term* term = ParseTerm();
    while (term) {
        if (for_struct && !term->isDef()) {
            UE_LOG(LogTemp, Log, TEXT("(Parse) non-definition term in struct"));
            return 0;
        }
        else if (!for_struct && term->isDef()) {
            UE_LOG(LogTemp, Log, TEXT("(Parse) illegal definition in array"));
            return 0;
        }

        tlist->append(term);
        Token t = lexer->Get();

        /*** OLD WAY: COMMA SEPARATORS REQUIRED ***
        if (t.type() != Token::Comma) {
           lexer->PutBack();
           term = 0;
        }
        else
           term = ParseTerm();
        /*******************************************/

        // NEW WAY: COMMA SEPARATORS OPTIONAL:
        if (t.type() != Token::Comma) {
            lexer->PutBack();
        }

        term = ParseTerm();
    }

    return tlist;
}




