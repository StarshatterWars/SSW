#pragma once

#include "Types.h"
#include "Geometry.h"

#include "Math/Vector.h"

class Camera
{
public:
	static const char* TYPENAME() { return "Camera"; }

	Camera(double x = 0.0, double y = 0.0, double z = 0.0);
	virtual ~Camera();

	void Aim(double roll, double pitch, double yaw);
	void Roll(double roll);
	void Pitch(double pitch);
	void Yaw(double yaw);

	void MoveTo(double x, double y, double z);
	void MoveTo(const FVector& p);
	void MoveBy(double dx, double dy, double dz);
	void MoveBy(const FVector& p);

	void Clone(const Camera& cam);

	void LookAt(const FVector& target);
	void LookAt(const FVector& target, const FVector& eye, const FVector& up);

	bool Padlock(
		const FVector& target,
		double alimit = -1,
		double e_lo = -1,
		double e_hi = -1);

	FVector Pos() const { return pos; }

	FVector vrt() const
	{
		return FVector(
			orientation(0, 0),
			orientation(0, 1),
			orientation(0, 2));
	}

	FVector vup() const
	{
		return FVector(
			orientation(1, 0),
			orientation(1, 1),
			orientation(1, 2));
	}

	FVector vpn() const
	{
		return FVector(
			orientation(2, 0),
			orientation(2, 1),
			orientation(2, 2));
	}

	void SetOrientation(
		const FVector& Right,
		const FVector& Up,
		const FVector& Forward);

	const Matrix& Orientation() const
	{
		return orientation;
	}

	void Normalize();

protected:
	FVector pos;
	Matrix orientation;

	
};