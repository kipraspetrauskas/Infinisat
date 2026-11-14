// Infinisat - mission evaluation and operations simulator.
// Everything here is deliberately transparent: each budget line carries the formula and
// the numbers that produced it, so the player can always ask "why?".
#pragma once

#include "CoreMinimal.h"
#include "ISData.h"

struct FISDesign
{
	TArray<FName> Parts;          // chosen parts (one per single-choice category, many for Payload/Extra)
	FName Vehicle;
	int32 Fleet = 1;
	EISDest Dest = EISDest::LEO;  // used by sandbox; missions force their own
	float AltKm = 500.f;

	bool Has(FName Id) const { return Parts.Contains(Id); }
	FName GetSingle(EISCat Cat) const;
	void SetSingle(EISCat Cat, FName Id);
	void Toggle(FName Id);
	bool HasTag(FName Tag) const;
};

enum class EISStatus : uint8 { Ok, Warn, Fail, Info };

struct FISLine
{
	FString Label;
	FString Value;
	FString Limit;
	EISStatus Status = EISStatus::Info;
	FString Why;      // formula + numbers + what to change
};

struct FISObjective
{
	FString Text;
	bool bMet = false;
	bool bBonus = false;
	FString Why;
	FName Needs;      // capability tag this objective depends on (for operations losses)
};

struct FISEval
{
	// Context
	EISDest Dest = EISDest::LEO;
	float AltKm = 0.f;
	float LifeYears = 1.f;
	double SunAU = 1.0;
	double PeriodMin = 0.0;
	double EclipseMin = 0.0;
	// Mass
	float DryKg = 0.f, PropKg = 0.f, TotalKg = 0.f, BusMaxKg = 0.f;
	// Power
	float LoadW = 0.f, SolarEolW = 0.f, RtgEolW = 0.f, AvgGenW = 0.f, BatteryWh = 0.f, BatteryNeedWh = 0.f;
	// Pointing / data
	float PointingDeg = 0.f, ResolutionM = 0.f, DataGenGb = 0.f, DataDownGb = 0.f, LinkKbps = 0.f, ContactMin = 0.f;
	// Propulsion
	float DvAvail = 0.f, DvNeed = 0.f;
	// Life & reliability
	float LifeEst = 0.f;
	FString LifeLimiter;
	float RelSat = 1.f, RelLaunch = 1.f;
	float RadDose = 0.f, RadTol = 0.f;
	// Interstellar
	float EscapeKms = 0.f, YearsTo120AU = 0.f;
	// Cost
	float SatCostM = 0.f, FleetCostM = 0.f, LaunchCostM = 0.f, GroundCostM = 0.f, TotalCostM = 0.f;
	int32 Launches = 1;
	float VehicleCap = 0.f;
	// CanSat
	float DescentMs = 0.f, RuntimeH = 0.f, RadioRangeKm = 0.f, FlightAltKm = 0.f;

	TArray<FISLine> Lines;
	TArray<FISObjective> Objectives;
	TArray<FString> Blockers;     // must fix before launch
	FString Hint;                 // BIT's top advice
	bool bCanLaunch = false;
};

namespace ISSim
{
	FISEval Evaluate(const FISDesign& D, const FISMission& M);
	FString Fmt(double V, int32 Decimals = 1);
	FString FmtMass(double Kg);
	FString FmtMoney(double M);
	FString FmtDays(double Days);
	double NaturalDecayYears(double AltKm);
	EISDest EffectiveDest(const FISDesign& D, const FISMission& M);
	float EffectiveAlt(const FISDesign& D, const FISMission& M);
	float DefaultAlt(EISDest Dest);
}

// ------------------------------------------------------------------ Operations
enum class EISEvt : uint8 { Info, Good, Warn, Anomaly, Fatal, End };

struct FISEvent
{
	double Day = 0.0;
	EISEvt Type = EISEvt::Info;
	FString Text;
	FName Part;                 // part involved (anomalies)
	FString Edu;                // what real engineers learned / did
};

struct FISChoice
{
	FString Label;
	FString Result;
};

class FISOps
{
public:
	void Start(const FISDesign& D, const FISMission& M, const FISEval& E, int32 Seed);
	// Advance simulation time. Returns true if something new was logged.
	bool Advance(double DeltaDays);
	bool IsPaused() const { return bAwaitingChoice || bFinished; }
	bool IsFinished() const { return bFinished; }
	bool AwaitingChoice() const { return bAwaitingChoice; }
	const FISEvent& PendingEvent() const { return Pending; }
	const TArray<FISChoice>& Choices() const { return PendingChoices; }
	void Choose(int32 Index);

	double Day = 0.0;
	double EndDay = 1.0;
	double DataGb = 0.0;
	double Battery = 1.0;   // 0..1 for telemetry display
	double Health = 1.0;
	bool bLaunchFailed = false;
	bool bMissionLost = false;
	double LostDay = -1.0;
	TArray<FISEvent> Log;
	TMap<FName, double> LostTagsDay;  // capability tag -> day it was lost
	TArray<FName> LostParts;

	// Final results
	TArray<FISObjective> FinalObjectives;
	int32 Stars = 0;
	float ScorePct = 0.f;
	FString Debrief;

private:
	void Schedule(double Day, EISEvt T, const FString& Text, FName Part = NAME_None, const FString& Edu = FString());
	void Finish();
	void LosePart(FName Part, double AtDay);
	bool IsCritical(const FISPart* P) const;

	FISDesign Design;
	FISMission Mission;
	FISEval Eval;
	FRandomStream Rng;
	TArray<FISEvent> Queue;
	int32 Next = 0;
	bool bAwaitingChoice = false;
	bool bFinished = false;
	FISEvent Pending;
	TArray<FISChoice> PendingChoices;
	TArray<int32> ChoiceKinds;
	bool bHasRedundancy = false;
};
