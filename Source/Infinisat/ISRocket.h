// Infinisat - rocket ascent physics (no engine dependencies beyond core math).
// Planar flight in the launch plane around a spherical, rotating Earth:
//   gravity (mu/r^2), US-standard-like atmosphere, Mach-dependent drag, pressure-dependent thrust (SL -> vacuum),
//   mass flow from Isp, serial stages + strap-on boosters, fairing jettison, max-Q structural limit.
// Real stage data (public figures, rounded): see ISRocket.cpp and Docs/References.md (Braeunig, user guides).
#pragma once

#include "CoreMinimal.h"

struct FISEngineStage
{
	FString Name;
	double DryKg = 0, PropKg = 0;
	double ThrustSLN = 0, ThrustVacN = 0;   // whole stage (all engines)
	double IspSL = 0, IspVac = 0;
	double MinThrottle = 1.0;               // solids: 1.0 (cannot throttle), liquids ~0.4-0.6
	bool bSolid = false;
	double DiameterM = 1, LengthM = 10;
	FLinearColor Color = FLinearColor::White;
	// strap-on boosters that burn together with this stage and drop when empty
	int32 Boosters = 0;
	double BoosterDryKg = 0, BoosterPropKg = 0, BoosterThrustSLN = 0, BoosterThrustVacN = 0, BoosterIspSL = 0, BoosterIspVac = 0, BoosterDiameterM = 1, BoosterLengthM = 10;
	bool bBoosterSolid = true;
};

struct FISLauncher
{
	FName Id;
	FString Name;
	TArray<FISEngineStage> Stages;   // bottom to top
	double FairingKg = 0, FairingDiameterM = 1, FairingLengthM = 2;
	double MaxQPa = 45000;            // structural limit with the nose pointed into the wind
	double SiteLatDeg = 28.5;
	FString Site;
	static const FISLauncher* Find(FName VehicleId);   // maps the game's vehicle ids to real launchers
};

enum class EISFlightPhase : uint8 { OnPad, Countdown, Ascent, Coast, Orbit, Failed };

struct FISFlightEvent { double T; FString Text; };

class FISRocketSim
{
public:
	// payload kg, target circular altitude km, target inclination deg
	void Setup(const FISLauncher& L, double PayloadKg, double TargetAltKm, double IncDeg);
	void StartCountdown(double Seconds = 10.0);
	void Step(double Dt);           // fixed-step integrator (call with dt <= 0.02 s)

	// --- controls
	double Throttle = 1.0;          // 0..1 (clamped to the stage's min throttle while burning; 0 = cut-off)
	double PitchDeg = 0.0;          // thrust angle from local vertical toward downrange
	bool bAutopilot = true;         // guidance computer flies the ascent (the player can take over)
	void Stage();                   // separate the current stage (or start the countdown on the pad)
	void Abort(const FString& Why);

	// --- state (SI units, Earth-centred inertial, planar: x = downrange, y = up at the launch site)
	FVector2D R = FVector2D(0, 6371000.0), V = FVector2D::ZeroVector;
	double T = -10.0;               // mission time, s (negative during the countdown)
	EISFlightPhase Phase = EISFlightPhase::OnPad;
	int32 StageIdx = 0;
	bool bBoostersOn = false, bFairingOn = true;
	double StageProp = 0, BoosterProp = 0;
	double MaxQSeen = 0, MaxAoASeen = 0, MaxGimbalSeen = 0;   // max AoA counted only in the air (q > 2 kPa)
	FString FailReason;
	TArray<FISFlightEvent> Events;

	// --- derived (updated every step)
	double AltM = 0, Speed = 0, SurfSpeed = 0, Mach = 0, QPa = 0, ApoKm = 0, PeriKm = 0, MassKg = 0, ThrustN = 0, TWR = 0, AoADeg = 0, DownrangeKm = 0;
	double DeltaVLeft() const;      // ideal vacuum delta-v of everything still attached
	double TargetAltKm = 400, IncDeg = 51.6, PayloadKg = 100, LaunchAzDeg = 90, EarthBoostMs = 0, LiftoffDv = 0;
	const FISLauncher* L = nullptr;
	bool IsFlying() const { return Phase == EISFlightPhase::Ascent || Phase == EISFlightPhase::Coast; }
	bool IsIgniting() const { return Phase == EISFlightPhase::Countdown && ThrustN > 0.0; }   // engines running, still clamped to the pad
	double Spool = 0.0;             // 0..1 engine start-up on the pad (1 once released)
	FVector2D ThrustDir() const;    // unit vector of the current thrust (nose direction tilted by the engine gimbal)
	FVector2D NoseDir() const;      // where the vehicle actually points

	// --- rigid-body attitude (the idea of Principia's Part -> PileUp: every attached piece adds mass, a centre of mass and
	// an inertia tensor; thrust that is off the centre line makes torque). PitchDeg above is only the COMMAND: the flight
	// computer swings the engines (gimbal) to turn the vehicle there, fighting the air that tries to flip a finless rocket.
	double AttDeg = 0;              // actual nose angle from local vertical toward downrange
	double RateDps = 0;             // pitch rate, deg/s (inertial)
	double GimbalDeg = 0;           // engine deflection in use (+ turns the nose downrange)
	double GimbalMaxDeg = 5;
	double InertiaKgm2 = 0, CoMHeightM = 0, StackLenM = 0, AeroTorqueNm = 0;
	bool bRcs = false;              // attitude thrusters doing the turning (no main-engine thrust)

	// --- trajectory, as Principia keeps it: an authoritative history (1 point per second of flight) and a prediction
	// (where the vehicle goes if every engine stopped now: gravity + drag, symplectic integration, ends at the ground or
	// after one revolution)
	TArray<FVector2D> History;
	void Predict(TArray<FVector2D>& Out, int32 MaxPoints = 400) const;

	FVector2D Up() const { return R.GetSafeNormal(); }
	FVector2D East() const { const FVector2D U = Up(); return FVector2D(U.Y, -U.X); }

	static constexpr double Mu = 3.986004418e14, Re = 6371000.0, G0 = 9.80665, OmegaE = 7.2921159e-5;
	static double Density(double H);
	static double PressureRatio(double H);
	static double SoundSpeed(double H);

private:
	void Event(const FString& S) { Events.Add({ T, S }); }
	void UpdateDerived();
	void Guidance(double Dt);
	double StackMass() const;
	void UpdateMassProperties();    // centre of mass, moment of inertia, length of what is still attached
	void StepAttitude(double Dt);
	double ThetaI = 0;              // inertial nose angle (rad, from +Y toward +X)
	double OmegaI = 0;              // inertial pitch rate (rad/s)
	double HistT = -1e9;
public:
	double TimeToApoapsis() const;
private:
	double CountdownT = 0;
	bool bMaxQAnnounced = false, bMaxQPassed = false, bCircBurn = false, bCircActive = false, bIgnition = false;
};
