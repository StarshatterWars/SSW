/*  Project nGenEx
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Camera.h
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Camera class - Position and Point of View
*/

// +--------------------------------------------------------------------+
#pragma once
#include "Types.h"
#include "Geometry.h"      // Matrix, orientation math (Starshatter core types)
#include "Math/Vector.h"   // FVector (Unreal)

// +--------------------------------------------------------------------+


class Camera
{
public:
	static const char* TYPENAME() { return "Camera"; }

	Camera(double x = 0.0, double y = 0.0, double z = 0.0);
	virtual ~Camera();

	void     Aim(double roll, double pitch, double yaw) { orientation.Rotate(roll, pitch, yaw); }
	void     Roll(double roll) { orientation.Roll(roll); }
	void     Pitch(double pitch) { orientation.Pitch(pitch); }
	void     Yaw(double yaw) { orientation.Yaw(yaw); }

	void     MoveTo(double x, double y, double z);
	void     MoveTo(const FVector& p);
	void     MoveBy(double dx, double dy, double dz);
	void     MoveBy(const FVector& p);

	void     Clone(const Camera& cam);
	void     LookAt(const FVector& target);
	void     LookAt(const FVector& target, const FVector& eye, const FVector& up);
	bool     Padlock(const FVector& target, double alimit = -1, double e_lo = -1, double e_hi = -1);

	FVector        Pos() const { return pos; }
	FVector        vrt() const { return FVector((float)orientation(0, 0), (float)orientation(0, 1), (float)orientation(0, 2)); }
	FVector        vup() const { return FVector((float)orientation(1, 0), (float)orientation(1, 1), (float)orientation(1, 2)); }
	FVector        vpn() const { return FVector((float)orientation(2, 0), (float)orientation(2, 1), (float)orientation(2, 2)); }

	const Matrix& Orientation() const { return orientation; }

	static Camera* emergency_cam;

protected:
	FVector  pos;
	Matrix   orientation;
};


