/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimObject.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simulation Object and Observer classes
*/

#include "SimObject.h"

#include "Graphic.h"
#include "SimLight.h"
#include "SimScene.h"

// Minimal Unreal logging support:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimObject, Log, All);


// +--------------------------------------------------------------------+

SimObserver::~SimObserver()
{
	ListIter<SimObject> observed = observe_list;
	while (++observed)
		observed->Unregister(this);
}

void
SimObserver::Observe(SimObject* obj)
{
	if (obj) {
		obj->Register(this);

		if (!observe_list.contains(obj))
			observe_list.append(obj);
	}
}

void
SimObserver::Ignore(SimObject* obj)
{
	if (obj) {
		obj->Unregister(this);
		observe_list.remove(obj);
	}
}

bool
SimObserver::Update(SimObject* obj)
{
	if (obj)
		observe_list.remove(obj);

	return true;
}

const char*
SimObserver::GetObserverName() const
{
	static char name[32];

	// uintptr_t is guaranteed to hold a pointer on 32/64-bit
	std::snprintf(
		name,
		sizeof(name),
		"SimObserver %p",
		static_cast<const void*>(this)
	);

	return name;
}

// +--------------------------------------------------------------------+

SimObject::~SimObject()
{
	Notify();
}

// +--------------------------------------------------------------------+

void
SimObject::Notify()
{
	if (!notifying) {
		notifying = true;

		int nobservers = observers.size();
		int nupdate = 0;

		if (nobservers > 0) {
			ListIter<SimObserver> iter = observers;
			while (++iter) {
				SimObserver* observer = iter.value();
				observer->Update(this);
				nupdate++;
			}

			observers.clear();
		}

		if (nobservers != nupdate) {
			UE_LOG(LogTemp, Warning,
				TEXT("WARNING: incomplete notify sim object '%s' - %d of %d notified"),
				ANSI_TO_TCHAR(Name()),
				nupdate,
				nobservers);
		}

		notifying = false;
	}
	else {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: double notify on sim object '%s'"),
			ANSI_TO_TCHAR(Name()));
	}
}

// +--------------------------------------------------------------------+

void
SimObject::Register(SimObserver* observer)
{
	if (!notifying && !observers.contains(observer))
		observers.append(observer);
}

// +--------------------------------------------------------------------+

void
SimObject::Unregister(SimObserver* observer)
{
	if (!notifying)
		observers.remove(observer);
}

// +--------------------------------------------------------------------+

void
SimObject::Activate(SimScene& scene)
{
	if (rep)
		scene.AddGraphic(rep);
	if (light)
		scene.AddLight(light);

	active = true;
}

void
SimObject::Deactivate(SimScene& scene)
{
	if (rep)
		scene.DelGraphic(rep);
	if (light)
		scene.DelLight(light);

	active = false;
}
