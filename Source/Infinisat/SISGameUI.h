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
	void ShowTitle();
	void ShowCanSatDebrief();
	void ShowPause();
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	enum class EScreen : uint8 { Title, Missions, Design, Launch, Ops, Debrief, Pedia, Help, Play, Bench, CanDebrief, Pause };

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
	int32 PediaSel = 0;
	float PartsScroll = 0.f, BudgetScroll = 0.f;
	TSharedPtr<SScrollBox> PartsBox, BudgetBox;

	// styles
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor::White, 10.f);
	FButtonStyle StyleNormal, StylePrimary, StyleSelected, StyleDisabled;
	FSlateBrush LogoBrush, BitBrush;
	FProgressBarStyle BarStyle;
	FString DevShot;
	TArray<TPair<FString, double>> ToastLog;
	int32 SeenDesignVersion = -1;
	double UITime = 0.0;
	float DevShotDelay = 12.f, DevShotT = 0.f;
	bool bDevShotTaken = false;
};
