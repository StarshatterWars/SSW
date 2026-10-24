#include "NativeSkinDefinition.h"
#include "ParseUtil.h"
#include "Parser.h"
#include "Reader.h"
#include "Term.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
FSkinMtlCell NativeSkinDefinition::ParseMaterial(TermStruct* Val, const char* Fn)
{
	FSkinMtlCell Out;

	if (!Val || !Fn || !*Fn)
	{
		return Out;
	}

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const Text& Key = Def->name()->value();

		if (Key == "index" || Key == "cell" || Key == "id")
		{
			GetDefNumber(Out.CellIndex, Def, Fn);
		}
		else if (Key == "material" || Key == "mtl" || Key == "name")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.MaterialName = FString(Buf);
			}
		}
		else if (Key == "texture" || Key == "bitmap" || Key == "diffuse" || Key == "map" || Key == "tex_d" || Key == "tex_diffuse")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.TextureName = FString(Buf);
			}
		}
        else if (Key == "tex_s" || Key == "tex_specular")
        { Text Value; if(GetDefText(Value,Def,Fn)) Out.SpecularTextureName=FString(Value); }
        else if (Key == "tex_b" || Key == "tex_bumpmap")
        { Text Value; if(GetDefText(Value,Def,Fn)) Out.BumpTextureName=FString(Value); }
        else if (Key == "tex_e" || Key == "tex_emissive")
        { Text Value; if(GetDefText(Value,Def,Fn)) Out.EmissiveTextureName=FString(Value); }
        else if (Key == "Ka")
        { FColor Value; if(GetDefFColor(Value,Def,Fn)) {Out.AmbientColor=FLinearColor::FromSRGBColor(Value);Out.bHasAmbientColor=true;} }
        else if (Key == "Kd")
        { FColor Value; if(GetDefFColor(Value,Def,Fn)) {Out.DiffuseColor=FLinearColor::FromSRGBColor(Value);Out.bHasDiffuseColor=true;} }
        else if (Key == "Ks")
        { FColor Value; if(GetDefFColor(Value,Def,Fn)) {Out.SpecularColor=FLinearColor::FromSRGBColor(Value);Out.bHasSpecularColor=true;} }
        else if (Key == "Ke")
        { FColor Value; if(GetDefFColor(Value,Def,Fn)) {Out.EmissiveColor=FLinearColor::FromSRGBColor(Value);Out.bHasEmissiveColor=true;} }
        else if(Key=="power" || Key=="Ns") { GetDefNumber(Out.SpecularPower,Def,Fn);Out.bHasSpecularPower=true; }
        else if(Key=="bump") { GetDefNumber(Out.BumpScale,Def,Fn);Out.bHasBumpScale=true; }
		else if (Key == "aux" || Key == "normal" || Key == "spec" || Key == "specular")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.AuxTextureName = FString(Buf);
			}
		}
	}

	return Out;
}


bool NativeSkinDefinition::LoadAuthoredSkins(const FString& SourceFile,TArray<FShipSkin>& Skins)
{
    Skins.Reset();if(SourceFile.IsEmpty()) return false;
    FString Contents;
    TArray<FString> Candidates={SourceFile,FPaths::ProjectContentDir()/TEXT("GameData/Ships")/FPaths::GetCleanFilename(SourceFile)};
    bool Loaded=false;
    for(const auto& Path:Candidates) if(FFileHelper::LoadFileToString(Contents,*Path)) {Loaded=true;break;}
    if(!Loaded) return false;
    const auto Source=StringCast<ANSICHAR>(*SourceFile);
    const auto InputBytes=StringCast<ANSICHAR>(*Contents);
    Parser Input(new BlockReader(InputBytes.Get()));
    while(Term* Value=Input.ParseTerm())
    {
        TermDef* Def=Value->isDef();
        if(Def && Def->name()->value()=="skin" && Def->term() && Def->term()->isStruct())
        {
            FShipSkin Skin;Skin.SourceFile=SourceFile;
            const auto* Fields=Def->term()->isStruct()->elements();
            for(int32 I=0;I<Fields->size();++I)
            {
                TermDef* Field=Fields->at(I)->isDef();if(!Field) continue;
                const Text& Key=Field->name()->value();
                if(Key=="name") {class Text Name;if(GetDefText(Name,Field,Source.Get())) Skin.Name=ANSI_TO_TCHAR(Name.data());}
                else if((Key=="mtl" || Key=="material") && Field->term() && Field->term()->isStruct())
                    Skin.Cells.Add(ParseMaterial(Field->term()->isStruct(),Source.Get()));
            }
            if(!Skin.Name.IsEmpty() && !Skin.Cells.IsEmpty()) Skins.Add(MoveTemp(Skin));
        }
        delete Value;
    }
    return !Skins.IsEmpty();
}
