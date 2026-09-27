// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

// ICampaignMissionGenerator.h

#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"

class ICampaignMissionGenerator
{
public:
    virtual ~ICampaignMissionGenerator() = default;

    // Return true if a mission was created and appended to mission list/save.
    virtual bool GenerateMission(const FCampaignMissionReq& Request) = 0;
};