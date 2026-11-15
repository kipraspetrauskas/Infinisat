// Infinisat - the whole game UI in Slate (code-only, no binary assets needed).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"
#include "ISData.h"
#include "ISSim.h"

class AISSpaceScene;
class APlayerController;
class SScrollBox;
class SBox;

class SISGameUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SISGameUI) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AISSpaceScene>, Scene)
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, PC)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ShowOrders();
	void OpenBuilding(FName Action);   // a console in the space centre: assembly hall / lab / tracking station
	void OpenGlobe(double AltM);       // zoomed out from the ground: the real Earth with your space centre
	double GlobeAlt = 150000.0;
	bool bHaveDesign = false;          // an orbital mission is in progress (the assembly hall continues it)
	void ShowTitle();
	void ShowCanSatDebrief();
	void ShowPause();
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override;

private:
	enum class EScreen : uint8 { Title, Missions, Design, Launch, Ops, Debrief, Pedia, Help, Play, Bench, CanDebrief, Pause, Cosmos, Globe };

	// navigation / state changes (deferred rebuild)
	void Go(EScreen S);
	void Rebuild();
	void Recompute(bool bPoke = false);
	void StartMission(int32 Index);
	void StartOps();
	void StarterDesign(const FISMission& M);
	const FISMission& Mission() const;
	bool IsUnlocked(const FISPart& P) const;
	void UpdateScene();

	// screens
	TSharedRef<SWidget> BuildTitle();
	TSharedRef<SWidget> BuildMissions();
	TSharedRef<SWidget> BuildDesign();
	TSharedRef<SWidget> BuildLaunch();
	TSharedRef<SWidget> BuildOps();
	TSharedRef<SWidget> BuildDebrief();
	TSharedRef<SWidget> BuildPedia();
	TSharedRef<SWidget> BuildCosmos();
	TSharedRef<SWidget> BuildGlobe();
	bool bGlobeInCosmos = false;   // the solar system was reached by zooming out from the globe
	TSharedPtr<SScrollBox> CosmosBox;
	float CosmosScroll = 0.f;
	TSharedRef<SWidget> BuildHelp();
	TSharedRef<SWidget> BuildPlay();
	TSharedRef<SWidget> BuildBench();
	TSharedRef<SWidget> BuildCanDebrief();
	TSharedRef<SWidget> BuildPause();
	class AISPlayerController* IPC() const;
	class AISHomeWorld* HW() const;
	bool InWorld() const;
	TSharedRef<SWidget> BuildTopBar(const FString& Title);
	TSharedRef<SWidget> BuildBudgets(bool bCompact);
	TSharedRef<SWidget> BuildObjectives(const TArray<FISObjective>& Objs);
	TSharedRef<SWidget> BuildBitTip(const FString& Text);

	// widget helpers
	TSharedRef<STextBlock> Txt(const FString& S, int32 Size = 12, const FLinearColor& C = FLinearColor::White, bool bBold = false);
	TSharedRef<SWidget> Btn(const FString& Label, TFunction<void()> OnClick, int32 Kind = 0, int32 Size = 12, bool bEnabled = true);
	TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, float Pad = 14.f, int32 Kind = 0);
	FLinearColor StatusColor(EISStatus S) const;

	TWeakObjectPtr<AISSpaceScene> Scene;
	TWeakObjectPtr<APlayerController> PC;
	TSharedPtr<SBox> Root;

	EScreen Screen = EScreen::Title;
	bool bDirty = true;
	bool bJudgeMode = true;
	int32 MissionIdx = 0;
	int32 MaxChapter = 0;
	TArray<int32> BestStars;
	FISDesign Design;
	FISEval Eval;
	EISCat Cat = EISCat::Bus;
	FName InfoPart;
	FString InfoLine;
	FISOps Ops;
	float Warp = 1.f;
	int32 RunSeed = 1;
	TArray<int32> QuizAnswers;
	// team play + scoring
	int32 TeamSize = 1, ActivePlayer = 0;
	TArray<bool> Ready;
	FString TeamName = TEXT("Team Infinisat");
	bool bAckFinish = false, bRecorded = false;
	FString LastReportPath, Notice;
	struct FISRole { FString Name; TArray<EISCat> Cats; bool bLaunch = false; FString Goal; int32 GoalId = 0; };
	TArray<FISRole> Roles() const;
	bool OwnsCat(EISCat C) const;
	bool OwnsLaunch() const;
	bool RoleGoalMet(int32 GoalId) const;
	struct FISScore { float Success = 0, Science = 0, Efficiency = 0, Decisions = 0, Team = 0, Total = 0; };
	FISScore ComputeScore() const;
	FString DesignCode() const;
	bool ApplyDesignCode(const FString& Code);
	void RecordScore(const FISScore& S);
	TArray<FString> Leaderboard() const;
	FString WriteReport(const FISScore& S);
	TSharedRef<SWidget> BuildTeamBar();
	int32 PediaSel = 0;
	float PartsScroll = 0.f, BudgetScroll = 0.f;
	TSharedPtr<SScrollBox> PartsBox, BudgetBox;

	// styles
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor::White, 3.f, FLinearColor(1.f, 1.f, 1.f, 0.07f), 1.f);
	FButtonStyle StyleNormal, StylePrimary, StyleSelected, StyleDisabled;
	FSlateBrush LogoBrush, BitBrush;
	bool bCardDrag = false;      // a part card is being dragged into the 3D can (mouse captured by this widget)
	double CardDragStart = 0.0;
	FGeometry RootGeo;
	void CardDragRay(const FPointerEvent& E);
	TMap<FName, TSharedPtr<FSlateBrush>> ThumbBrushes;   // part pictures (render targets owned by the world)
	TMap<int32, TSharedPtr<FSlateBrush>> BodyBrushes;    // round planet pictures for the cosmos body strip
	const FSlateBrush* BodyBrush(int32 Body);
	bool bOpsInCosmos = false;   // the mission is running while the player looks at the solar system
	bool bShowLog = false;       // mission view: full event log open
	int32 BenchCat = 0;          // builder catalogue tab (0 = all)
	const FSlateBrush* ThumbBrush(FName PartId);
	FProgressBarStyle BarStyle;
	FString DevShot;
	TArray<TPair<FString, double>> ToastLog;
	int32 SeenDesignVersion = -1;
	double UITime = 0.0;
	float DevShotDelay = 12.f, DevShotT = 0.f;
	bool bDevShotTaken = false;
};
