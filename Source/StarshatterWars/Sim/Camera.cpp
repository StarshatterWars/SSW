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
		Z = FVector(0.0f, 0.0f, 1.0f);
	}

	Y = Y - Z * FVector::DotProduct(Y, Z);

	if (!Y.Normalize())
	{
		Y = FVector(0.0f, 1.0f, 0.0f);
	}

	X = FVector::CrossProduct(Y, Z);

	if (!X.Normalize())
	{
		X = FVector(1.0f, 0.0f, 0.0f);
	}

	Y = FVector::CrossProduct(Z, X);
	Y.Normalize();

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
	{
		return;
	}

	const double C = FMath::Cos(yaw);
	const double S = FMath::Sin(yaw);

	const FVector X = vrt();
	const FVector Z = vpn();

	const FVector NewX =
		X * (float)C +
		Z * (float)S;

	const FVector NewZ =
		Z * (float)C -
		X * (float)S;

	orientation(0, 0) = NewX.X;
	orientation(0, 1) = NewX.Y;
	orientation(0, 2) = NewX.Z;

	orientation(2, 0) = NewZ.X;
	orientation(2, 1) = NewZ.Y;
	orientation(2, 2) = NewZ.Z;

	Normalize();
}

void
Camera::Pitch(double pitch)
{
	if (!FMath::IsFinite(pitch))
	{
		return;
	}

	const double C = FMath::Cos(pitch);
	const double S = FMath::Sin(pitch);

	const FVector Y = vup();
	const FVector Z = vpn();

	const FVector NewY =
		Y * (float)C +
		Z * (float)S;

	const FVector NewZ =
		Z * (float)C -
		Y * (float)S;

	orientation(1, 0) = NewY.X;
	orientation(1, 1) = NewY.Y;
	orientation(1, 2) = NewY.Z;

	orientation(2, 0) = NewZ.X;
	orientation(2, 1) = NewZ.Y;
	orientation(2, 2) = NewZ.Z;

	Normalize();
}

void
Camera::Roll(double roll)
{
	if (!FMath::IsFinite(roll))
	{
		return;
	}

	const double C = FMath::Cos(roll);
	const double S = FMath::Sin(roll);

	const FVector X = vrt();
	const FVector Y = vup();

	const FVector NewX =
		X * (float)C +
		Y * (float)S;

	const FVector NewY =
		Y * (float)C -
		X * (float)S;

	orientation(0, 0) = NewX.X;
	orientation(0, 1) = NewX.Y;
	orientation(0, 2) = NewX.Z;

	orientation(1, 0) = NewY.X;
	orientation(1, 1) = NewY.Y;
	orientation(1, 2) = NewY.Z;

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
Camera::LookAt(
	const FVector& target,
	const FVector& eye,
	const FVector& up)
{
	FVector ZAxis = target - eye;

	if (!ZAxis.Normalize())
	{
		return;
	}

	FVector XAxis = FVector::CrossProduct(up, ZAxis);

	if (!XAxis.Normalize())
	{
		XAxis = FVector(1.0f, 0.0f, 0.0f);
	}

	FVector YAxis = FVector::CrossProduct(ZAxis, XAxis);

	if (!YAxis.Normalize())
	{
		YAxis = FVector(0.0f, 1.0f, 0.0f);
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

void
Camera::LookAt(const FVector& target)
{
	if (target == Pos())
	{
		return;
	}

	const FVector Tmp = target - Pos();

	FVector Tgt;
	Tgt.X = FVector::DotProduct(Tmp, vrt());
	Tgt.Y = FVector::DotProduct(Tmp, vup());
	Tgt.Z = FVector::DotProduct(Tmp, vpn());

	if (FMath::Abs(Tgt.Z) < KINDA_SMALL_NUMBER)
	{
		Pitch(0.5);
		Yaw(0.5);
		LookAt(target);
		return;
	}

	double Az = FMath::Atan2((double)Tgt.X, (double)Tgt.Z);
	double El = FMath::Atan2((double)Tgt.Y, (double)Tgt.Z);

	Pitch(-El);
	Yaw(Az);

	double Deflection = (double)vrt().Y;

	while (FMath::Abs(Deflection) > 0.001)
	{
		const double VLen = (double)vrt().Size();

		if (VLen <= 1.0e-6)
		{
			break;
		}

		const double Theta =
			FMath::Asin(Deflection / VLen);

		Roll(-Theta);

		Deflection = (double)vrt().Y;
	}

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

	const FVector Tmp = target - Pos();

	FVector Tgt;
	Tgt.X = FVector::DotProduct(Tmp, vrt());
	Tgt.Y = FVector::DotProduct(Tmp, vup());
	Tgt.Z = FVector::DotProduct(Tmp, vpn());

	if (FMath::Abs(Tgt.Z) < KINDA_SMALL_NUMBER)
	{
		Yaw(0.1);

		Tgt.X = FVector::DotProduct(Tmp, vrt());
		Tgt.Y = FVector::DotProduct(Tmp, vup());
		Tgt.Z = FVector::DotProduct(Tmp, vpn());

		if (FMath::Abs(Tgt.Z) < KINDA_SMALL_NUMBER)
		{
			return false;
		}
	}

	bool bLocked = true;

	double Az =
		FMath::Atan2((double)Tgt.X, (double)Tgt.Z);

	while (Az > PI)
	{
		Az -= 2.0 * PI;
	}

	while (Az < -PI)
	{
		Az += 2.0 * PI;
	}

	if (alimit > 0)
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

	double El =
		FMath::Atan2((double)Tgt.Y, (double)Tgt.Z);

	if (e_lo > 0 && El < -e_lo)
	{
		El = -e_lo;
		bLocked = false;
	}
	else if (e_hi > 0 && El > e_hi)
	{
		El = e_hi;
		bLocked = false;
	}

	Pitch(-El);

	Normalize();

	return bLocked;
}