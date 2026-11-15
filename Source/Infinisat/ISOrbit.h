// Infinisat - real orbital mechanics: two-body Kepler propagation (universal variables),
// patched conics with spheres of influence, and a small solar system with real sizes and orbits.
// Units: km, s, km/s. Doubles everywhere (distances reach billions of km).
#pragma once

#include "CoreMinimal.h"

struct FISBody
{
	FString Name;
	int32 Parent = -1;        // index of the body it orbits (-1 = Sun, fixed at origin)
	double Mu = 0;            // km^3/s^2
	double Radius = 0;        // km
	double OrbitR = 0;        // km, circular orbit radius around parent
	double OrbitInc = 0;      // deg
	double Phase0 = 0;        // rad at t = 0
	double Soi = 0;           // km, sphere of influence radius
	double AtmoTop = 0;       // km, altitude where the atmosphere ends (0 = none)
	FLinearColor Color = FLinearColor::White;
	FString Texture;          // planet texture asset path (optional)
};

struct FISElements
{
	double A = 0, E = 0, Inc = 0, Raan = 0, ArgP = 0, Nu = 0; // a km, angles deg
	double Periapsis = 0, Apoapsis = 0;  // altitudes above the body surface, km (apo < 0 = escape)
	double Period = 0;                   // s (0 for escape trajectories)
};

namespace ISOrbit
{
	const TArray<FISBody>& Bodies();
	int32 FindBody(const FString& Name);
	// position / velocity of a body relative to the Sun (or to its parent if bLocal)
	FVector3d BodyPos(int32 Index, double T, bool bLocal = false);
	FVector3d BodyVel(int32 Index, double T, bool bLocal = false);

	// propagate a two-body state by Dt seconds (works for ellipses and hyperbolas)
	void Kepler(double Mu, FVector3d& R, FVector3d& V, double Dt);
	FISElements Elements(double Mu, double BodyRadius, const FVector3d& R, const FVector3d& V);
	double CircularSpeed(double Mu, double R);
	// Lambert's problem: the conic from R1 to R2 in Tof seconds, flown prograde about Normal. False if it did not converge
	// (e.g. a transfer angle of exactly 180 deg, where the plane is undefined).
	bool Lambert(double Mu, const FVector3d& R1, const FVector3d& R2, double Tof, const FVector3d& Normal, FVector3d& V1, FVector3d& V2);
	// time from periapsis to radius R on a hyperbola with excess speed Vinf and periapsis radius Rp (s)
	double HyperbolicTime(double Mu, double Rp, double Vinf, double R);
}

// A spacecraft flying on patched conics.
struct FISCraft
{
	int32 Body = 1;           // body whose SOI we are in
	FVector3d R = FVector3d::ZeroVector, V = FVector3d::ZeroVector;  // relative to Body
	double DvUsed = 0;
	bool bCrashed = false;
	FString LastEvent;

	void Step(double T, double Dt);                 // advance with SOI transitions, crash detection
	void Burn(double Prograde, double Normal, double Radial);   // impulsive burn, km/s
	FISElements Elements() const;
	// sample the predicted path for drawing (relative to Body); stops at SOI exit / surface
	void Predict(double T, int32 Samples, TArray<FVector3d>& OutPts, int32& OutNextBody) const;
	void Place(int32 InBody, double AltKm, double IncDeg, double LongDeg);  // circular parking orbit
};
