/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         FlightDeck.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Everything needed to launch and recover space craft

	See Also: Hangar
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "SimObject.h"
#include "Text.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"

// +----------------------------------------------------------------------+

class Hoop;
class SimLight;
class Ship;
class ShipDesign;
class FlightDeck;
class FlightDeckSlot;
class Physical;

// +======================================================================+

class InboundSlot : public SimObserver
{
public:
	static const char* TYPENAME() { return "InboundSlot"; }

	InboundSlot() : ship(0), deck(0), squadron(0), slot(0), cleared(0), final(0), approach(0), offset(FVector::ZeroVector) {}
	InboundSlot(Ship* s, FlightDeck* d, int squad, int index);

	int operator <  (const InboundSlot& that) const;
	int operator <= (const InboundSlot& that) const;
	int operator == (const InboundSlot& that) const;

	// SimObserver:
	virtual bool         Update(SimObject* obj);
	virtual const char* GetObserverName() const;

	Ship* GetShip() { return ship; }
	FlightDeck* GetDeck() { return deck; }
	int               Squadron() { return squadron; }
	int               Index() { return slot; }
	int               Cleared() { return cleared; }
	int               Final() { return final; }
	int               Approach() { return approach; }
	FVector           Offset() { return offset; }
	double            Distance();

	void              SetApproach(int a) { approach = a; }
	void              SetOffset(const FVector& p) { offset = p; }
	void              SetFinal(int f) { final = f; }
	void              Clear(bool clear = true);

private:
	Ship* ship;
	FlightDeck* deck;
	int               squadron;
	int               slot;
	int               cleared;
	int               final;
	int               approach;
	FVector           offset;
};

// +----------------------------------------------------------------------+

class FlightDeck : public SimSystem, public SimObserver
{
public:
	static const char* TYPENAME() { return "FlightDeck"; }

	FlightDeck();
	FlightDeck(const FlightDeck& rhs);
	virtual ~FlightDeck();

	enum FLIGHT_DECK_MODE { FLIGHT_DECK_LAUNCH, FLIGHT_DECK_RECOVERY };
	enum FLIGHT_SLOT_STATE { CLEAR, READY, QUEUED, LOCKED, LAUNCH, DOCKING };
	enum CONSTANTS { NUM_APPROACH_PTS = 8 };

	static void    Initialize();
	static void    Close();

	virtual void   ExecFrame(double seconds);
	void           SetCarrier(Ship* s) { ship = carrier = s; }
	void           SetIndex(int n) { index = n; }

	virtual int    SpaceLeft(int type) const;

	virtual bool   Spot(Ship* s, int& index);
	virtual bool   Clear(int index);
	virtual bool   Launch(int index);
	virtual bool   LaunchShip(Ship* s);
	virtual bool   Recover(Ship* s);
	virtual bool   Dock(Ship* s);
	virtual int    Inbound(InboundSlot*& s);
	virtual void   GrantClearance();

	virtual void   AddSlot(const FVector& loc, DWORD filter = 0xf);

	virtual bool   IsLaunchDeck()       const { return subtype == FLIGHT_DECK_LAUNCH; }
	virtual void   SetLaunchDeck() { subtype = FLIGHT_DECK_LAUNCH; }
	virtual bool   IsRecoveryDeck()     const { return subtype == FLIGHT_DECK_RECOVERY; }
	virtual void   SetRecoveryDeck() { subtype = FLIGHT_DECK_RECOVERY; }

	FVector        BoundingBox()        const { return box; }
	FVector        ApproachPoint(int i) const { return approach_point[i]; }
	FVector        RunwayPoint(int i)   const { return runway_point[i]; }
	FVector        StartPoint()         const { return start_point; }
	FVector        EndPoint()           const { return end_point; }
	FVector        CamLoc()             const { return cam_loc; }
	double         Azimuth()            const { return azimuth; }

	virtual void   SetBoundingBox(FVector dimensions) { box = dimensions; }
	virtual void   SetApproachPoint(int i, FVector loc);
	virtual void   SetRunwayPoint(int i, FVector loc);
	virtual void   SetStartPoint(FVector loc);
	virtual void   SetEndPoint(FVector loc);
	virtual void   SetCamLoc(FVector loc);
	virtual void   SetCycleTime(double time);
	virtual void   SetAzimuth(double az) { azimuth = az; }
	virtual void   SetLight(double l);

	virtual void   Orient(const Physical* rep);

	// SimObserver:
	virtual bool         Update(SimObject* obj);
	virtual const char* GetObserverName() const;

	// accessors:
	int            NumSlots()                 const { return num_slots; }
	double         TimeRemaining(int index)   const;
	int            State(int index)           const;
	int            Sequence(int index)        const;
	const Ship* GetCarrier()               const { return carrier; }
	int            GetIndex()                 const { return index; }
	Ship* GetShip(int index)         const;
	int            NumHoops()                 const { return num_hoops; }
	Hoop* GetHoops()                 const { return hoops; }
	SimLight* GetLight() { return light; }

	List<InboundSlot>& GetRecoveryQueue() { return recovery_queue; }
	void           PrintQueue();

	bool           OverThreshold(Ship* s)           const;
	bool           ContainsPoint(const FVector& p)  const;

protected:
	Ship* carrier;
	int               index;
	int               num_slots;
	FlightDeckSlot* slots;

	FVector           box;
	FVector           start_rel;
	FVector           end_rel;
	FVector           cam_rel;
	FVector           approach_rel[NUM_APPROACH_PTS];
	FVector           runway_rel[2];

	FVector           start_point;
	FVector           end_point;
	FVector           cam_loc;
	FVector           approach_point[NUM_APPROACH_PTS];
	FVector           runway_point[2];

	double            azimuth;
	double            cycle_time;

	int               num_approach_pts;
	int               num_catsounds;
	int               num_hoops;
	Hoop* hoops;
	SimLight* light;
	List<InboundSlot> recovery_queue;
};
