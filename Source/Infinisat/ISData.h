// Infinisat - game data: parts, launch vehicles, ground services, missions.
// All numbers are simplified but grounded in real spacecraft (sources noted in Edu text).
#pragma once

#include "CoreMinimal.h"

enum class EISCat : uint8
{
	Bus, Power, Battery, ADCS, Comms, Propulsion, Computer, Thermal, Payload, Extra, Ground, Count
};

enum class EISDest : uint8
{
	Backyard, LEO, SSO, MEO, GEO, Moon, Jupiter, Interstellar, Count
};

// Payload / capability tags used by mission requirements
namespace ISTag
{
	static const FName Parachute(TEXT("Parachute"));
	static const FName TempSensor(TEXT("TempSensor"));
	static const FName Camera(TEXT("Camera"));
	static const FName GNSSRx(TEXT("GNSSRx"));
	static const FName Clock(TEXT("Clock"));
	static const FName NavPayload(TEXT("NavPayload"));
	static const FName Imager(TEXT("Imager"));
	static const FName WeatherImager(TEXT("WeatherImager"));
	static const FName Sounder(TEXT("Sounder"));
	static const FName Transponder(TEXT("Transponder"));
	static const FName Altimeter(TEXT("Altimeter"));
	static const FName Neutron(TEXT("Neutron"));
	static const FName Fields(TEXT("Fields"));
	static const FName Radiometer(TEXT("Radiometer"));
	static const FName EntryProbe(TEXT("EntryProbe"));
	static const FName Record(TEXT("Record"));
	static const FName Vault(TEXT("Vault"));
	static const FName Redundancy(TEXT("Redundancy"));
	static const FName DragSail(TEXT("DragSail"));
	static const FName SolarSail(TEXT("SolarSail"));
	static const FName Retro(TEXT("Retro"));
	static const FName Heaters(TEXT("Heaters"));
	static const FName RTG(TEXT("RTG"));
	static const FName HGA(TEXT("HGA"));
	static const FName Wiring(TEXT("Wiring"));
	static const FName Buzzer(TEXT("Buzzer"));
	static const FName IMU(TEXT("IMU"));        // gyro + accelerometer (CanSat MPU6050)
	static const FName SDLog(TEXT("SDLog"));    // onboard data logger (CanSat microSD)
}

struct FISPart
{
	FName Id;
	EISCat Cat = EISCat::Extra;
	FString Name;
	float MassKg = 0.f;        // dry mass of the part
	float PowerW = 0.f;        // average power draw
	float SolarW = 0.f;        // solar generation at 1 AU, in sunlight (beginning of life)
	float RtgW = 0.f;          // constant RTG power (beginning of life)
	float BatteryWh = 0.f;
	float CostM = 0.f;         // million USD
	float RelPerYear = 0.999f; // probability the part survives one year
	float MaxTotalKg = 0.f;    // Bus only: max spacecraft mass the structure supports
	float PointingDeg = 0.f;   // ADCS only: pointing accuracy (0 = cannot point)
	float RadioMbps = 0.f;     // Comms: max downlink data rate
	float RadioRangeKm = 0.f;  // Comms: for small radios (CanSat)
	float DishM = 0.f;         // Comms: antenna diameter (for deep-space link budget)
	float TxW = 0.f;           // Comms: transmitter RF power
	float PropKg = 0.f;        // Propulsion: propellant mass
	float IspS = 0.f;          // Propulsion: specific impulse
	float SailM2 = 0.f;        // Solar sail area / drag sail area / parachute area
	float ResolutionM = 0.f;   // Imagers: ground resolution at RefAltKm
	float RefAltKm = 700.f;    // Imagers: altitude at which ResolutionM applies
	float DataGbDay = 0.f;     // Payload: data generated per day
	float RadTolKrad = 0.f;    // Computer: total radiation dose tolerated
	FName Tag;                 // capability tag (see ISTag)
	int32 Unlock = 0;          // chapter that unlocks it
	FString MeshPath;          // optional mesh for 3D assembly
	FLinearColor Color = FLinearColor(0.7f, 0.7f, 0.7f);
	FString Edu;               // what it is + why it matters (educational text)
};

struct FISVehicle
{
	FName Id;
	FString Name;
	FString Site;
	float CostM = 0.f;
	float CostPerKgM = 0.f;     // rideshare pricing
	float Reliability = 0.95f;
	float Capacity[(int32)EISDest::Count] = {};
	FString Edu;
};

struct FISQuiz
{
	FString Q;
	TArray<FString> A;
	int32 Correct = 0;
	FString Why;
};

struct FISMission
{
	FName Id;
	int32 Chapter = 0;
	FString Title;
	FString Client;
	FString Brief;
	EISDest Dest = EISDest::LEO;
	float AltKm = 500.f;          // orbit altitude above the central body
	float IncDeg = 0.f;
	float BudgetM = 100.f;
	float LifeYears = 1.f;        // required mission life
	TArray<FName> Required;       // required capability tags
	TArray<FName> Bonus;          // optional tags that give bonus score
	float MaxResolutionM = 0.f;   // 0 = not required
	float MaxPointingDeg = 0.f;   // 0 = not required
	float MinDataGbDay = 0.f;     // downlinked data per day
	float MaxMassKg = 0.f;        // hard mass limit (CanSat)
	int32 Fleet = 1;              // number of identical satellites needed
	TArray<FString> Learn;        // learning objectives
	FString RealName;             // real mission to compare with
	FString RealMesh;
	FString RealFacts;
	TArray<FISQuiz> Quiz;
	FString Lesson;               // key takeaway shown in debrief
};

namespace ISData
{
	const TArray<FISPart>& Parts();
	const TArray<FISVehicle>& Vehicles();
	const TArray<FISMission>& Missions();
	const FISPart* FindPart(FName Id);
	const FISVehicle* FindVehicle(FName Id);
	FString CatName(EISCat C);
	FString CatHelp(EISCat C);
	FString DestName(EISDest D);

	// Physical constants
	constexpr double MuEarth = 398600.4418;   // km^3/s^2
	constexpr double REarth = 6371.0;         // km
	constexpr double MuMoon = 4902.8;
	constexpr double RMoon = 1737.4;
	constexpr double MuJupiter = 126686534.0;
	constexpr double RJupiter = 69911.0;
	constexpr double G0 = 9.80665;            // m/s^2
	constexpr double SolarConst = 1361.0;     // W/m^2 at 1 AU
}
