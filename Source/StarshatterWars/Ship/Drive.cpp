/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Drive.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Drive (system) class
*/

#include "Drive.h"

#include "Power.h"
#include "Ship.h"
#include "Sim.h"
#include "DriveSprite.h"
#include "CameraManager.h"
#include "AudioConfig.h"

#include "SimLight.h"
#include "Bitmap.h"
#include "Sound.h"
#include "DataLoader.h"
#include "Bolt.h"
#include "Solid.h"
#include "Game.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

static int drive_value[] =
{
    1, 1, 1, 1, 1, 1, 1, 1
};

static float drive_light[] =
{
    10.0f, 100.0f, 5.0f, 1.0e3f, 100.0f, 10.0f, 0.0f, 0.0f
};

Bitmap* drive_flare_bitmap[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
Bitmap* drive_trail_bitmap[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
Bitmap* drive_glow_bitmap[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

static USound* sound_resource[3] = { nullptr, nullptr, nullptr };

#define CLAMP(x, a, b) if ((x) < (a)) (x) = (a); else if ((x) > (b)) (x) = (b);

// +----------------------------------------------------------------------+

DrivePort::DrivePort(const FVector& InLoc, float InScale)
    : loc(InLoc),
    scale(InScale),
    flare(nullptr),
    trail(nullptr)
{
}

DrivePort::~DrivePort()
{
    GRAPHIC_DESTROY(flare);
    GRAPHIC_DESTROY(trail);
}

// +----------------------------------------------------------------------+

Drive::Drive(SUBTYPE InSubtype, float MaxThrust, float MaxAug, bool bShow)
    : SimSystem(SYSTEM_CATEGORY::DRIVE, InSubtype, "Drive", drive_value[InSubtype],
        MaxThrust * 2.0f, MaxThrust * 2.0f, MaxThrust * 2.0f),
    thrust(MaxThrust),
    augmenter(MaxAug),
    scale(0.0f),
    throttle(0.0f),
    augmenter_throttle(0.0f),
    intensity(0.0f),
    sound(nullptr),
    burner_sound(nullptr),
    show_trail(bShow)
{
    power_flags = POWER_WATTS;

    switch (InSubtype) {
    default:
    case PLASMA:  name = Game::GetText("sys.drive.plasma");  break;
    case FUSION:  name = Game::GetText("sys.drive.fusion");  break;
    case GREEN:   name = Game::GetText("sys.drive.green");   break;
    case RED:     name = Game::GetText("sys.drive.red");     break;
    case BLUE:    name = Game::GetText("sys.drive.blue");    break;
    case YELLOW:  name = Game::GetText("sys.drive.yellow");  break;
    case STEALTH: name = Game::GetText("sys.drive.stealth"); break;
    }

    abrv = Game::GetText("sys.drive.abrv");

    emcon_power[0] = 0;
    emcon_power[1] = 50;
    emcon_power[2] = 100;
}

// +----------------------------------------------------------------------+

Drive::Drive(const Drive& d)
    : SimSystem(d),
    thrust(d.thrust),
    augmenter(d.augmenter),
    scale(d.scale),
    throttle(0.0f),
    augmenter_throttle(0.0f),
    intensity(0.0f),
    sound(nullptr),
    burner_sound(nullptr),
    show_trail(d.show_trail)
{
    power_flags = POWER_WATTS;

    Mount(d);

    if (subtype != Drive::STEALTH) {
        for (int i = 0; i < d.ports.size(); i++) {
            DrivePort* p = d.ports[i];
            CreatePort(p->loc, p->scale);
        }
    }
}

// +--------------------------------------------------------------------+

Drive::~Drive()
{
    if (sound) {
        sound->Stop();
        sound->Release();
        sound = nullptr;
    }

    if (burner_sound) {
        burner_sound->Stop();
        burner_sound->Release();
        burner_sound = nullptr;
    }

    ports.destroy();
}

// +--------------------------------------------------------------------+

void
Drive::Initialize()
{
    static int initialized = 0;
    if (initialized) return;

    DataLoader* loader = DataLoader::GetLoader();
    loader->SetDataPath("Drive/");

    loader->LoadTexture("Drive0.pcx", drive_flare_bitmap[0], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Drive1.pcx", drive_flare_bitmap[1], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Drive2.pcx", drive_flare_bitmap[2], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Drive3.pcx", drive_flare_bitmap[3], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Drive4.pcx", drive_flare_bitmap[4], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Drive5.pcx", drive_flare_bitmap[5], Bitmap::BMP_TRANSLUCENT);

    loader->LoadTexture("Trail0.pcx", drive_trail_bitmap[0], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Trail1.pcx", drive_trail_bitmap[1], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Trail2.pcx", drive_trail_bitmap[2], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Trail3.pcx", drive_trail_bitmap[3], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Trail4.pcx", drive_trail_bitmap[4], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Trail5.pcx", drive_trail_bitmap[5], Bitmap::BMP_TRANSLUCENT);

    loader->LoadTexture("Glow0.pcx", drive_glow_bitmap[0], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Glow1.pcx", drive_glow_bitmap[1], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Glow2.pcx", drive_glow_bitmap[2], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Glow3.pcx", drive_glow_bitmap[3], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Glow4.pcx", drive_glow_bitmap[4], Bitmap::BMP_TRANSLUCENT);
    loader->LoadTexture("Glow5.pcx", drive_glow_bitmap[5], Bitmap::BMP_TRANSLUCENT);

    const int SOUND_FLAGS = USound::LOCALIZED |
        USound::LOC_3D |
        USound::LOOP |
        USound::LOCKED;

    loader->SetDataPath("Sounds/");
    loader->LoadSound("engine.wav", sound_resource[0], SOUND_FLAGS);
    loader->LoadSound("burner2.wav", sound_resource[1], SOUND_FLAGS);
    loader->LoadSound("rumble.wav", sound_resource[2], SOUND_FLAGS);
    loader->SetDataPath("");

    if (sound_resource[0])
        sound_resource[0]->SetMaxDistance(30.0e3f);

    if (sound_resource[1])
        sound_resource[1]->SetMaxDistance(30.0e3f);

    if (sound_resource[2])
        sound_resource[2]->SetMaxDistance(50.0e3f);

    initialized = 1;
}

// +--------------------------------------------------------------------+

void
Drive::Close()
{
    for (int i = 0; i < 3; i++) {
        delete sound_resource[i];
        sound_resource[i] = nullptr;
    }
}

// +--------------------------------------------------------------------+

void
Drive::StartFrame()
{
}

// +--------------------------------------------------------------------+

void
Drive::AddPort(const FVector& InLoc, float FlareScale)
{
    if (FlareScale == 0)
        FlareScale = scale;

    DrivePort* Port = new DrivePort(InLoc, FlareScale);
    ports.append(Port);
}

// +--------------------------------------------------------------------+

void
Drive::CreatePort(const FVector& InLoc, float FlareScale)
{
    Bitmap* FlareBmp = drive_flare_bitmap[subtype];
    Bitmap* TrailBmp = drive_trail_bitmap[subtype];
    Bitmap* GlowBmp = nullptr;

    if (FlareScale <= 0)
        FlareScale = scale;

    if (augmenter <= 0)
        GlowBmp = drive_glow_bitmap[subtype];

    if (subtype != Drive::STEALTH && FlareScale > 0) {
        DrivePort* Port = new DrivePort(InLoc, FlareScale);

        if (FlareBmp) {
            DriveSprite* FlareRep = new DriveSprite(FlareBmp, GlowBmp);
            FlareRep->Scale(FlareScale * 1.5f);
            FlareRep->SetShade(0);
            Port->flare = FlareRep;
        }

        if (TrailBmp && show_trail) {
            Bolt* TrailRep = new Bolt(FlareScale * 30.0f, FlareScale * 8.0f, TrailBmp, true);
            Port->trail = TrailRep;
        }

        ports.append(Port);
    }
}

// +--------------------------------------------------------------------+

void
Drive::Orient(const Physical* rep)
{
    SimSystem::Orient(rep);

    const FVector ShipLoc = rep->Location();

    // Use explicit basis-vector transform (avoids Matrix layout/handedness bugs):
    const FVector Vrt = rep->Cam().vrt();
    const FVector Vup = rep->Cam().vup();
    const FVector Vpn = rep->Cam().vpn();

    for (int i = 0; i < ports.size(); i++) {
        DrivePort* p = ports[i];

        const FVector Local = p->loc;

        const FVector Projector =
            ShipLoc +
            (Vrt * Local.X) +
            (Vup * Local.Y) +
            (Vpn * Local.Z);

        if (p->flare) {
            p->flare->MoveTo(Projector);
            p->flare->SetFront(Vpn * (-10.0f * p->scale));
        }

        if (p->trail) {
            if (intensity > 0.5f) {
                double Len = -60.0 * p->scale * intensity;

                if (augmenter > 0 && augmenter_throttle > 0)
                    Len += Len * augmenter_throttle;

                p->trail->Show();
                p->trail->SetEndPoints(Projector, Projector + (Vpn * (float)Len));
            }
            else {
                p->trail->Hide();
            }
        }
    }
}

// +--------------------------------------------------------------------+

static double drive_seconds = 0;

// +--------------------------------------------------------------------+

void
Drive::SetThrottle(double t, bool aug)
{
    const double Spool = 1.2 * drive_seconds;
    const double ThrottleRequest = t / 100.0;

    if (throttle < ThrottleRequest) {
        if (ThrottleRequest - throttle < Spool) {
            throttle = (float)ThrottleRequest;
        }
        else {
            throttle += (float)Spool;
        }
    }
    else if (throttle > ThrottleRequest) {
        if (throttle - ThrottleRequest < Spool) {
            throttle = (float)ThrottleRequest;
        }
        else {
            throttle -= (float)Spool;
        }
    }

    if (throttle < 0.5f)
        aug = false;

    if (aug && augmenter_throttle < 1.0f) {
        augmenter_throttle += (float)Spool;

        if (augmenter_throttle > 1.0f)
            augmenter_throttle = 1.0f;
    }
    else if (!aug && augmenter_throttle > 0.0f) {
        augmenter_throttle -= (float)Spool;

        if (augmenter_throttle < 0.0f)
            augmenter_throttle = 0.0f;
    }
}

// +----------------------------------------------------------------------+

double
Drive::GetRequest(double seconds) const
{
    if (!power_on) return 0;

    const double TFactor = FMath::Max(throttle + (0.5 * augmenter_throttle), 0.3);

    return TFactor * power_level * sink_rate * seconds;
}

bool
Drive::IsAugmenterOn() const
{
    return augmenter > 0 &&
        augmenter_throttle > 0.05f &&
        IsPowerOn() &&
        Status > SYSTEM_STATUS::CRITICAL;
}

// +--------------------------------------------------------------------+

int
Drive::NumEngines() const
{
    return ports.size();
}

DriveSprite*
Drive::GetFlare(int port) const
{
    if (port >= 0 && port < ports.size()) {
        return ports[port]->flare;
    }

    return nullptr;
}

Bolt*
Drive::GetTrail(int port) const
{
    if (port >= 0 && port < ports.size()) {
        return ports[port]->trail;
    }

    return nullptr;
}

// +--------------------------------------------------------------------+

float
Drive::Thrust(double seconds)
{
    drive_seconds = seconds;

    const float Denom = (capacity > 0.0f) ? capacity : 1.0f;
    float eff = (energy / Denom) * availability * 100.0f;

    float output = throttle * thrust * eff;
    bool  aug_on = IsAugmenterOn();

    if (aug_on) {
        output += augmenter * augmenter_throttle * eff;

        // augmenter burns extra fuel:
        PowerSource* reac = ship->Reactors()[source_index];
        reac->SetCapacity(reac->GetCapacity() - (0.1 * drive_seconds));
    }

    energy = 0.0f;

    if (output < 0 || GetPowerLevel() < 0.01)
        output = 0.0f;

    int    vol = -10000;
    int    vol_aug = -10000;
    double fraction = (thrust != 0.0f) ? (output / thrust) : 0.0;

    for (int i = 0; i < ports.size(); i++) {
        DrivePort* p = ports[i];

        if (p->flare) {
            if (i == 0) {
                if (fraction > 0)
                    intensity += (float)seconds;
                else
                    intensity -= (float)seconds;

                // capture volume based on actual output:
                CLAMP(intensity, 0.0f, 1.0f);

                if (intensity > 0.25f) {
                    vol = (int)((intensity - 1.0f) * 10000.0f);
                    CLAMP(vol, -10000, -1500);

                    if (aug_on && intensity > 0.5f) {
                        vol_aug = (int)((5.0f * augmenter_throttle - 1.0f) * 10000.0f);
                        CLAMP(vol_aug, -10000, -1000);
                    }
                }
            }

            p->flare->SetShade(intensity);
        }

        if (p->trail) {
            p->trail->SetShade(intensity);
        }
    }

    CameraManager* cam_dir = CameraManager::GetInstance();

    // no sound when paused!
    if (!Game::Paused() && subtype != STEALTH && cam_dir && cam_dir->GetCamera()) {
        if (ship && ship->GetRegion() == Sim::GetSim()->GetActiveRegion()) {
            if (!sound) {
                int sound_index = 0;
                if (thrust > 100)
                    sound_index = 2;

                if (sound_resource[sound_index])
                    sound = sound_resource[sound_index]->Duplicate();
            }

            if (aug_on && !burner_sound) {
                if (sound_resource[1])
                    burner_sound = sound_resource[1]->Duplicate();
            }

            const FVector CamLoc = cam_dir->GetCamera()->Pos();
            const double Dist = (ship->Location() - CamLoc).Size();

            if (sound && Dist < sound->GetMaxDistance()) {
                long max_vol = AudioConfig::EfxVolume();

                if (vol > max_vol)
                    vol = max_vol;

                sound->SetLocation(ship->Location());
                sound->SetVolume(vol);
                sound->Play();

                if (burner_sound) {
                    if (vol_aug > max_vol)
                        vol_aug = max_vol;

                    burner_sound->SetLocation(ship->Location());
                    burner_sound->SetVolume(vol_aug);
                    burner_sound->Play();
                }
            }
            else {
                if (sound && sound->IsPlaying())
                    sound->Stop();

                if (burner_sound && burner_sound->IsPlaying())
                    burner_sound->Stop();
            }
        }
        else {
            if (sound && sound->IsPlaying())
                sound->Stop();

            if (burner_sound && burner_sound->IsPlaying())
                burner_sound->Stop();
        }
    }

    return output;
}
