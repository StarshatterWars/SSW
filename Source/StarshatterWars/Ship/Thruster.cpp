/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Thruster.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Thruster (system) class
*/

#include "Thruster.h"

// Starshatter core headers:
#include "Types.h"
#include "Text.h"
#include "List.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// Forward declarations were used in the header; cpp can include what it needs:
#include "SimComponent.h"
#include "Drive.h"
#include "FlightComputer.h"
#include "SystemDesign.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "CameraManager.h"
#include "AudioConfig.h"
#include "Random.h"

#include "SimLight.h"
#include "Bitmap.h"
#include "Sound.h"
#include "DataLoader.h"
#include "Bolt.h"
#include "Sprite.h"
#include "Game.h"
#include "GameStructs.h"

// Unreal color replacement:
#include "Math/Color.h"

#define CLAMP(x, a, b) if ((x) < (a)) (x) = (a); else if ((x) > (b)) (x) = (b);

// +----------------------------------------------------------------------+

static USound* thruster_resource = nullptr;
static USound* thruster_sound = nullptr;

extern Bitmap* drive_flare_bitmap[8];
extern Bitmap* drive_trail_bitmap[8];

// +----------------------------------------------------------------------+

ThrusterPort::ThrusterPort(int t, const FVector& l, DWORD f, float s)
    : type(t),
    fire(f),
    burn(0.0f),
    scale(s),
    loc(l),
    flare(nullptr),
    trail(nullptr)
{
}

ThrusterPort::~ThrusterPort()
{
    GRAPHIC_DESTROY(flare);
    GRAPHIC_DESTROY(trail);
}

// +----------------------------------------------------------------------+

static int sys_value = 2;

// +----------------------------------------------------------------------+

Thruster::Thruster(int dtype, double max_thrust, float flare_scale)
    : SimSystem(SYSTEM_CATEGORY::DRIVE, dtype, "Thruster", sys_value, max_thrust, max_thrust, max_thrust),
    ship(nullptr),
    thrust(1.0f),
    scale(flare_scale),
    avail_x(1.0f),
    avail_y(1.0f),
    avail_z(1.0f)
{
    name = Game::GetText("sys.thruster");
    abrv = Game::GetText("sys.thruster.abrv");

    power_flags = POWER_WATTS;

    for (int i = 0; i < 12; ++i)
        burn[i] = 0.0f;

    emcon_power[0] = 50;
    emcon_power[1] = 50;
    emcon_power[2] = 100;
}

// +----------------------------------------------------------------------+

Thruster::Thruster(const Thruster& t)
    : SimSystem(t),
    ship(nullptr),
    thrust(1.0f),
    scale(t.scale),
    avail_x(1.0f),
    avail_y(1.0f),
    avail_z(1.0f)
{
    power_flags = POWER_WATTS;
    Mount(t);

    for (int i = 0; i < 12; ++i)
        burn[i] = 0.0f;

    if (subtype != Drive::STEALTH) {
        for (int i = 0; i < t.ports.size(); i++) {
            ThrusterPort* p = t.ports[i];
            CreatePort(p->type, p->loc, p->fire, p->scale);
        }
    }
}

// +--------------------------------------------------------------------+

Thruster::~Thruster()
{
    ports.destroy();

    if (thruster_sound && thruster_sound->IsPlaying()) {
        thruster_sound->Stop();
    }
}

// +--------------------------------------------------------------------+

void
Thruster::Initialize()
{
    static int initialized = 0;
    if (initialized) return;

    DataLoader* loader = DataLoader::GetLoader();

    const int SOUND_FLAGS = USound::LOCALIZED |
        USound::LOC_3D |
        USound::LOOP |
        USound::LOCKED;

    loader->SetDataPath("Sounds/");
    loader->LoadSound("thruster.wav", thruster_resource, SOUND_FLAGS);
    loader->SetDataPath("");

    if (thruster_resource)
        thruster_resource->SetMaxDistance(15.0e3f);

    initialized = 1;
}

void
Thruster::Close()
{
    delete thruster_resource;
    thruster_resource = nullptr;

    if (thruster_sound) {
        thruster_sound->Stop();
        thruster_sound->Release();
    }

    thruster_sound = nullptr;
}

// +--------------------------------------------------------------------+

void
Thruster::Orient(const Physical* rep)
{
    SimSystem::Orient(rep);

    bool hide_all = false;

    if (!ship || (ship->IsAirborne() && ship->Class() != CLASSIFICATION::LCA))
        hide_all = true;

    if (ship->Rep() && ship->Rep()->Hidden())
        hide_all = true;

    if (thrust <= 0)
        hide_all = true;

    const FVector ship_loc = rep->Location();

    // Camera/ship basis vectors (must be FVectors for this to work correctly):
    const FVector Vrt = rep->Cam().vrt(); // "right" / lateral axis
    const FVector Vup = rep->Cam().vup(); // "up" axis
    const FVector Vpn = rep->Cam().vpn(); // "forward" (note: in some systems vpn points *toward* screen)

    for (int i = 0; i < ports.size(); i++) {
        ThrusterPort* p = ports[i];
        if (!p) continue;

        // p->loc is local offset (X,Y,Z) in ship space.
        // Convert to world using basis vectors explicitly:
        const FVector local = p->loc;

        const FVector projector =
            ship_loc +
            (Vrt * local.X) +
            (Vup * local.Y) +
            (Vpn * local.Z);

        if (p->flare)
            p->flare->MoveTo(projector);

        if (p->trail) {
            const double intensity = p->burn;

            if (intensity > 0.5 && !hide_all) {
                Bolt* t = (Bolt*)p->trail;
                const double len = -50.0 * p->scale * intensity;

                t->Show();

                switch (p->type) {
                case LEFT:     t->SetEndPoints(projector, projector + (Vrt * len)); break;
                case RIGHT:    t->SetEndPoints(projector, projector - (Vrt * len)); break;
                case AFT:      t->SetEndPoints(projector, projector + (Vpn * len)); break;
                case FORE:     t->SetEndPoints(projector, projector - (Vpn * len)); break;
                case BOTTOM:   t->SetEndPoints(projector, projector + (Vup * len)); break;
                case TOP:      t->SetEndPoints(projector, projector - (Vup * len)); break;
                default:       t->Hide(); break;
                }
            }
            else {
                p->trail->Hide();
                if (p->flare)
                    p->flare->Hide();
            }
        }
    }
}

// +--------------------------------------------------------------------+

void
Thruster::ExecFrame(double seconds)
{
    SimSystem::ExecFrame(seconds);

    if (!ship)
        return;

    double rr = 0.0, pr = 0.0, yr = 0.0;
    double rd = 0.0, pd = 0.0, yd = 0.0;

    // Rename locals to avoid shadowing any class members named "agility" or "stability":
    double agility_factor = 1.0;
    double stability_factor = 1.0;

    FlightComputer* flcs = ship->GetFLCS();

    if (flcs) {
        if (!flcs->IsPowerOn() || flcs->GetStatus() < SYSTEM_STATUS::DEGRADED) {
            agility_factor = 0.3;
            stability_factor = 0.0;
        }
    }

    // Check for thruster damage here:
    if (components.size() >= 3) {
        SYSTEM_STATUS stat = components[0]->GetStatus();
        if (stat == SYSTEM_STATUS::NOMINAL)       this->avail_x = 1.0f;
        else if (stat == SYSTEM_STATUS::DEGRADED) this->avail_x = 0.5f;
        else                                     this->avail_x = 0.0f;

        stat = components[1]->GetStatus();
        if (stat == SYSTEM_STATUS::NOMINAL)       this->avail_z = 1.0f;
        else if (stat == SYSTEM_STATUS::DEGRADED) this->avail_z = 0.5f;
        else                                     this->avail_z = 0.0f;

        stat = components[2]->GetStatus();
        if (stat == SYSTEM_STATUS::NOMINAL)       this->avail_y = 1.0f;
        else if (stat == SYSTEM_STATUS::DEGRADED) this->avail_y = 0.5f;
        else                                     this->avail_y = 0.0f;
    }

    // Thrust limited by power distribution:
    const float denom = (capacity > 0.0f) ? capacity : 1.0f;
    this->thrust = energy / denom;
    energy = 0.0f;

    if (this->thrust < 0.0f)
        this->thrust = 0.0f;

    agility_factor *= this->thrust;
    stability_factor *= this->thrust;

    rr = roll_rate * agility_factor * this->avail_y;
    pr = pitch_rate * agility_factor * this->avail_y;
    yr = yaw_rate * agility_factor * this->avail_x;

    rd = roll_drag * stability_factor * this->avail_y;
    pd = pitch_drag * stability_factor * this->avail_y;
    yd = yaw_drag * stability_factor * this->avail_x;

    ship->SetAngularRates(rr, pr, yr);
    ship->SetAngularDrag(rd, pd, yd);
}

// +--------------------------------------------------------------------+

void
Thruster::SetShip(Ship* S)
{
    const double RollSpeed = PI * 0.0400;
    const double PitchSpeed = PI * 0.0250;
    const double YawSpeed = PI * 0.0250;

    ship = S;

    if (ship) {
        ShipDesign* ShipDesignData = (ShipDesign*)ship->Design();

        trans_x = ShipDesignData->trans_x;
        trans_y = ShipDesignData->trans_y;
        trans_z = ShipDesignData->trans_z;

        roll_drag = ShipDesignData->roll_drag;
        pitch_drag = ShipDesignData->pitch_drag;
        yaw_drag = ShipDesignData->yaw_drag;

        roll_rate = (float)(ShipDesignData->roll_rate * PI / 180.0);
        pitch_rate = (float)(ShipDesignData->pitch_rate * PI / 180.0);
        yaw_rate = (float)(ShipDesignData->yaw_rate * PI / 180.0);

        const double Agility = ShipDesignData->agility;

        if (roll_rate == 0.0f) roll_rate = (float)(Agility * RollSpeed);
        if (pitch_rate == 0.0f) pitch_rate = (float)(Agility * PitchSpeed);
        if (yaw_rate == 0.0f) yaw_rate = (float)(Agility * YawSpeed);
    }
}
// +--------------------------------------------------------------------+

double
Thruster::TransXLimit()
{
    return trans_x * avail_x;
}

double
Thruster::TransYLimit()
{
    return trans_y * avail_y;
}

double
Thruster::TransZLimit()
{
    return trans_z * avail_z;
}

// +--------------------------------------------------------------------+

void
Thruster::ExecTrans(double x, double y, double z)
{
    if (!ship || (ship->IsAirborne() && ship->Class() != CLASSIFICATION::LCA)) {
        if (thruster_sound && thruster_sound->IsPlaying())
            thruster_sound->Stop();

        for (int i = 0; i < ports.size(); i++) {
            ThrusterPort* p = ports[i];
            if (p->flare) p->flare->Hide();
            if (p->trail) p->trail->Hide();
        }

        return;
    }

    bool sound_on = false;
    bool show_flare = true;

    if (ship->Rep() && ship->Rep()->Hidden())
        show_flare = false;

    if (ship->Class() == CLASSIFICATION::LCA &&
        ship->IsAirborne() &&
        ship->Velocity().Length() < 250 &&
        ship->AltitudeAGL() > ship->Radius() / 2) {

        sound_on = true;
        IncBurn(BOTTOM, TOP);
    }

    else if (!ship->IsAirborne()) {
        const double tx_limit = ship->Design()->trans_x;
        const double ty_limit = ship->Design()->trans_y;
        const double tz_limit = ship->Design()->trans_z;

        if (x < -0.15 * tx_limit)      IncBurn(RIGHT, LEFT);
        else if (x > 0.15 * tx_limit)  IncBurn(LEFT, RIGHT);
        else                           DecBurn(LEFT, RIGHT);

        if (y < -0.15 * ty_limit)      IncBurn(FORE, AFT);
        else if (y > 0.15 * ty_limit)  IncBurn(AFT, FORE);
        else                           DecBurn(FORE, AFT);

        if (z < -0.15 * tz_limit)      IncBurn(TOP, BOTTOM);
        else if (z > 0.15 * tz_limit)  IncBurn(BOTTOM, TOP);
        else                           DecBurn(TOP, BOTTOM);

        double r = 0, p = 0, yaw = 0;
        ship->GetAngularThrust(r, p, yaw);

        // Roll seems to have the opposite sign from
        // the pitch and yaw thrust factors.  Not sure why.

        if (r > 0)        IncBurn(ROLL_L, ROLL_R);
        else if (r < 0)   IncBurn(ROLL_R, ROLL_L);
        else              DecBurn(ROLL_R, ROLL_L);

        if (yaw < 0)      IncBurn(YAW_L, YAW_R);
        else if (yaw > 0) IncBurn(YAW_R, YAW_L);
        else              DecBurn(YAW_R, YAW_L);

        if (p < 0)        IncBurn(PITCH_D, PITCH_U);
        else if (p > 0)   IncBurn(PITCH_U, PITCH_D);
        else              DecBurn(PITCH_U, PITCH_D);
    }

    else {
        for (int i = 0; i < 12; i++) {
            burn[i] -= 0.1f;
            if (burn[i] < 0)
                burn[i] = 0.0f;
        }
    }

    for (int i = 0; i < ports.size(); i++) {
        ThrusterPort* p = ports[i];

        if (p->fire) {
            p->burn = 0;

            int flag = 1;

            for (int n = 0; n < 12; n++) {
                if ((p->fire & flag) != 0 && burn[n] > p->burn)
                    p->burn = burn[n];

                flag <<= 1;
            }
        }

        else {
            p->burn = burn[p->type];
        }

        if (p->burn > 0 && thrust > 0) {
            sound_on = true;

            if (show_flare) {
                Sprite* flare_rep = (Sprite*)p->flare;
                if (flare_rep) {
                    flare_rep->Show();
                    flare_rep->SetShade(1);
                }

                if (p->trail) {
                    Bolt* t = (Bolt*)p->trail;
                    t->Show();
                    t->SetShade(1);
                }
            }
        }
        else {
            if (p->flare) p->flare->Hide();
            if (p->trail) p->trail->Hide();
        }
    }

    // thruster sound:
    if (ship && ship == Sim::GetSim()->GetPlayerShip() && ports.size() > 0) {
        CameraManager* cam_dir = CameraManager::GetInstance();

        // no sound when paused!
        if (!Game::Paused() && cam_dir && cam_dir->GetCamera()) {
            if (!thruster_sound) {
                if (thruster_resource)
                    thruster_sound = thruster_resource->Duplicate();
            }

            if (thruster_sound) {
                if (sound_on) {
                    const FVector cam_loc = cam_dir->GetCamera()->Pos();
                    const double  dist = (ship->Location() - cam_loc).Size();

                    const long max_vol = AudioConfig::EfxVolume();
                    long       volume = -2000;

                    if (volume > max_vol)
                        volume = max_vol;

                    if (dist < thruster_sound->GetMaxDistance()) {
                        thruster_sound->SetLocation(ship->Location());
                        thruster_sound->SetVolume(volume);
                        thruster_sound->Play();
                    }
                    else if (thruster_sound->IsPlaying()) {
                        thruster_sound->Stop();
                    }
                }
                else if (thruster_sound->IsPlaying()) {
                    thruster_sound->Stop();
                }
            }
        }
    }

    ship->SetTransX(x * thrust);
    ship->SetTransY(y * thrust);
    ship->SetTransZ(z * thrust);
}

// +--------------------------------------------------------------------+

void
Thruster::AddPort(int ptype, const FVector& loc, DWORD fire, float flare_scale)
{
    if (flare_scale == 0)
        flare_scale = scale;

    ThrusterPort* port = new ThrusterPort(ptype, loc, fire, flare_scale);
    ports.append(port);
}

void
Thruster::CreatePort(int ptype, const FVector& loc, DWORD fire, float flare_scale)
{
    Bitmap* flare_bmp = drive_flare_bitmap[subtype];
    Bitmap* trail_bmp = drive_trail_bitmap[subtype];

    if (subtype != Drive::STEALTH) {

        Sprite* flare_rep = new Sprite(flare_bmp);
        flare_rep->Scale(flare_scale * 0.667f);
        flare_rep->SetShade(0);

        Bolt* trail_rep = new Bolt(flare_scale * 30, flare_scale * 8, trail_bmp, true);

        ThrusterPort* port = new ThrusterPort(ptype, loc, fire, flare_scale);
        port->flare = flare_rep;
        port->trail = trail_rep;
        ports.append(port);
    }
}

// +--------------------------------------------------------------------+

int
Thruster::NumThrusters() const
{
    return ports.size();
}

Graphic*
Thruster::Flare(int engine) const
{
    if (engine >= 0 && engine < ports.size())
        return ports[engine]->flare;

    return nullptr;
}

Graphic*
Thruster::Trail(int engine) const
{
    if (engine >= 0 && engine < ports.size())
        return ports[engine]->trail;

    return nullptr;
}

// +--------------------------------------------------------------------+

void
Thruster::IncBurn(int inc, int dec)
{
    burn[inc] += 0.1f;
    if (burn[inc] > 1)
        burn[inc] = 1.0f;

    burn[dec] -= 0.1f;
    if (burn[dec] < 0)
        burn[dec] = 0.0f;
}

void
Thruster::DecBurn(int a, int b)
{
    burn[a] -= 0.1f;
    if (burn[a] < 0)
        burn[a] = 0.0f;

    burn[b] -= 0.1f;
    if (burn[b] < 0)
        burn[b] = 0.0f;
}

// +----------------------------------------------------------------------+

double
Thruster::GetRequest(double seconds) const
{
    if (!power_on)
        return 0;

    for (int i = 0; i < 12; i++)
        if (burn[i] != 0)
            return power_level * sink_rate * seconds;

    return 0;
}
