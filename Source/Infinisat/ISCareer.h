// Infinisat - the career: one saved space company that ties every building together (the KSP / Mars Horizon spine).
//   Mission Control: contracts open up with REPUTATION.
//   Assembly Hall:   you can only build with parts of the TECH TIERS you have researched.
//   R&D Lab:         research tiers with SCIENCE (data your satellites send home) and FUNDS (contract payments).
//   Every finished mission pays funds, reputation and science, so each flight feeds the next one.
// Saved in Saved/Config/<platform>/GameUserSettings.ini, section [InfinisatCareer].
#pragma once

#include "CoreMinimal.h"

struct FISMission;

struct FISCareerReward
{
	double FundsM = 0.0;
	int32 Rep = 0;
	int32 Science = 0;
	int32 NewStars = 0;      // stars above the previous best (repeat flights only pay for improvements)
	bool bRepeat = false;
	TArray<int32> NewContracts;   // mission indices this flight opened up
};

namespace ISCareer
{
	static constexpr int32 MaxTier = 7;

	struct FState
	{
		bool bStarted = false;
		double FundsM = 0.0;
		int32 Rep = 0;
		int32 Science = 0;
		int32 Tier = 0;          // highest researched tech tier (parts with Unlock <= Tier can be built)
		int32 Flights = 0;
		TArray<int32> Best;      // best stars per mission (-1 = never flown)
	};

	FState& Get();
	void Load();
	void Save();
	void NewCareer();
	bool HasSave();

	// teacher / judge mode: everything open, nothing gated (Settings)
	extern bool bAllOpen;
	// New career reloads the level; the player controller then walks straight into the world
	extern bool bEnterWorldOnLoad;

	// contracts
	int32 RepNeeded(int32 Chapter);
	bool ContractOpen(int32 MissionIdx);
	bool TierOk(const FISMission& M);         // the parts this contract needs are researched
	int32 BestStars(int32 MissionIdx);
	FString PreviewReward(int32 MissionIdx);   // "Pays up to $37.5M, +30 rep, science from data"

	// R&D
	int32 TierScience(int32 T);
	double TierFundsM(int32 T);
	FString TierName(int32 T);
	FString TierBlurb(int32 T);
	bool CanResearch(int32 T, FString* WhyNot = nullptr);
	bool Research(int32 T);

	// a mission is over: pay the company (once per flight)
	FISCareerReward Award(int32 MissionIdx, int32 Stars, int32 MetObjectives, double DataGb, double CostM);
	FString RewardText(const FISCareerReward& R);

	FString StatusLine();   // "FUNDS $12.4M   REPUTATION 40   SCIENCE 22   TECH 2"
}
