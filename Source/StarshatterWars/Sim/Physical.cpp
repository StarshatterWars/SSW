/*  Project nGenEx
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Physical.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Abstract Physical Object
*/

#include "Physical.h"
#include "Graphic.h"
#include "SimLight.h"
#include "SimDirector.h"
#include "GameStructs.h"

// Unreal (minimal):
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

// +--------------------------------------------------------------------+

int    Physical::id_key = 1;
double Physical::sub_frame = 1.0 / 60.0;

// Local helpers to preserve Starshatter-style semantics while using FVector:

static inline double NormalizeAndReturnLength(FVector& V)
{
	const double Len = (double)V.Size();
	if (Len > 1e-9)
	{
		V /= (float)Len;
		return Len;
	}
	V = FVector::ZeroVector;
	return 0.0;
}

static inline bool IsFiniteVector(const FVector& V)
{
	return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
}

static inline double RandomSigned()
{
	// legacy behavior: rand() - 16384
	return (double)(std::rand() - 16384);
}

// +--------------------------------------------------------------------+

Physical::Physical()
	: id(id_key++)
	, obj_type(0)
	, rep(0)
	, light(0)
	, velocity(FVector::ZeroVector)
	, arcade_velocity(FVector::ZeroVector)
	, accel(FVector::ZeroVector)
	, thrust(0.0f)
	, trans_x(0.0f)
	, trans_y(0.0f)
	, trans_z(0.0f)
	, drag(0.0f)
	, roll(0.0f)
	, pitch(0.0f)
	, yaw(0.0f)
	, dr(0.0f)
	, dp(0.0f)
	, dy(0.0f)
	, dr_acc(0.0f)
	, dp_acc(0.0f)
	, dy_acc(0.0f)
	, dr_drg(0.0f)
	, dp_drg(0.0f)
	, dy_drg(0.0f)
	, flight_path_yaw(0.0f)
	, flight_path_pitch(0.0f)
	, primary_loc(FVector::ZeroVector)
	, primary_mass(0)
	, g_accel(0.0f)
	, Do(0.0f)
	, CL(0.0f)
	, CD(0.0f)
	, alpha(0.0f)
	, stall(0.0f)
	, lat_thrust(false)
	, straight(false)
	, shake(0.0f)
	, vibration(FVector::ZeroVector)
	, roll_rate(1.0f)
	, pitch_rate(1.0f)
	, yaw_rate(1.0f)
	, life(-1.0)
	, radius(0.0f)
	, mass(1.0f)
	, integrity(1.0f)
	, dir(0)
{
	std::strcpy(name, "unknown object");
}

// +--------------------------------------------------------------------+

Physical::Physical(const char* n, int t)
	: id(id_key++)
	, obj_type(t)
	, rep(0)
	, light(0)
	, velocity(FVector::ZeroVector)
	, arcade_velocity(FVector::ZeroVector)
	, accel(FVector::ZeroVector)
	, thrust(0.0f)
	, trans_x(0.0f)
	, trans_y(0.0f)
	, trans_z(0.0f)
	, drag(0.0f)
	, roll(0.0f)
	, pitch(0.0f)
	, yaw(0.0f)
	, dr(0.0f)
	, dp(0.0f)
	, dy(0.0f)
	, dr_acc(0.0f)
	, dp_acc(0.0f)
	, dy_acc(0.0f)
	, dr_drg(0.0f)
	, dp_drg(0.0f)
	, dy_drg(0.0f)
	, flight_path_yaw(0.0f)
	, flight_path_pitch(0.0f)
	, primary_loc(FVector::ZeroVector)
	, primary_mass(0)
	, g_accel(0.0f)
	, Do(0.0f)
	, CL(0.0f)
	, CD(0.0f)
	, alpha(0.0f)
	, stall(0.0f)
	, lat_thrust(false)
	, straight(false)
	, shake(0.0f)
	, vibration(FVector::ZeroVector)
	, roll_rate(1.0f)
	, pitch_rate(1.0f)
	, yaw_rate(1.0f)
	, life(-1.0)
	, radius(0.0f)
	, mass(1.0f)
	, integrity(1.0f)
	, dir(0)
{
	std::strncpy(name, n ? n : "unknown object", NAMELEN - 1);
	name[NAMELEN - 1] = 0;
}

// +--------------------------------------------------------------------+

Physical::~Physical()
{
	// inform graphic rep and light that we are leaving:
	GRAPHIC_DESTROY(rep);
	SIMLIGHT_DESTROY(light);

	// we own the director
	delete dir;
	dir = 0;
}

// +--------------------------------------------------------------------+

void
Physical::ExecFrame(double s)
{
	const FVector OrigVelocity = Velocity();
	arcade_velocity = FVector::ZeroVector;

	// if this object is under direction,
	// but doesn't need subframe accuracy,
	// update the control parameters:
	if (dir && !dir->Subframe())
		dir->ExecFrame(s);

	// decrement life before destroying the frame time:
	if (life > 0)
		life -= s;

	// integrate equations using slices no larger than sub_frame:
	double SecondsThisSlice = s;

	while (s > 0.0)
	{
		SecondsThisSlice = (s > sub_frame) ? sub_frame : s;

		// if the director needs subframe accuracy, run it now:
		if (dir && dir->Subframe())
			dir->ExecFrame(SecondsThisSlice);

		if (!straight)
			AngularFrame(SecondsThisSlice);

		// LINEAR MOVEMENT ----------------------------
		FVector Pos = cam.Pos();

		// if the object is thrusting, accelerate along the camera normal:
		if (thrust != 0.0f)
		{
			FVector ThrustVec = cam.vpn();
			ThrustVec *= (float)(((double)thrust / (double)mass) * SecondsThisSlice);
			velocity += ThrustVec;
		}

		LinearFrame(SecondsThisSlice);

		// move the position by the (time-frame scaled) velocity:
		Pos += velocity * (float)SecondsThisSlice;
		cam.MoveTo(Pos);

		s -= SecondsThisSlice;
	}

	alpha = 0.0f;

	// --------------------------------------------------
	// UPDATE GRAPHIC REPRESENTATION (UE-SAFE)
	// --------------------------------------------------
	if (rep)
	{
		rep->MoveTo(cam.Pos());

		// Convert Starshatter Matrix -> UE FMatrix
		const Matrix& M = cam.Orientation();

		const FMatrix UEOrientation(
			FPlane((float)M(0, 0), (float)M(0, 1), (float)M(0, 2), 0.0f),
			FPlane((float)M(1, 0), (float)M(1, 1), (float)M(1, 2), 0.0f),
			FPlane((float)M(2, 0), (float)M(2, 1), (float)M(2, 2), 0.0f),
			FPlane(0.f, 0.f, 0.f, 1.f)
		);

		rep->SetOrientation(UEOrientation);
	}

	if (light)
	{
		light->MoveTo(cam.Pos());
	}

	if (!straight)
		CalcFlightPath();

	// accel over last slice duration:
	accel = (Velocity() - OrigVelocity) * (float)(1.0 / SecondsThisSlice);
	if (!IsFiniteVector(accel))
		accel = FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

void
Physical::AeroFrame(double s)
{
	arcade_velocity = FVector::ZeroVector;

	// if this object is under direction,
	// but doesn't need subframe accuracy,
	// update the control parameters:
	if (dir && !dir->Subframe())
		dir->ExecFrame(s);

	// decrement life before destroying the frame time:
	if (life > 0)
		life -= s;

	// integrate equations using slices no larger than sub_frame:
	double SecondsThisSlice = s;

	while (s > 0.0)
	{
		SecondsThisSlice = (s > sub_frame) ? sub_frame : s;

		// if the director needs subframe accuracy, run it now:
		if (dir && dir->Subframe())
			dir->ExecFrame(SecondsThisSlice);

		AngularFrame(SecondsThisSlice);

		// LINEAR MOVEMENT ----------------------------
		FVector Pos = cam.Pos();

		// if the object is thrusting, accelerate along the camera normal:
		if (thrust != 0.0f)
		{
			FVector ThrustVec = cam.vpn();
			ThrustVec *= (float)(((double)thrust / (double)mass) * SecondsThisSlice);
			velocity += ThrustVec;
		}

		// AERODYNAMICS ------------------------------
		if (lat_thrust)
		{
			LinearFrame(SecondsThisSlice);
		}

		// if no thrusters, do constant gravity:
		else if (g_accel > 0.0f)
		{
			velocity += FVector(0.0f, -(float)g_accel, 0.0f) * (float)SecondsThisSlice;
		}

		// compute alpha, rho, drag, and lift:
		FVector Vfp = velocity;
		const double V = NormalizeAndReturnLength(Vfp);

		double V2 = 0.0;
		double Rho = GetDensity();
		double Lift = 0.0;

		if (V > 150.0)
		{
			V2 = (V - 150.0) * (V - 150.0);

			FVector Vrt = cam.vrt();
			FVector Vup = cam.vup();
			FVector Vpn = cam.vpn();

			// vfp1 = vfp - vrt * (vfp dot vrt)
			FVector Vfp1 = Vfp - Vrt * (float)(Vfp | Vrt);
			NormalizeAndReturnLength(Vfp1);

			const double CosAlpha = (double)(Vfp1 | Vpn);

			if (CosAlpha >= 1.0)
			{
				alpha = 0.0f;
			}
			else
			{
				alpha = (float)std::acos(CosAlpha);
			}

			// if flight path is above nose, alpha is negative:
			if ((double)(Vfp1 | Vup) > 0.0)
				alpha = -alpha;

			if (alpha <= stall)
				Lift = (double)CL * (double)alpha * Rho * V2;
			else
				Lift = (double)CL * (double)(2.0f * stall - alpha) * Rho * V2;

			// add lift to velocity:
			if (FMath::IsFinite((float)Lift))
				velocity += Vup * (float)(Lift * SecondsThisSlice);
			else
				Lift = 0.0;

			// if drag applies, decelerate:
			const double Alpha2 = (double)alpha * (double)alpha;
			const double DragEff = ((double)drag + ((double)CD * Alpha2)) * Rho * V2;

			FVector Vn = velocity;
			NormalizeAndReturnLength(Vn);

			velocity += Vn * (float)(-DragEff * SecondsThisSlice);
		}
		else
		{
			velocity *= (float)std::exp(-(double)drag * SecondsThisSlice);
		}

		// move the position by the (time-frame scaled) velocity:
		Pos += velocity * (float)SecondsThisSlice;
		cam.MoveTo(Pos);

		s -= SecondsThisSlice;
	}

	// now update the graphic rep and light sources:
	if (rep)
	{
		rep->MoveTo(cam.Pos());

		// Starshatter Matrix -> UE FMatrix (Graphic/Solid now expects FMatrix)
		const Matrix& M = cam.Orientation();

		const FMatrix UEOrientation(
			FPlane((float)M(0, 0), (float)M(0, 1), (float)M(0, 2), 0.0f),
			FPlane((float)M(1, 0), (float)M(1, 1), (float)M(1, 2), 0.0f),
			FPlane((float)M(2, 0), (float)M(2, 1), (float)M(2, 2), 0.0f),
			FPlane(0.f, 0.f, 0.f, 1.f)
		);

		rep->SetOrientation(UEOrientation);
	}

	if (light)
	{
		light->MoveTo(cam.Pos());
	}
}


double Physical::GetDensity() const
{
	const double Alt = (double)cam.Pos().Y;
	const double Rho = 0.75 * (double)Do * (250e3 - Alt) / 250e3;
	return Rho;
}

// +--------------------------------------------------------------------+

void
Physical::ArcadeFrame(double s)
{
	// if this object is under direction,
	// but doesn't need subframe accuracy,
	// update the control parameters:
	if (dir && !dir->Subframe())
		dir->ExecFrame(s);

	// decrement life before destroying the frame time:
	if (life > 0)
		life -= s;

	// integrate equations using slices no larger than sub_frame:
	double SecondsThisSlice = s;

	while (s > 0.0)
	{
		SecondsThisSlice = (s > sub_frame) ? sub_frame : s;

		// if the director needs subframe accuracy, run it now:
		if (dir && dir->Subframe())
			dir->ExecFrame(SecondsThisSlice);

		if (!straight)
			AngularFrame(SecondsThisSlice);

		FVector Pos = cam.Pos();

		// ARCADE FLIGHT MODEL:
		// arcade_velocity vector is always in line with heading
		double Speed = NormalizeAndReturnLength(arcade_velocity);
		const double Bleed = (double)(arcade_velocity | cam.vpn());

		Speed *= std::pow(Bleed, 30.0);
		arcade_velocity = cam.vpn() * (float)Speed;

		if (thrust != 0.0f)
		{
			FVector ThrustVec = cam.vpn();
			ThrustVec *= (float)(((double)thrust / (double)mass) * SecondsThisSlice);
			arcade_velocity += ThrustVec;
		}

		if (drag != 0.0f)
			arcade_velocity *= (float)std::exp(-(double)drag * SecondsThisSlice);

		LinearFrame(SecondsThisSlice);

		// move the position by the (time-frame scaled) velocity:
		Pos += arcade_velocity * (float)SecondsThisSlice +
			velocity * (float)SecondsThisSlice;

		cam.MoveTo(Pos);

		s -= SecondsThisSlice;
	}

	alpha = 0.0f;

	// now update the graphic rep and light sources:
	if (rep)
	{
		rep->MoveTo(cam.Pos());

		// Starshatter Matrix -> UE FMatrix (Graphic/Solid now expects FMatrix)
		const Matrix& M = cam.Orientation();

		const FMatrix UEOrientation(
			FPlane((float)M(0, 0), (float)M(0, 1), (float)M(0, 2), 0.0f),
			FPlane((float)M(1, 0), (float)M(1, 1), (float)M(1, 2), 0.0f),
			FPlane((float)M(2, 0), (float)M(2, 1), (float)M(2, 2), 0.0f),
			FPlane(0.f, 0.f, 0.f, 1.f)
		);

		rep->SetOrientation(UEOrientation);
	}

	if (light)
	{
		light->MoveTo(cam.Pos());
	}
}

// +--------------------------------------------------------------------+

void Physical::AngularFrame(double SecondsThisSlice)
{
	if (!straight)
	{
		dr += (float)((double)dr_acc * SecondsThisSlice);
		dy += (float)((double)dy_acc * SecondsThisSlice);
		dp += (float)((double)dp_acc * SecondsThisSlice);

		dr *= (float)std::exp(-(double)dr_drg * SecondsThisSlice);
		dy *= (float)std::exp(-(double)dy_drg * SecondsThisSlice);
		dp *= (float)std::exp(-(double)dp_drg * SecondsThisSlice);

		roll = (float)((double)dr * SecondsThisSlice);
		pitch = (float)((double)dp * SecondsThisSlice);
		yaw = (float)((double)dy * SecondsThisSlice);

		if (shake > 0.01f)
		{
			vibration = FVector((float)RandomSigned(), (float)RandomSigned(), (float)RandomSigned());
			NormalizeAndReturnLength(vibration);
			vibration *= (float)((double)shake * SecondsThisSlice);

			shake *= (float)std::exp(-1.5 * SecondsThisSlice);
		}
		else
		{
			vibration = FVector::ZeroVector;
			shake = 0.0f;
		}

		cam.Aim(roll, pitch, yaw);
	}
}

// +--------------------------------------------------------------------+

void Physical::LinearFrame(double SecondsThisSlice)
{
	// deal with lateral thrusters:

	if (trans_x != 0.0f) // side-to-side
	{
		FVector TransVec = cam.vrt();
		TransVec *= (float)(((double)trans_x / (double)mass) * SecondsThisSlice);
		velocity += TransVec;
	}

	if (trans_y != 0.0f) // fore-and-aft
	{
		FVector TransVec = cam.vpn();
		TransVec *= (float)(((double)trans_y / (double)mass) * SecondsThisSlice);
		velocity += TransVec;
	}

	if (trans_z != 0.0f) // up-and-down
	{
		FVector TransVec = cam.vup();
		TransVec *= (float)(((double)trans_z / (double)mass) * SecondsThisSlice);
		velocity += TransVec;
	}

	// if gravity applies, attract:
	if (primary_mass > 0.0)
	{
		FVector G = primary_loc - cam.Pos();
		const double R = NormalizeAndReturnLength(G);

		// g *= GRAV * primary_mass / (r*r)
		if (R > 1e-6)
		{
			const double Scale = GRAV * primary_mass / (R * R);
			G *= (float)Scale;
			velocity += G * (float)SecondsThisSlice;
		}
	}

	// constant gravity:
	else if (g_accel > 0.0f)
	{
		velocity += FVector(0.0f, -(float)g_accel, 0.0f) * (float)SecondsThisSlice;
	}

	// if drag applies, decelerate:
	if (drag != 0.0f)
		velocity *= (float)std::exp(-(double)drag * SecondsThisSlice);
}

// +--------------------------------------------------------------------+

void Physical::CalcFlightPath()
{
	flight_path_yaw = 0.0f;
	flight_path_pitch = 0.0f;

	// transform flight path into camera frame:
	FVector FlightPath = velocity;
	if (NormalizeAndReturnLength(FlightPath) < 1.0)
		return;

	FVector Tmp = FlightPath;

	// dot into camera basis:
	FlightPath.X = (float)(Tmp | cam.vrt());
	FlightPath.Y = (float)(Tmp | cam.vup());
	FlightPath.Z = (float)(Tmp | cam.vpn());

	if (FlightPath.Z < 0.1f)
		return;

	// first, compute azimuth:
	flight_path_yaw = (float)std::atan((double)FlightPath.X / (double)FlightPath.Z);
	if (FlightPath.Z < 0.0f)      flight_path_yaw -= (float)PI;
	if (flight_path_yaw < -PI)    flight_path_yaw += (float)(2.0 * PI);

	// then, rotate path into azimuth frame to compute elevation:
	Camera YawCam;
	YawCam.Clone(cam);
	YawCam.Yaw(flight_path_yaw);

	FlightPath.X = (float)(Tmp | YawCam.vrt());
	FlightPath.Y = (float)(Tmp | YawCam.vup());
	FlightPath.Z = (float)(Tmp | YawCam.vpn());

	flight_path_pitch = (float)std::atan((double)FlightPath.Y / (double)FlightPath.Z);
}

// +--------------------------------------------------------------------+

void Physical::MoveTo(const FVector& NewLoc)
{
	cam.MoveTo(NewLoc);
}

void Physical::TranslateBy(const FVector& Ref)
{
	const FVector NewLoc = cam.Pos() - Ref;
	cam.MoveTo(NewLoc);
}

void Physical::ApplyForce(const FVector& Force)
{
	velocity += Force / (float)mass;
}

void Physical::ApplyTorque(const FVector& Torque)
{
	dr += (float)((double)Torque.X / (double)mass);
	dp += (float)((double)Torque.Y / (double)mass);
	dy += (float)((double)Torque.Z / (double)mass);
}

void Physical::SetThrust(double t)
{
	thrust = (float)t;
}

void Physical::SetTransX(double t)
{
	trans_x = (float)t;
}

void Physical::SetTransY(double t)
{
	trans_y = (float)t;
}

void Physical::SetTransZ(double t)
{
	trans_z = (float)t;
}

// +--------------------------------------------------------------------+

void Physical::SetHeading(double r, double p, double y)
{
	roll = (float)r;
	pitch = (float)p;
	yaw = (float)y;

	cam.Aim(roll, pitch, yaw);
}

void Physical::LookAt(const FVector& Dst)
{
	cam.LookAt(Dst);
}

void Physical::CloneCam(const Camera& c)
{
	cam.Clone(c);
}

void Physical::SetAbsoluteOrientation(double r, double p, double y)
{
	roll = (float)r;
	pitch = (float)p;
	yaw = (float)y;

	const FVector L = Location();
	Camera Work(L.X, L.Y, L.Z);
	Work.Aim(r, p, y);
	cam.Clone(Work);
}

void Physical::ApplyRoll(double r)
{
	if (r > 1)       r = 1;
	else if (r < -1) r = -1;

	dr_acc = (float)r * roll_rate;
}

void Physical::ApplyPitch(double p)
{
	if (p > 1)       p = 1;
	else if (p < -1) p = -1;

	dp_acc = (float)p * pitch_rate;
}

void Physical::ApplyYaw(double y)
{
	if (y > 1)       y = 1;
	else if (y < -1) y = -1;

	dy_acc = (float)y * yaw_rate;
}

void Physical::SetAngularRates(double r, double p, double y)
{
	roll_rate = (float)r;
	pitch_rate = (float)p;
	yaw_rate = (float)y;
}

void Physical::GetAngularRates(double& r, double& p, double& y)
{
	r = roll_rate;
	p = pitch_rate;
	y = yaw_rate;
}

void Physical::SetAngularDrag(double r, double p, double y)
{
	dr_drg = (float)r;
	dp_drg = (float)p;
	dy_drg = (float)y;
}

void Physical::GetAngularDrag(double& r, double& p, double& y)
{
	r = dr_drg;
	p = dp_drg;
	y = dy_drg;
}

void Physical::GetAngularThrust(double& r, double& p, double& y)
{
	r = 0;
	p = 0;
	y = 0;

	if (dr_acc > 0.05f * roll_rate) r = 1;
	else if (dr_acc < -0.05f * roll_rate) r = -1;
	else if (dr > 0.01f * roll_rate) r = -1;
	else if (dr < -0.01f * roll_rate) r = 1;

	if (dy_acc > 0.05f * yaw_rate)  y = 1;
	else if (dy_acc < -0.05f * yaw_rate)  y = -1;
	else if (dy > 0.01f * yaw_rate)  y = -1;
	else if (dy < -0.01f * yaw_rate)  y = 1;

	if (dp_acc > 0.05f * pitch_rate) p = 1;
	else if (dp_acc < -0.05f * pitch_rate) p = -1;
	else if (dp > 0.01f * pitch_rate) p = -1;
	else if (dp < -0.01f * pitch_rate) p = 1;
}

void Physical::SetPrimary(const FVector& l, double m)
{
	primary_loc = l;
	primary_mass = m;
}

void Physical::SetGravity(double g)
{
	if (g >= 0)
		g_accel = (float)g;
}

void Physical::SetBaseDensity(double d)
{
	if (d >= 0)
		Do = (float)d;
}

// +--------------------------------------------------------------------+

void Physical::InflictDamage(double damage, int /*type*/)
{
	integrity -= (float)damage;

	if (integrity < 1.0f)
		integrity = 0.0f;
}

// +--------------------------------------------------------------------+

int Physical::CollidesWith(Physical& o)
{
	// representation collision test (will do bounding spheres first):
	if (rep && o.rep)
		return rep->CollidesWith(*o.rep);

	const FVector DeltaLoc = Location() - o.Location();

	// bounding spheres test:
	if ((double)DeltaLoc.Size() > (double)radius + (double)o.radius)
		return 0;

	// assume collision:
	return 1;
}

// +--------------------------------------------------------------------+

void Physical::ElasticCollision(Physical& a, Physical& b)
{
	const double MassSum = (double)a.mass + (double)b.mass;
	const double MassDelta = (double)a.mass - (double)b.mass;

	const FVector VelA = (b.velocity * (float)(2.0 * (double)b.mass) + a.velocity * (float)MassDelta) * (float)(1.0 / MassSum);
	const FVector VelB = (a.velocity * (float)(2.0 * (double)a.mass) - b.velocity * (float)MassDelta) * (float)(1.0 / MassSum);

	a.velocity = VelA;
	b.velocity = VelB;
}

// +--------------------------------------------------------------------+

void Physical::InelasticCollision(Physical& a, Physical& b)
{
	const double MassSum = (double)a.mass + (double)b.mass;

	const FVector VelA = (a.velocity * (float)(double)a.mass + b.velocity * (float)(double)b.mass) * (float)(1.0 / MassSum);

	a.velocity = VelA;
	b.velocity = VelA;
}

// +--------------------------------------------------------------------+

void Physical::SemiElasticCollision(Physical& a, Physical& b)
{
	const double MassSum = (double)a.mass + (double)b.mass;
	const double MassDelta = (double)a.mass - (double)b.mass;

	const FVector AVel = a.Velocity();
	const FVector BVel = b.Velocity();
	const FVector DV = AVel - BVel;

	// low delta-v: stick
	if ((double)DV.Size() < 20.0)
	{
		if ((double)a.mass > (double)b.mass)
			b.velocity = a.velocity;
		else
			a.velocity = b.velocity;
	}

	// high delta-v: bounce
	else
	{
		const FVector VeA = (BVel * (float)(2.0 * (double)b.mass) + AVel * (float)MassDelta) * (float)(1.0 / MassSum) * 0.65f;
		const FVector VeB = (AVel * (float)(2.0 * (double)a.mass) - BVel * (float)MassDelta) * (float)(1.0 / MassSum) * 0.65f;
		const FVector ViAB = (AVel * (float)(double)a.mass + BVel * (float)(double)b.mass) * (float)(1.0 / MassSum) * 0.35f;

		a.arcade_velocity = FVector::ZeroVector;
		b.arcade_velocity = FVector::ZeroVector;

		a.velocity = VeA + ViAB;
		b.velocity = VeB + ViAB;
	}
}
