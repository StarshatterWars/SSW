#include "Camera.h"

#include "Math/UnrealMathUtility.h"

static bool CameraVectorIsFinite(const FVector& V)
{
	return FMath::IsFinite(V.X) &&
		FMath::IsFinite(V.Y) &&
		FMath::IsFinite(V.Z);
}

Camera::Camera(double x, double y, double z)
	: pos((float)x, (float)y, (float)z)
{
	orientation.Identity();
}

Camera::~Camera()
{
}

void
Camera::Normalize()
{
	FVector X = vrt();
	FVector Y = vup();
	FVector Z = vpn();

	if (!CameraVectorIsFinite(X) ||
		!CameraVectorIsFinite(Y) ||
		!CameraVectorIsFinite(Z))
	{
		orientation.Identity();
		return;
	}

	if (!Z.Normalize())
	{
		Z = FVector(0.0f, 0.0f, -1.0f);
	}

	Y = Y - Z * FVector::DotProduct(Y, Z);

	if (!Y.Normalize())
	{
		Y = FVector(0.0f, 1.0f, 0.0f);
	}

	X = FVector::CrossProduct(Z, Y);

	if (!X.Normalize())
	{
		X = FVector(1.0f, 0.0f, 0.0f);
	}

	Y = FVector::CrossProduct(X, Z);

	if (!Y.Normalize())
	{
		Y = FVector(0.0f, 1.0f, 0.0f);
	}

	orientation(0, 0) = X.X;
	orientation(0, 1) = X.Y;
	orientation(0, 2) = X.Z;

	orientation(1, 0) = Y.X;
	orientation(1, 1) = Y.Y;
	orientation(1, 2) = Y.Z;

	orientation(2, 0) = Z.X;
	orientation(2, 1) = Z.Y;
	orientation(2, 2) = Z.Z;
}

void
Camera::Aim(double roll, double pitch, double yaw)
{
	if (!FMath::IsFinite(roll) ||
		!FMath::IsFinite(pitch) ||
		!FMath::IsFinite(yaw))
	{
		return;
	}

	if (yaw != 0.0)
	{
		Yaw(yaw);
	}

	if (pitch != 0.0)
	{
		Pitch(pitch);
	}

	if (roll != 0.0)
	{
		Roll(roll);
	}

	Normalize();
}

void
Camera::Yaw(double yaw)
{
	if (!FMath::IsFinite(yaw))
		return;

	const double c = cos(yaw);
	const double s = sin(yaw);

	const FVector x = vrt();
	const FVector z = vpn();

	const FVector x1 = x * c + z * s;
	const FVector z1 = z * c - x * s;

	orientation(0, 0) = x1.X;
	orientation(0, 1) = x1.Y;
	orientation(0, 2) = x1.Z;

	orientation(2, 0) = z1.X;
	orientation(2, 1) = z1.Y;
	orientation(2, 2) = z1.Z;

	Normalize();
}


void
Camera::Pitch(double pitch)
{
	if (!FMath::IsFinite(pitch))
		return;

	const double c = cos(pitch);
	const double s = sin(pitch);

	const FVector y = vup();
	const FVector z = vpn();

	const FVector y1 = y * c + z * s;
	const FVector z1 = z * c - y * s;

	orientation(1, 0) = y1.X;
	orientation(1, 1) = y1.Y;
	orientation(1, 2) = y1.Z;

	orientation(2, 0) = z1.X;
	orientation(2, 1) = z1.Y;
	orientation(2, 2) = z1.Z;

	Normalize();
}

void
Camera::Roll(double roll)
{
	if (!FMath::IsFinite(roll))
		return;

	const double c = cos(roll);
	const double s = sin(roll);

	const FVector x = vrt();
	const FVector y = vup();

	const FVector x1 = x * c + y * s;
	const FVector y1 = y * c - x * s;

	orientation(0, 0) = x1.X;
	orientation(0, 1) = x1.Y;
	orientation(0, 2) = x1.Z;

	orientation(1, 0) = y1.X;
	orientation(1, 1) = y1.Y;
	orientation(1, 2) = y1.Z;

	Normalize();
}

void
Camera::MoveTo(double x, double y, double z)
{
	pos.X = (float)x;
	pos.Y = (float)y;
	pos.Z = (float)z;
}

void
Camera::MoveTo(const FVector& p)
{
	pos = p;
}

void
Camera::MoveBy(double dx, double dy, double dz)
{
	pos.X += (float)dx;
	pos.Y += (float)dy;
	pos.Z += (float)dz;
}

void
Camera::MoveBy(const FVector& p)
{
	pos += p;
}

void
Camera::Clone(const Camera& cam)
{
	pos = cam.pos;
	orientation = cam.orientation;
	Normalize();
}

void
Camera::LookAt(const FVector& target)
{
	if (target == Pos())
	{
		return;
	}

	FVector Tgt;
	const FVector Tmp =
		target - Pos();

	Tgt.X =
		FVector::DotProduct(Tmp, vrt());

	Tgt.Y =
		FVector::DotProduct(Tmp, vup());

	Tgt.Z =
		FVector::DotProduct(Tmp, vpn());

	if (Tgt.Z == 0.0f)
	{
		Pitch(0.5);
		Yaw(0.5);
		LookAt(target);
		return;
	}

	double Az =
		FMath::Atan(
			(double)(Tgt.X / Tgt.Z));

	double El =
		FMath::Atan(
			(double)(Tgt.Y / Tgt.Z));

	if (Tgt.Z < 0.0f)
	{
		Az -= PI;
	}

	Pitch(-El);
	Yaw(Az);

	double Deflection =
		vrt().Y;

	int32 Guard = 0;

	while (FMath::Abs(Deflection) > 0.001 &&
		Guard++ < 32)
	{
		const double Len =
			vrt().Size();

		if (Len <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		const double Theta =
			FMath::Asin(
				FMath::Clamp(
					Deflection / Len,
					-1.0,
					1.0));

		Roll(-Theta);

		Deflection =
			vrt().Y;
	}
}

void
Camera::LookAt(
	const FVector& target,
	const FVector& eye,
	const FVector& up)
{
	FVector ZAxis =
		target - eye;

	if (!ZAxis.Normalize())
	{
		return;
	}

	FVector XAxis =
		FVector::CrossProduct(up, ZAxis);

	if (!XAxis.Normalize())
	{
		return;
	}

	FVector YAxis =
		FVector::CrossProduct(ZAxis, XAxis);

	if (!YAxis.Normalize())
	{
		return;
	}

	orientation(0, 0) = XAxis.X;
	orientation(0, 1) = XAxis.Y;
	orientation(0, 2) = XAxis.Z;

	orientation(1, 0) = YAxis.X;
	orientation(1, 1) = YAxis.Y;
	orientation(1, 2) = YAxis.Z;

	orientation(2, 0) = ZAxis.X;
	orientation(2, 1) = ZAxis.Y;
	orientation(2, 2) = ZAxis.Z;

	pos = eye;

	Normalize();
}

bool
Camera::Padlock(
	const FVector& target,
	double alimit,
	double e_lo,
	double e_hi)
{
	if (target == Pos())
	{
		return false;
	}

	FVector Tgt;
	const FVector Tmp =
		target - Pos();

	Tgt.X = FVector::DotProduct(Tmp, vrt());
	Tgt.Y = FVector::DotProduct(Tmp, vup());
	Tgt.Z = FVector::DotProduct(Tmp, vpn());

	if (Tgt.Z == 0.0f)
	{
		Yaw(0.1);

		Tgt.X = FVector::DotProduct(Tmp, vrt());
		Tgt.Y = FVector::DotProduct(Tmp, vup());
		Tgt.Z = FVector::DotProduct(Tmp, vpn());

		if (Tgt.Z == 0.0f)
		{
			return false;
		}
	}

	bool bLocked = true;

	double Az =
		FMath::Atan(
			(double)(Tgt.X / Tgt.Z));

	if (Tgt.Z < 0.0f)
	{
		Az -= PI;
	}

	if (alimit > 0.0)
	{
		if (Az < -alimit)
		{
			Az = -alimit;
			bLocked = false;
		}
		else if (Az > alimit)
		{
			Az = alimit;
			bLocked = false;
		}
	}

	Yaw(Az);

	Tgt.X = FVector::DotProduct(Tmp, vrt());
	Tgt.Y = FVector::DotProduct(Tmp, vup());
	Tgt.Z = FVector::DotProduct(Tmp, vpn());

	if (Tgt.Z == 0.0f)
	{
		return false;
	}

	double El =
		FMath::Atan(
			(double)(Tgt.Y / Tgt.Z));

	if (e_lo > 0.0 && El < -e_lo)
	{
		El = -e_lo;
		bLocked = false;
	}
	else if (e_hi > 0.0 && El > e_hi)
	{
		El = e_hi;
		bLocked = false;
	}

	Pitch(-El);

	return bLocked;
}

void
Camera::SetOrientation(
	const FVector& Right,
	const FVector& Up,
	const FVector& Forward)
{
	orientation(0, 0) = Right.X;
	orientation(0, 1) = Right.Y;
	orientation(0, 2) = Right.Z;

	orientation(1, 0) = Up.X;
	orientation(1, 1) = Up.Y;
	orientation(1, 2) = Up.Z;

	orientation(2, 0) = Forward.X;
	orientation(2, 1) = Forward.Y;
	orientation(2, 2) = Forward.Z;

	Normalize();
}