// Infinisat - where a part is mounted on a satellite (the CanSat bench idea for real spacecraft).
// Every physical part goes onto one of six places on the bus; the place changes how it works in orbit:
// an imager on the zenith face photographs empty space, solar panels tucked inside the bus make no power,
// an antenna pointing away from Earth loses most of the link, an engine off the base pushes off-centre.
#pragma once

#include "CoreMinimal.h"
#include "ISData.h"
#include "ISSim.h"

namespace ISMount
{
	enum : int32 { Stack = 0, Nadir, Zenith, Wings, Base, Body, Count };

	struct FInfo { const TCHAR* Name; const TCHAR* Why; FLinearColor Color; FVector Normal; };

	inline const FInfo& Info(int32 M)
	{
		static const FInfo I[Count] = {
			{ TEXT("Avionics stack (inside)"), TEXT("Inside the bus: shielded from radiation and kept warm. The computer, battery, reaction wheels and heaters live here."), FLinearColor(0.3f, 0.55f, 1.f), FVector::ZeroVector },
			{ TEXT("Nadir face (looks at Earth)"), TEXT("This face always points down at Earth: cameras, instruments and the downlink antenna must be here to see the ground."), FLinearColor(0.3f, 0.95f, 0.45f), FVector(1, 0, 0) },
			{ TEXT("Zenith face (looks at space)"), TEXT("Points away from Earth, at the sky: GNSS antennas to hear the navigation satellites, radiators to dump heat into cold space."), FLinearColor(0.3f, 0.9f, 1.f), FVector(-1, 0, 0) },
			{ TEXT("Solar wings"), TEXT("Deployable wings turn to follow the Sun: the full panel area makes power all orbit long (except in Earth's shadow)."), FLinearColor(1.f, 0.85f, 0.2f), FVector(0, 1, 0) },
			{ TEXT("Base (separation ring)"), TEXT("Engines push through the centre of mass, here at the base; the rocket's adapter ring is here too."), FLinearColor(1.f, 0.55f, 0.15f), FVector(0, 0, -1) },
			{ TEXT("Side panels"), TEXT("Body-mounted cells, booms and sails go on the sides. Body cells only make power when that side faces the Sun."), FLinearColor(0.75f, 0.4f, 1.f), FVector(0, -1, 0) },
		};
		return I[FMath::Clamp(M, 0, Count - 1)];
	}

	// parts without a place on the bus: the bus itself (it is the frame) and the ground network (on the ground)
	inline bool Physical(const FISPart* P) { return P && P->Cat != EISCat::Bus && P->Cat != EISCat::Ground && !P->Id.ToString().EndsWith(TEXT("_none")); }

	inline int32 Ideal(const FISPart* P)
	{
		if (!P) return Stack;
		switch (P->Cat)
		{
		case EISCat::Power: return P->RtgW > 0 ? Body : (P->Id == FName("pwr_body") ? Body : Wings);
		case EISCat::Battery: case EISCat::Computer: case EISCat::ADCS: case EISCat::Thermal: return Stack;
		case EISCat::Comms: return Nadir;
		case EISCat::Propulsion: return Base;
		default: break;
		}
		if (P->Tag == ISTag::GNSSRx) return Zenith;
		if (P->Tag == ISTag::Clock || P->Tag == ISTag::Heaters || P->Tag == ISTag::Redundancy || P->Tag == ISTag::Wiring) return Stack;
		if (P->Tag == ISTag::Fields || P->Tag == ISTag::Record || P->Tag == ISTag::Vault || P->Tag == ISTag::DragSail || P->Tag == ISTag::SolarSail) return Body;
		if (P->Tag == ISTag::EntryProbe || P->Tag == ISTag::Retro) return Base;
		if (P->Cat == EISCat::Payload) return Nadir;
		return Body;
	}

	// where it is: the player's choice, else the usual place (sandbox, codes, older saves)
	inline int32 Of(const FISDesign& D, const FISPart* P)
	{
		if (!P) return Stack;
		if (const uint8* M = D.Mount.Find(P->Id)) return *M;
		return Ideal(P);
	}

	// instruments that must see the ground
	inline bool LooksDown(const FISPart* P)
	{
		return P && (P->Tag == ISTag::Imager || P->Tag == ISTag::WeatherImager || P->Tag == ISTag::Sounder || P->Tag == ISTag::Altimeter
			|| P->Tag == ISTag::Radiometer || P->Tag == ISTag::Camera || P->Tag == ISTag::Transponder || P->Tag == ISTag::NavPayload || P->Tag == ISTag::Neutron);
	}

	// share of the panel power that a power part makes on this mount
	inline float SolarFactor(const FISPart* P, int32 M)
	{
		if (!P || P->RtgW > 0) return 1.f;
		switch (M)
		{
		case Wings: return 1.f;
		case Body: return 0.35f;
		case Zenith: return 0.6f;
		case Nadir: return 0.15f;
		case Base: return 0.1f;
		default: return 0.f;
		}
	}

	inline TArray<FString> Issues(const FISDesign& D)
	{
		TArray<FString> I;
		for (const FName& Id : D.Parts)
		{
			const FISPart* P = ISData::FindPart(Id);
			if (!Physical(P)) continue;
			const int32 M = Of(D, P);
			if (M == Ideal(P)) continue;
			if (P->Cat == EISCat::Power && P->SolarW > 0)
				I.Add(FString::Printf(TEXT("%s on the %s makes only %d%% of its power."), *P->Name, Info(M).Name, FMath::RoundToInt(SolarFactor(P, M) * 100.f)));
			else if (LooksDown(P))
				I.Add(FString::Printf(TEXT("%s is not on the nadir face: it cannot see the Earth, no useful data."), *P->Name));
			else if (P->Cat == EISCat::Comms)
				I.Add(FString::Printf(TEXT("%s does not point at Earth: the link drops to about 15%%."), *P->Name));
			else if (P->Cat == EISCat::Propulsion)
				I.Add(FString::Printf(TEXT("%s is off the base: it pushes off-centre, half the delta-v goes into spinning."), *P->Name));
			else if (P->Cat == EISCat::Computer)
				I.Add(FString::Printf(TEXT("%s is outside the stack: unshielded, it takes twice the radiation."), *P->Name));
			else if (P->Cat == EISCat::Battery)
				I.Add(FString::Printf(TEXT("%s is outside the stack: in the cold it holds about 70%% of its charge."), *P->Name));
			else
				I.Add(FString::Printf(TEXT("%s: the usual place is the %s."), *P->Name, Info(Ideal(P)).Name));
		}
		return I;
	}
}
