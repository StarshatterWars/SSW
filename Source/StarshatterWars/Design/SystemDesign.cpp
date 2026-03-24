/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SystemDesign.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic ship System Design class
*/

#include "SystemDesign.h"
#include "CoreMinimal.h" // UE_LOG, ANSI_TO_TCHAR
#include "SimComponent.h"

#include "Game.h"
// Bitmap removed: render assets are Unreal UTexture2D*
#include "DataLoader.h"
#include "ParseUtil.h"
#include "List.h"

// +--------------------------------------------------------------------+

List<SystemDesign> SystemDesign::catalog;

#define GET_DEF_TEXT(p,d,x) if(p->name()->value()==(#x))GetDefText(d->x,p,filename)
#define GET_DEF_NUM(p,d,x)  if(p->name()->value()==(#x))GetDefNumber(d->x,p,filename)

// +--------------------------------------------------------------------+

SystemDesign::SystemDesign()
{
}

SystemDesign::~SystemDesign()
{
	components.destroy();
}

// +--------------------------------------------------------------------+

void
SystemDesign::Initialize(const char* filename)
{
	UE_LOG(LogTemp, Log, TEXT("Loading System Designs '%s'"), ANSI_TO_TCHAR(filename));

	// Load Design File:
	DataLoader* Loader = DataLoader::GetLoader();
	BYTE* Block = nullptr;

	const int32 BlockLen = Loader->LoadBuffer(filename, Block, true);
	Parser ParserObj(new BlockReader((const char*)Block, BlockLen));
	Term* TermObj = ParserObj.ParseTerm();

	if (!TermObj) {
		UE_LOG(LogTemp, Error, TEXT("ERROR: could not parse '%s'"), ANSI_TO_TCHAR(filename));
		checkf(false, TEXT("SystemDesign::Initialize parse failure: %s"), ANSI_TO_TCHAR(filename));
		return;
	}
	else {
		TermText* FileType = TermObj->isText();
		if (!FileType || FileType->value() != "SYSTEM") {
			UE_LOG(LogTemp, Error, TEXT("ERROR: invalid system design file '%s'"), ANSI_TO_TCHAR(filename));
			checkf(false, TEXT("SystemDesign::Initialize invalid file type: %s"), ANSI_TO_TCHAR(filename));
			return;
		}
	}

	do {
		delete TermObj;
		TermObj = ParserObj.ParseTerm();

		if (TermObj) {
			TermDef* Def = TermObj->isDef();
			if (Def) {
				if (Def->name()->value() == "system") {

					if (!Def->term() || !Def->term()->isStruct()) {
						UE_LOG(LogTemp, Warning, TEXT("WARNING: system structure missing in '%s'"), ANSI_TO_TCHAR(filename));
					}
					else {
						TermStruct* SystemStruct = Def->term()->isStruct();
						SystemDesign* Design = new SystemDesign;

						for (int32 i = 0; i < SystemStruct->elements()->size(); i++) {
							TermDef* ParamDef = SystemStruct->elements()->at(i)->isDef();
							if (ParamDef) {
								GET_DEF_TEXT(ParamDef, Design, name);

							else if (ParamDef->name()->value() == ("component")) {
								if (!ParamDef->term() || !ParamDef->term()->isStruct()) {
									UE_LOG(LogTemp, Warning,
										TEXT("WARNING: component structure missing in system '%s' in '%s'"),
										ANSI_TO_TCHAR(Design->name.data()),
										ANSI_TO_TCHAR(filename));
								}
								else {
									TermStruct* ComponentStruct = ParamDef->term()->isStruct();
									ComponentDesign* Component = new ComponentDesign;

									for (int32 j = 0; j < ComponentStruct->elements()->size(); j++) {
										TermDef* ComponentParamDef = ComponentStruct->elements()->at(j)->isDef();
										if (ComponentParamDef) {
											GET_DEF_TEXT(ComponentParamDef, Component, name);
										else GET_DEF_TEXT(ComponentParamDef, Component, abrv);
							else GET_DEF_NUM(ComponentParamDef, Component, repair_time);
				else GET_DEF_NUM(ComponentParamDef, Component, replace_time);
			else GET_DEF_NUM(ComponentParamDef, Component, spares);
		else GET_DEF_NUM(ComponentParamDef, Component, affects);

else {
	UE_LOG(LogTemp, Warning,
		TEXT("WARNING: parameter '%s' ignored in '%s'"),
		ANSI_TO_TCHAR(ComponentParamDef->name()->value().data()),
		ANSI_TO_TCHAR(filename));
}
										}
									}

									Design->components.append(Component);
								}
							}

							else {
								UE_LOG(LogTemp, Warning,
									TEXT("WARNING: parameter '%s' ignored in '%s'"),
									ANSI_TO_TCHAR(ParamDef->name()->value().data()),
									ANSI_TO_TCHAR(filename));
							}
							}
							else {
								UE_LOG(LogTemp, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
								SystemStruct->elements()->at(i)->print();
							}
						}

						catalog.append(Design);
					}
				}

				else {
					UE_LOG(LogTemp, Warning,
						TEXT("WARNING: unknown definition '%s' in '%s'"),
						ANSI_TO_TCHAR(Def->name()->value().data()),
						ANSI_TO_TCHAR(filename));
				}
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
				TermObj->print();
			}
		}
	} while (TermObj);

	Loader->ReleaseBuffer(Block);
}

// +--------------------------------------------------------------------+

void
SystemDesign::Close()
{
	catalog.destroy();
}

// +--------------------------------------------------------------------+

SystemDesign*
SystemDesign::Find(const char* name)
{
	SystemDesign test;
	test.name = name;
	return catalog.find(&test);
}
