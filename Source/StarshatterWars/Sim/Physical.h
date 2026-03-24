/*
    Project nGenEx
    Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Physical.h
    AUTHOR:       John DiCamillo

    OVERVIEW
    ========
    Abstract Physical Object
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Camera.h"

#include "Math/Vector.h"   // FVector

// +--------------------------------------------------------------------+

class SimDirector;
class Graphic;
class SimLight;

// +--------------------------------------------------------------------+

class Physical
{
public:
    static const char* TYPENAME() { return "Physical"; }

    Physical();
    Physical(const char* n, int t = 0);
    virtual ~Physical();

    int operator==(const Physical& p) const { return id == p.id; }

    // Integration Loop Control:
    static void       SetSubFrameLength(double seconds) { sub_frame = seconds; }
    static double     GetSubFrameLength() { return sub_frame; }

    // operations
    virtual void      ExecFrame(double seconds);
    virtual void      AeroFrame(double seconds);
    virtual void      ArcadeFrame(double seconds);

    virtual void      AngularFrame(double seconds);
    virtual void      LinearFrame(double seconds);

    virtual void      CalcFlightPath();

    virtual void      MoveTo(const FVector& NewLoc);
    virtual void      TranslateBy(const FVector& Ref);
    virtual void      ApplyForce(const FVector& Force);
    virtual void      ApplyTorque(const FVector& Torque);
    virtual void      SetThrust(double T);
    virtual void      SetTransX(double T);
    virtual void      SetTransY(double T);
    virtual void      SetTransZ(double T);
    virtual void      SetHeading(double R, double P, double Y);
    virtual void      LookAt(const FVector& Dst);
    virtual void      ApplyRoll(double RollAcc);
    virtual void      ApplyPitch(double PitchAcc);
    virtual void      ApplyYaw(double YawAcc);

    virtual int       CollidesWith(Physical& o);
    static  void      ElasticCollision(Physical& a, Physical& b);
    static  void      InelasticCollision(Physical& a, Physical& b);
    static  void      SemiElasticCollision(Physical& a, Physical& b);
    virtual void      InflictDamage(double Damage, int Type = 0);

    // accessors:
    int               Identity()  const { return id; }
    int               Type()      const { return obj_type; }
    const char* Name()      const { return name; }

    // NOTE:
    // Camera is a Starshatter/nGenEx core type. This class keeps Camera as-is.
    // Geometry's Point/Vec3 have been ported/aliased in Geometry.h in your project,
    // but this header uses FVector externally per the template.
    FVector           Location()  const { return cam.Pos(); }
    FVector           Heading()   const { return cam.vpn(); }
    FVector           LiftLine()  const { return cam.vup(); }
    FVector           BeamLine()  const { return cam.vrt(); }
    FVector           Velocity()  const { return velocity + arcade_velocity; }
    FVector           Acceleration() const { return accel; }

    double            Thrust()    const { return thrust; }
    double            TransX()    const { return trans_x; }
    double            TransY()    const { return trans_y; }
    double            TransZ()    const { return trans_z; }
    double            Drag()      const { return drag; }

    double            Roll()      const { return roll; }
    double            Pitch()     const { return pitch; }
    double            Yaw()       const { return yaw; }
    FVector           Rotation()  const { return FVector(dp, dr, dy); }

    double            Alpha()     const { return alpha; }

    double            FlightPathYawAngle()   const { return flight_path_yaw; }
    double            FlightPathPitchAngle() const { return flight_path_pitch; }

    double            Radius()    const { return radius; }
    double            Mass()      const { return mass; }
    double            Integrity() const { return integrity; }
    double            Life()      const { return life; }

    double            Shake()     const { return shake; }
    const FVector& Vibration() const { return vibration; }

    const Camera& Cam()       const { return cam; }
    Graphic* Rep()       const { return rep; }
    SimLight* LightSrc()  const { return light; }

    SimDirector* GetDirector() const { return dir; }

    // mutators:
    virtual void      SetAngularRates(double R, double P, double Y);
    virtual void      GetAngularRates(double& R, double& P, double& Y);
    virtual void      SetAngularDrag(double R, double P, double Y);
    virtual void      GetAngularDrag(double& R, double& P, double& Y);
    virtual void      GetAngularThrust(double& R, double& P, double& Y);
    virtual void      SetVelocity(const FVector& V) { velocity = V; }
    virtual void      SetAbsoluteOrientation(double InRoll, double InPitch, double InYaw);
    virtual void      CloneCam(const Camera& InCam);
    virtual void      SetDrag(double D) { drag = (float)D; }

    virtual void      SetPrimary(const FVector& Loc, double InMass);
    virtual void      SetGravity(double G);
    virtual void      SetBaseDensity(double D);

    virtual double    GetBaseDensity() const { return Do; }
    virtual double    GetDensity() const;

    enum { NAMELEN = 48 };

protected:
    static int        id_key;

    // identification:
    int               id;
    int               obj_type;
    char              name[NAMELEN];

    // position, velocity, and acceleration:
    Camera            cam;
    FVector           velocity;
    FVector           arcade_velocity;
    FVector           accel;
    float             thrust;
    float             trans_x;
    float             trans_y;
    float             trans_z;
    float             drag;

    // attitude and angular velocity:
    float             roll, pitch, yaw;
    float             dr, dp, dy;
    float             dr_acc, dp_acc, dy_acc;
    float             dr_drg, dp_drg, dy_drg;

    float             flight_path_yaw;
    float             flight_path_pitch;

    // gravitation:
    FVector           primary_loc;
    double            primary_mass;

    // aerodynamics:
    float             g_accel;    // acceleration due to gravity (constant)
    float             Do;         // atmospheric density at sea level
    float             CL;         // base coefficient of lift
    float             CD;         // base coefficient of drag
    float             alpha;      // current angle of attack (radians)
    float             stall;      // stall angle of attack (radians)
    bool              lat_thrust; // lateral thrusters enabled in aero mode?
    bool              straight;

    // vibration:
    float             shake;
    FVector           vibration;

    // scale factors for ApplyXxx():
    float             roll_rate, pitch_rate, yaw_rate;

    // physical properties:
    double            life;
    float             radius;
    float             mass;
    float             integrity;

    // graphic representation:
    Graphic* rep;
    SimLight* light;

    // AI or human controller:
    SimDirector* dir;        // null implies an autonomous object

    static double     sub_frame;
};
