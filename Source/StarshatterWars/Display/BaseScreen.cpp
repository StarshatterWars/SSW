/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         BaseScreen.cpp
    AUTHOR:       Carlos Bott
*/

#include "BaseScreen.h"

// Core:
#include "Misc/Char.h"
#include "Logging/LogMacros.h"

// Input:
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// UMG:
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
#include "Components/Slider.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ContentWidget.h"

#include "Blueprint/WidgetTree.h"

// Slate styling:
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "Engine/Font.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SlateSound.h"

#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaseScreen, Log, All);


namespace
{
    enum class ETok : uint8
    {
        End,
        Ident,
        String,
        Number,
        LBrace,
        RBrace,
        LParen,
        RParen,
        Colon,
        Comma,
        Hash
    };

    struct FTok
    {
        ETok Type = ETok::End;
        FString Text;
        double Number = 0.0;
    };

    static bool IsIdentStart(TCHAR C)
    {
        return FChar::IsAlpha(C) || C == '_' || C == '$';
    }

    static bool IsIdentChar(TCHAR C)
    {
        return FChar::IsAlnum(C) || C == '_' || C == '.' || C == '-' || C == '$';
    }

    static void StripLineComments(FString& S)
    {
        TArray<FString> Lines;
        S.ParseIntoArrayLines(Lines, false);

        for (FString& L : Lines)
        {
            for (int32 i = 0; i < L.Len() - 1; ++i)
            {
                if (L[i] == TEXT('/') && L[i + 1] == TEXT('/'))
                {
                    L = L.Left(i);
                    break;
                }
            }
        }

        S = FString::Join(Lines, TEXT("\n"));
    }

    static void StripBlockComments(FString& S)
    {
        while (true)
        {
            const int32 Start = S.Find(TEXT("/*"));
            if (Start == INDEX_NONE)
                break;

            const int32 End = S.Find(TEXT("*/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Start + 2);
            if (End == INDEX_NONE)
            {
                S = S.Left(Start);
                break;
            }

            const int32 RemoveLen = (End + 2) - Start;
            S.RemoveAt(Start, RemoveLen, false);
        }
    }

    class FFormLexer
    {
    public:
        explicit FFormLexer(const FString& In)
            : Src(In), Len(In.Len())
        {
        }

        FTok Next()
        {
            SkipWs();

            if (Pos >= Len)
                return { ETok::End };

            const TCHAR C = Src[Pos];

            if (C == '{') { ++Pos; return { ETok::LBrace, TEXT("{") }; }
            if (C == '}') { ++Pos; return { ETok::RBrace, TEXT("}") }; }
            if (C == '(') { ++Pos; return { ETok::LParen, TEXT("(") }; }
            if (C == ')') { ++Pos; return { ETok::RParen, TEXT(")") }; }
            if (C == ':') { ++Pos; return { ETok::Colon, TEXT(":") }; }
            if (C == ',') { ++Pos; return { ETok::Comma, TEXT(",") }; }
            if (C == '#') { ++Pos; return { ETok::Hash, TEXT("#") }; }

            // "string"
            if (C == '"')
            {
                ++Pos;
                FString Out;
                while (Pos < Len)
                {
                    const TCHAR D = Src[Pos++];
                    if (D == '"') break;
                    Out.AppendChar(D);
                }
                FTok T;
                T.Type = ETok::String;
                T.Text = Out;
                return T;
            }

            // 0x... as ident
            if (C == '0' && (Pos + 1) < Len && (Src[Pos + 1] == 'x' || Src[Pos + 1] == 'X'))
            {
                const int32 Start = Pos;
                Pos += 2;
                while (Pos < Len && FChar::IsHexDigit(Src[Pos]))
                    ++Pos;

                FTok T;
                T.Type = ETok::Ident;
                T.Text = Src.Mid(Start, Pos - Start);
                return T;
            }

            // number
            if (FChar::IsDigit(C) || C == '-' || C == '+')
            {
                const int32 Start = Pos;
                ++Pos;
                while (Pos < Len)
                {
                    const TCHAR D = Src[Pos];
                    if (!(FChar::IsDigit(D) || D == '.' || D == 'e' || D == 'E' || D == '-' || D == '+'))
                        break;
                    ++Pos;
                }

                const FString NumStr = Src.Mid(Start, Pos - Start);

                FTok T;
                T.Type = ETok::Number;
                T.Text = NumStr;
                T.Number = FCString::Atod(*NumStr);
                return T;
            }

            // identifier
            if (IsIdentStart(C))
            {
                const int32 Start = Pos;
                ++Pos;
                while (Pos < Len && IsIdentChar(Src[Pos]))
                    ++Pos;

                FTok T;
                T.Type = ETok::Ident;
                T.Text = Src.Mid(Start, Pos - Start);
                return T;
            }

            ++Pos;
            return Next();
        }

    private:
        void SkipWs()
        {
            while (Pos < Len)
            {
                const TCHAR C = Src[Pos];
                if (C == ' ' || C == '\t' || C == '\r' || C == '\n')
                {
                    ++Pos;
                    continue;
                }
                break;
            }
        }

        const FString& Src;
        int32 Len = 0;
        int32 Pos = 0;
    };

    class FFormParser
    {
    public:
        explicit FFormParser(const FString& InText)
            : Lex(InText)
        {
            Tok = Lex.Next();
        }

        bool ParseForm(FParsedForm& Out, FString& OutError)
        {
            if (!MatchIdent(TEXT("form")))
            {
                OutError = TEXT("Expected 'form'");
                return false;
            }
            if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after 'form'"))) return false;
            if (!Consume(ETok::LBrace, OutError, TEXT("Expected '{' after 'form:'"))) return false;

            FParsedCtrl CurrentDefaults;
            CurrentDefaults.Type = EFormCtrlType::None;

            while (!Is(ETok::RBrace) && !Is(ETok::End))
            {
                if (IsIdent(TEXT("back_color")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after back_color"))) return false;
                    Out.BackColor = ParseColor(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("fore_color")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after fore_color"))) return false;
                    Out.ForeColor = ParseColor(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                // form-level font
                if (IsIdent(TEXT("font")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after font"))) return false;
                    Out.Font = ParseStringOrIdent(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("texture")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after texture"))) return false;
                    Out.Texture = ParseStringOrIdent(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("margins")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after margins"))) return false;
                    Out.Margins = ParseRect(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("layout")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after layout"))) return false;
                    if (!ParseLayoutBlock(Out.Layout, OutError)) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("defctrl")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after defctrl"))) return false;
                    if (!Consume(ETok::LBrace, OutError, TEXT("Expected '{' after defctrl:"))) return false;
                    if (!ParseCtrlBody(CurrentDefaults, true, OutError)) return false;
                    if (!Consume(ETok::RBrace, OutError, TEXT("Expected '}' after defctrl body"))) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("ctrl")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after ctrl"))) return false;
                    if (!Consume(ETok::LBrace, OutError, TEXT("Expected '{' after ctrl:"))) return false;

                    FParsedCtrl Ctrl;
                    if (!ParseCtrlBody(Ctrl, false, OutError)) return false;
                    if (!Consume(ETok::RBrace, OutError, TEXT("Expected '}' after ctrl body"))) return false;

                    ApplyDefaults(Ctrl, CurrentDefaults);
                    Out.Controls.Add(Ctrl);

                    EatOptionalComma();
                    continue;
                }

                Tok = Lex.Next();
            }

            if (!Consume(ETok::RBrace, OutError, TEXT("Expected closing '}' for form"))) return false;
            return true;
        }

    private:
        bool Is(ETok T) const { return Tok.Type == T; }

        bool IsIdent(const TCHAR* S) const
        {
            return Tok.Type == ETok::Ident && Tok.Text.Equals(S, ESearchCase::IgnoreCase);
        }

        bool MatchIdent(const TCHAR* S)
        {
            if (!IsIdent(S))
                return false;
            Tok = Lex.Next();
            return true;
        }

        void ConsumeIdent()
        {
            Tok = Lex.Next();
        }

        bool Consume(ETok T, FString& OutError, const TCHAR* Msg)
        {
            if (Tok.Type != T)
            {
                OutError = Msg;
                return false;
            }
            Tok = Lex.Next();
            return true;
        }

        void EatOptionalComma()
        {
            if (Tok.Type == ETok::Comma)
                Tok = Lex.Next();
        }

        static bool IsHexLiteral(const FString& S)
        {
            return S.Len() > 2 && S[0] == TCHAR('0') && (S[1] == TCHAR('x') || S[1] == TCHAR('X'));
        }

        int32 ParseIntFlexible(FString& OutError)
        {
            if (Tok.Type == ETok::Number)
            {
                const int32 V = (int32)Tok.Number;
                Tok = Lex.Next();
                return V;
            }

            if (Tok.Type == ETok::Ident && IsHexLiteral(Tok.Text))
            {
                const FString HexDigits = Tok.Text.Mid(2);
                const int32 V = FCString::Strtoi(*HexDigits, nullptr, 16);
                Tok = Lex.Next();
                return V;
            }

            OutError = TEXT("Expected integer");
            return 0;
        }

        FString ParseStringOrIdent(FString& OutError)
        {
            if (Tok.Type == ETok::String || Tok.Type == ETok::Ident)
            {
                const FString S = Tok.Text;
                Tok = Lex.Next();
                return S;
            }
            OutError = TEXT("Expected string or identifier");
            return FString();
        }

        bool ParseBool(FString& OutError)
        {
            if (Tok.Type == ETok::Ident)
            {
                const bool bTrue =
                    Tok.Text.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
                    Tok.Text.Equals(TEXT("1"), ESearchCase::IgnoreCase);

                const bool bFalse =
                    Tok.Text.Equals(TEXT("false"), ESearchCase::IgnoreCase) ||
                    Tok.Text.Equals(TEXT("0"), ESearchCase::IgnoreCase);

                if (!bTrue && !bFalse)
                {
                    OutError = TEXT("Expected boolean");
                    return false;
                }

                Tok = Lex.Next();
                return bTrue;
            }

            if (Tok.Type == ETok::Number)
            {
                const bool b = Tok.Number != 0.0;
                Tok = Lex.Next();
                return b;
            }

            OutError = TEXT("Expected boolean");
            return false;
        }

        FFormIntRect ParseRect(FString& OutError)
        {
            if (!Consume(ETok::LParen, OutError, TEXT("Expected '(' for rect"))) return {};
            const int32 A = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return {};
            if (!Consume(ETok::Comma, OutError, TEXT("Expected ',' in rect"))) return {};
            const int32 B = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return {};
            if (!Consume(ETok::Comma, OutError, TEXT("Expected ',' in rect"))) return {};
            const int32 C = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return {};
            if (!Consume(ETok::Comma, OutError, TEXT("Expected ',' in rect"))) return {};
            const int32 D = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return {};
            if (!Consume(ETok::RParen, OutError, TEXT("Expected ')' for rect"))) return {};
            return FFormIntRect(A, B, C, D);
        }

        FLinearColor ParseColor(FString& OutError)
        {
            if (!Consume(ETok::LParen, OutError, TEXT("Expected '(' for color"))) return FLinearColor::Transparent;

            const int32 R = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return FLinearColor::Transparent;
            if (!Consume(ETok::Comma, OutError, TEXT("Expected ',' in color"))) return FLinearColor::Transparent;
            const int32 G = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return FLinearColor::Transparent;
            if (!Consume(ETok::Comma, OutError, TEXT("Expected ',' in color"))) return FLinearColor::Transparent;
            const int32 B = ParseIntFlexible(OutError); if (!OutError.IsEmpty()) return FLinearColor::Transparent;

            int32 A = 255;
            if (Tok.Type == ETok::Comma)
            {
                Tok = Lex.Next();
                A = ParseIntFlexible(OutError);
                if (!OutError.IsEmpty()) return FLinearColor::Transparent;
            }

            if (!Consume(ETok::RParen, OutError, TEXT("Expected ')' for color"))) return FLinearColor::Transparent;

            return FLinearColor(
                FMath::Clamp(R / 255.0f, 0.0f, 1.0f),
                FMath::Clamp(G / 255.0f, 0.0f, 1.0f),
                FMath::Clamp(B / 255.0f, 0.0f, 1.0f),
                FMath::Clamp(A / 255.0f, 0.0f, 1.0f)
            );
        }

        EFormAlign ParseAlign(FString& OutError)
        {
            if (Tok.Type != ETok::Ident)
            {
                OutError = TEXT("Expected align identifier");
                return EFormAlign::None;
            }

            const FString S = Tok.Text;
            Tok = Lex.Next();

            if (S.Equals(TEXT("left"), ESearchCase::IgnoreCase))   return EFormAlign::Left;
            if (S.Equals(TEXT("center"), ESearchCase::IgnoreCase)) return EFormAlign::Center;
            if (S.Equals(TEXT("right"), ESearchCase::IgnoreCase))  return EFormAlign::Right;

            OutError = TEXT("Unknown align value");
            return EFormAlign::None;
        }

        EFormCtrlType ParseCtrlType(FString& OutError)
        {
            if (Tok.Type != ETok::Ident)
            {
                OutError = TEXT("Expected ctrl type identifier");
                return EFormCtrlType::None;
            }

            const FString S = Tok.Text;
            Tok = Lex.Next();

            if (S.Equals(TEXT("label"), ESearchCase::IgnoreCase))      return EFormCtrlType::Label;
            if (S.Equals(TEXT("text"), ESearchCase::IgnoreCase))       return EFormCtrlType::Text;
            if (S.Equals(TEXT("button"), ESearchCase::IgnoreCase))     return EFormCtrlType::Button;
            if (S.Equals(TEXT("image"), ESearchCase::IgnoreCase))      return EFormCtrlType::Image;
            if (S.Equals(TEXT("edit"), ESearchCase::IgnoreCase))       return EFormCtrlType::Edit;
            if (S.Equals(TEXT("combo"), ESearchCase::IgnoreCase))      return EFormCtrlType::Combo;
            if (S.Equals(TEXT("list"), ESearchCase::IgnoreCase))       return EFormCtrlType::List;
            if (S.Equals(TEXT("slider"), ESearchCase::IgnoreCase))     return EFormCtrlType::Slider;
            if (S.Equals(TEXT("panel"), ESearchCase::IgnoreCase))      return EFormCtrlType::Panel;
            if (S.Equals(TEXT("background"), ESearchCase::IgnoreCase)) return EFormCtrlType::Background;

            OutError = TEXT("Unknown ctrl type");
            return EFormCtrlType::None;
        }

        TArray<int32> ParseIntArray(FString& OutError)
        {
            TArray<int32> Out;
            if (!Consume(ETok::LParen, OutError, TEXT("Expected '(' for int array"))) return Out;

            while (!Is(ETok::RParen) && !Is(ETok::End))
            {
                Out.Add(ParseIntFlexible(OutError));
                if (!OutError.IsEmpty()) return Out;

                if (Tok.Type == ETok::Comma) { Tok = Lex.Next(); continue; }
                break;
            }

            if (!Consume(ETok::RParen, OutError, TEXT("Expected ')' for int array"))) return Out;
            return Out;
        }

        TArray<float> ParseFloatArray(FString& OutError)
        {
            TArray<float> Out;
            if (!Consume(ETok::LParen, OutError, TEXT("Expected '(' for float array"))) return Out;

            while (!Is(ETok::RParen) && !Is(ETok::End))
            {
                if (Tok.Type != ETok::Number)
                {
                    OutError = TEXT("Expected number in float array");
                    return Out;
                }

                Out.Add((float)Tok.Number);
                Tok = Lex.Next();

                if (Tok.Type == ETok::Comma) { Tok = Lex.Next(); continue; }
                break;
            }

            if (!Consume(ETok::RParen, OutError, TEXT("Expected ')' for float array"))) return Out;
            return Out;
        }

        bool ParseLayoutBlock(FFormLayout& Out, FString& OutError)
        {
            if (!Consume(ETok::LBrace, OutError, TEXT("Expected '{' for layout block"))) return false;

            while (!Is(ETok::RBrace) && !Is(ETok::End))
            {
                if (IsIdent(TEXT("x_mins")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after x_mins"))) return false;
                    Out.XMins = ParseIntArray(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("x_weights")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after x_weights"))) return false;
                    Out.XWeights = ParseFloatArray(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("y_mins")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after y_mins"))) return false;
                    Out.YMins = ParseIntArray(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (IsIdent(TEXT("y_weights")))
                {
                    ConsumeIdent();
                    if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' after y_weights"))) return false;
                    Out.YWeights = ParseFloatArray(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                Tok = Lex.Next();
            }

            if (!Consume(ETok::RBrace, OutError, TEXT("Expected '}' closing layout block"))) return false;
            return true;
        }

        bool ParseCtrlBody(FParsedCtrl& OutCtrl, bool bIsDefctrl, FString& OutError)
        {
            while (!Is(ETok::RBrace) && !Is(ETok::End))
            {
                if (Tok.Type != ETok::Ident)
                {
                    Tok = Lex.Next();
                    continue;
                }

                const FString Key = Tok.Text;
                Tok = Lex.Next();

                if (!Consume(ETok::Colon, OutError, TEXT("Expected ':' in ctrl block"))) return false;

                if (Key.Equals(TEXT("id"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Id = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("pid"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.ParentId = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("type"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Type = ParseCtrlType(OutError);
                    if (!OutError.IsEmpty()) return false;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("text"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Text = ParseStringOrIdent(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasText = !bIsDefctrl;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("texture"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Texture = ParseStringOrIdent(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasTexture = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("font"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Font = ParseStringOrIdent(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasFont = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("align"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Align = ParseAlign(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasAlign = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("back_color"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.BackColor = ParseColor(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasBackColor = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("fore_color"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.ForeColor = ParseColor(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasForeColor = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("active_color"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.ActiveColor = ParseColor(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasActiveColor = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("transparent"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.bTransparent = ParseBool(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasTransparent = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("sticky"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.bSticky = ParseBool(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasSticky = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("border"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.bBorder = ParseBool(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasBorder = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("hide_partial"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.bHidePartial = ParseBool(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasHidePartial = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("show_headings"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.bShowHeadings = ParseBool(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasShowHeadings = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("fixed_width"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.FixedWidth = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasFixedWidth = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("fixed_height"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.FixedHeight = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasFixedHeight = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("cells"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Cells = ParseRect(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasCells = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("cell_insets"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.CellInsets = ParseRect(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasCellInsets = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("margins"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Margins = ParseRect(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasMargins = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("scroll_bar"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.ScrollBar = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasScrollBar = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("style"), ESearchCase::IgnoreCase))
                {
                    OutCtrl.Style = ParseIntFlexible(OutError);
                    if (!OutError.IsEmpty()) return false;
                    OutCtrl.bHasStyle = true;
                    EatOptionalComma();
                    continue;
                }

                if (Key.Equals(TEXT("layout"), ESearchCase::IgnoreCase))
                {
                    if (!ParseLayoutBlock(OutCtrl.Layout, OutError)) return false;
                    OutCtrl.bHasLayout = true;
                    EatOptionalComma();
                    continue;
                }

                Tok = Lex.Next();
            }

            return true;
        }

        static void ApplyDefaults(FParsedCtrl& Ctrl, const FParsedCtrl& Def)
        {
            if (!Ctrl.bHasTexture && Def.bHasTexture)          Ctrl.Texture = Def.Texture;
            if (!Ctrl.bHasFont && Def.bHasFont)               Ctrl.Font = Def.Font;
            if (!Ctrl.bHasAlign && Def.bHasAlign)             Ctrl.Align = Def.Align;
            if (!Ctrl.bHasBackColor && Def.bHasBackColor)     Ctrl.BackColor = Def.BackColor;
            if (!Ctrl.bHasForeColor && Def.bHasForeColor)     Ctrl.ForeColor = Def.ForeColor;

            if (!Ctrl.bHasActiveColor && Def.bHasActiveColor) Ctrl.ActiveColor = Def.ActiveColor;

            if (!Ctrl.bHasTransparent && Def.bHasTransparent) Ctrl.bTransparent = Def.bTransparent;
            if (!Ctrl.bHasSticky && Def.bHasSticky)           Ctrl.bSticky = Def.bSticky;
            if (!Ctrl.bHasBorder && Def.bHasBorder)           Ctrl.bBorder = Def.bBorder;
            if (!Ctrl.bHasHidePartial && Def.bHasHidePartial) Ctrl.bHidePartial = Def.bHidePartial;
            if (!Ctrl.bHasShowHeadings && Def.bHasShowHeadings) Ctrl.bShowHeadings = Def.bShowHeadings;

            if (!Ctrl.bHasFixedWidth && Def.bHasFixedWidth)   Ctrl.FixedWidth = Def.FixedWidth;
            if (!Ctrl.bHasFixedHeight && Def.bHasFixedHeight) Ctrl.FixedHeight = Def.FixedHeight;

            if (!Ctrl.bHasCells && Def.bHasCells)             Ctrl.Cells = Def.Cells;
            if (!Ctrl.bHasCellInsets && Def.bHasCellInsets)   Ctrl.CellInsets = Def.CellInsets;
            if (!Ctrl.bHasMargins && Def.bHasMargins)         Ctrl.Margins = Def.Margins;

            if (!Ctrl.bHasScrollBar && Def.bHasScrollBar)     Ctrl.ScrollBar = Def.ScrollBar;
            if (!Ctrl.bHasStyle && Def.bHasStyle)             Ctrl.Style = Def.Style;

            if (!Ctrl.bHasLayout && Def.bHasLayout)           Ctrl.Layout = Def.Layout;
        }

        FFormLexer Lex;
        FTok Tok;
    };
} // namespace

// --------------------------------------------------------------------
// UBaseScreen lifecycle
// --------------------------------------------------------------------

static FSlateBrush MakeStarshatterBrush(UTexture2D* Tex, const FMargin& Margin, const FVector2D& ImageSize)
{
    FSlateBrush B;

    if (Tex)
    {
        B.SetResourceObject(Tex);
        B.DrawAs = ESlateBrushDrawType::Box;   // key for 9-slice
        B.Margin = Margin;

        if (!ImageSize.IsNearlyZero())
            B.ImageSize = ImageSize;
    }
    else
    {
        // fallback: simple tinted box (won’t crash)
        B.DrawAs = ESlateBrushDrawType::Box;
        B.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
        B.Margin = FMargin(4.f / 16.f);
    }

    return B;
}
// --------------------------------------------------------------------
// Font paths (UMG-friendly *_Font assets)
// --------------------------------------------------------------------
const TCHAR* UBaseScreen::PATH_Font_LimerickBold = TEXT("/Game/Font/Limerick-Serial_Bold_Font.Limerick-Serial_Bold_Font");
const TCHAR* UBaseScreen::PATH_Font_VerdanaItalic = TEXT("/Game/Font/VERDANAI_Font.VERDANAI_Font");
const TCHAR* UBaseScreen::PATH_Font_SerpentBold = TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font");

static FSlateBrush MakeTintBrushBox(const FLinearColor& Tint)
{
    FSlateBrush B;
    B.DrawAs = ESlateBrushDrawType::Box;
    B.TintColor = FSlateColor(Tint);
    B.Margin = FMargin(4.f / 16.f);
    return B;
}

UBaseScreen::UBaseScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UBaseScreen::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    FormMap.Reset();
    BindFormWidgets();

    ApplyFormDefaults();
    EnsureUiFontsLoaded();

    const FString Frm = GetLegacyFormText();
    if (!Frm.IsEmpty())
    {
        FString Clean = Frm;
        StripLineComments(Clean);
        StripBlockComments(Clean);

        FString Err;
        FParsedForm Parsed;
        if (ParseLegacyForm(Clean, Parsed, Err))
        {
            ParsedForm = Parsed;
            ApplyLegacyFormDefaults(ParsedForm);
        }
        else
        {
            UE_LOG(LogBaseScreen, Warning, TEXT("BaseScreen: ParseLegacyForm failed: %s"), *Err);
        }
    }
}

void UBaseScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (UStarshatterUIStyleSubsystem* Style =
        GetGameInstance()->GetSubsystem<UStarshatterUIStyleSubsystem>())
    {
        if (MenuButton)
        {
            Style->ApplyMenuButtonStyle(MenuButton);
        }

        if (ApplyButton)
        {
            Style->ApplyDefaultButtonStyle(ApplyButton, 20);
        }

        if (CancelButton)
        {
            Style->ApplyDefaultButtonStyle(CancelButton, 20);
        }

        if (Title)
        {
            Style->ApplyTitleTextStyle(Title, 28);
        }
    }
}

void UBaseScreen::NativeDestruct()
{
    Super::NativeDestruct();
}

void UBaseScreen::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UBaseScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UBaseScreen::ExecFrame(double DeltaTime) {

}

// --------------------------------------------------------------------
// Centralized dialog input (Enter/Escape)
// --------------------------------------------------------------------

FReply UBaseScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (!bDialogInputEnabled)
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);

    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
    {
        HandleAccept();
        return FReply::Handled();
    }

    if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
    {
        HandleCancel();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBaseScreen::HandleAccept()
{
    if (ApplyButton && ApplyButton->GetIsEnabled())
    {
        ApplyButton->OnClicked.Broadcast();
        return;
    }
}

void UBaseScreen::HandleCancel()
{
    if (CancelButton && CancelButton->GetIsEnabled())
    {
        CancelButton->OnClicked.Broadcast();
        return;
    }
}

void UBaseScreen::Show()
{
    if (bIsShown)
        return;
   
   bIsShown = true;
}

void UBaseScreen::Hide()
{
    if (!bIsShown)
        return;

    HideAll();
    bIsShown = false;
}

void UBaseScreen::HideAll()
{

}

// --------------------------------------------------------------------
// Bind helpers
// --------------------------------------------------------------------

void UBaseScreen::BindLabel(int32 Id, UTextBlock* Widget) { if (Widget) FormMap.Labels.Add(Id, Widget); }
void UBaseScreen::BindText(int32 Id, URichTextBlock* Widget) { if (Widget) FormMap.Texts.Add(Id, Widget); }
void UBaseScreen::BindButton(int32 Id, UButton* Widget) { if (Widget) FormMap.Buttons.Add(Id, Widget); }
void UBaseScreen::BindImage(int32 Id, UImage* Widget) { if (Widget) FormMap.Images.Add(Id, Widget); }
void UBaseScreen::BindEdit(int32 Id, UEditableTextBox* Widget) { if (Widget) FormMap.Edits.Add(Id, Widget); }
void UBaseScreen::BindCombo(int32 Id, UComboBoxString* Widget) { if (Widget) FormMap.Combos.Add(Id, Widget); }
void UBaseScreen::BindList(int32 Id, UListView* Widget) { if (Widget) FormMap.Lists.Add(Id, Widget); }
void UBaseScreen::BindSlider(int32 Id, USlider* Widget) { if (Widget) FormMap.Sliders.Add(Id, Widget); }

// --------------------------------------------------------------------
// Lookup helpers
// --------------------------------------------------------------------

UTextBlock* UBaseScreen::GetLabel(int32 Id) const
{
    if (const TObjectPtr<UTextBlock>* Found = FormMap.Labels.Find(Id))
        return Found->Get();
    return nullptr;
}

URichTextBlock* UBaseScreen::GetText(int32 Id) const
{
    if (const TObjectPtr<URichTextBlock>* Found = FormMap.Texts.Find(Id))
        return Found->Get();
    return nullptr;
}

UButton* UBaseScreen::GetButton(int32 Id) const
{
    if (const TObjectPtr<UButton>* Found = FormMap.Buttons.Find(Id))
        return Found->Get();
    return nullptr;
}

UImage* UBaseScreen::GetImage(int32 Id) const
{
    if (const TObjectPtr<UImage>* Found = FormMap.Images.Find(Id))
        return Found->Get();
    return nullptr;
}

UEditableTextBox* UBaseScreen::GetEdit(int32 Id) const
{
    if (const TObjectPtr<UEditableTextBox>* Found = FormMap.Edits.Find(Id))
        return Found->Get();
    return nullptr;
}

UComboBoxString* UBaseScreen::GetCombo(int32 Id) const
{
    if (const TObjectPtr<UComboBoxString>* Found = FormMap.Combos.Find(Id))
        return Found->Get();
    return nullptr;
}

UListView* UBaseScreen::GetList(int32 Id) const
{
    if (const TObjectPtr<UListView>* Found = FormMap.Lists.Find(Id))
        return Found->Get();
    return nullptr;
}

USlider* UBaseScreen::GetSlider(int32 Id) const
{
    if (const TObjectPtr<USlider>* Found = FormMap.Sliders.Find(Id))
        return Found->Get();
    return nullptr;
}

// --------------------------------------------------------------------
// Operations helpers
// --------------------------------------------------------------------

void UBaseScreen::SetLabelText(int32 Id, const FText& Text)
{
    if (UTextBlock* L = GetLabel(Id))
        L->SetText(Text);
}

void UBaseScreen::SetEditText(int32 Id, const FText& Text)
{
    if (UEditableTextBox* E = GetEdit(Id))
        E->SetText(Text);
}

void UBaseScreen::SetVisible(int32 Id, bool bVisible)
{
    const ESlateVisibility Vis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

    if (UTextBlock* L = GetLabel(Id))          L->SetVisibility(Vis);
    if (URichTextBlock* T = GetText(Id))       T->SetVisibility(Vis);
    if (UButton* B = GetButton(Id))           B->SetVisibility(Vis);
    if (UImage* I = GetImage(Id))             I->SetVisibility(Vis);
    if (UEditableTextBox* E = GetEdit(Id))    E->SetVisibility(Vis);
    if (UComboBoxString* C = GetCombo(Id))    C->SetVisibility(Vis);
    if (UListView* LV = GetList(Id))          LV->SetVisibility(Vis);
    if (USlider* S = GetSlider(Id))           S->SetVisibility(Vis);
}

void UBaseScreen::SetEnabled(int32 Id, bool bEnabled)
{
    if (UButton* B = GetButton(Id))           B->SetIsEnabled(bEnabled);
    if (UEditableTextBox* E = GetEdit(Id))    E->SetIsEnabled(bEnabled);
    if (UComboBoxString* C = GetCombo(Id))    C->SetIsEnabled(bEnabled);
    if (UListView* LV = GetList(Id))          LV->SetIsEnabled(bEnabled);
    if (USlider* S = GetSlider(Id))           S->SetIsEnabled(bEnabled);
}

// --------------------------------------------------------------------
// Optional compiled defaults hook
// --------------------------------------------------------------------

void UBaseScreen::ApplyFormDefaults()
{
    // Intentionally empty.
}

// --------------------------------------------------------------------
// Legacy FORM parse + defaults application
// --------------------------------------------------------------------

bool UBaseScreen::ParseLegacyForm(const FString& InText, FParsedForm& OutForm, FString& OutError) const
{
    FFormParser Parser(InText);
    return Parser.ParseForm(OutForm, OutError);
}

bool UBaseScreen::ResolveTextJustification(EFormAlign InAlign, ETextJustify::Type& OutJustify) const
{
    switch (InAlign)
    {
    case EFormAlign::Left:   OutJustify = ETextJustify::Left;   return true;
    case EFormAlign::Center: OutJustify = ETextJustify::Center; return true;
    case EFormAlign::Right:  OutJustify = ETextJustify::Right;  return true;
    default: break;
    }
    return false;
}

bool UBaseScreen::ResolveFont(const FString& InFontName, FSlateFontInfo& OutFont) const
{
    for (const FFormFontMapEntry& E : FontMappings)
    {
        if (E.LegacyName.Equals(InFontName, ESearchCase::IgnoreCase))
        {
            if (!E.Font)
                break;

            OutFont = FSlateFontInfo(E.Font, (E.bOverrideSize && E.Size > 0) ? E.Size : DefaultFontSize);
            return true;
        }
    }

    if (DefaultFont)
    {
        OutFont = FSlateFontInfo(DefaultFont, DefaultFontSize);
        return true;
    }

    return false;
}

void UBaseScreen::ApplyLegacyFormDefaults(const FParsedForm& Parsed)
{
    FSlateFontInfo FormFont;
    bool bHasFormFont = false;

    if (!Parsed.Font.IsEmpty())
    {
        bHasFormFont = ResolveFont(Parsed.Font, FormFont);
    }

    for (const FParsedCtrl& C : Parsed.Controls)
    {
        // LABEL
        if (C.Type == EFormCtrlType::Label)
        {
            UTextBlock* L = GetLabel(C.Id);
            if (!L) continue;

            if (!C.Text.IsEmpty())
                L->SetText(FText::FromString(C.Text));

            if (C.ForeColor != FLinearColor::Transparent)
                L->SetColorAndOpacity(FSlateColor(C.ForeColor));

            ETextJustify::Type Just;
            if (ResolveTextJustification(C.Align, Just))
                L->SetJustification(Just);

            if (!C.Font.IsEmpty())
            {
                FSlateFontInfo CtrlFont;
                if (ResolveFont(C.Font, CtrlFont))
                    L->SetFont(CtrlFont);
            }
            else if (bHasFormFont)
            {
                L->SetFont(FormFont);
            }

            continue;
        }

        // TEXT (RichText)
        if (C.Type == EFormCtrlType::Text)
        {
            URichTextBlock* T = GetText(C.Id);
            if (!T) continue;

            if (!C.Text.IsEmpty())
                T->SetText(FText::FromString(C.Text));

            if (C.ForeColor != FLinearColor::Transparent)
                T->SetDefaultColorAndOpacity(C.ForeColor);

            ETextJustify::Type Just;
            if (ResolveTextJustification(C.Align, Just))
                T->SetJustification(Just);

            if (!C.Font.IsEmpty())
            {
                FSlateFontInfo CtrlFont;
                if (ResolveFont(C.Font, CtrlFont))
                {
                    FTextBlockStyle Style = T->GetDefaultTextStyle();
                    Style.SetFont(CtrlFont);
                    T->SetDefaultTextStyle(Style);
                }
            }
            else if (bHasFormFont)
            {
                FTextBlockStyle Style = T->GetDefaultTextStyle();
                Style.SetFont(FormFont);
                T->SetDefaultTextStyle(Style);
            }

            continue;
        }

        // SLIDER
        if (C.Type == EFormCtrlType::Slider)
        {
            USlider* S = GetSlider(C.Id);
            if (!S) continue;

            if (C.bHasActiveColor && C.ActiveColor != FLinearColor::Transparent)
            {
                S->SetSliderBarColor(C.ActiveColor);
                S->SetSliderHandleColor(C.ActiveColor);
            }

            continue;
        }
    }
}

void UBaseScreen::SetDialogInputEnabled(bool bEnable)
{
    bDialogInputEnabled = bEnable;

    // IMPORTANT:
    // Never SetIsEnabled(false) here. That disables the entire widget tree,
    // makes everything look grey, and overrides per-button SetIsEnabled(true).
    SetIsEnabled(true);

    // Visible but non-interactive when disabled (modal background behavior)
    SetVisibility(bEnable ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);

    // Focus should track interactivity
    SetIsFocusable(bEnable);
}

void UBaseScreen::SetOptionsManager_Implementation(UOptionsScreen* InManager)
{
    OptionsManager = InManager;
}

UVerticalBox* UBaseScreen::EnsureAutoVerticalBox()
{
    // Top = 64, Left/Right/Bottom = 32
    return EnsureAutoVerticalBoxWithOffsets(FMargin(32.f, 32.f, 32.f, 32.f));
}

UVerticalBox* UBaseScreen::EnsureAutoVerticalBoxWithOffsets(const FMargin& Offsets)
{
    if (IsValid(AutoVBox))
        return AutoVBox;

    if (!WidgetTree)
        return nullptr;

    // If RootCanvas isn't bound, try to recover it:
    if (!RootCanvas)
    {
        // Option A: root widget is the canvas
        RootCanvas = Cast<UCanvasPanel>(GetRootWidget());

        // Option B: find a widget literally named "RootCanvas" in the BP
        if (!RootCanvas)
            RootCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RootCanvas")));
    }

    if (!RootCanvas)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] EnsureAutoVerticalBox: RootCanvas is NULL (BindWidgetOptional name mismatch or not a CanvasPanel)."),
            *GetName());
        return nullptr;
    }

    static const FName VBoxName(TEXT("AutoVBox"));

    if (UWidget* Existing = WidgetTree->FindWidget(VBoxName))
    {
        AutoVBox = Cast<UVerticalBox>(Existing);
        if (AutoVBox)
        {
            AutoVBox->SetVisibility(ESlateVisibility::Visible);
        }
        return AutoVBox;
    }

    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), VBoxName);
    if (!VBox)
        return nullptr;

    VBox->SetVisibility(ESlateVisibility::Visible);

    // IMPORTANT: AddChildToCanvas gives you a CanvasPanelSlot
    if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(VBox))
    {
        CanvasSlot->SetAutoSize(false);
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        CanvasSlot->SetOffsets(Offsets);

        // CRITICAL: keep it above any BP content on the canvas
        CanvasSlot->SetZOrder(100);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] EnsureAutoVerticalBox: AddChildToCanvas failed."), *GetName());
        return nullptr;
    }

    AutoVBox = VBox;
    return AutoVBox;
}

UHorizontalBox* UBaseScreen::AddLabeledRow(
    const FString& LabelText,
    UWidget* Control,
    float ControlWidth,
    float ControlHeight,
    float RowPaddingY,
    float LabelRightPad
)
{
    if (!Control || !WidgetTree)
        return nullptr;

    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return nullptr;

    // -----------------------------
    // Horizontal row container
    // -----------------------------
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!Row)
        return nullptr;

    // Add row to VBox:
    if (UVerticalBoxSlot* RowSlot = VBox->AddChildToVerticalBox(Row))
    {
        RowSlot->SetHorizontalAlignment(HAlign_Fill);
        RowSlot->SetVerticalAlignment(VAlign_Fill);
        // Optional: you can also set per-row padding here if you want:
        // RowSlot->SetPadding(FMargin(0.f, RowPaddingY));
    }

    // -----------------------------
    // LEFT: Label (fills remaining space)
    // -----------------------------
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (Label)
    {
        Label->SetText(FText::FromString(LabelText));
        Label->SetJustification(ETextJustify::Left);

        if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LabelSlot->SetHorizontalAlignment(HAlign_Left);
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetPadding(FMargin(0.f, RowPaddingY, LabelRightPad, RowPaddingY));
        }
    }

    // -----------------------------
    // RIGHT: SizeBox wrapper (fixed width)
    // -----------------------------
    USizeBox* SizeWrapper = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    if (!SizeWrapper)
        return Row;

    SizeWrapper->SetWidthOverride(ControlWidth);
    if (ControlHeight > 0.f)
        SizeWrapper->SetHeightOverride(ControlHeight);

    SizeWrapper->AddChild(Control);

    if (UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(SizeWrapper))
    {
        ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ControlSlot->SetHorizontalAlignment(HAlign_Right);
        ControlSlot->SetVerticalAlignment(VAlign_Center);
        ControlSlot->SetPadding(FMargin(0.f, RowPaddingY, 0.f, RowPaddingY));
    }

    return Row;
}

// ------------------------------------------------------------
// Row helpers
// ------------------------------------------------------------

UTextBlock* UBaseScreen::MakeLabelText(const FName& Name, const FText& Text) const
{
    if (!WidgetTree)
        return nullptr;

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    if (!Label)
        return nullptr;

    Label->SetText(Text);
    Label->SetJustification(ETextJustify::Left);

    return Label;
}

UHorizontalBox* UBaseScreen::AddRow(const FName& RowName)
{
    if (!WidgetTree)
        return nullptr;

    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return nullptr;

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
    if (!Row)
        return nullptr;

    if (UVerticalBoxSlot* RowSlot = VBox->AddChildToVerticalBox(Row))
    {
        RowSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 6.f));
        RowSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    return Row;
}

UHorizontalBox* UBaseScreen::AddLabeledControlRow(
    const FName& RowName,
    const FText& LabelText,
    UWidget* ControlWidget,
    UTextBlock** OutLabel,
    float LabelMinWidth,
    float RowHeight
)
{
    if (!ControlWidget || !WidgetTree)
        return nullptr;

    UHorizontalBox* Row = AddRow(RowName);
    if (!Row)
        return nullptr;

    // LEFT: label
    const FName LabelName(*FString::Printf(TEXT("%s_Label"), *RowName.ToString()));
    UTextBlock* Label = MakeLabelText(LabelName, LabelText);
    if (!Label)
        return Row;

    if (UHorizontalBoxSlot* LSlot = Row->AddChildToHorizontalBox(Label))
    {
        LSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        LSlot->SetHorizontalAlignment(HAlign_Left);
        LSlot->SetVerticalAlignment(VAlign_Center);

        // Keep label stable so controls line up nicely
        LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    // Optional: enforce a minimum label width so right controls line up
    // (This is done by wrapping the label in a SizeBox if you want strict width,
    // but “minimum width” on TextBlock itself isn’t a thing.)
    // If you need strict label width, tell me and I’ll add a SizeBox wrapper.

    // SPACER / FILL: gives right side room
    // We’ll instead set the CONTROL slot to Fill and align right, which is simplest.

    // RIGHT: control
    if (UHorizontalBoxSlot* CSlot = Row->AddChildToHorizontalBox(ControlWidget))
    {
        // Fill takes remaining space; Right alignment pushes the control to the right edge
        CSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CSlot->SetHorizontalAlignment(HAlign_Right);
        CSlot->SetVerticalAlignment(VAlign_Center);
        CSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    }

    // Optional row height (if you want consistent row sizing)
    if (RowHeight > 0.0f)
    {
        // If you want strict row height, wrap Row in a SizeBox.
        // Keeping it simple here; ask and I’ll add a "SizeBoxRowHeight" path.
    }

    if (OutLabel)
        *OutLabel = Label;

    return Row;
}

UCanvasPanel* UBaseScreen::ResolveRootCanvas()
{
    if (RootCanvas)
        return RootCanvas;

    if (!WidgetTree)
        return nullptr;

    // 1) If the root widget itself is a canvas:
    if (UCanvasPanel* AsCanvas = Cast<UCanvasPanel>(GetRootWidget()))
    {
        RootCanvas = AsCanvas;
        return RootCanvas;
    }

    // 2) If there is a widget literally named RootCanvas:
    if (UCanvasPanel* Named = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RootCanvas"))))
    {
        RootCanvas = Named;
        return RootCanvas;
    }

    // 3) Walk the widget tree: first CanvasPanel found wins.
    if (UWidget* Root = WidgetTree->RootWidget)
    {
        TArray<UWidget*> Stack;
        Stack.Add(Root);

        while (Stack.Num() > 0)
        {
            UWidget* W = Stack.Pop(false);

            if (UCanvasPanel* C = Cast<UCanvasPanel>(W))
            {
                RootCanvas = C;
                return RootCanvas;
            }

            if (UPanelWidget* P = Cast<UPanelWidget>(W))
            {
                const int32 N = P->GetChildrenCount();
                for (int32 i = 0; i < N; ++i)
                    Stack.Add(P->GetChildAt(i));
            }
        }
    }

    return nullptr;
}

void UBaseScreen::EnsureUiFontsLoaded()
{
    // Safe to call repeatedly
    if (!Font_LimerickBold)
    {
        Font_LimerickBold = LoadObject<UFont>(nullptr, PATH_Font_LimerickBold);
        UE_LOG(LogBaseScreen, Log, TEXT("[BaseScreen] Load Font_LimerickBold: %s -> %s"),
            PATH_Font_LimerickBold, Font_LimerickBold ? TEXT("OK") : TEXT("FAILED"));
    }

    if (!Font_VerdanaItalic)
    {
        Font_VerdanaItalic = LoadObject<UFont>(nullptr, PATH_Font_VerdanaItalic);
        UE_LOG(LogBaseScreen, Log, TEXT("[BaseScreen] Load Font_VerdanaItalic: %s -> %s"),
            PATH_Font_VerdanaItalic, Font_VerdanaItalic ? TEXT("OK") : TEXT("FAILED"));
    }

    if (!Font_Serpntb)
    {
        Font_Serpntb = LoadObject<UFont>(nullptr, PATH_Font_SerpentBold);
        UE_LOG(LogBaseScreen, Log, TEXT("[BaseScreen] Load Font_SerpentBold: %s -> %s"),
            PATH_Font_SerpentBold, Font_Serpntb ? TEXT("OK") : TEXT("FAILED"));
    }
}

void UBaseScreen::ApplyDefaultEditBoxStyle(UEditableTextBox* Edit, int32 FontSize) const
{
    if (!Edit)
        return;

    // Ensure fonts are loaded (const_cast ok for cache loaders)
    const_cast<UBaseScreen*>(this)->EnsureThemeFontsLoaded();

    UFont* FontObj = DefaultFont ? DefaultFont : Font_LimerickBold;

    // ------------------------------------------------------------
    // Modify WidgetStyle directly (UE versions without SetStyle())
    // ------------------------------------------------------------
    FEditableTextBoxStyle& Style = Edit->WidgetStyle;

    // Text Style (typed text)
    if (FontObj)
    {
        // NOTE: In your UE headers, HintTextStyle doesn't exist.
        // So we only update TextStyle.
        Style.TextStyle.SetFont(FSlateFontInfo(FontObj, FontSize));
        Style.TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    // ------------------------------------------------------------
    // Background styling (UMG way)
    // ------------------------------------------------------------
    FSlateBrush BackgroundBrush;
    BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
    BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
    BackgroundBrush.Margin = FMargin(4.f / 16.f);

    Style.SetBackgroundImageNormal(BackgroundBrush);
    Style.SetBackgroundImageHovered(BackgroundBrush);
    Style.SetBackgroundImageFocused(BackgroundBrush);
    Style.SetBackgroundImageReadOnly(BackgroundBrush);

    // Optional: caret / selection are not exposed reliably via UMG

    // ------------------------------------------------------------
    // Push changes to Slate (UE versions without SetStyle)
    // ------------------------------------------------------------
    Edit->SynchronizeProperties();
}

// ------------------------------------------------------------
// Traversal (no lambdas)
// ------------------------------------------------------------

void UBaseScreen::ApplyDefaultTextStyle(UTextBlock* Text, int32 FontSize) const
{
    if (!Text)
        return;

    // Ensure your font assets are loaded/assigned (your setup code)
    // EnsureUiFontsLoaded();  // call this if you have it

    if (Font_LimerickBold)
    {
        FSlateFontInfo FontInfo;
        FontInfo.FontObject = Font_LimerickBold;
        FontInfo.Size = FontSize;

        Text->SetFont(FontInfo);
    }

    // Common default
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
}

void UBaseScreen::ApplyDefaultTextStyle_AllTextBlocks(int32 FontSize) const
{
    if (!WidgetTree)
        return;

    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* W : AllWidgets)
    {
        UTextBlock* TB = Cast<UTextBlock>(W);
        if (TB)
        {
            ApplyDefaultTextStyle(TB, FontSize);
        }
    }
}

void UBaseScreen::ApplyTitleTextStyle(UTextBlock* Text, int32 FontSize) const
{
    if (!Text)
        return;

    // Title: Serpent Bold if available, else Limerick
    if (Font_Serpntb)
        Text->SetFont(FSlateFontInfo(Font_Serpntb, FontSize));
    else if (Font_LimerickBold)
        Text->SetFont(FSlateFontInfo(Font_Serpntb, FontSize));

    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
}

void UBaseScreen::ApplyDefaultButtonStyle(UButton* Button, int32 FontSize) const
{
    if (!Button)
        return;

    // Style the button chrome first (your existing SetStyle code is fine)
    // Button->SetStyle(...);

    UTextBlock* TextChild = FindFirstTextBlockRecursive(Button);
    if (!TextChild)
    {
        UE_LOG(LogBaseScreen, Warning, TEXT("[BaseScreen] Button '%s' has no TextBlock content."), *Button->GetName());
        return;
    }

    // Color
    TextChild->SetColorAndOpacity(FSlateColor(UITextColor));

    // Font
    UFont* FontObj = GetThemeFont(DefaultUIFont);
    if (!FontObj)
        FontObj = Font_LimerickBold; // fallback

    if (FontObj)
    {
        FSlateFontInfo FontInfo = TextChild->GetFont();
        FontInfo.FontObject = FontObj;
        FontInfo.Size = FontSize;
        TextChild->SetFont(FontInfo);
    }

    TextChild->SetJustification(ETextJustify::Center);
}

void UBaseScreen::EnsureThemeFontsLoaded()
{
    if (Font_LimerickBold && Font_VerdanaItalic && Font_Serpntb)
        return;

    // Replace with your exact Copy Reference strings if different:
    const FSoftObjectPath LimerickPath(TEXT("/Game/Font/Limerick-Serial_Bold_Font.Limerick-Serial_Bold_Font"));
    const FSoftObjectPath VerdanaPath(TEXT("/Game/Font/VERDANAI_Font.VERDANAI_Font"));
    const FSoftObjectPath SerpntbPath(TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font"));

    if (!Font_LimerickBold)  Font_LimerickBold = Cast<UFont>(LimerickPath.TryLoad());
    if (!Font_VerdanaItalic) Font_VerdanaItalic = Cast<UFont>(VerdanaPath.TryLoad());
    if (!Font_Serpntb)       Font_Serpntb = Cast<UFont>(SerpntbPath.TryLoad());
}

UFont* UBaseScreen::GetThemeFont(ESSWThemeFont Which) const
{
    switch (Which)
    {
    default:
    case ESSWThemeFont::LimerickBold:  return Font_LimerickBold;
    case ESSWThemeFont::VerdanaItalic: return Font_VerdanaItalic;
    case ESSWThemeFont::Serpntb:       return Font_Serpntb;
    }
}

void UBaseScreen::ApplyGlobalTheme(bool bStyleButtons, bool bStyleText, bool bStyleEdits)
{
    EnsureThemeFontsLoaded();

    if (!WidgetTree)
        return;

    UWidget* Root = GetRootWidget();
    if (!Root)
        return;

    if (bStyleButtons)
    {
        TArray<UButton*> Buttons;
        GatherButtonsRecursive(Root, Buttons);

        for (int32 i = 0; i < Buttons.Num(); ++i)
        {
            StyleButton_Default(Buttons[i], 20);
        }
    }

    if (bStyleText)
    {
        TArray<UTextBlock*> Texts;
        GatherTextBlocksRecursive(Root, Texts);

        for (int32 i = 0; i < Texts.Num(); ++i)
        {
            StyleText_Default(Texts[i], 18);
        }
    }

    if (bStyleEdits)
    {
        TArray<UEditableTextBox*> Edits;
        GatherEditableTextBoxesRecursive(Root, Edits);

        for (int32 i = 0; i < Edits.Num(); ++i)
        {
            StyleEdit_Default(Edits[i], 18);
        }
    }
}

void UBaseScreen::StyleButton_Default(
    UButton* Button,
    int32 FontSize,
    const FLinearColor* OverrideTextColor,
    UFont* OverrideFont
)
{
    if (!Button)
        return;

    EnsureThemeFontsLoaded();

    // ------------------------------------------------------------
    // Button chrome (Starshatter 4.5 art)
    // ------------------------------------------------------------
    const FSlateBrush Normal = MakeStarshatterBrush(Btn_NormalTex, Btn_9SliceMargin, Btn_ImageSize);
    const FSlateBrush Hovered = MakeStarshatterBrush(Btn_HoverTex, Btn_9SliceMargin, Btn_ImageSize);
    const FSlateBrush Pressed = MakeStarshatterBrush(Btn_PressedTex, Btn_9SliceMargin, Btn_ImageSize);
    const FSlateBrush Disabled = MakeStarshatterBrush(Btn_DisabledTex ? Btn_DisabledTex : Btn_NormalTex,
        Btn_9SliceMargin, Btn_ImageSize);

    FButtonStyle Style = Button->WidgetStyle;
    Style.SetNormal(Normal);
    Style.SetHovered(Hovered);
    Style.SetPressed(Pressed);
    Style.SetDisabled(Disabled);

    // ------------------------------------------------------------
// UI SFX via Slate (no delegate binding required)
// ------------------------------------------------------------
    if (bEnableButtonSfx)
    {
        if (ButtonHoverSfx)
        {
            FSlateSound HoverSound;
            HoverSound.SetResourceObject(ButtonHoverSfx);
            Style.SetHoveredSound(HoverSound);
        }

        if (ButtonClickSfx)
        {
            FSlateSound PressSound;
            PressSound.SetResourceObject(ButtonClickSfx);
            Style.SetPressedSound(PressSound);
        }
    }

    // Prefer SetStyle if present; fallback to WidgetStyle
#if ENGINE_MAJOR_VERSION >= 5
    Button->SetStyle(Style);
#else
    Button->WidgetStyle = Style;
    Button->SynchronizeProperties();
#endif

    // ------------------------------------------------------------
    // Text styling (font + color)
    // ------------------------------------------------------------
    UTextBlock* TextChild = FindFirstTextBlockRecursive(Button);
    if (!TextChild)
        return;

    TextChild->SetJustification(ETextJustify::Center);

    const FLinearColor FinalColor = OverrideTextColor ? *OverrideTextColor : UITextColor;
    TextChild->SetColorAndOpacity(FSlateColor(FinalColor));

    UFont* FontObj = OverrideFont ? OverrideFont : GetThemeFont(DefaultUIFont);
    if (!FontObj)
        FontObj = Font_LimerickBold; // hard fallback

    if (FontObj)
    {
        FSlateFontInfo FontInfo = TextChild->GetFont();
        FontInfo.FontObject = FontObj;
        FontInfo.Size = FontSize;
        TextChild->SetFont(FontInfo);
    }

    // Force UMG to repaint with new text style in some cases:
    TextChild->SynchronizeProperties();
}

void UBaseScreen::StyleText_Default(UTextBlock* Text, int32 FontSize)
{
    if (!Text)
        return;

    Text->SetColorAndOpacity(FSlateColor(UITextColor));

    UFont* FontObj = GetThemeFont(DefaultUIFont);
    if (FontObj)
    {
        FSlateFontInfo FontInfo = Text->GetFont();
        FontInfo.FontObject = FontObj;
        FontInfo.Size = FontSize;
        Text->SetFont(FontInfo);
    }
}

void UBaseScreen::StyleEdit_Default(UEditableTextBox* Edit, int32 FontSize)
{
    if (!Edit)
        return;

    EnsureThemeFontsLoaded();

    UFont* FontObj = GetThemeFont(DefaultUIFont);

    // Modify the live style directly (your UE version has no SetStyle)
    FEditableTextBoxStyle& Style = Edit->WidgetStyle;

    if (FontObj)
    {
        Style.TextStyle.SetFont(FSlateFontInfo(FontObj, FontSize));
        Style.TextStyle.SetColorAndOpacity(FSlateColor(UITextColor));
    }

    // Optional: set a consistent dark background for all states
    FSlateBrush Bg;
    Bg.DrawAs = ESlateBrushDrawType::Box;
    Bg.TintColor = FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
    Bg.Margin = FMargin(4.f / 16.f);

    Style.SetBackgroundImageNormal(Bg);
    Style.SetBackgroundImageHovered(Bg);
    Style.SetBackgroundImageFocused(Bg);
    Style.SetBackgroundImageReadOnly(Bg);

    // Push the updated style into the underlying Slate widget
    Edit->SynchronizeProperties();
}

void UBaseScreen::GatherButtonsRecursive(UWidget* Root, TArray<UButton*>& OutButtons)
{
    if (!Root) return;

    if (UButton* AsButton = Cast<UButton>(Root))
        OutButtons.Add(AsButton);

    if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; ++i)
            GatherButtonsRecursive(Panel->GetChildAt(i), OutButtons);
    }
    else if (UContentWidget* Content = Cast<UContentWidget>(Root))
    {
        if (UWidget* Child = Content->GetContent())
            GatherButtonsRecursive(Child, OutButtons);
    }
}

void UBaseScreen::GatherTextBlocksRecursive(UWidget* Root, TArray<UTextBlock*>& OutTexts)
{
    if (!Root) return;

    if (UTextBlock* AsText = Cast<UTextBlock>(Root))
        OutTexts.Add(AsText);

    if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; ++i)
            GatherTextBlocksRecursive(Panel->GetChildAt(i), OutTexts);
    }
    else if (UContentWidget* Content = Cast<UContentWidget>(Root))
    {
        if (UWidget* Child = Content->GetContent())
            GatherTextBlocksRecursive(Child, OutTexts);
    }
}

void UBaseScreen::GatherEditableTextBoxesRecursive(UWidget* Root, TArray<UEditableTextBox*>& OutEdits)
{
    if (!Root) return;

    if (UEditableTextBox* AsEdit = Cast<UEditableTextBox>(Root))
        OutEdits.Add(AsEdit);

    if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; ++i)
            GatherEditableTextBoxesRecursive(Panel->GetChildAt(i), OutEdits);
    }
    else if (UContentWidget* Content = Cast<UContentWidget>(Root))
    {
        if (UWidget* Child = Content->GetContent())
            GatherEditableTextBoxesRecursive(Child, OutEdits);
    }
}

UTextBlock* UBaseScreen::FindFirstTextBlockRecursive(const UWidget* Root) const
{
    if (!Root)
        return nullptr;

    if (const UTextBlock* TB = Cast<UTextBlock>(Root))
        return const_cast<UTextBlock*>(TB);

    // UButton, UBorder, USizeBox, etc.
    if (const UContentWidget* Content = Cast<UContentWidget>(Root))
    {
        return FindFirstTextBlockRecursive(Content->GetContent());
    }

    if (const UPanelWidget* Panel = Cast<UPanelWidget>(Root))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; ++i)
        {
            if (UTextBlock* Found = FindFirstTextBlockRecursive(Panel->GetChildAt(i)))
                return Found;
        }
    }

    return nullptr;
}

void UBaseScreen::PlayButtonSfx(USoundBase* Sound) const
{
    if (!Sound)
        return;

    // UI-only, non-spatialized, no attenuation concerns:
    UGameplayStatics::PlaySound2D(this, Sound, ButtonSfxVolume);
}

void UBaseScreen::HookButtonSfx(UButton* Button)
{
    if (!bEnableButtonSfx || !Button)
        return;

    // Don’t bind twice (UMG can reconstruct widgets)
    if (SfxBoundButtons.Contains(Button))
        return;

    // NOTE: no lambdas; use UFUNCTION-style binding via AddDynamic
    // But these handlers must be UFUNCTIONs OR we use AddUObject with non-dynamic delegates.
    //
    // UButton delegates are dynamic multicast:
    // - OnClicked, OnHovered, OnUnhovered are DECLARE_DYNAMIC_MULTICAST_DELEGATE
    // So we need UFUNCTION handlers with no params.
    //
    // To keep this generic per-button, we’ll store “current button” at call time?
    // That doesn’t work for multiple buttons.
    //
    // Solution: bind per-button to generic handlers using AddUniqueDynamic,
    // then in handler identify sender? Dynamic delegates do NOT provide sender.
    //
    // Therefore, we do the robust approach:
    // Create a tiny UObject “router” per button, or use Slate sound style.
    //
    // BUT simplest for your project: use button style sounds (Slate) OR create router objects.
}


