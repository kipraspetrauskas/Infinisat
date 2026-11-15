// Infinisat - the flight plan of a deep-space mission as patched conics (km, s; bodies from ISOrbit):
// parking orbit -> departure burn -> transfer (Lambert-targeted) -> arrival hyperbola -> orbit insertion -> science orbit.
// Every leg is a two-body conic around one body, propagated with ISOrbit::Kepler; positions join exactly at the leg
// boundaries (the targeting aims at the next leg's entry point), velocities change only at burns.
#pragma once

#include "CoreMinimal.h"

struct FISLeg
{
	int32 Body = 1;            // ISOrbit body the conic is relative to
	double T0 = 0, T1 = 1e30;  // mission seconds this leg covers
	double TRef = 0;           // the state below is at TRef
	FVector3d R = FVector3d::ZeroVector, V = FVector3d::ZeroVector;
};

struct FISBurn { double T = 0, DvMs = 0; FString What; };

struct FISTrajectory
{
	double Epoch = 0;          // ISOrbit time (s) at mission second 0: chosen so the planets are where the transfer needs them
	TArray<FISLeg> Legs;
	TArray<FISBurn> Burns;
	double DepartT = 0, ArriveT = 0;   // departure burn, orbit insertion (mission seconds)
	int32 Target = -1;         // destination body (-1: none, interstellar)
	bool bValid = false;

	int32 LegAt(double T) const;
	// state relative to the leg's body; returns that body index (-1 if empty)
	int32 State(double T, FVector3d& R, FVector3d& V) const;
	FVector3d HelioPos(double T) const;                      // relative to the Sun
	FVector3d PosAround(int32 Body, double T) const;         // relative to any body (e.g. Earth for the 3D scene)

	// ParkR/ParkV: the parking orbit the rocket really reached, Earth-centred, at mission second 0
	static FISTrajectory Moon(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double ArriveDay, double AltKm);
	static FISTrajectory Jupiter(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double ArriveDay, double PeriAltKm);
	static FISTrajectory Interstellar(const FVector3d& ParkR, const FVector3d& ParkV, double DepartDay, double EndDay);
};
