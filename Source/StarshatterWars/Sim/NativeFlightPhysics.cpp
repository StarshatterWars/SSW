#include "NativeFlightPhysics.h"
#include "PerformanceCapture.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include <cmath>
namespace
{
constexpr double NativeGravity=6.673e-11; // SSW GRAV, no alternate balance
inline double NativeFlightNormalize(FVector& V)
{const double Len=(double)V.Size();if(Len>1e-9){V/=Len;return Len;}V=FVector::ZeroVector;return 0;}
inline bool NativeFlightFinite(const FVector& V){return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);}
}
double FNativeFlightState::GetDensity()const
{const double Alt=(double)cam.Pos().Y;return .75*(double)Do*(250e3-Alt)/250e3;}

void FNativeFlightState::AngularFrame(double SecondsThisSlice,const FVector& RawRandom)
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
			vibration = RawRandom;

			NativeFlightNormalize(vibration);

			vibration *=
				(float)((double)shake * SecondsThisSlice);

			shake *=
				(float)std::exp(-1.5 * SecondsThisSlice);
		}
		else
		{
			vibration = FVector::ZeroVector;
			shake = 0.0f;
		}


		cam.Aim(roll, pitch, yaw);
	}
}

void FNativeFlightState::LinearFrame(double SecondsThisSlice)
{
	// deal with lateral thrusters:

	if (trans_x) { // side-to-side
		FVector transvec = cam.vrt();
		transvec *= ((trans_x/mass) * SecondsThisSlice);

		velocity += transvec;
	}

	if (trans_y) { // fore-and-aft
		FVector transvec = cam.vpn();
		transvec *= ((trans_y/mass) * SecondsThisSlice);

		velocity += transvec;
	}

	if (trans_z) { // up-and-down
		FVector transvec = cam.vup();
		transvec *= ((trans_z/mass) * SecondsThisSlice);

		velocity += transvec;
	}

	// if gravity applies, attract:
	if (primary_mass > 0) {
		FVector  g = primary_loc - cam.Pos();
		double r = NativeFlightNormalize(g);

		if (r > 1e-6) g *= NativeGravity * primary_mass / (r*r);
        else g = FVector::ZeroVector;

		velocity += g * SecondsThisSlice;
	}

	// constant gravity:
	else if (g_accel > 0)
	velocity += FVector(0, -g_accel, 0) * SecondsThisSlice;

	// if drag applies, decellerate:
	if (drag)
	velocity *= exp(-drag * SecondsThisSlice);

    if(bAirborneLCADamping)
    {
// damp lateral movement in atmosphere:

	// side-to-side
	if (!trans_x) {
		FVector transvec = cam.vrt();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * SecondsThisSlice * 0.5);
		velocity -= transvec;
	}

	// fore-and-aft
	if (!trans_y && FMath::Abs(thrust) < 1.0f) {
		FVector transvec = cam.vpn();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * SecondsThisSlice * 0.25);
		velocity -= transvec;
	}

	// up-and-down
	if (!trans_z) {
		FVector transvec = cam.vup();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * SecondsThisSlice * 0.5);
		velocity -= transvec;
	}

    }
}

void FNativeFlightState::CalcFlightPath()
{
	flight_path_yaw = 0.0f;
	flight_path_pitch = 0.0f;

	// transform flight path into camera frame:
	FVector FlightPath = velocity;
	if (NativeFlightNormalize(FlightPath) < 1.0)
		return;

	FVector Tmp = FlightPath;

	// dot into camera basis:
	FlightPath.X = (Tmp | cam.vrt());
	FlightPath.Y = (Tmp | cam.vup());
	FlightPath.Z = (Tmp | cam.vpn());

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

	FlightPath.X = (Tmp | YawCam.vrt());
	FlightPath.Y = (Tmp | YawCam.vup());
	FlightPath.Z = (Tmp | YawCam.vpn());

	flight_path_pitch = (float)std::atan((double)FlightPath.Y / (double)FlightPath.Z);
}

void FNativeFlightState::InertialStep(double SecondsThisSlice)
{    FVector Pos=cam.Pos();
    if(thrust){FVector ThrustVec=cam.vpn();ThrustVec*=((thrust/mass)*SecondsThisSlice);velocity+=ThrustVec;}
    LinearFrame(SecondsThisSlice);Pos+=velocity*SecondsThisSlice;cam.MoveTo(Pos);
}

void FNativeFlightState::AtmosphericStep(double SecondsThisSlice)
{		FVector Pos = cam.Pos();

		// if the object is thrusting, accelerate along the camera normal:
		if (thrust != 0.0f)
		{
			FVector ThrustVec = cam.vpn();
			ThrustVec *= ((thrust/mass) * SecondsThisSlice);
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
			velocity += FVector(0.0f, -(float)g_accel, 0.0f) * SecondsThisSlice;
		}

		// compute alpha, rho, drag, and lift:
		FVector Vfp = velocity;
		const double V = NativeFlightNormalize(Vfp);

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
			FVector Vfp1 = Vfp - Vrt * (Vfp | Vrt);
			NativeFlightNormalize(Vfp1);

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
				Lift = (CL * alpha) * Rho * V2;
			else
				Lift = (CL * (2*stall - alpha)) * Rho * V2;

			// add lift to velocity:
			if (FMath::IsFinite(Lift))
				velocity += Vup * Lift * SecondsThisSlice;
			else
				Lift = 0.0;

			// if drag applies, decelerate:
			const double Alpha2 = alpha * alpha;
			const double DragEff = ((double)drag + ((double)CD * Alpha2)) * Rho * V2;

			FVector Vn = velocity;
			NativeFlightNormalize(Vn);

			velocity += Vn * -DragEff * SecondsThisSlice;
		}
		else
		{
			velocity *= std::exp(-drag * SecondsThisSlice);
		}

		// move the position by the (time-frame scaled) velocity:
		Pos += velocity * SecondsThisSlice;
		cam.MoveTo(Pos);

}

void FNativeFlightState::ArcadeStep(double SecondsThisSlice)
{		FVector Pos = cam.Pos();

		// ARCADE FLIGHT MODEL:
		// arcade_velocity vector is always in line with heading
		double Speed = NativeFlightNormalize(arcade_velocity);
		const double Bleed = (double)(arcade_velocity | cam.vpn());

		Speed *= std::pow(Bleed, 30.0);
		arcade_velocity = cam.vpn() * Speed;

		if (thrust != 0.0f)
		{
			FVector ThrustVec = cam.vpn();
			ThrustVec *= ((thrust/mass) * SecondsThisSlice);
			arcade_velocity += ThrustVec;
		}

		if (drag != 0.0f)
			arcade_velocity *= std::exp(-drag * SecondsThisSlice);

		LinearFrame(SecondsThisSlice);

		// move the position by the (time-frame scaled) velocity:
		Pos += arcade_velocity * SecondsThisSlice +
			velocity * SecondsThisSlice;

		cam.MoveTo(Pos);

}

FNativeFlightResult SSWFlightPhysics::Calculate(FNativeFlightRequest&& Request)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_FlightPhysicsKernel);
    const double Started=FPlatformTime::Seconds();
    FNativeFlightResult Out;Out.Sequence=Request.Sequence;Out.bOffGameThread=!IsInGameThread();Out.State=MoveTemp(Request.State);
    if(Request.Seconds<=0 || !FMath::IsFinite(Request.Seconds) || Request.Subframe<=0 || !FMath::IsFinite(Request.Subframe) ||
       Out.State.mass<=0 || Request.Seconds/Request.Subframe>4096 || Request.RandomSamples.IsEmpty())return Out;
    auto& State=Out.State;double Left=Request.Seconds,LastSlice=0;int32 Index=0;
    while(Left>0)
    {
        LastSlice=Left>Request.Subframe?Request.Subframe:Left;
        if(!Request.RandomSamples.IsValidIndex(Index))return Out;
        State.AngularFrame(LastSlice,Request.RandomSamples[Index++]);
        if(Request.Model==ENativeFlightModel::Inertial)State.InertialStep(LastSlice);
        else if(Request.Model==ENativeFlightModel::Atmospheric)State.AtmosphericStep(LastSlice);
        else State.ArcadeStep(LastSlice);
        Left-=LastSlice;
    }
    if(Request.bFinalize)
    {
        if(Request.Model==ENativeFlightModel::Inertial)
        {
            State.alpha=0;if(!State.straight)State.CalcFlightPath();
            State.accel=(State.GetVelocity()-Request.OriginalVelocity)*(1/LastSlice);
            if(!NativeFlightFinite(State.accel))State.accel=FVector::ZeroVector;
        }
        else if(Request.Model==ENativeFlightModel::Arcade)State.alpha=0;
    }
    Out.bValid=true;Out.CalculationSeconds=FPlatformTime::Seconds()-Started;if(Out.bOffGameThread)SSWPerformanceCapture::RecordSimulation(Out.CalculationSeconds);return Out;
}
