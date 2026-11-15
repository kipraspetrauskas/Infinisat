// Infinisat - the career (see ISCareer.h).
#include "ISCareer.h"
#include "ISData.h"
#include "ISSim.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace ISCareer
{
	bool bAllOpen = false;
	bool bEnterWorldOnLoad = false;

	static const TCHAR* Section = TEXT("InfinisatCareer");
	// dev / test runs (-ISFresh) start from an empty company and never touch the player's save
	static bool Fresh() { return FParse::Param(FCommandLine::Get(), TEXT("ISFresh")); }

	FState& Get()
	{
		static FState S;
		const int32 N = ISData::Missions().Num();
		if (S.Best.Num() != N) { const int32 Old = S.Best.Num(); S.Best.SetNum(N); for (int32 i = Old; i < N; ++i) S.Best[i] = -1; }
		return S;
	}

	bool HasSave()
	{
		if (Fresh()) return Get().bStarted;
		bool b = false;
		GConfig->GetBool(Section, TEXT("Started"), b, GGameUserSettingsIni);
		return b;
	}

	void Load()
	{
		if (Fresh()) return;
		FState& S = Get();
		GConfig->GetBool(Section, TEXT("Started"), S.bStarted, GGameUserSettingsIni);
		FString F;
		if (GConfig->GetString(Section, TEXT("Funds"), F, GGameUserSettingsIni)) S.FundsM = FCString::Atod(*F);
		GConfig->GetInt(Section, TEXT("Rep"), S.Rep, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("Science"), S.Science, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("Tier"), S.Tier, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("Flights"), S.Flights, GGameUserSettingsIni);
		FString B;
		if (GConfig->GetString(Section, TEXT("Best"), B, GGameUserSettingsIni))
		{
			TArray<FString> P; B.ParseIntoArray(P, TEXT(","));
			for (int32 i = 0; i < P.Num() && i < S.Best.Num(); ++i) S.Best[i] = FCString::Atoi(*P[i]);
		}
		GConfig->GetBool(Section, TEXT("AllOpen"), bAllOpen, GGameUserSettingsIni);
	}

	void Save()
	{
		if (Fresh()) return;
		const FState& S = Get();
		GConfig->SetBool(Section, TEXT("Started"), S.bStarted, GGameUserSettingsIni);
		GConfig->SetString(Section, TEXT("Funds"), *FString::Printf(TEXT("%.6f"), S.FundsM), GGameUserSettingsIni);
		GConfig->SetInt(Section, TEXT("Rep"), S.Rep, GGameUserSettingsIni);
		GConfig->SetInt(Section, TEXT("Science"), S.Science, GGameUserSettingsIni);
		GConfig->SetInt(Section, TEXT("Tier"), S.Tier, GGameUserSettingsIni);
		GConfig->SetInt(Section, TEXT("Flights"), S.Flights, GGameUserSettingsIni);
		FString B;
		for (int32 i = 0; i < S.Best.Num(); ++i) B += (i ? TEXT(",") : TEXT("")) + FString::FromInt(S.Best[i]);
		GConfig->SetString(Section, TEXT("Best"), *B, GGameUserSettingsIni);
		GConfig->SetBool(Section, TEXT("AllOpen"), bAllOpen, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	void NewCareer()
	{
		FState& S = Get();
		S = FState();
		Get();   // re-size Best
		S.bStarted = true;
		S.FundsM = 0.0005;   // $500 of pocket money for the first CanSat
		if (!Fresh()) GConfig->EmptySection(TEXT("InfinisatProgress"), GGameUserSettingsIni);   // the house starts over too (parts to find, empty bench)
		Save();
	}

	// ------------------------------------------------------------------ contracts
	int32 RepNeeded(int32 Chapter)
	{
		static const int32 R[] = { 0, 10, 30, 50, 70, 90, 110, 130 };
		return R[FMath::Clamp(Chapter, 0, 7)];
	}

	static bool IsSandbox(int32 Idx) { const TArray<FISMission>& Ms = ISData::Missions(); return Ms.IsValidIndex(Idx) && Ms[Idx].Id == "sandbox"; }

	bool ContractOpen(int32 Idx)
	{
		const TArray<FISMission>& Ms = ISData::Missions();
		if (!Ms.IsValidIndex(Idx)) return false;
		if (bAllOpen || IsSandbox(Idx)) return true;
		return Get().Rep >= RepNeeded(Ms[Idx].Chapter);
	}

	bool TierOk(const FISMission& M) { return bAllOpen || M.Id == "sandbox" || M.Chapter <= Get().Tier; }

	int32 BestStars(int32 Idx) { const FState& S = Get(); return S.Best.IsValidIndex(Idx) ? S.Best[Idx] : -1; }

	static double FeePerStar(const FISMission& M) { return M.BudgetM * 0.05; }

	FString PreviewReward(int32 Idx)
	{
		const TArray<FISMission>& Ms = ISData::Missions();
		if (!Ms.IsValidIndex(Idx) || IsSandbox(Idx)) return TEXT("Sandbox: no payment, no reputation.");
		const FISMission& M = Ms[Idx];
		const int32 Prev = FMath::Max(0, BestStars(Idx));
		if (Prev >= 3) return TEXT("Completed with 3 stars: flying it again only earns some science from the data.");
		return FString::Printf(TEXT("Pays %s per star (+ a quarter of any money you save), +10 reputation per star, science from the data you send home.%s"),
			*ISSim::FmtMoney(FeePerStar(M)), Prev > 0 ? *FString::Printf(TEXT(" You already have %d star(s): only better results pay."), Prev) : TEXT(""));
	}

	// ------------------------------------------------------------------ R&D
	int32 TierScience(int32 T) { static const int32 C[] = { 0, 15, 30, 45, 60, 80, 100, 120 }; return C[FMath::Clamp(T, 0, MaxTier)]; }
	double TierFundsM(int32 T) { static const double C[] = { 0, 0, 5, 10, 20, 30, 45, 60 }; return C[FMath::Clamp(T, 0, MaxTier)]; }

	FString TierName(int32 T)
	{
		static const TCHAR* N[] = { TEXT("Hobby electronics & CubeSats"), TEXT("Satellite buses & atomic clocks"), TEXT("Earth observation"), TEXT("Geostationary platforms"),
			TEXT("Constellations & electric propulsion"), TEXT("Deep space & the Moon"), TEXT("Outer planets & nuclear power"), TEXT("Interstellar & solar sails") };
		return N[FMath::Clamp(T, 0, MaxTier)];
	}

	FString TierBlurb(int32 T)
	{
		FString S;
		int32 N = 0;
		for (const FISPart& P : ISData::Parts())
			if (P.Unlock == T) { S += (N ? TEXT(", ") : TEXT("")) + P.Name; ++N; }
		return N ? S : FString(TEXT("-"));
	}

	bool CanResearch(int32 T, FString* WhyNot)
	{
		const FState& S = Get();
		auto No = [WhyNot](const FString& W) { if (WhyNot) *WhyNot = W; return false; };
		if (T <= S.Tier) return No(TEXT("Researched"));
		if (T != S.Tier + 1) return No(FString::Printf(TEXT("Research tier %d first"), S.Tier + 1));
		if (S.Science < TierScience(T)) return No(FString::Printf(TEXT("Needs %d science (you have %d)"), TierScience(T), S.Science));
		if (S.FundsM + 1e-9 < TierFundsM(T)) return No(FString::Printf(TEXT("Needs %s (you have %s)"), *ISSim::FmtMoney(TierFundsM(T)), *ISSim::FmtMoney(S.FundsM)));
		return true;
	}

	bool Research(int32 T)
	{
		if (!CanResearch(T)) return false;
		FState& S = Get();
		S.Science -= TierScience(T);
		S.FundsM -= TierFundsM(T);
		S.Tier = T;
		Save();
		return true;
	}

	// ------------------------------------------------------------------ results
	FISCareerReward Award(int32 Idx, int32 Stars, int32 Met, double DataGb, double CostM)
	{
		FISCareerReward R;
		FState& S = Get();
		const TArray<FISMission>& Ms = ISData::Missions();
		if (!Ms.IsValidIndex(Idx) || IsSandbox(Idx)) return R;
		const FISMission& M = Ms[Idx];
		TArray<bool> WasOpen;
		for (int32 i = 0; i < Ms.Num(); ++i) WasOpen.Add(ContractOpen(i));

		const int32 Prev = S.Best[Idx];
		R.bRepeat = Prev >= 0;
		R.NewStars = FMath::Max(0, Stars - FMath::Max(0, Prev));
		R.FundsM = FeePerStar(M) * R.NewStars;
		if (Stars > 0 && !R.bRepeat) R.FundsM += 0.25 * FMath::Max(0.0, (double)M.BudgetM - CostM);
		R.Rep = Stars == 0 ? -5 : 10 * R.NewStars;
		int32 Sci = 3 * Met + 5 * Stars + FMath::Min(20, FMath::RoundToInt(4.0 * FMath::Log2(1.0 + FMath::Max(0.0, DataGb))));
		if (R.bRepeat) Sci /= 2;
		R.Science = Sci;

		S.bStarted = true;
		S.FundsM += R.FundsM;
		S.Rep = FMath::Max(0, S.Rep + R.Rep);
		S.Science += R.Science;
		S.Best[Idx] = FMath::Max(Prev, Stars);
		++S.Flights;
		for (int32 i = 0; i < Ms.Num(); ++i) if (!WasOpen[i] && ContractOpen(i)) R.NewContracts.Add(i);
		Save();
		return R;
	}

	FString RewardText(const FISCareerReward& R)
	{
		FString S = FString::Printf(TEXT("%s%s funds     %s%d reputation     +%d science"),
			R.FundsM >= 0 ? TEXT("+") : TEXT("-"), *ISSim::FmtMoney(FMath::Abs(R.FundsM)), R.Rep >= 0 ? TEXT("+") : TEXT(""), R.Rep, R.Science);
		if (R.bRepeat && R.NewStars == 0) S += TEXT("\n(A repeat flight: no new stars, so the client pays nothing extra.)");
		for (int32 i : R.NewContracts)
			if (ISData::Missions().IsValidIndex(i)) S += TEXT("\nNEW CONTRACT at Mission Control: ") + ISData::Missions()[i].Title;
		const FState& St = Get();
		if (St.Tier < MaxTier && CanResearch(St.Tier + 1))
			S += FString::Printf(TEXT("\nThe R&D Lab can now research tier %d: %s."), St.Tier + 1, *TierName(St.Tier + 1));
		return S;
	}

	FString StatusLine()
	{
		const FState& S = Get();
		return FString::Printf(TEXT("FUNDS %s     REPUTATION %d     SCIENCE %d     TECH TIER %d"), *ISSim::FmtMoney(S.FundsM), S.Rep, S.Science, S.Tier);
	}
}
