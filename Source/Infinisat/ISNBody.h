// Infinisat - real mutual gravity for the Cosmos sandbox: every body pulls on every other (Newton), integrated with
// velocity Verlet (kick-drift-kick), colliding bodies merge. Units as ISOrbit: km, s, km/s, Mu = G*M in km^3/s^2.
// Masses are only ever compared as ratios, so Mu stands in for mass (G cancels).
#pragma once

#include "CoreMinimal.h"

struct FISNBody
{
	FString Name;
	double Mu = 0, Radius = 0;
	FVector3d R = FVector3d::ZeroVector, V = FVector3d::ZeroVector;
	bool bAlive = true;
};

struct FISNBodySim
{
	TArray<FISNBody> B;
	double T = 0;                 // s
	FString LastEvent;            // "Io crashed into Jupiter." - the reader clears it
	bool bLimited = false;        // the last Advance ran out of substeps: sim time lags the clock
	double StepFraction = 0.02;   // substep = this * shortest pairwise dynamical time sqrt(d^3 / (Mu_i + Mu_j)) (~1/300 orbit)

	int32 Add(const FString& Name, double Mu, double Radius, const FVector3d& R, const FVector3d& V);
	void Advance(double Dt, int32 MaxSubsteps = 4000);
	int32 Heaviest() const;
	// what i orbits: the smallest Hill sphere (of a heavier body) it is inside, else the heaviest body; -1 for the heaviest itself
	int32 Primary(int32 i) const;
	void Primaries(TArray<int32>& Out) const;   // Primary() for every body at once
	int32 Find(const FString& Name) const { for (int32 i = 0; i < B.Num(); ++i) if (B[i].Name == Name) return i; return -1; }
	double Energy() const;        // times G (constant), for drift checks
	FVector3d Momentum() const;   // times G
	void Changed() { bAccValid = false; }   // call after editing Mu / Radius / R / V from outside

private:
	TArray<FVector3d> Acc;
	double NextDt = 3600.0;
	bool bAccValid = false;
	void ComputeAcc();
	bool Collide();
};

namespace ISNBody
{
	// the ISOrbit solar system plus the big moons the Cosmos explorer shows, at ISOrbit time T0, momentum zeroed
	void SeedSolarSystem(FISNBodySim& Sim, double T0 = 0.0);
	// big moons not in ISOrbit (NASA fact sheets): circular orbits tilted like their planet's equator (Triton retrograde)
	struct FMoon { const TCHAR* Name; const TCHAR* Parent; double Mu, R, A, IncDeg, Phase; };
	TArrayView<const FMoon> Moons();
}
