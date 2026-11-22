/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Explosion.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Explosion Sprite animation class
*/

#include "Explosion.h"

#include "ParticleManager.h"
#include "Ship.h"
#include "Sim.h"
#include "CameraManager.h"
#include "AudioConfig.h"

#include "SimLight.h"
#include "Sprite.h"
#include "Solid.h"
#include "DataLoader.h"
#include "Game.h"
#include "SimScene.h"
#include "GameStructs_System.h"
// Unreal logging (for legacy Print replacement):
#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+
//
// NOTE:
// - MemDebug.h removed (Unreal build).
// - Bitmap assets replaced with UTexture2D* (forward-declared here to keep includes light).
// - All legacy Print(...) converted to UE_LOG(...).
// - Vec3/Point migrated to FVector; minimal include added.
// +--------------------------------------------------------------------+

#include "Math/Vector.h"

class UTexture2D;

// +--------------------------------------------------------------------+

DEFINE_LOG_CATEGORY_STATIC(LogTempExplos, Log, All);

// +--------------------------------------------------------------------+

namespace { uint64 NextExplosionPresentationSerial=0;TArray<FExplosionPresentation> ExplosionPresentationEvents; }
const int MAX_EXPLOSION_TYPES = 32;

static float      lifetimes[MAX_EXPLOSION_TYPES];
static float      scales[MAX_EXPLOSION_TYPES];

static Bitmap* bitmaps[MAX_EXPLOSION_TYPES];
static Bitmap* particle_bitmaps[MAX_EXPLOSION_TYPES];

static int        part_frames[MAX_EXPLOSION_TYPES];
static int        lengths[MAX_EXPLOSION_TYPES];
static float      light_levels[MAX_EXPLOSION_TYPES];
static float      light_decays[MAX_EXPLOSION_TYPES];
static FColor     light_colors[MAX_EXPLOSION_TYPES];
static int        num_parts[MAX_EXPLOSION_TYPES];
static int        part_types[MAX_EXPLOSION_TYPES];
static float      part_speeds[MAX_EXPLOSION_TYPES];
static float      part_drags[MAX_EXPLOSION_TYPES];
static float      part_scales[MAX_EXPLOSION_TYPES];
static float      part_blooms[MAX_EXPLOSION_TYPES];
static float      part_rates[MAX_EXPLOSION_TYPES];
static float      part_decays[MAX_EXPLOSION_TYPES];
static bool       part_trails[MAX_EXPLOSION_TYPES];
static int        part_alphas[MAX_EXPLOSION_TYPES];
static USound*    sounds[MAX_EXPLOSION_TYPES];
static bool       recycles[MAX_EXPLOSION_TYPES];

// +--------------------------------------------------------------------+

Explosion::Explosion(EExplosionType InType, const FVector& InPos, const FVector& InVel,
    float InExplosionScale, float InParticleScale,
    SimRegion* InRegion, SimObject* InSource)
    : SimObject("Explosion", ESimObject::EXPLOSION)
    , type(InType)
    , particles(nullptr)
    , source(InSource)
{
    PresentationSerial=++NextExplosionPresentationSerial;
    PresentationExplosionScale=InExplosionScale;PresentationParticleScale=InParticleScale;
    Observe(source);

    MoveTo(InPos);
    velocity = InVel;
    drag = 0.3f;
    rep = nullptr;
    light = nullptr;
    life = 0.0;

    // Default relative mount offset:
    mount_rel = FVector::ZeroVector;

    if (type == EExplosionType::QUANTUM_FLASH) {
        life = 1.1;

        // The UE QuantumEffectsActor supplies the flash; no legacy mesh or PCX.

        light = new SimLight(1e9, 0.66f);
        light->SetColor(FColor(180, 200, 255, 255));
    }
    else if (type >= EExplosionType::NONE && type < EExplosionType::MAX) {
        life = lifetimes[(int)type];

        // ------------------------------------------------------------
        // Compute mount_rel in SOURCE-LOCAL space (UE-safe math)
        // Old code: mount_rel = (pos - source->Location()) * src_orient;
        // where src_orient was transposed so it behaved like an inverse basis.
        //
        // UE fix: project delta onto camera basis vectors with dot products.
        if (source) {
            const Camera& SrcCam = source->GetCam();
            const FVector Delta = InPos - source->GetLocation();

            mount_rel.X = FVector::DotProduct(Delta, SrcCam.vrt());
            mount_rel.Y = FVector::DotProduct(Delta, SrcCam.vup());
            mount_rel.Z = FVector::DotProduct(Delta, SrcCam.vpn());
        }

        // ------------------------------------------------------------
        // Optional sprite rep
        if (lengths[(int)type] > 0) {
            const bool bRepeat = (lengths[(int)type] == 1);

            Sprite* SpriteRep = new Sprite(bitmaps[(int)type], lengths[(int)type], bRepeat);
            SpriteRep->Scale(InExplosionScale * scales[(int)type]);
            SpriteRep->SetAngle(PI * (double)rand() / 16384.0);
            SpriteRep->SetLuminous(true);
            rep = SpriteRep;
        }

        // ------------------------------------------------------------
        // Optional light rep
        if (light_levels[(int)type] > 0) {
            light = new SimLight(light_levels[(int)type], light_decays[(int)type]);
            light->SetColor(light_colors[(int)type]); // ensure this is FColor
        }

        // ------------------------------------------------------------
        // Optional particle burst
        if (num_parts[(int)type] > 0) {
            particles = new ParticleManager(
                particle_bitmaps[(int)type],
                num_parts[(int)type],
                InPos,
                FVector::ZeroVector,
                part_speeds[(int)type] * InParticleScale,
                part_drags[(int)type],
                part_scales[(int)type] * InParticleScale,
                part_blooms[(int)type] * InParticleScale,
                part_decays[(int)type],
                part_rates[(int)type],
                recycles[(int)type],
                part_trails[(int)type],
                (InRegion && InRegion->IsAirSpace()),
                part_alphas[(int)type],
                part_frames[(int)type]
            );
        }
    }

    if (rep) {
        rep->MoveTo(InPos);
    }

    if (light) {
        light->MoveTo(InPos);
    }
}


FExplosionPresentation Explosion::GetPresentation() const
{
 FExplosionPresentation P;P.Serial=PresentationSerial;P.Type=type;P.Location=GetLocation();
 P.ExplosionScale=PresentationExplosionScale;P.ParticleScale=PresentationParticleScale;P.Lifetime=GetLife();P.Region=GetRegion();
 Sim* S=Sim::GetSim();Ship* Player=S?S->GetPlayerShip():nullptr;
 P.bHidden=Player && source==Player && CameraManager::GetCameraMode()==CameraManager::MODE_COCKPIT && !Player->IsDying();
 return P;
}
TArray<FExplosionPresentation> Explosion::ConsumePresentationEvents(){TArray<FExplosionPresentation> Result=MoveTemp(ExplosionPresentationEvents);ExplosionPresentationEvents.Reset();return Result;}
void Explosion::QueueMuzzleFlash(const FVector& Position,const FVector& Direction,float FlashScale,SimRegion* Region,const Ship* Shooter,const FString& FlashId)
{
 if(FlashScale<=0)return;
 FExplosionPresentation P;P.Serial=++NextExplosionPresentationSerial;P.Location=Position;P.Direction=Direction;
 P.ExplosionScale=P.ParticleScale=FlashScale;P.Region=Region;P.bMuzzleFlash=true;P.Shooter=Shooter;P.FlashId=FlashId;
 ExplosionPresentationEvents.Add(P);
}
void Explosion::QueuePresentationEvent() const{ExplosionPresentationEvents.Add(GetPresentation());}
void Explosion::ClearPresentationEvents(){ExplosionPresentationEvents.Reset();}


// +--------------------------------------------------------------------+

Explosion::~Explosion()
{
    GRAPHIC_DESTROY(particles);
}

// +--------------------------------------------------------------------+

bool
Explosion::Update(SimObject* obj)
{
    if (obj == source) {
        source = 0;

        if (life < 0 || life > 4)
            life = 4;

        if (particles)
            particles->StopEmitting();
    }

    return SimObserver::Update(obj);
}

const char*
Explosion::GetObserverName() const
{
    static char NameBuffer[128];

    if (source && source->GetName()) {
        // UE-safe formatting, no __FILE__/__LINE__, no wide chars
        snprintf(NameBuffer, sizeof(NameBuffer),
            "Explosion(%s)", source->GetName());
    }
    else {
        snprintf(NameBuffer, sizeof(NameBuffer), "Explosion");
    }

    return NameBuffer;
}


// +--------------------------------------------------------------------+

void
Explosion::Initialize()
{
    static int initialized = 0;
    if (initialized) return;

    ZeroMemory(lifetimes, sizeof(lifetimes));
    ZeroMemory(scales, sizeof(scales));
    ZeroMemory(bitmaps, sizeof(bitmaps));
    ZeroMemory(particle_bitmaps, sizeof(particle_bitmaps));
    ZeroMemory(lengths, sizeof(lengths));
    ZeroMemory(light_levels, sizeof(light_levels));
    ZeroMemory(light_decays, sizeof(light_decays));
    ZeroMemory(light_colors, sizeof(light_colors));
    ZeroMemory(num_parts, sizeof(num_parts));
    ZeroMemory(part_types, sizeof(part_types));
    ZeroMemory(part_speeds, sizeof(part_speeds));
    ZeroMemory(part_drags, sizeof(part_drags));
    ZeroMemory(part_scales, sizeof(part_scales));
    ZeroMemory(part_blooms, sizeof(part_blooms));
    ZeroMemory(part_decays, sizeof(part_decays));
    ZeroMemory(part_rates, sizeof(part_rates));
    ZeroMemory(part_trails, sizeof(part_trails));
    ZeroMemory(part_alphas, sizeof(part_alphas));
    ZeroMemory(sounds, sizeof(sounds));
    ZeroMemory(recycles, sizeof(recycles));

    // Legacy Explosions.def is not shipped. Presentation is handled by native UE
    // effects; retain the existing zeroed optional legacy sprite/particle state.
    // Quantum flashes retain their explicit simulation lifetime above.
    initialized = 1;
}

// +--------------------------------------------------------------------+

void
Explosion::Close()
{
    ClearPresentationEvents();
    // Texture lifetime is expected to be managed by Unreal / DataLoader, notxmanual delete.
    for (int t = 0; t < MAX_EXPLOSION_TYPES; t++) {
        bitmaps[t] = 0;
        particle_bitmaps[t] = 0;

        if (sounds[t]) sounds[t]->Release();
        sounds[t] = 0;
    }
}

// +--------------------------------------------------------------------+

void
Explosion::ExecFrame(double DeltaSeconds)
{
    // Follow the source attachment (UE-friendly vector math: no Matrix * Vector operator assumptions)
    if (source) {
        const Camera& SrcCam = source->GetCam();

        // Build a rotation matrix from the camera basis vectors.
        // Assumes Cam.vrt/vup/vpn return normalized world-space basis vectors as FVector.
        const FMatrix Basis(
            FPlane(SrcCam.vrt(), 0.0f),
            FPlane(SrcCam.vup(), 0.0f),
            FPlane(SrcCam.vpn(), 0.0f),
            FPlane(0.0f, 0.0f, 0.0f, 1.0f)
        );

        // Transform mount_rel from source-local into world using the basis:
        const FVector WorldOffset = Basis.TransformVector(mount_rel);

        MoveTo(WorldOffset + source->GetLocation());

        if (rep)       rep->Show();
        if (particles) particles->Show();

        // Hide in cockpit view when attached to player and not dying:
        Sim* SimInst = Sim::GetSim();
        Ship* PlayerShip = SimInst ? SimInst->GetPlayerShip() : nullptr;

        if (PlayerShip && source == PlayerShip) {
            if (CameraManager::GetCameraMode() == CameraManager::MODE_COCKPIT &&
                !PlayerShip->IsDying()) {
                if (rep)       rep->Hide();
                if (particles) particles->Hide();
            }
        }
    }

    life -= DeltaSeconds;

    // ------------------------------------------------------------
    // Visual rep updates
    if (rep) {
        rep->MoveTo(GetLocation());

        if (rep->Life() == 0) {
            rep = nullptr; // about to be GC'd / owned elsewhere
        }
        else if (rep->IsSprite()) {
            Sprite* SpriteRep = (Sprite*)rep;
            SpriteRep->SetAngle(SpriteRep->Angle() + DeltaSeconds * 0.5);
        }

    }

    // ------------------------------------------------------------
    // Light updates
    if (light) {
        light->Update();

        if (light->Life() == 0) {
            light = nullptr; // about to be GC'd / owned elsewhere
        }
    }

    // ------------------------------------------------------------
    // Particle updates
    if (particles) {
        particles->MoveTo(GetLocation());
        particles->ExecFrame(DeltaSeconds);
    }

    // If source died, end this explosion immediately
    if (source && source->GetLife() == 0) {
        life = 0;
    }
}


// +--------------------------------------------------------------------+

void
Explosion::Activate(SimScene& scene)
{
    bool filter = false;

    CameraManager* cam_dir = CameraManager::GetInstance();
    if (cam_dir && cam_dir->GetCamera()) {
        if (FVector(cam_dir->GetCamera()->Pos() - GetLocation()).Length() < 100.0)
            filter = true;
    }

    if (rep && !filter)
        scene.AddGraphic(rep);

    if (light)
        scene.AddLight(light);

    if (particles && !filter)
        scene.AddGraphic(particles);

    if (sounds[(int)type]) {
        USound* sound = sounds[(int)type]->Duplicate();

        // fire and forget:
        if (sound) {
            sound->SetLocation(FVector(GetLocation().Z,GetLocation().X,GetLocation().Y));
            sound->SetVolume(AudioConfig::EfxVolume());
            sound->PlayAndRelease();
        }
    }

    active = true;
}

void
Explosion::Deactivate(SimScene& scene)
{
    SimObject::Deactivate(scene);

    if (particles)
        scene.DelGraphic(particles);
}

// +--------------------------------------------------------------------+
