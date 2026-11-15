#include "ISSim.h"

// =====================================================================================
//  Design helpers
// =====================================================================================
FName FISDesign::GetSingle(EISCat Cat) const
{
	for (const FName& Id : Parts)
	{
		const FISPart* P = ISData::FindPart(Id);
		if (P && P->Cat == Cat) return Id;
	}
	return NAME_None;
}

void FISDesign::SetSingle(EISCat Cat, FName Id)
{
	Parts.RemoveAll([Cat](const FName& X) { const FISPart* P = ISData::FindPart(X); return P && P->Cat == Cat; });
	if (!Id.IsNone()) Parts.Add(Id);
}

void FISDesign::Toggle(FName Id)
{
	if (Parts.Contains(Id)) Parts.Remove(Id); else Parts.Add(Id);
}

bool FISDesign::HasTag(FName Tag) const
{
	for (const FName& Id : Parts)
	{
		const FISPart* P = ISData::FindPart(Id);
		if (P && P->Tag == Tag) return true;
	}
	return false;
}

namespace
{
	FString TagLabel(FName T)
	{
		static TMap<FName, FString> M = {
			{ ISTag::Parachute, TEXT("a parachute") }, { ISTag::TempSensor, TEXT("a temperature sensor") },
			{ ISTag::Camera, TEXT("a camera") }, { ISTag::GNSSRx, TEXT("a GNSS receiver") },
			{ ISTag::Clock, TEXT("atomic clocks") }, { ISTag::NavPayload, TEXT("a navigation signal payload") },
			{ ISTag::Imager, TEXT("an imager") }, { ISTag::WeatherImager, TEXT("a weather imager") },
			{ ISTag::Sounder, TEXT("a sounder") }, { ISTag::Transponder, TEXT("broadband transponders") },
			{ ISTag::Altimeter, TEXT("a laser altimeter") }, { ISTag::Neutron, TEXT("a neutron spectrometer") },
			{ ISTag::Fields, TEXT("a magnetometer + particle detectors") }, { ISTag::Radiometer, TEXT("a microwave radiometer") },
			{ ISTag::EntryProbe, TEXT("an atmospheric entry probe") }, { ISTag::Record, TEXT("a golden record") },
			{ ISTag::Vault, TEXT("a radiation vault") }, { ISTag::Redundancy, TEXT("redundant electronics") },
			{ ISTag::DragSail, TEXT("a deorbit drag sail") }, { ISTag::SolarSail, TEXT("a solar sail") },
			{ ISTag::Retro, TEXT("a laser retroreflector") }, { ISTag::HGA, TEXT("a deep-space high-gain antenna") },
		};
		const FString* S = M.Find(T);
		return S ? *S : T.ToString();
	}

	const TCHAR* ADCS_TAG = TEXT("ADCS");

	struct FBody { double Mu = 0, R = 0; bool bValid = false; };
	FBody BodyFor(EISDest D)
	{
		switch (D)
		{
		case EISDest::LEO: case EISDest::SSO: case EISDest::MEO: case EISDest::GEO:
			return { ISData::MuEarth, ISData::REarth, true };
		case EISDest::Moon: return { ISData::MuMoon, ISData::RMoon, true };
		default: return {};
		}
	}

	double SunDistanceAU(EISDest D)
	{
		if (D == EISDest::Jupiter) return 5.2;
		if (D == EISDest::Interstellar) return 120.0;
		return 1.0;
	}

	// Distance used for the radio link (km)
	double LinkDistanceKm(EISDest D, float AltKm)
	{
		switch (D)
		{
		case EISDest::Backyard: return FMath::Max(2.0, AltKm * 1.5);
		case EISDest::LEO: case EISDest::SSO: return 3000.0;
		case EISDest::MEO: return 26000.0;
		case EISDest::GEO: return 40000.0;
		case EISDest::Moon: return 405000.0;
		case EISDest::Jupiter: return 5.2 * 1.496e8;
		case EISDest::Interstellar: return 120.0 * 1.496e8;
		default: return 1000.0;
		}
	}

	// Minutes per day the ground segment can talk to the spacecraft
	float ContactMinutes(FName G, EISDest D, FString& Note)
	{
		const bool bLow = D == EISDest::LEO || D == EISDest::SSO;
		const bool bDeep = D == EISDest::Jupiter || D == EISDest::Interstellar;
		if (D == EISDest::Backyard) { Note = TEXT("CanSat is in view the whole flight."); return 1440.f; }
		if (G == "gnd_yard")
		{
			if (bLow) { Note = TEXT("~4 short passes/day over your garden (~6 min each)."); return 24.f; }
			Note = TEXT("A garden antenna is far too small to hear a satellite this far away."); return 0.f;
		}
		if (G == "gnd_own")
		{
			if (bLow) { Note = TEXT("One station sees a low satellite ~5 passes/day, ~8 min each."); return 40.f; }
			if (D == EISDest::MEO) { Note = TEXT("A MEO satellite is visible from one station ~1/3 of the time."); return 480.f; }
			if (D == EISDest::GEO) { Note = TEXT("A GEO satellite never moves in the sky: always in view."); return 1440.f; }
			if (D == EISDest::Moon) { Note = TEXT("The Moon is above your horizon ~10 h/day."); return 600.f; }
			Note = TEXT("One small station cannot close the link at planetary distances: you need huge dishes."); return 0.f;
		}
		if (G == "gnd_net")
		{
			if (bLow) { Note = TEXT("A global network (incl. polar stations) gives ~10x more passes."); return 300.f; }
			if (bDeep) { Note = TEXT("Commercial networks don't have deep-space dishes."); return 0.f; }
			Note = TEXT("Stations around the world keep the satellite in view almost all the time."); return D == EISDest::Moon ? 1200.f : 1440.f;
		}
		if (G == "gnd_relay")
		{
			if (bLow) { Note = TEXT("Relay satellites in GEO see you ~85% of every orbit."); return 1200.f; }
			if (D == EISDest::MEO || D == EISDest::GEO) { Note = TEXT("Relays help a bit at higher orbits."); return 1000.f; }
			Note = TEXT("Earth relay satellites point at Earth orbits, not beyond the Moon."); return 0.f;
		}
		if (G == "gnd_dsn")
		{
			if (bLow) { Note = TEXT("DSN dishes track too slowly for low orbits and are booked for deep space."); return 30.f; }
			if (bDeep) { Note = TEXT("DSN time is shared by ~40 missions: you typically get ~8 h/day."); return 480.f; }
			Note = TEXT("Three DSN complexes keep you in view all day."); return 1440.f;
		}
		Note = TEXT("No ground network chosen: nobody is listening!");
		return 0.f;
	}

	double ReceiverDishM(FName G)
	{
		if (G == "gnd_dsn") return 70.0;
		if (G == "gnd_net") return 13.0;
		if (G == "gnd_own") return 10.0;
		if (G == "gnd_relay") return 5.0;
		return 2.0;
	}

	double RadDosePerYear(EISDest D)
	{
		switch (D)
		{
		case EISDest::LEO: return 0.5;
		case EISDest::SSO: return 1.5;
		case EISDest::MEO: return 30.0;
		case EISDest::GEO: return 5.0;
		case EISDest::Moon: return 2.0;
		case EISDest::Jupiter: return 1000.0;
		case EISDest::Interstellar: return 0.5;
		default: return 0.0;
		}
	}

	FISLine& AddLine(FISEval& E, const FString& L, const FString& V, const FString& Lim, EISStatus S, const FString& Why)
	{
		FISLine X; X.Label = L; X.Value = V; X.Limit = Lim; X.Status = S; X.Why = Why;
		return E.Lines.Add_GetRef(X);
	}

	FISObjective& AddObj(FISEval& E, const FString& T, bool bMet, const FString& Why, bool bBonus = false, FName Needs = NAME_None)
	{
		FISObjective O; O.Text = T; O.bMet = bMet; O.Why = Why; O.bBonus = bBonus; O.Needs = Needs;
		return E.Objectives.Add_GetRef(O);
	}

	double Hohmann1(double Mu, double R1, double R2)
	{
		// first burn of a Hohmann transfer from circular r1 toward r2 (km/s)
		const double V1 = FMath::Sqrt(Mu / R1);
		const double VT = FMath::Sqrt(Mu * 2.0 * R2 / (R1 * (R1 + R2)));
		return FMath::Abs(VT - V1);
	}
}

namespace ISSim
{
	int32 Difficulty = 1;
	FString DifficultyName(int32 D) { return D == 0 ? TEXT("Easy") : D == 2 ? TEXT("Hard") : TEXT("Normal"); }
	FISMission ApplyDifficulty(const FISMission& In)
	{
		FISMission M = In;
		if (M.Id == "sandbox") return M;
		if (Difficulty == 0) { M.BudgetM *= 1.5f; M.MaxPointingDeg *= 3.f; M.MinDataGbDay *= 0.5f; }
		if (Difficulty == 2) { M.BudgetM *= 0.8f; }
		return M;
	}

	FString Fmt(double V, int32 Dec)
	{
		FNumberFormattingOptions O; O.MinimumFractionalDigits = 0; O.MaximumFractionalDigits = Dec; O.UseGrouping = true;
		return FText::AsNumber(V, &O).ToString();
	}
	FString FmtMass(double Kg)
	{
		if (Kg < 1.0) return Fmt(Kg * 1000.0, 0) + TEXT(" g");
		if (Kg >= 10000.0) return Fmt(Kg / 1000.0, 1) + TEXT(" t");
		return Fmt(Kg, Kg < 10 ? 2 : 0) + TEXT(" kg");
	}
	FString FmtMoney(double M)
	{
		if (M < 0.001) return TEXT("$") + Fmt(M * 1e6, 0);
		if (M < 1.0) return TEXT("$") + Fmt(M * 1000.0, 1) + TEXT("k");
		if (M >= 1000.0) return TEXT("$") + Fmt(M / 1000.0, 2) + TEXT("B");
		return TEXT("$") + Fmt(M, 1) + TEXT("M");
	}
	FString FmtDays(double D)
	{
		if (D < 1.0 / 24.0) return Fmt(D * 1440.0, 0) + TEXT(" min");
		if (D < 2.0) return Fmt(D * 24.0, 1) + TEXT(" h");
		if (D < 400.0) return Fmt(D, 0) + TEXT(" days");
		return Fmt(D / 365.25, 1) + TEXT(" years");
	}

	double NaturalDecayYears(double Alt)
	{
		// Typical time for a ~100 kg/m^2 satellite to re-enter from a circular orbit (average solar activity)
		static const double A[] = { 200, 300, 400, 500, 550, 600, 700, 800, 900, 1000, 1200 };
		static const double Y[] = { 0.003, 0.1, 1.0, 3.5, 5.0, 15.0, 60.0, 150.0, 400.0, 1000.0, 5000.0 };
		if (Alt <= A[0]) return Y[0];
		for (int32 i = 1; i < 11; ++i)
			if (Alt <= A[i])
			{
				const double T = (Alt - A[i - 1]) / (A[i] - A[i - 1]);
				return FMath::Exp(FMath::Lerp(FMath::Loge(Y[i - 1]), FMath::Loge(Y[i]), T));
			}
		return 10000.0;
	}

	float DefaultAlt(EISDest D)
	{
		switch (D)
		{
		case EISDest::Backyard: return 1.f;
		case EISDest::LEO: return 550.f;
		case EISDest::SSO: return 705.f;
		case EISDest::MEO: return 20200.f;
		case EISDest::GEO: return 35786.f;
		case EISDest::Moon: return 50.f;
		case EISDest::Jupiter: return 4200.f;
		default: return 0.f;
		}
	}

	bool IsDeepSpace(EISDest D) { return D == EISDest::Moon || D == EISDest::Jupiter || D == EISDest::Interstellar; }
	double DepartDay(EISDest D) { return IsDeepSpace(D) ? 0.1 : 0.0; }   // ~1.5 parking orbits after launch, like Apollo's TLI
	double CruiseDays(EISDest D)
	{
		switch (D)
		{
		case EISDest::Moon: return 4.5;
		case EISDest::GEO: return 10.0;
		case EISDest::Jupiter:
		{
			// direct Hohmann transfer Earth -> Jupiter: half an ellipse touching both orbits (~2.7 years)
			const double MuSun = 1.32712440018e11, R1 = 149.598e6, R2 = 778.57e6, A = 0.5 * (R1 + R2);
			return DepartDay(D) + PI * FMath::Sqrt(A * A * A / MuSun) / 86400.0;
		}
		default: return 0.0;
		}
	}

	EISDest EffectiveDest(const FISDesign& D, const FISMission& M) { return M.Id == "sandbox" ? D.Dest : M.Dest; }
	float EffectiveAlt(const FISDesign& D, const FISMission& M) { return M.Id == "sandbox" ? DefaultAlt(D.Dest) : M.AltKm; }

	FISEval Evaluate(const FISDesign& D, const FISMission& MIn)
	{
		const FISMission M = ApplyDifficulty(MIn);
		FISEval E;
		const bool bSandbox = M.Id == "sandbox";
		E.Dest = EffectiveDest(D, M);
		E.AltKm = EffectiveAlt(D, M);
		E.LifeYears = M.LifeYears;
		if (bSandbox)
		{
			E.LifeYears = E.Dest == EISDest::Backyard ? 2.f / 8760.f : E.Dest == EISDest::Interstellar ? 40.f : E.Dest == EISDest::Jupiter ? 3.f : 5.f;
		}
		const float Life = E.LifeYears;
		const bool bBackyard = E.Dest == EISDest::Backyard;
		const bool bDeep = E.Dest == EISDest::Jupiter || E.Dest == EISDest::Interstellar;
		E.SunAU = SunDistanceAU(E.Dest);

		// ---------------------------------------------------------------- gather parts
		TArray<const FISPart*> P;
		for (const FName& Id : D.Parts) if (const FISPart* X = ISData::FindPart(Id)) P.Add(X);
		auto Single = [&](EISCat C) -> const FISPart* { for (const FISPart* X : P) if (X->Cat == C) return X; return nullptr; };
		const FISPart* Bus = Single(EISCat::Bus);
		const FISPart* Adcs = Single(EISCat::ADCS);
		const FISPart* Radio = Single(EISCat::Comms);
		const FISPart* Cpu = Single(EISCat::Computer);
		const FISPart* Ground = Single(EISCat::Ground);
		const FISVehicle* LV = ISData::FindVehicle(D.Vehicle);
		const bool bRedund = D.HasTag(ISTag::Redundancy);

		const EISCat Needed[] = { EISCat::Bus, EISCat::Power, EISCat::Battery, EISCat::ADCS, EISCat::Comms, EISCat::Propulsion, EISCat::Computer, EISCat::Thermal, EISCat::Ground };
		for (EISCat C : Needed)
			if (!Single(C)) E.Blockers.Add(FString::Printf(TEXT("Choose a part for: %s"), *ISData::CatName(C)));

		// ---------------------------------------------------------------- mass
		float SolarBol = 0, Rtg = 0, Load = 0, Bat = 0, Prop = 0, Isp = 0, Sail = 0, Chute = 0, Gen = 0, SatCost = 0;
		TArray<const FISPart*> ByMass = P;
		for (const FISPart* X : P)
		{
			if (X->Cat == EISCat::Ground) continue;
			E.DryKg += X->MassKg; SolarBol += X->SolarW; Rtg += X->RtgW; Load += X->PowerW; Bat += X->BatteryWh;
			Prop += X->PropKg; if (X->PropKg > 0) Isp = X->IspS; Gen += X->DataGbDay; SatCost += X->CostM;
			if (X->Tag == ISTag::SolarSail) Sail += X->SailM2;
			if (X->Tag == ISTag::Parachute) Chute += X->SailM2;
		}
		if (bRedund) { E.DryKg *= 1.10f; SatCost *= 1.15f; }
		E.PropKg = Prop; E.TotalKg = E.DryKg + Prop; E.BusMaxKg = Bus ? Bus->MaxTotalKg : 0.f;
		ByMass.Sort([](const FISPart& A, const FISPart& B) { return A.MassKg > B.MassKg; });
		FString Heavy;
		for (int32 i = 0; i < FMath::Min(3, ByMass.Num()); ++i)
			if (ByMass[i]->MassKg > 0) Heavy += FString::Printf(TEXT("%s%s (%s)"), Heavy.IsEmpty() ? TEXT("") : TEXT(", "), *ByMass[i]->Name, *FmtMass(ByMass[i]->MassKg));
		{
			const bool bOver = Bus && E.TotalKg > E.BusMaxKg;
			const bool bLimit = M.MaxMassKg > 0 && E.TotalKg > M.MaxMassKg;
			AddLine(E, TEXT("Mass"), FmtMass(E.TotalKg), Bus ? FmtMass(M.MaxMassKg > 0 ? FMath::Min(M.MaxMassKg, E.BusMaxKg) : E.BusMaxKg) : TEXT("-"),
				(bOver || bLimit) ? EISStatus::Fail : EISStatus::Ok,
				FString::Printf(TEXT("Total mass = dry %s + propellant %s = %s%s.\nThe structure (bus) can carry up to %s.%s\nHeaviest parts: %s.\nMass drives launch cost: every kg to orbit costs thousands of dollars."),
					*FmtMass(E.DryKg), *FmtMass(Prop), *FmtMass(E.TotalKg), bRedund ? TEXT(" (redundant electronics add 10%)") : TEXT(""),
					*FmtMass(E.BusMaxKg), M.MaxMassKg > 0 ? *FString::Printf(TEXT(" Competition rule: max %s."), *FmtMass(M.MaxMassKg)) : TEXT(""), *Heavy));
			if (bOver) E.Blockers.Add(TEXT("Too heavy for the chosen structure: pick a bigger bus or lighter parts."));
			if (bLimit) E.Blockers.Add(FString::Printf(TEXT("Over the %s mass limit."), *FmtMass(M.MaxMassKg)));
		}

		// ---------------------------------------------------------------- orbit
		const FBody B = BodyFor(E.Dest);
		double EclFrac = 0.0;
		if (B.bValid)
		{
			const double A = B.R + E.AltKm;
			E.PeriodMin = 2.0 * PI * FMath::Sqrt(A * A * A / B.Mu) / 60.0;
			EclFrac = FMath::Asin(B.R / A) / PI;
			E.EclipseMin = E.PeriodMin * EclFrac;
			AddLine(E, TEXT("Orbit period"), FmtDays(E.PeriodMin / 1440.0), TEXT(""), EISStatus::Info,
				FString::Printf(TEXT("Kepler's 3rd law: T = 2 pi sqrt(a^3 / mu)\na = radius %s km + altitude %s km = %s km, mu = %s km^3/s^2\nT = %s minutes.\nWorst-case shadow (eclipse) each orbit: asin(R/a)/pi = %s%% of the orbit = %s minutes."),
					*Fmt(B.R, 0), *Fmt(E.AltKm, 0), *Fmt(A, 0), *Fmt(B.Mu, 0), *Fmt(E.PeriodMin, 1), *Fmt(EclFrac * 100.0, 1), *Fmt(E.EclipseMin, 1)));
		}
		else if (E.Dest == EISDest::Jupiter)
		{
			E.PeriodMin = 53.0 * 1440.0;
			AddLine(E, TEXT("Orbit period"), TEXT("53 days"), TEXT(""), EISStatus::Info,
				TEXT("A long, polar, elliptical orbit (like Juno's) dives close to Jupiter for a few hours, then swings far out of the worst radiation. It also avoids Jupiter's shadow, so solar panels always see the Sun."));
		}

		// ---------------------------------------------------------------- power
		const double AU2 = E.SunAU * E.SunAU;
		E.SolarEolW = bBackyard ? SolarBol : SolarBol * FMath::Pow(0.975, Life) / AU2;
		E.RtgEolW = Rtg * FMath::Pow(0.984, Life);
		E.AvgGenW = E.SolarEolW * (1.0 - EclFrac) + E.RtgEolW;
		E.LoadW = Load; E.BatteryWh = Bat;
		const float NeedW = SolarBol > 0 ? Load / 0.85f : Load;
		if (!bBackyard)
		{
			const bool bOk = E.AvgGenW >= NeedW;
			AddLine(E, TEXT("Power (end of life)"), Fmt(E.AvgGenW, 0) + TEXT(" W"), Fmt(NeedW, 0) + TEXT(" W"), bOk ? EISStatus::Ok : EISStatus::Fail,
				FString::Printf(TEXT("Average power = solar x sunlit fraction + RTG\nSolar: %s W x 0.975^%s years (radiation damage) / %s AU^2 (inverse-square law) = %s W\nSunlit fraction: %s%%\nRTG: %s W x 0.984^%s years (Pu-238 decay) = %s W\nAverage = %s W. Needed: load %s W / 0.85 battery-charging losses = %s W.%s"),
					*Fmt(SolarBol, 0), *Fmt(Life, 1), *Fmt(E.SunAU, 1), *Fmt(E.SolarEolW, 0), *Fmt((1.0 - EclFrac) * 100.0, 0),
					*Fmt(Rtg, 0), *Fmt(Life, 1), *Fmt(E.RtgEolW, 0), *Fmt(E.AvgGenW, 0), *Fmt(Load, 0), *Fmt(NeedW, 0),
					bOk ? TEXT("") : (E.SunAU > 2 ? TEXT("\nFIX: far from the Sun, panels give little. Use much bigger arrays or an RTG.") : TEXT("\nFIX: bigger solar array, or remove power-hungry parts."))));
			// battery
			const double DoD = (E.Dest == EISDest::LEO || E.Dest == EISDest::SSO) ? 0.3 : 0.6;
			E.BatteryNeedWh = FMath::Max(0.0, (double)Load - (double)E.RtgEolW) * (E.EclipseMin / 60.0) / DoD;
			if (E.EclipseMin > 0)
			{
				const bool bOkB = Bat >= E.BatteryNeedWh;
				AddLine(E, TEXT("Battery (eclipse)"), Fmt(Bat, 0) + TEXT(" Wh"), Fmt(E.BatteryNeedWh, 0) + TEXT(" Wh"), bOkB ? EISStatus::Ok : EISStatus::Fail,
					FString::Printf(TEXT("Battery needed = load x eclipse time / depth of discharge\n= %s W x %s h / %s = %s Wh\nOnly %s%% of the battery may be used per orbit so it survives %s charge cycles (one per orbit!).%s"),
						*Fmt(FMath::Max(0.0, (double)Load - E.RtgEolW), 0), *Fmt(E.EclipseMin / 60.0, 2), *Fmt(DoD, 1), *Fmt(E.BatteryNeedWh, 0), *Fmt(DoD * 100, 0),
						*Fmt(Life * 365.25 * 1440.0 / FMath::Max(1.0, E.PeriodMin), 0), bOkB ? TEXT("") : TEXT("\nFIX: choose a bigger battery.")));
			}
		}
		else
		{
			E.FlightAltKm = (LV && LV->Id == "lv_balloon") ? 30.f : 1.f;
			const double Cd = 0.8, Rho = 1.2;
			const double Mass = FMath::Max(0.01f, E.TotalKg);
			E.DescentMs = Chute > 0 ? FMath::Sqrt(2.0 * Mass * ISData::G0 / (Rho * Cd * Chute)) : 50.f;
			const double FlightH = E.FlightAltKm > 5 ? 2.5 : (E.FlightAltKm * 1000.0 / E.DescentMs) / 3600.0;
			const double NeedH = FlightH + 2.0;
			E.RuntimeH = Bat <= 0 ? 0.0 : (Load > 0 ? Bat * 0.9 / Load : 999.0);   // no battery = no runtime
			AddLine(E, TEXT("Battery runtime"), Fmt(E.RuntimeH, 1) + TEXT(" h"), Fmt(NeedH, 1) + TEXT(" h"), E.RuntimeH >= NeedH ? EISStatus::Ok : EISStatus::Fail,
				FString::Printf(TEXT("Runtime = battery energy x 90%% usable / load\n= %s Wh x 0.9 / %s W = %s hours.\nNeeded: flight (%s h) + 2 h on the ground so you can find it = %s h.\nEnergy (Wh) = Power (W) x Time (h)."),
					*Fmt(Bat, 1), *Fmt(Load, 2), *Fmt(E.RuntimeH, 1), *Fmt(FlightH, 2), *Fmt(NeedH, 1)));
			AddLine(E, TEXT("Descent speed"), Fmt(E.DescentMs, 1) + TEXT(" m/s"), TEXT("10 m/s"), E.DescentMs <= 10.0 ? EISStatus::Ok : EISStatus::Fail,
				Chute > 0 ? FString::Printf(TEXT("Terminal velocity v = sqrt(2 m g / (rho Cd A))\n= sqrt(2 x %s kg x 9.81 / (1.2 kg/m^3 x 0.8 x %s m^2)) = %s m/s.\nMore area or less mass = slower landing. Speed scales with 1/sqrt(area)."),
					*Fmt(Mass, 3), *Fmt(Chute, 2), *Fmt(E.DescentMs, 1))
				: FString(TEXT("No parachute: your CanSat falls at ~50 m/s and breaks. Add one from Extras.")));
		}

		// ---------------------------------------------------------------- pointing
		E.PointingDeg = Adcs ? Adcs->PointingDeg : 0.f;
		const bool bMagnetic = Adcs && (Adcs->Id == "adcs_mag" || Adcs->Id == "adcs_mtq");
		if (bMagnetic && (E.Dest == EISDest::Moon || bDeep)) E.PointingDeg = 0.f;
		if (M.MaxPointingDeg > 0)
		{
			const bool bOk = E.PointingDeg > 0 && E.PointingDeg <= M.MaxPointingDeg;
			AddLine(E, TEXT("Pointing accuracy"), E.PointingDeg > 0 ? Fmt(E.PointingDeg, 2) + TEXT(" deg") : TEXT("none"), Fmt(M.MaxPointingDeg, 2) + TEXT(" deg"),
				bOk ? EISStatus::Ok : EISStatus::Fail,
				FString::Printf(TEXT("Your instruments and antennas must point within %s deg.\nYour attitude control gives %s.%s\nAt %s km altitude an error of 1 deg moves the view by %s km on the ground!"),
					*Fmt(M.MaxPointingDeg, 2), E.PointingDeg > 0 ? *(Fmt(E.PointingDeg, 2) + TEXT(" deg")) : TEXT("no control (tumbling)"),
					(bMagnetic && E.PointingDeg == 0) ? TEXT("\nMagnetic control needs a planetary magnetic field: it does not work out here.") : TEXT(""),
					*Fmt(E.AltKm, 0), *Fmt(E.AltKm * FMath::Tan(FMath::DegreesToRadians(1.0)), 1)));
		}

		// ---------------------------------------------------------------- resolution
		if (M.MaxResolutionM > 0)
		{
			float Best = 0;
			FString BestName;
			for (const FISPart* X : P)
				if (X->ResolutionM > 0 && X->Tag == ISTag::Imager)
				{
					const float R = X->ResolutionM * (E.AltKm / X->RefAltKm);
					if (Best == 0 || R < Best) { Best = R; BestName = X->Name; }
				}
			E.ResolutionM = Best;
			const bool bOk = Best > 0 && Best <= M.MaxResolutionM;
			AddLine(E, TEXT("Image resolution"), Best > 0 ? Fmt(Best, 1) + TEXT(" m") : TEXT("no imager"), Fmt(M.MaxResolutionM, 1) + TEXT(" m"), bOk ? EISStatus::Ok : EISStatus::Fail,
				Best > 0 ? FString::Printf(TEXT("Resolution scales with distance: %s gives its rated pixel size at its design altitude.\nAt %s km: %s m per pixel.\nTo see smaller things: fly lower or use a bigger telescope."), *BestName, *Fmt(E.AltKm, 0), *Fmt(Best, 2))
				: FString(TEXT("This mission needs an imager. Add one from Payload.")));
		}

		// ---------------------------------------------------------------- comms / data
		{
			const double Dist = LinkDistanceKm(E.Dest, bBackyard ? E.FlightAltKm : E.AltKm);
			FString CNote;
			E.ContactMin = Ground ? ContactMinutes(Ground->Id, E.Dest, CNote) : 0.f;
			double RateMbps = 0;
			FString LinkWhy;
			if (Radio)
			{
				E.RadioRangeKm = Radio->RadioRangeKm;
				if (Radio->DishM > 0 && (bDeep || E.Dest == EISDest::Moon))
				{
					const double Dau = Dist / 1.496e8;
					const double Dr = Ground ? ReceiverDishM(Ground->Id) : 2.0;
					const double Kbps = 115.2 * FMath::Square(5.2 / Dau) * (Radio->TxW / 23.0) * FMath::Square(Radio->DishM / 3.7) * FMath::Square(Dr / 70.0);
					RateMbps = FMath::Min(Kbps / 1000.0, (double)Radio->RadioMbps);
					LinkWhy = FString::Printf(TEXT("Link budget (Friis): rate ~ P_tx x D_sat^2 x D_ground^2 / distance^2\nScaled from Voyager (115 kbps from 5.2 AU with 23 W, 3.7 m dish -> 70 m DSN):\n= 115 kbps x (5.2/%s AU)^2 x (%s W/23) x (%s m/3.7)^2 x (%s m/70)^2 = %s kbps"),
						*Fmt(Dau, 3), *Fmt(Radio->TxW, 0), *Fmt(Radio->DishM, 1), *Fmt(Dr, 0), *Fmt(Kbps, 3));
				}
				else if (Radio->RadioRangeKm >= Dist)
				{
					RateMbps = Radio->RadioMbps;
					LinkWhy = FString::Printf(TEXT("%s: %s Mbps, works out to ~%s km."), *Radio->Name, *Fmt(RateMbps, 3), *Fmt(Radio->RadioRangeKm, 0));
				}
				else
				{
					LinkWhy = FString::Printf(TEXT("%s only reaches ~%s km, but you are ~%s km away. Signal strength falls with distance^2."), *Radio->Name, *Fmt(Radio->RadioRangeKm, 0), *Fmt(Dist, 0));
				}
			}
			E.LinkKbps = RateMbps * 1000.0;
			E.DataGenGb = Gen;
			const double Capacity = RateMbps * E.ContactMin * 60.0 / 1000.0; // Gb/day
			E.DataDownGb = FMath::Min((double)Gen, Capacity);
			if (bBackyard)
			{
				const bool bOk = Radio && Radio->RadioRangeKm >= Dist;
				AddLine(E, TEXT("Radio range"), Radio ? Fmt(Radio->RadioRangeKm, 0) + TEXT(" km") : TEXT("none"), Fmt(Dist, 0) + TEXT(" km"), bOk ? EISStatus::Ok : EISStatus::Fail,
					LinkWhy + FString::Printf(TEXT("\nYour CanSat flies to %s km, so you need ~%s km of range."), *Fmt(E.FlightAltKm, 0), *Fmt(Dist, 0)));
			}
			else
			{
				const bool bNeed = M.MinDataGbDay > 0;
				const bool bOk = !bNeed || E.DataDownGb >= M.MinDataGbDay;
				AddLine(E, TEXT("Data downlinked / day"), Fmt(E.DataDownGb, 3) + TEXT(" Gb"), bNeed ? Fmt(M.MinDataGbDay, 3) + TEXT(" Gb") : TEXT(""),
					bNeed ? (bOk ? EISStatus::Ok : EISStatus::Fail) : (Capacity > 0 ? EISStatus::Info : EISStatus::Warn),
					FString::Printf(TEXT("%s\nGround: %s (%s min/day of contact). %s\nCapacity = rate x contact time = %s Mbps x %s min x 60 s = %s Gb/day.\nYour payloads produce %s Gb/day. Downlinked = the smaller of the two = %s Gb/day.%s"),
						*LinkWhy, Ground ? *Ground->Name : TEXT("none"), *Fmt(E.ContactMin, 0), *CNote, *Fmt(RateMbps, 3), *Fmt(E.ContactMin, 0), *Fmt(Capacity, 3),
						*Fmt(Gen, 3), *Fmt(E.DataDownGb, 3),
						(bNeed && !bOk) ? (Gen < M.MinDataGbDay ? TEXT("\nFIX: your payloads don't produce enough data.") : TEXT("\nFIX: faster radio, more ground stations (rent a network!), or a relay.")) : TEXT("")));
			}
		}

		// ---------------------------------------------------------------- delta-v
		double DvCore = 0, DvPerYear = 0, DvDeorbit = 0;
		FString DvWhy;
		const double NatLife = (E.Dest == EISDest::LEO || E.Dest == EISDest::SSO) ? NaturalDecayYears(E.AltKm) : 1e9;
		switch (E.Dest)
		{
		case EISDest::LEO: case EISDest::SSO:
			DvPerYear = NatLife < Life ? 25.0 : 2.0;
			DvDeorbit = Hohmann1(ISData::MuEarth, ISData::REarth + E.AltKm, ISData::REarth + 50.0) * 1000.0;
			DvWhy = FString::Printf(TEXT("Drag make-up / debris avoidance: %s m/s per year. Deorbit burn (lower perigee to 50 km): %s m/s."), *Fmt(DvPerYear, 0), *Fmt(DvDeorbit, 0));
			break;
		case EISDest::MEO: DvPerYear = 2.0; DvWhy = TEXT("Small station-keeping burns: ~2 m/s per year."); break;
		case EISDest::GEO: DvCore = 1500.0; DvPerYear = 50.0; DvDeorbit = 11.0; DvWhy = TEXT("The rocket drops you in a transfer orbit (GTO). Circularising at GEO: ~1,500 m/s. Station-keeping: ~50 m/s per year. Graveyard orbit at end of life: ~11 m/s."); break;
		case EISDest::Moon: DvCore = 900.0; DvPerYear = 100.0; DvWhy = TEXT("Lunar orbit insertion: ~900 m/s. Low lunar orbits are unstable (lumpy gravity 'mascons'): ~100 m/s per year to stay up."); break;
		case EISDest::Jupiter: DvCore = 650.0; DvPerYear = 30.0; DvWhy = TEXT("Jupiter orbit insertion: ~650 m/s (Juno: 542 m/s) + trajectory corrections ~30 m/s per year."); break;
		case EISDest::Interstellar: DvCore = 50.0; DvWhy = TEXT("Only small trajectory-correction burns: ~50 m/s."); break;
		default: break;
		}
		E.DvNeed = DvCore + DvPerYear * Life + DvDeorbit;
		if (Prop > 0 && Isp > 0 && E.TotalKg > Prop)
			E.DvAvail = Isp * ISData::G0 * FMath::Loge(E.TotalKg / (E.TotalKg - Prop));
		if (!bBackyard)
		{
			const bool bCoreOk = E.DvAvail >= DvCore;
			const bool bAllOk = E.DvAvail >= E.DvNeed;
			AddLine(E, TEXT("Delta-v"), Fmt(E.DvAvail, 0) + TEXT(" m/s"), Fmt(E.DvNeed, 0) + TEXT(" m/s"), !bCoreOk ? EISStatus::Fail : bAllOk ? EISStatus::Ok : EISStatus::Warn,
				FString::Printf(TEXT("Tsiolkovsky rocket equation: dv = Isp x g0 x ln(m_full / m_empty)\n= %s s x 9.81 x ln(%s / %s) = %s m/s\nNeeded: %s\nTotal needed for %s years: %s m/s.%s"),
					*Fmt(Isp, 0), *FmtMass(E.TotalKg), *FmtMass(E.TotalKg - Prop), *Fmt(E.DvAvail, 0), *DvWhy, *Fmt(Life, 1), *Fmt(E.DvNeed, 0),
					!bCoreOk ? TEXT("\nFIX: you cannot even reach your orbit. Add propellant or a more efficient engine (higher Isp).") : (!bAllOk ? TEXT("\nWarning: you will run out of fuel before end of mission.") : TEXT(""))));
		}

		// ---------------------------------------------------------------- debris rule
		bool bDebrisOk = true;
		if (E.Dest == EISDest::LEO || E.Dest == EISDest::SSO)
		{
			const bool bNatural = NatLife <= Life + 5.0;
			const bool bSail = D.HasTag(ISTag::DragSail) && E.AltKm <= 900;
			const bool bProp = E.DvAvail >= E.DvNeed;
			bDebrisOk = bNatural || bSail || bProp;
			const bool bRule = M.Id == "m4";
			AddLine(E, TEXT("Space-debris rule"), bDebrisOk ? TEXT("compliant") : TEXT("violates"), TEXT("deorbit <= 5 yr after mission"),
				bDebrisOk ? EISStatus::Ok : (bRule ? EISStatus::Fail : EISStatus::Warn),
				FString::Printf(TEXT("Modern rules (e.g. FCC 2024) require removing a LEO satellite within 5 years after its mission.\nWithout help, a dead satellite at %s km stays up ~%s years (air drag slowly pulls it down).\nOptions: fly low (natural decay), a drag sail (below ~900 km), or keep %s m/s of fuel for a deorbit burn.%s"),
					*Fmt(E.AltKm, 0), *Fmt(NatLife, NatLife < 10 ? 1 : 0), *Fmt(DvDeorbit, 0),
					(!bRule && !bDebrisOk) ? TEXT("\n(Landsat 1 flew in 1972, before such rules: it is still in orbit today as debris!)") : TEXT("")));
		}

		// ---------------------------------------------------------------- radiation & thermal
		{
			const bool bVault = D.HasTag(ISTag::Vault);
			const double DoseYr = RadDosePerYear(E.Dest) / (bVault ? 50.0 : 1.0);
			E.RadDose = DoseYr * Life; E.RadTol = Cpu ? Cpu->RadTolKrad : 0.f;
			if (!bBackyard)
			{
				const bool bOk = E.RadTol >= E.RadDose;
				AddLine(E, TEXT("Radiation dose"), Fmt(E.RadDose, 1) + TEXT(" krad"), Fmt(E.RadTol, 0) + TEXT(" krad"), bOk ? EISStatus::Ok : EISStatus::Fail,
					FString::Printf(TEXT("Dose = %s krad/year%s x %s years = %s krad.\nYour computer tolerates %s krad.\nThe Van Allen belts make MEO harsh; Jupiter's belts are the worst in the solar system.%s"),
						*Fmt(RadDosePerYear(E.Dest), 1), bVault ? TEXT(" / 50 (vault)") : TEXT(""), *Fmt(Life, 1), *Fmt(E.RadDose, 1), *Fmt(E.RadTol, 0),
						bOk ? TEXT("") : TEXT("\nFIX: radiation-hardened computer and/or a radiation vault.")));
			}
			if (D.GetSingle(EISCat::Thermal) == FName("thm_none") && E.Dest != EISDest::Backyard)
				AddLine(E, TEXT("Thermal"), TEXT("none"), TEXT("passive or better"), EISStatus::Fail,
					TEXT("In vacuum there is no air to carry heat away: the sunlit side bakes above +100 C while the shadow side drops below -100 C.\nFIX: pick at least passive thermal control (paint + insulation)."));
			if (E.SunAU > 2.0)
			{
				const bool bWarm = D.HasTag(ISTag::Heaters) || Rtg > 0;
				AddLine(E, TEXT("Thermal"), bWarm ? TEXT("heated") : TEXT("freezing"), TEXT("> -20 C"), bWarm ? EISStatus::Ok : EISStatus::Fail,
					FString::Printf(TEXT("At %s AU sunlight is %s times weaker than at Earth. Without heaters (or the waste heat of an RTG), electronics and fuel lines freeze."),
						*Fmt(E.SunAU, 1), *Fmt(AU2, 0)));
			}
		}

		// ---------------------------------------------------------------- lifetime
		{
			double LifeEst = 99.0;
			FString Lim = TEXT("design");
			auto Limit = [&](double Y, const TCHAR* Why) { if (Y < LifeEst) { LifeEst = FMath::Max(0.0, Y); Lim = Why; } };
			if (!bBackyard)
			{
				// power
				for (double Y = 0; Y <= 60; Y += 0.1)
				{
					const double S = SolarBol * FMath::Pow(0.975, Y) / AU2;
					const double G = S * (1.0 - EclFrac) + Rtg * FMath::Pow(0.984, Y);
					if (G < NeedW) { Limit(Y, TEXT("power")); break; }
				}
				// radiation
				const double DoseYr = RadDosePerYear(E.Dest) / (D.HasTag(ISTag::Vault) ? 50.0 : 1.0);
				if (DoseYr > 0) Limit(E.RadTol / DoseYr, TEXT("radiation"));
				// propellant
				if (DvPerYear > 0 && E.DvAvail >= DvCore)
				{
					const double Spare = E.DvAvail - DvCore - DvDeorbit;
					if (E.Dest == EISDest::LEO || E.Dest == EISDest::SSO)
					{
						if (NatLife < Life) Limit(Spare > 0 ? NatLife + Spare / DvPerYear : NatLife, TEXT("orbit decay"));
					}
					else Limit(FMath::Max(0.0, Spare) / DvPerYear, TEXT("fuel"));
				}
				if (E.DvAvail < DvCore) Limit(0, TEXT("never reached orbit"));
				if (E.SunAU > 2.0 && !(D.HasTag(ISTag::Heaters) || Rtg > 0)) Limit(0.05, TEXT("freezing"));
			}
			E.LifeEst = (float)LifeEst; E.LifeLimiter = Lim;
			if (!bBackyard)
			{
				const bool bOk = LifeEst >= Life;
				AddLine(E, TEXT("Expected lifetime"), LifeEst >= 99 ? TEXT("99+ yr") : Fmt(LifeEst, 1) + TEXT(" yr"), Fmt(Life, 1) + TEXT(" yr"), bOk ? EISStatus::Ok : EISStatus::Fail,
					FString::Printf(TEXT("Lifetime is limited by whichever runs out first: power, fuel, radiation tolerance, or orbit decay.\nYour limiting factor: %s.\nRequired: %s years."), *Lim, *Fmt(Life, 1)));
			}
		}

		// ---------------------------------------------------------------- reliability
		{
			double R = 1.0;
			const double Years = FMath::Max((double)Life, 0.01);
			for (const FISPart* X : P)
			{
				if (X->Cat == EISCat::Ground) continue;
				double Rel = X->RelPerYear;
				if (bRedund && X->Cat != EISCat::Bus && X->Cat != EISCat::Payload) Rel = 1.0 - FMath::Square(1.0 - Rel);
				R *= FMath::Pow(Rel, Years);
			}
			E.RelSat = R; E.RelLaunch = LV ? LV->Reliability : 0.f;
			AddLine(E, TEXT("Chance all parts survive"), Fmt(R * 100.0, 0) + TEXT("%"), TEXT(""), R > 0.7 ? EISStatus::Ok : R > 0.4 ? EISStatus::Warn : EISStatus::Fail,
				FString::Printf(TEXT("Reliability R(t) = product of every part's yearly survival ^ years.\nOver %s years: %s%%.%s\nLaunch success: %s%%.\nNot every failure ends a mission: you will be able to react to anomalies during operations."),
					*Fmt(Years, 1), *Fmt(R * 100.0, 1), bRedund ? TEXT(" Redundancy: each backed-up part fails only if BOTH copies fail: 1 - (1-R)^2.") : TEXT(" TIP: redundant electronics raise this a lot."),
					*Fmt(E.RelLaunch * 100.0, 0)));
		}

		// ---------------------------------------------------------------- launch
		{
			float Cap = 0;
			if (LV)
			{
				Cap = LV->Capacity[(int32)E.Dest];
				if (E.Dest == EISDest::Interstellar && Sail > 0) Cap = LV->Capacity[(int32)EISDest::Moon];
			}
			E.VehicleCap = Cap;
			const int32 Fleet = FMath::Max(1, M.Id == "m4" ? D.Fleet : 1);
			if (LV && Cap > 0 && E.TotalKg > 0)
			{
				const int32 PerLaunch = FMath::Max(0, FMath::FloorToInt(Cap / E.TotalKg));
				E.Launches = PerLaunch > 0 ? FMath::DivideAndRoundUp(Fleet, PerLaunch) : 0;
				E.LaunchCostM = LV->CostPerKgM > 0 ? FMath::Max(LV->CostM, LV->CostPerKgM * E.TotalKg) * Fleet : LV->CostM * E.Launches;
			}
			const bool bFits = LV && Cap > 0 && E.TotalKg <= Cap;
			if (!LV) E.Blockers.Add(TEXT("Choose a launch vehicle."));
			else if (!bFits) E.Blockers.Add(Cap <= 0 ? FString::Printf(TEXT("%s cannot reach %s."), *LV->Name, *ISData::DestName(E.Dest))
				: FString::Printf(TEXT("%s can carry only %s to %s."), *LV->Name, *FmtMass(Cap), *ISData::DestName(E.Dest)));
			if (LV)
				AddLine(E, TEXT("Launch"), bFits ? FString::Printf(TEXT("%d launch%s"), E.Launches, E.Launches == 1 ? TEXT("") : TEXT("es")) : TEXT("does not fit"), FmtMass(Cap),
					bFits ? EISStatus::Ok : EISStatus::Fail,
					FString::Printf(TEXT("%s from %s can lift %s to %s.\nYour satellite: %s%s.\n%s"),
						*LV->Name, *LV->Site, *FmtMass(Cap), *ISData::DestName(E.Dest), *FmtMass(E.TotalKg),
						Fleet > 1 ? *FString::Printf(TEXT(" x %d satellites = %d launches"), Fleet, E.Launches) : TEXT(""), *LV->Edu));

			// ------------------------------------------------------------ cost
			E.SatCostM = SatCost * 1.25f;
			E.FleetCostM = E.SatCostM * FMath::Pow((float)Fleet, 0.848f);
			E.GroundCostM = Ground ? Ground->CostM : 0.f;
			E.TotalCostM = E.FleetCostM + E.LaunchCostM + E.GroundCostM;
			const bool bOk = E.TotalCostM <= M.BudgetM;
			AddLine(E, TEXT("Cost"), FmtMoney(E.TotalCostM), FmtMoney(M.BudgetM), bOk ? EISStatus::Ok : EISStatus::Fail,
				FString::Printf(TEXT("Parts %s + assembly & testing 25%% = %s per satellite%s\nLaunch: %s\nGround network: %s\nTotal: %s of %s budget.%s"),
					*FmtMoney(SatCost), *FmtMoney(E.SatCostM),
					Fleet > 1 ? *FString::Printf(TEXT("\nFleet of %d with a 90%% learning curve (each doubling of production cuts unit cost 10%%): %s"), Fleet, *FmtMoney(E.FleetCostM)) : TEXT(""),
					*FmtMoney(E.LaunchCostM), *FmtMoney(E.GroundCostM), *FmtMoney(E.TotalCostM), *FmtMoney(M.BudgetM),
					bOk ? TEXT("") : TEXT("\nFIX: cheaper parts, a cheaper rocket (rideshare?), or rent instead of build.")));
			if (!bOk) E.Blockers.Add(TEXT("Over budget: the client will not pay for this design."));
		}

		// ---------------------------------------------------------------- interstellar speed
		if (E.Dest == EISDest::Interstellar)
		{
			double V = 0;
			FString How;
			if (Sail > 0 && E.TotalKg > 0)
			{
				const double Ac = 9.08e-6 * Sail / E.TotalKg; // m/s^2 at 1 AU, perfect reflection
				const double Beta = Ac / 5.93e-3;
				const double Vs = Beta > 0.5 ? 29.8 * FMath::Sqrt(2.0 * Beta - 1.0) : 0.0;
				How = FString::Printf(TEXT("Sail: acceleration = 9.08 uN/m^2 x %s m^2 / %s = %s mm/s^2.\nLightness number beta = sail push / Sun's gravity (5.93 mm/s^2) = %s.\nEscape speed = 29.8 km/s x sqrt(2 beta - 1) = %s km/s%s"),
					*Fmt(Sail, 0), *FmtMass(E.TotalKg), *Fmt(Ac * 1000.0, 3), *Fmt(Beta, 3), *Fmt(Vs, 1), Beta <= 0.5 ? TEXT(" (beta must exceed 0.5 to escape!)") : TEXT(""));
				V = Vs;
			}
			if (LV && LV->Capacity[(int32)EISDest::Interstellar] >= E.TotalKg)
			{
				if (V < 17.0) { V = 17.0; How += TEXT("\nRocket + Jupiter/Saturn gravity assists (like Voyager 1): ~17 km/s."); }
			}
			E.EscapeKms = V;
			E.YearsTo120AU = V > 0 ? (120.0 * 1.496e8) / V / 3.156e7 : 999.0;
			const bool bOk = E.YearsTo120AU <= Life;
			AddLine(E, TEXT("Speed leaving the Sun"), Fmt(V, 1) + TEXT(" km/s"), TEXT("14.2 km/s"), bOk ? EISStatus::Ok : EISStatus::Fail,
				FString::Printf(TEXT("To reach 120 AU (18 billion km) in %s years you need ~14.2 km/s.\n%s\nTime to the heliopause: %s years."),
					*Fmt(Life, 0), How.IsEmpty() ? TEXT("No way to escape: a launcher with 'Escape' capacity or a solar sail is needed.") : *How, *Fmt(FMath::Min(999.0, (double)E.YearsTo120AU), 1)));
		}

		// ================================================================ objectives
		if (!bSandbox)
		{
			const bool bReach = E.Blockers.Num() == 0 && E.DvAvail >= DvCore;
			AddObj(E, FString::Printf(TEXT("Reach %s"), *ISData::DestName(E.Dest)), bReach,
				bReach ? TEXT("Launch and propulsion get you there.") : TEXT("Launch capacity, propulsion or a blocking problem prevents reaching the destination."));
			for (FName T : M.Required)
			{
				const bool bHas = D.HasTag(T);
				AddObj(E, TEXT("Carry ") + TagLabel(T), bHas, bHas ? TEXT("Installed.") : TEXT("Required by the client: add it."), false, T);
			}
			if (M.MaxResolutionM > 0) AddObj(E, FString::Printf(TEXT("Image resolution %s m or better"), *Fmt(M.MaxResolutionM, 1)), E.ResolutionM > 0 && E.ResolutionM <= M.MaxResolutionM, TEXT("See 'Image resolution' budget."), false, ISTag::Imager);
			if (M.MaxPointingDeg > 0) AddObj(E, FString::Printf(TEXT("Point within %s deg"), *Fmt(M.MaxPointingDeg, 2)), E.PointingDeg > 0 && E.PointingDeg <= M.MaxPointingDeg, TEXT("See 'Pointing accuracy' budget."), false, FName(ADCS_TAG));
			if (M.MinDataGbDay > 0)
			{
				const FName DataTag = M.Required.Num() ? M.Required[0] : NAME_None;
				AddObj(E, FString::Printf(TEXT("Downlink %s Gb per day"), *Fmt(M.MinDataGbDay, 3)), E.DataDownGb >= M.MinDataGbDay, TEXT("See 'Data downlinked' budget."), false, DataTag);
			}
			if (bBackyard)
			{
				AddObj(E, TEXT("Land under 10 m/s"), E.DescentMs <= 10.0, TEXT("See 'Descent speed'."));
				AddObj(E, TEXT("Radio contact the whole flight"), E.RadioRangeKm >= LinkDistanceKm(E.Dest, E.FlightAltKm), TEXT("See 'Radio range'."));
				AddObj(E, TEXT("Keep transmitting 2 h after landing"), E.RuntimeH >= ((E.FlightAltKm > 5 ? 2.5 : (E.FlightAltKm * 1000.0 / FMath::Max(1.f, E.DescentMs)) / 3600.0) + 2.0), TEXT("See 'Battery runtime'."));
			}
			else
			{
				AddObj(E, FString::Printf(TEXT("Operate for %s years"), *Fmt(Life, 1)), E.LifeEst >= Life, FString::Printf(TEXT("Expected lifetime %s years (limited by %s)."), *Fmt(E.LifeEst, 1), *E.LifeLimiter), false, FName(TEXT("Life")));
			}
			if (M.Id == "m4")
			{
				AddObj(E, TEXT("Follow the 5-year deorbit rule"), bDebrisOk, TEXT("See 'Space-debris rule'."));
				AddObj(E, TEXT("Fleet of at least 60 satellites"), D.Fleet >= 60, FString::Printf(TEXT("You plan %d."), D.Fleet));
			}
			if (E.Dest == EISDest::Interstellar) AddObj(E, TEXT("Reach the heliopause (120 AU) within 40 years"), E.YearsTo120AU <= Life, TEXT("See 'Speed leaving the Sun'."));
			AddObj(E, TEXT("Stay within budget"), E.TotalCostM <= M.BudgetM, TEXT("See 'Cost'."));
			for (FName T : M.Bonus)
				AddObj(E, TEXT("BONUS: carry ") + TagLabel(T), D.HasTag(T), D.HasTag(T) ? TEXT("Nice extra!") : TEXT("Optional."), true, T);
		}

		// ---------------------------------------------------------------- hint
		E.bCanLaunch = E.Blockers.Num() == 0;
		for (const FISLine& L : E.Lines)
			if (L.Status == EISStatus::Fail)
			{
				E.Hint = FString::Printf(TEXT("Look at %s: %s (need %s). Click it to see why."), *L.Label, *L.Value, *L.Limit);
				break;
			}
		if (E.Hint.IsEmpty() && E.Blockers.Num()) E.Hint = E.Blockers[0];
		if (E.Hint.IsEmpty()) E.Hint = TEXT("All budgets close! Check warnings, then choose your launch.");
		return E;
	}
}

// =====================================================================================
//  Operations
// =====================================================================================
void FISOps::Schedule(double D, EISEvt T, const FString& Text, FName Part, const FString& Edu)
{
	FISEvent Ev; Ev.Day = D; Ev.Type = T; Ev.Text = Text; Ev.Part = Part; Ev.Edu = Edu;
	Queue.Add(Ev);
}

bool FISOps::IsCritical(const FISPart* P) const
{
	if (!P) return false;
	switch (P->Cat)
	{
	case EISCat::Bus: case EISCat::Computer: case EISCat::Comms: return true;
	case EISCat::Power: return true;
	case EISCat::Battery: return Eval.EclipseMin > 0;
	default: return false;
	}
}

void FISOps::Start(const FISDesign& D, const FISMission& M, const FISEval& E, int32 Seed)
{
	Design = D; Mission = M; Eval = E;
	Rng.Initialize(Seed);
	Queue.Reset(); Log.Reset(); LostTagsDay.Reset(); LostParts.Reset();
	Next = 0; Day = 0; DataGb = 0; Health = 1; Battery = 1; GoodChoices = 0; TotalChoices = 0; GoodChoices = 0; TotalChoices = 0; GoodChoices = 0; TotalChoices = 0;
	bAwaitingChoice = false; bFinished = false; bLaunchFailed = false; bMissionLost = false; LostDay = -1;
	bHasRedundancy = D.HasTag(ISTag::Redundancy);

	const FISVehicle* LV = ISData::FindVehicle(D.Vehicle);
	const FString Site = LV ? LV->Site : TEXT("?");
	const FString Veh = LV ? LV->Name : TEXT("?");
	const bool bBackyard = E.Dest == EISDest::Backyard;
	const double Cruise = ISSim::CruiseDays(E.Dest);
	const double LifeDays = E.LifeYears * 365.25;
	EndDay = Cruise + LifeDays;
	if (E.Dest == EISDest::Interstellar) EndDay = FMath::Min((double)E.YearsTo120AU, 60.0) * 365.25;

	Schedule(0.0, EISEvt::Info, FString::Printf(TEXT("Lift-off! %s from %s."), *Veh, *Site), NAME_None,
		LV ? LV->Edu : FString());

	// Launch roll
	if (Rng.FRand() > E.RelLaunch)
	{
		Schedule(bBackyard ? 0.00005 : 0.003, EISEvt::Fatal, TEXT("Launch vehicle failure! The rocket was lost during ascent."), NAME_None,
			TEXT("Launch failures still happen (1-10% of flights for most rockets). Satellites are insured, and agencies sometimes build a spare. Try again: the design was not the problem."));
		bLaunchFailed = true;
		Queue.Sort([](const FISEvent& A, const FISEvent& B) { return A.Day < B.Day; });
		return;
	}

	if (bBackyard)
	{
		const double ApogeeDay = E.FlightAltKm > 5 ? 1.5 / 24.0 : 8.0 / 86400.0;
		const double LandDay = ApogeeDay + (E.FlightAltKm * 1000.0 / FMath::Max(1.0f, E.DescentMs)) / 86400.0;
		EndDay = LandDay + 2.0 / 24.0;
		Schedule(ApogeeDay, EISEvt::Info, FString::Printf(TEXT("Apogee: %s km. CanSat released!"), *ISSim::Fmt(E.FlightAltKm, 0)), NAME_None,
			E.FlightAltKm > 5 ? TEXT("At 30 km the air is 1/80 as dense as at the ground and about -50 C. The balloon expands until it bursts.") : TEXT("The rocket motor burns for ~1-2 s; the rocket coasts up to apogee."));
		if (Design.HasTag(ISTag::Parachute))
			Schedule(ApogeeDay + 2.0 / 86400.0, EISEvt::Good, FString::Printf(TEXT("Parachute open. Descending at %s m/s."), *ISSim::Fmt(E.DescentMs, 1)));
		if (E.RadioRangeKm < E.FlightAltKm * 1.5)
			Schedule(ApogeeDay + 1.0 / 86400.0, EISEvt::Warn, TEXT("Radio signal lost: the CanSat is out of range."), NAME_None,
				TEXT("Radio power falls with distance squared. Bigger antennas or more transmit power increase range."));
		else
			Schedule(ApogeeDay + 3.0 / 86400.0, EISEvt::Good, TEXT("Telemetry received: temperature falling as altitude rises... ~6.5 C per km!"), NAME_None,
				TEXT("This is the 'lapse rate' of the lower atmosphere (troposphere)."));
		if (E.DescentMs > 15.0)
			Schedule(LandDay, EISEvt::Fatal, FString::Printf(TEXT("Hard landing at %s m/s: the CanSat broke."), *ISSim::Fmt(E.DescentMs, 0)), NAME_None,
				TEXT("Kinetic energy = 1/2 m v^2: doubling speed means 4x the impact energy."));
		else
			Schedule(LandDay, E.DescentMs > 10 ? EISEvt::Warn : EISEvt::Good, FString::Printf(TEXT("Touchdown at %s m/s.%s"), *ISSim::Fmt(E.DescentMs, 1), Design.HasTag(ISTag::GNSSRx) ? TEXT(" GNSS position received: found it in 5 minutes!") : TEXT(" Searching the field...")));
		const double DeadDay = E.RuntimeH / 24.0;
		if (DeadDay < EndDay) Schedule(DeadDay, EISEvt::Fatal, TEXT("Battery empty: the CanSat went silent."), NAME_None, TEXT("Energy (Wh) = power (W) x time (h). A bigger battery or lower power draw would last longer."));
		Schedule(EndDay, EISEvt::End, TEXT("Recovery complete. Flight data saved!"));
		Queue.Sort([](const FISEvent& A, const FISEvent& B) { return A.Day < B.Day; });
		return;
	}

	// ---------------------------------------------------------------- orbital / deep-space flow
	// deep space: the upper stage stays attached in the parking orbit and fires once more to leave Earth, then lets go
	const double Dep = ISSim::DepartDay(E.Dest);
	if (Dep > 0.0)
		Schedule(Dep, EISEvt::Info,
			E.Dest == EISDest::Moon ? TEXT("Departure burn: translunar injection, about +3.1 km/s. Next stop: the Moon.") : TEXT("Departure burn: the upper stage pushes the spacecraft out of Earth's gravity."),
			NAME_None, E.Dest == EISDest::Moon ? TEXT("Apollo did the same: one or two orbits to check everything, then a ~6 minute burn of the S-IVB stage.")
				: TEXT("To escape Earth you need ~11.2 km/s from the ground; a parking orbit already gives 7.8, the upper stage adds the rest."));
	Schedule(Dep + 0.01, EISEvt::Good, TEXT("Separation from the rocket. Hello, space!"));
	if (E.SolarEolW > 0 || Eval.AvgGenW > 0)
		Schedule(Dep + 0.03, EISEvt::Good, FString::Printf(TEXT("Power on: generating %s W."), *ISSim::Fmt(E.AvgGenW, 0)));
	Schedule(Dep + 0.06, E.PointingDeg > 0 ? EISEvt::Good : EISEvt::Warn,
		E.PointingDeg > 0 ? FString::Printf(TEXT("Detumbling complete. Pointing accuracy %s deg."), *ISSim::Fmt(E.PointingDeg, 2)) : FString(TEXT("No attitude control: the satellite is tumbling.")),
		NAME_None, TEXT("After separation every satellite spins a little. Detumbling removes that spin."));
	const FISPart* G = ISData::FindPart(D.GetSingle(EISCat::Ground));
	if (E.ContactMin > 0 && E.LinkKbps > 0)
		Schedule(Dep + 0.12, EISEvt::Good, FString::Printf(TEXT("First contact through %s!"), G ? *G->Name : TEXT("ground")));
	else
		Schedule(Dep + 0.12, EISEvt::Fatal, TEXT("No signal. Nobody on Earth can hear the satellite."), NAME_None,
			TEXT("Without a working radio link (range, data rate, ground stations), a perfect satellite is useless."));

	if (E.Dest == EISDest::GEO)
		Schedule(3.0, E.DvAvail >= 1500 ? EISEvt::Good : EISEvt::Fatal,
			E.DvAvail >= 1500 ? TEXT("Apogee engine burns complete: arrived in geostationary orbit.") : FString::Printf(TEXT("Out of propellant %s m/s short of GEO. Stranded in transfer orbit."), *ISSim::Fmt(1500 - E.DvAvail, 0)),
			NAME_None, TEXT("Rockets drop GEO satellites in an elliptical transfer orbit; the satellite climbs the last ~1.5 km/s itself."));
	if (E.Dest == EISDest::Moon)
		Schedule(Cruise, E.DvAvail >= 900 ? EISEvt::Good : EISEvt::Fatal,
			E.DvAvail >= 900 ? TEXT("Lunar orbit insertion burn complete! Orbiting the Moon at 50 km.") : TEXT("Not enough propellant to brake: the probe flew past the Moon."),
			NAME_None, TEXT("The Moon's gravity is weak: if you don't brake ~0.9 km/s you simply fly past it."));
	if (E.Dest == EISDest::Jupiter)
	{
		Schedule(200.0, EISEvt::Info, TEXT("Mid-course correction burn: a few m/s to aim at Jupiter precisely."), NAME_None,
			TEXT("This probe flies the direct route (half an ellipse, 2.7 years). Galileo and Juno took 5-6 years: their rockets were too small for the direct route, so they swung past Venus or Earth to steal orbital momentum (gravity assists)."));
		Schedule(Cruise, E.DvAvail >= 650 ? EISEvt::Good : EISEvt::Fatal,
			E.DvAvail >= 650 ? TEXT("Jupiter orbit insertion! Signal took 48 minutes to reach Earth.") : TEXT("Not enough propellant for orbit insertion: the probe flew past Jupiter."),
			NAME_None, TEXT("With ~45 min light-time, the burn was fully automatic: engineers could only watch."));
	}
	if (E.Dest == EISDest::Interstellar)
	{
		Schedule(1.5 * 365.25, EISEvt::Info, TEXT("Passing Jupiter's orbit."));
		Schedule(FMath::Min(EndDay * 0.25, 3.5 * 365.25), EISEvt::Info, TEXT("Passing Saturn's orbit. The Sun is now just a very bright star."));
		Schedule(EndDay * 0.75, EISEvt::Info, TEXT("Termination shock crossed: the solar wind slows below the speed of sound."), NAME_None,
			TEXT("Voyager 1 crossed the termination shock in 2004 at 94 AU."));
	}

	const double OpsStart = Cruise + 0.5;
	Schedule(OpsStart + 1.0, EISEvt::Good, TEXT("Instruments switched on: science operations begin!"));

	// Scripted historical anomaly: Landsat 1 RBV
	if (Mission.Id == "m2")
		Schedule(OpsStart + 15.0, EISEvt::Anomaly, TEXT("Power-switching fault in the imager electronics! The spacecraft's attitude was upset."), D.GetSingle(EISCat::Payload),
			TEXT("This really happened to Landsat 1 on 6 Aug 1972: a fault in the RBV camera's power switching upset the spacecraft. Engineers switched the RBV off for good and relied on the MSS scanner, which then worked for 5.5 years."));

	// Random part failures (exponential failure times from yearly reliability)
	for (const FName& Id : D.Parts)
	{
		const FISPart* P = ISData::FindPart(Id);
		if (!P || P->Cat == EISCat::Ground || P->RelPerYear >= 0.9999f) continue;
		const double Lambda = -FMath::Loge(P->RelPerYear) * (ISSim::Difficulty == 0 ? 0.5 : ISSim::Difficulty == 2 ? 2.0 : 1.0);
		const double U = FMath::Max(1e-6, (double)Rng.FRand());
		const double TDays = (-FMath::Loge(U) / Lambda) * 365.25;
		const double At = OpsStart + TDays;
		if (At < EndDay)
			Schedule(At, EISEvt::Anomaly, FString::Printf(TEXT("ANOMALY: %s is not responding!"), *P->Name), Id, P->Edu);
	}

	// Deterministic limits
	if (E.LifeEst < E.LifeYears && E.LifeEst >= 0)
	{
		const double At = OpsStart + E.LifeEst * 365.25;
		if (E.LifeLimiter == TEXT("radiation"))
			Schedule(At, EISEvt::Fatal, TEXT("The flight computer failed: radiation damage."), NAME_None, TEXT("Each particle hit slowly damages chips. Radiation-hardened parts and shielding extend life."));
		else if (E.LifeLimiter == TEXT("power"))
			Schedule(At, EISEvt::Fatal, TEXT("Power fell below the load: the satellite can no longer stay on."), NAME_None, TEXT("Solar cells degrade and RTGs decay. Design with margin for END of life, not launch."));
		else if (E.LifeLimiter == TEXT("fuel"))
			Schedule(At, EISEvt::Fatal, TEXT("Out of propellant: the satellite drifts out of its slot."), NAME_None, TEXT("Fuel is usually what ends a GEO satellite's life."));
		else if (E.LifeLimiter == TEXT("orbit decay"))
			Schedule(At, EISEvt::Fatal, TEXT("Air drag pulled the satellite down: it burned up in the atmosphere."), NAME_None, TEXT("At low altitude the thin upper atmosphere slowly steals orbital energy."));
		else if (E.LifeLimiter == TEXT("freezing"))
			Schedule(At, EISEvt::Fatal, TEXT("Electronics froze far from the Sun."), NAME_None, TEXT("Add heaters or an RTG (its waste heat keeps the probe warm)."));
	}

	// Yearly progress messages
	for (int32 Y = 1; Y < 60; ++Y)
	{
		const double At = OpsStart + Y * 365.25;
		if (At >= EndDay - 1) break;
		if (E.LifeYears > 4 && (Y % FMath::Max(1, (int32)(E.LifeYears / 4))) != 0) continue;
		Schedule(At, EISEvt::Info, FString::Printf(TEXT("Year %d of operations."), Y));
	}

	// End of mission
	FString EndText = TEXT("Design life reached: mission complete!");
	FString EndEdu;
	switch (E.Dest)
	{
	case EISDest::LEO: case EISDest::SSO:
		EndText += Eval.DvAvail >= Eval.DvNeed ? TEXT(" Final burn: deorbiting safely over the ocean.") : (Design.HasTag(ISTag::DragSail) ? TEXT(" Drag sail deployed: re-entry within a few years.") : TEXT(" No way to deorbit: the satellite becomes space debris."));
		break;
	case EISDest::GEO: EndText += TEXT(" Boosted 300 km up into the graveyard orbit."); EndEdu = TEXT("GEO slots are precious: dead satellites move above GEO so they don't block the slot."); break;
	case EISDest::Jupiter:
		if (Design.HasTag(ISTag::EntryProbe)) EndText += TEXT(" Entry probe released... beep... beep... beep...... signal lost 150 km below the clouds.");
		EndEdu = TEXT("Juno and Galileo ended by diving into Jupiter, so they could never contaminate the possibly habitable moon Europa.");
		break;
	case EISDest::Interstellar: EndText = TEXT("HELIOPAUSE CROSSED: you are in interstellar space! The magnetometer saw the field change direction."); EndEdu = TEXT("Voyager 1 did this in August 2012 at ~121 AU, Voyager 2 in Nov 2018."); break;
	default: break;
	}
	Schedule(EndDay, EISEvt::End, EndText, NAME_None, EndEdu);
	Queue.Sort([](const FISEvent& A, const FISEvent& B) { return A.Day < B.Day; });
}

void FISOps::LosePart(FName Part, double AtDay)
{
	const FISPart* P = ISData::FindPart(Part);
	if (!P) return;
	LostParts.AddUnique(Part);
	if (!P->Tag.IsNone()) LostTagsDay.FindOrAdd(P->Tag) = AtDay;
	if (P->Cat == EISCat::ADCS) LostTagsDay.FindOrAdd(FName(ADCS_TAG)) = AtDay;
	if (P->Cat == EISCat::Payload)
	{
		bool bOther = false;
		for (const FName& Id : Design.Parts)
		{
			const FISPart* Q = ISData::FindPart(Id);
			if (Q && Q->Cat == EISCat::Payload && Q->Tag == P->Tag && !LostParts.Contains(Id)) bOther = true;
		}
		if (bOther) LostTagsDay.Remove(P->Tag);
	}
	Health = FMath::Max(0.0, Health - 0.25);
	if (IsCritical(P))
	{
		bMissionLost = true; LostDay = AtDay;
		FISEvent Ev; Ev.Day = AtDay; Ev.Type = EISEvt::Fatal; Ev.Text = FString::Printf(TEXT("Loss of %s: the spacecraft cannot continue."), *P->Name);
		Ev.Edu = TEXT("Critical systems (power, computer, radio, structure) are single points of failure unless doubled.");
		Log.Add(Ev);
		Finish();
	}
}

bool FISOps::Advance(double DeltaDays)
{
	if (bFinished || bAwaitingChoice) return false;
	const double Target = FMath::Min(Day + DeltaDays, EndDay);
	bool bNew = false;
	while (Next < Queue.Num() && Queue[Next].Day <= Target)
	{
		const FISEvent& Ev = Queue[Next];
		const double Prev = Day;
		Day = Ev.Day;
		if (!bLaunchFailed && Day > Prev && LostTagsDay.Num() == 0) DataGb += Eval.DataDownGb * (Day - Prev);
		Log.Add(Ev);
		bNew = true;
		if (Ev.Type == EISEvt::Anomaly)
		{
			Pending = Ev;
			PendingChoices.Reset(); ChoiceKinds.Reset();
			if (Mission.Id == "m2" && Ev.Text.Contains(TEXT("Power-switching")))
			{
				PendingChoices.Add(FISChoice{ TEXT("Switch off the faulty circuit for good (lose some imaging capacity)"), FString() }); ChoiceKinds.Add(4);
				PendingChoices.Add(FISChoice{ TEXT("Keep operating and hope it stabilises"), FString() }); ChoiceKinds.Add(3);
			}
			else
			{
				if (bHasRedundancy) { PendingChoices.Add(FISChoice{ TEXT("Switch to the backup unit"), FString() }); ChoiceKinds.Add(0); }
				PendingChoices.Add(FISChoice{ TEXT("Enter safe mode and let engineers debug (lose 7 days)"), FString() }); ChoiceKinds.Add(1);
				PendingChoices.Add(FISChoice{ TEXT("Switch it off and continue without it"), FString() }); ChoiceKinds.Add(2);
			}
			bAwaitingChoice = true;
			return true;
		}
		++Next;
		if (Ev.Type == EISEvt::Fatal) { bMissionLost = true; LostDay = Ev.Day; Finish(); return true; }
		if (Ev.Type == EISEvt::End) { Finish(); return true; }
	}
	if (!bLaunchFailed && LostTagsDay.Num() == 0) DataGb += Eval.DataDownGb * (Target - Day);
	Day = Target;
	// battery telemetry: drains in eclipse, recharges in sunlight
	if (Eval.PeriodMin > 0 && Eval.EclipseMin > 0)
	{
		const double Phase = FMath::Fmod(Day * 1440.0 / Eval.PeriodMin, 1.0);
		const double F = Eval.EclipseMin / Eval.PeriodMin;
		const double DoD = Eval.BatteryWh > 0 ? FMath::Min(1.0, Eval.BatteryNeedWh * ((Eval.Dest == EISDest::LEO || Eval.Dest == EISDest::SSO) ? 0.3 : 0.6) / Eval.BatteryWh) : 1.0;
		Battery = Phase < F ? 1.0 - DoD * (Phase / F) : (1.0 - DoD) + DoD * FMath::Min(1.0, (Phase - F) / (1.0 - F) * 2.0);
	}
	else if (Eval.Dest == EISDest::Backyard) Battery = FMath::Clamp(1.0 - (Day * 24.0) / FMath::Max(0.01f, Eval.RuntimeH), 0.0, 1.0);
	else Battery = 1.0;
	return bNew;
}

void FISOps::Retime(const FString& Prefix, double NewDay)
{
	for (int32 i = Next; i < Queue.Num(); ++i)
		if (Queue[i].Text.StartsWith(Prefix)) { Queue[i].Day = FMath::Max(NewDay, Day); break; }
	// keep the not-yet-reached part ordered (the already-logged part stays as it is)
	TArray<FISEvent> Rest(Queue.GetData() + Next, Queue.Num() - Next);
	Rest.StableSort([](const FISEvent& A, const FISEvent& B) { return A.Day < B.Day; });
	for (int32 i = 0; i < Rest.Num(); ++i) Queue[Next + i] = Rest[i];
}

void FISOps::Choose(int32 Index)
{
	if (!bAwaitingChoice || !ChoiceKinds.IsValidIndex(Index)) return;
	const int32 Kind = ChoiceKinds[Index];
	++TotalChoices;
	if (Kind == 0 || Kind == 1 || Kind == 4) ++GoodChoices;
	const FName Part = Pending.Part;
	const FISPart* P = ISData::FindPart(Part);
	bAwaitingChoice = false;
	++Next;
	FISEvent R; R.Day = Day;
	switch (Kind)
	{
	case 0:
		R.Type = EISEvt::Good; R.Text = TEXT("Switched to the backup unit. Everything nominal again!");
		R.Edu = TEXT("This is why critical electronics are doubled: a 10% mass cost buys years of extra life.");
		Log.Add(R);
		break;
	case 1:
		if (Rng.FRand() < 0.6f)
		{
			R.Type = EISEvt::Good; R.Text = TEXT("Safe mode worked: engineers found a software workaround and uploaded a patch.");
			R.Edu = TEXT("Many anomalies are fixed from the ground: Voyager's engineers still patch 1970s software from 24 billion km away.");
			Log.Add(R);
		}
		else
		{
			R.Type = EISEvt::Warn; R.Text = FString::Printf(TEXT("Debugging failed: %s is permanently lost."), P ? *P->Name : TEXT("the unit"));
			Log.Add(R);
			LosePart(Part, Day);
		}
		Day += 7.0;
		break;
	case 2:
		R.Type = EISEvt::Warn; R.Text = FString::Printf(TEXT("%s switched off. Continuing without it."), P ? *P->Name : TEXT("Unit"));
		Log.Add(R);
		LosePart(Part, Day);
		break;
	case 3:
		if (Rng.FRand() < 0.4f)
		{
			R.Type = EISEvt::Fatal; R.Text = TEXT("The fault spread and the spacecraft lost attitude control permanently.");
			R.Edu = TEXT("Sometimes the safest choice is to give up a part to save the whole mission.");
			Log.Add(R); bMissionLost = true; LostDay = Day; Finish();
		}
		else
		{
			R.Type = EISEvt::Warn; R.Text = TEXT("Lucky! It stabilised, but the risk was high.");
			Log.Add(R);
		}
		break;
	case 4:
		R.Type = EISEvt::Good; R.Text = TEXT("Faulty circuit isolated. The main imager continues: exactly what Landsat 1's engineers did.");
		R.Edu = Pending.Edu;
		Log.Add(R);
		Health = FMath::Max(0.0, Health - 0.1);
		break;
	}
}

void FISOps::Finish()
{
	if (bFinished) return;
	bFinished = true;
	const double Span = FMath::Max(1e-6, EndDay);
	FinalObjectives = Eval.Objectives;
	int32 Req = 0, ReqMet = 0, Bonus = 0;
	FString FirstFail;
	for (FISObjective& O : FinalObjectives)
	{
		if (bLaunchFailed) { O.bMet = false; O.Why = TEXT("Launch failure."); }
		else if (O.bMet)
		{
			if (const double* LostAt = LostTagsDay.Find(O.Needs))
			{
				const double Frac = *LostAt / Span;
				if (Frac < 0.9) { O.bMet = false; O.Why = FString::Printf(TEXT("Lost during operations after %s (%s%% of the mission)."), *ISSim::FmtDays(*LostAt), *ISSim::Fmt(Frac * 100, 0)); }
			}
			if (bMissionLost && LostDay < Span * 0.9 && !O.Text.StartsWith(TEXT("Stay within")) && !O.Text.StartsWith(TEXT("Carry")) && !O.Text.StartsWith(TEXT("BONUS")) && !O.Text.StartsWith(TEXT("Follow")) && !O.Text.StartsWith(TEXT("Fleet")))
			{
				const bool bReachedEarly = O.Text.StartsWith(TEXT("Reach ")) && !O.Text.Contains(TEXT("heliopause")) && LostDay > 1.0 && Eval.Dest != EISDest::Backyard;
				if (!bReachedEarly) { O.bMet = false; O.Why = FString::Printf(TEXT("Mission ended after %s."), *ISSim::FmtDays(LostDay)); }
			}
		}
		if (O.bBonus) { if (O.bMet) ++Bonus; }
		else { ++Req; if (O.bMet) ++ReqMet; else if (FirstFail.IsEmpty()) FirstFail = O.Text + TEXT(": ") + O.Why; }
	}
	ScorePct = Req > 0 ? 100.f * ReqMet / Req : 100.f;
	Stars = (ReqMet == Req && Bonus > 0) ? 3 : (ReqMet == Req) ? 2 : (ScorePct >= 50.f ? 1 : 0);
	if (bLaunchFailed) Debrief = TEXT("The rocket failed: bad luck, not bad design. Real programmes face this too. Fly again!");
	else if (FirstFail.IsEmpty()) Debrief = FString::Printf(TEXT("All objectives met! %s"), *Mission.Lesson);
	else Debrief = FString::Printf(TEXT("Limiting factor -> %s\n%s"), *FirstFail, *Mission.Lesson);
}
