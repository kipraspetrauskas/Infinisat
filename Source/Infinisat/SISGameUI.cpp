#include "SISGameUI.h"
#include "ISScene.h"
#include "ISGame.h"
#include "ISWorld.h"
#include "ISBit.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	const FLinearColor CText(0.93f, 0.96f, 1.f);
	const FLinearColor CDim(0.6f, 0.69f, 0.8f);
	const FLinearColor CAccent(0.36f, 0.86f, 1.f);
	const FLinearColor COk(0.4f, 1.f, 0.55f);
	const FLinearColor CWarn(1.f, 0.75f, 0.25f);
	const FLinearColor CFail(1.f, 0.4f, 0.35f);
	const FLinearColor CGold(1.f, 0.82f, 0.35f);
	const FLinearColor CPanel(0.012f, 0.022f, 0.05f, 0.78f);
	const FLinearColor CPanel2(0.025f, 0.06f, 0.13f, 0.86f);


	// Titillium Web (UI text), Rajdhani (headings), Share Tech Mono (readouts) - SIL Open Font License
	FSlateFontInfo ISFont(const char* Style, int32 Size)
	{
		const FString S(Style);
		FString File = TEXT("TitilliumWeb-Regular.ttf");
		float Scale = 1.08f;
		if (S == TEXT("Bold")) { File = TEXT("Rajdhani-Bold.ttf"); Scale = 1.2f; }
		else if (S == TEXT("SemiBold")) { File = TEXT("TitilliumWeb-SemiBold.ttf"); }
		else if (S == TEXT("Mono")) { File = TEXT("ShareTechMono-Regular.ttf"); Scale = 1.f; }
		const FString Path = FPaths::ProjectContentDir() / TEXT("Slate/Fonts") / File;
		if (!FPaths::FileExists(Path)) return FCoreStyle::GetDefaultFontStyle(Style, Size);
		return FSlateFontInfo(Path, FMath::RoundToInt(Size * Scale));
	}

	FButtonStyle MakeStyle(const FLinearColor& N, const FLinearColor& H, const FLinearColor& P)
	{
		FButtonStyle S;
		const FLinearColor Edge(0.4f, 0.8f, 1.f, 0.28f);
		S.SetNormal(FSlateRoundedBoxBrush(N, 8.f, Edge, 1.f));
		S.SetHovered(FSlateRoundedBoxBrush(H, 8.f, FLinearColor(0.55f, 0.9f, 1.f, 0.7f), 1.f));
		S.SetPressed(FSlateRoundedBoxBrush(P, 8.f, Edge, 1.f));
		S.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.07f, 0.09f, 0.7f), 8.f));
		S.SetNormalPadding(FMargin(14, 8));
		S.SetPressedPadding(FMargin(14, 9, 14, 7));
		return S;
	}

	struct FPediaEntry { const TCHAR* Name; const TCHAR* Mesh; const TCHAR* Facts; };
	const FPediaEntry Pedia[] = {
		{ TEXT("Landsat 1 (1972)"), TEXT("Landsat1"), TEXT("The first Earth-resources satellite (launched as ERTS-1, 23 July 1972, Delta 900, Vandenberg). 953 kg, 917 km sun-synchronous orbit at 99.2 deg, 103-minute period, 18-day repeat. Its Multispectral Scanner (4 bands, 80 m, 185 km swath) imaged nearly all of Earth's land by 1973. The RBV camera failed after 15 days. Design life 1 year, operated 5.5 years. Started the longest continuous record of Earth's surface (still running with Landsat 8 and 9).") },
		{ TEXT("Landsat 8 (2013)"), TEXT("Landsat8"), TEXT("~2.6 t, 705 km sun-synchronous orbit. Operational Land Imager (30 m, 15 m panchromatic) and Thermal Infrared Sensor. Takes ~700 scenes per day. All Landsat data has been free since 2008, which led to a huge increase in scientific use.") },
		{ TEXT("GOES-R series (weather, GEO)"), TEXT("GOES"), TEXT("NOAA's geostationary weather satellites. ~5.2 t at launch. The ABI imager scans the full Earth disk every 10 minutes in 16 bands, and the Geostationary Lightning Mapper sees lightning day and night. Forecasters use them to watch hurricanes and storms develop in real time.") },
		{ TEXT("Suomi NPP (polar weather)"), TEXT("SuomiNPP"), TEXT("Launched 2011 into an 824 km sun-synchronous orbit. Instruments: VIIRS imager, ATMS microwave sounder, CrIS infrared sounder. Polar sounders like these give the temperature/humidity profiles that are the biggest single input to global weather forecasts.") },
		{ TEXT("TDRS (relay, GEO)"), TEXT("TDRS"), TEXT("NASA's Tracking and Data Relay Satellites, flying since 1983. From GEO they relay data from the ISS, Hubble and many low satellites almost continuously, instead of waiting for a pass over a ground station. An existing network others can use!") },
		{ TEXT("CYGNSS (uses GPS!)"), TEXT("CYGNSS"), TEXT("8 small satellites (2016, ~29 kg each, 510 km, 35 deg). They measure hurricane wind speeds by catching GPS signals reflected off the rough ocean: a clever reuse of an existing satellite network.") },
		{ TEXT("CubeSat 1U"), TEXT("CubeSat1U"), TEXT("The 10 cm cube standard (Cal Poly & Stanford, 1999). Thousands have flown: student projects, Earth imaging fleets, even deep-space CubeSats (MarCO relayed InSight's Mars landing in 2018).") },
		{ TEXT("Lunar Reconnaissance Orbiter"), TEXT("LRO"), TEXT("Launched 2009. 1,916 kg including 898 kg of propellant. 50 km polar lunar orbit. Its camera (0.5 m pixels) photographed the Apollo landing sites; its laser altimeter LOLA made the best topographic map of the Moon; LEND searched for ice. Still mapping landing sites for Artemis.") },
		{ TEXT("Juno (Jupiter)"), TEXT("Juno"), TEXT("Launched 2011, arrived 2016. First solar-powered probe at Jupiter: three 9 m wings (~14 kW at Earth, ~500 W at Jupiter). Electronics in a 180 kg titanium vault. Spin-stabilised. Polar orbit dives under the radiation belts. Microwave radiometer sees deep into the atmosphere.") },
		{ TEXT("Galileo (Jupiter)"), TEXT("Galileo"), TEXT("Launched 1989, orbited Jupiter 1995-2003, RTG powered. Its entry probe radioed for 58 minutes inside Jupiter. Its big umbrella-like high-gain antenna never opened, so engineers reprogrammed it to send data through a small antenna at ~160 bit/s using clever compression. A famous lesson in recovering from failure.") },
		{ TEXT("Voyager 1"), TEXT("Voyager"), TEXT("Launched 5 Sept 1977. 722 kg, 3 RTGs (470 W at launch), 3.7 m dish. Gravity assists at Jupiter and Saturn. Crossed the heliopause in Aug 2012 at ~121 AU: the first human-made object in interstellar space. Carries the Golden Record. Still talking to the Deep Space Network.") },
		{ TEXT("Solar sails"), TEXT("SolarSail"), TEXT("Sails push on sunlight: no fuel at all. JAXA's IKAROS (2010, 196 m^2) was the first interplanetary sail; NASA's ACS3 (2024) tested 80 m^2 of composite booms. Concepts with ultralight films could reach interstellar space faster than Voyager.") },
		{ TEXT("Parker Solar Probe"), TEXT("Parker"), TEXT("Launched 2018. Flies through the Sun's outer atmosphere behind a carbon heat shield. In Dec 2024 it passed 6.1 million km from the Sun at ~192 km/s: the fastest human-made object ever.") },
		{ TEXT("International Space Station"), TEXT("ISS"), TEXT("~420 km, 51.6 deg, about 92 minutes per orbit, ~450 tonnes. Continuously crewed since November 2000. Needs regular reboosts because air drag at its altitude is significant.") },
		{ TEXT("SSL-1300 GEO platform"), TEXT("SSL1300"), TEXT("One of the most-used commercial GEO communications platforms. Large solar arrays, heavy propellant load for ~15 years of station-keeping, many transponders for TV and internet.") },
		{ TEXT("Deep Space Network 70 m"), TEXT("DSN70"), TEXT("Giant dishes at Goldstone (USA), Madrid (Spain) and Canberra (Australia), about 120 degrees apart so at least one always sees any deep-space probe. Shared by dozens of missions.") },
	};
	constexpr int32 PediaCount = UE_ARRAY_COUNT(Pedia);

	// Live telemetry plot: altitude (cyan) and temperature (orange) against time.
	class SISTelemetryGraph : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SISTelemetryGraph) {}
			SLATE_ARGUMENT(TWeakObjectPtr<AISHomeWorld>, World)
		SLATE_END_ARGS()
		void Construct(const FArguments& A) { World = A._World; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(360, 150); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FVector2D Sz = G.GetLocalSize();
			TArray<FVector2D> Box = { FVector2D(0, 0), FVector2D(Sz.X, 0), FVector2D(Sz.X, Sz.Y), FVector2D(0, Sz.Y), FVector2D(0, 0) };
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Box, ESlateDrawEffect::None, FLinearColor(0.3f, 0.4f, 0.55f), true, 1.f);
			if (!World.IsValid() || World->Telemetry.Num() < 2) return Layer + 1;
			const TArray<FISTelemetry>& T = World->Telemetry;
			float MaxT = 1, MaxA = 10, MinC = 100, MaxC = -100;
			for (const FISTelemetry& X : T) { MaxT = FMath::Max(MaxT, X.T); MaxA = FMath::Max(MaxA, X.AltM); MinC = FMath::Min(MinC, X.TempC); MaxC = FMath::Max(MaxC, X.TempC); }
			MaxC += 0.5f; MinC -= 0.5f;
			TArray<FVector2D> A, C;
			for (const FISTelemetry& X : T)
			{
				const float Px = X.T / MaxT * (Sz.X - 8) + 4;
				A.Add(FVector2D(Px, Sz.Y - 4 - X.AltM / MaxA * (Sz.Y - 8)));
				C.Add(FVector2D(Px, Sz.Y - 4 - (X.TempC - MinC) / (MaxC - MinC) * (Sz.Y - 8)));
			}
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), A, ESlateDrawEffect::None, FLinearColor(0.35f, 0.82f, 1.f), true, 2.f);
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), C, ESlateDrawEffect::None, FLinearColor(1.f, 0.6f, 0.2f), true, 2.f);
			return Layer + 2;
		}
	private:
		TWeakObjectPtr<AISHomeWorld> World;
	};
	FString MeshOf(const TCHAR* N) { return FString::Printf(TEXT("/Game/Infinisat/Models/%s/SM_%s.SM_%s"), N, N, N); }
}

// =====================================================================================================
void SISGameUI::Construct(const FArguments& InArgs)
{
	Scene = InArgs._Scene;
	PC = InArgs._PC;
	StyleNormal = MakeStyle(FLinearColor(0.04f, 0.08f, 0.15f, 0.92f), FLinearColor(0.07f, 0.16f, 0.28f, 0.95f), FLinearColor(0.02f, 0.05f, 0.1f));
	StylePrimary = MakeStyle(FLinearColor(0.0f, 0.42f, 0.66f), FLinearColor(0.02f, 0.58f, 0.86f), FLinearColor(0.0f, 0.3f, 0.48f));
	StyleSelected = MakeStyle(FLinearColor(0.78f, 0.46f, 0.08f), FLinearColor(0.92f, 0.58f, 0.14f), FLinearColor(0.6f, 0.35f, 0.05f));
	StyleDisabled = MakeStyle(FLinearColor(0.12f, 0.12f, 0.14f), FLinearColor(0.15f, 0.15f, 0.17f), FLinearColor(0.1f, 0.1f, 0.12f));
	BarStyle = FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar");

	if (UTexture2D* L = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Infinisat/Textures/T_UI_Logo.T_UI_Logo")))
	{
		LogoBrush.SetResourceObject(L);
		LogoBrush.ImageSize = FVector2D(L->GetSizeX(), L->GetSizeY()) * (200.f / FMath::Max(1, L->GetSizeY()));
	}
	if (UTexture2D* B = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Infinisat/Textures/T_UI_Bit.T_UI_Bit")))
	{
		BitBrush.SetResourceObject(B);
		BitBrush.ImageSize = FVector2D(B->GetSizeX(), B->GetSizeY()) * (90.f / FMath::Max(1, B->GetSizeY()));
	}
	BestStars.Init(-1, ISData::Missions().Num());

	ChildSlot
	[
		SAssignNew(Root, SBox)
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
	Rebuild();

	// Dev / demo shortcuts: -ISMission=2 -ISScreen=design|launch|ops|debrief|pedia|missions|help
	int32 DevMission = -1;
	FString DevScreen;
	FParse::Value(FCommandLine::Get(), TEXT("ISTeam="), TeamSize);
	FParse::Value(FCommandLine::Get(), TEXT("ISDiff="), ISSim::Difficulty);
	if (FParse::Value(FCommandLine::Get(), TEXT("ISMission="), DevMission)) StartMission(FMath::Clamp(DevMission, 0, ISData::Missions().Num() - 1));
	if (FParse::Value(FCommandLine::Get(), TEXT("ISScreen="), DevScreen))
	{
		if (DevScreen == TEXT("launch")) Go(EScreen::Launch);
		else if (DevScreen == TEXT("ops")) { StartOps(); if (Scene.IsValid() && FParse::Param(FCommandLine::Get(), TEXT("ISNoLaunch"))) Scene->SkipLaunch(); if (Scene.IsValid() && FParse::Param(FCommandLine::Get(), TEXT("ISMap"))) Scene->ToggleMap(); }
		else if (DevScreen == TEXT("debrief")) { StartOps(); while (!Ops.IsFinished()) { if (Ops.AwaitingChoice()) Ops.Choose(0); else Ops.Advance(Ops.EndDay / 50.0); } Go(EScreen::Debrief); }
		else if (DevScreen == TEXT("pedia")) Go(EScreen::Pedia);
		else if (DevScreen == TEXT("cosmos")) { Go(EScreen::Cosmos); int32 F = -1; if (Scene.IsValid() && FParse::Value(FCommandLine::Get(), TEXT("ISCosmosFocus="), F)) { Scene->FocusCosmos(F); } }
		else if (DevScreen == TEXT("missions")) Go(EScreen::Missions);
		else if (DevScreen == TEXT("help")) Go(EScreen::Help);
	}
	FParse::Value(FCommandLine::Get(), TEXT("ISShot="), DevShot);
	FParse::Value(FCommandLine::Get(), TEXT("ISShotDelay="), DevShotDelay);
}

void SISGameUI::Tick(const FGeometry& G, const double Now, const float Dt)
{
	SCompoundWidget::Tick(G, Now, Dt);
	UITime += Dt;
	if (AISPlayerController* P = IPC())
	{
		const EISMode M = P->GetMode();
		if (M == EISMode::Walk && Screen != EScreen::Play) { Screen = EScreen::Play; bDirty = true; }
		if (M == EISMode::Bench && Screen != EScreen::Bench) { Screen = EScreen::Bench; bDirty = true; }
	}
	if (AISHomeWorld* W = HW())
	{
		for (const FString& S : W->Toasts) ToastLog.Add(TPair<FString, double>(S, UITime));
		W->Toasts.Reset();
		if (Screen == EScreen::Bench && SeenDesignVersion != W->DesignVersion) { SeenDesignVersion = W->DesignVersion; Eval = W->Eval; bDirty = true; }
	}
	if (Screen == EScreen::Ops && !Ops.IsPaused() && Warp > 0 && !(Scene.IsValid() && Scene->IsLaunching()))
	{
		// real time: 1x = one second per second; Warp multiplies
		if (Ops.Advance(Warp * Dt / 86400.0)) bDirty = true;
		if (Ops.IsFinished() && Scene.IsValid()) Scene->SetBitMood(Ops.Stars >= 2 ? 1 : 2);
	}
	if (Screen == EScreen::Ops && Scene.IsValid())
		Scene->SetMissionTime(Ops.Day, Ops.EndDay, Eval.PeriodMin, !Ops.bMissionLost);
	if (bDirty) Rebuild();

	// Dev: automatic screenshot then quit (used to review screens without a human)
	if (!DevShot.IsEmpty())
	{
		DevShotT += Dt;
		if (!bDevShotTaken && DevShotT > DevShotDelay)
		{
			FScreenshotRequest::RequestScreenshot(DevShot, true, false);
			bDevShotTaken = true;
		}
		if (bDevShotTaken && DevShotT > DevShotDelay + 3.f && PC.IsValid()) PC->ConsoleCommand(TEXT("quit"));
	}
}

// =====================================================================================================
//  helpers
// =====================================================================================================
TSharedRef<STextBlock> SISGameUI::Txt(const FString& S, int32 Size, const FLinearColor& C, bool bBold)
{
	return SNew(STextBlock)
		.Text(FText::FromString(S))
		.Font(ISFont(bBold ? "Bold" : "Regular", Size))
		.ColorAndOpacity(C)
		.AutoWrapText(true);
}

TSharedRef<SWidget> SISGameUI::Btn(const FString& Label, TFunction<void()> OnClick, int32 Kind, int32 Size, bool bEnabled)
{
	const FButtonStyle* St = !bEnabled ? &StyleDisabled : Kind == 1 ? &StylePrimary : Kind == 2 ? &StyleSelected : &StyleNormal;
	return SNew(SButton)
		.ButtonStyle(St)
		.IsEnabled(bEnabled)
		.OnClicked_Lambda([OnClick]() { if (OnClick) OnClick(); return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.Font(ISFont(Kind == 1 ? "Bold" : "Regular", Size))
			.ColorAndOpacity(bEnabled ? CText : CDim)
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SISGameUI::Panel(TSharedRef<SWidget> Content, float Pad, int32 Kind)
{
	return SNew(SBorder)
		.BorderImage(&PanelBrush)
		.BorderBackgroundColor(Kind == 1 ? CPanel2 : CPanel)
		.Padding(Pad)
		[
			Content
		];
}

FLinearColor SISGameUI::StatusColor(EISStatus S) const
{
	switch (S)
	{
	case EISStatus::Ok: return COk;
	case EISStatus::Warn: return CWarn;
	case EISStatus::Fail: return CFail;
	default: return CDim;
	}
}

const FISMission& SISGameUI::Mission() const
{
	const TArray<FISMission>& M = ISData::Missions();
	return M[FMath::Clamp(MissionIdx, 0, M.Num() - 1)];
}

bool SISGameUI::IsUnlocked(const FISPart& P) const
{
	if (bJudgeMode || Mission().Id == "sandbox") return true;
	return P.Unlock <= FMath::Max(Mission().Chapter, MaxChapter);
}

// =====================================================================================================
//  state changes
// =====================================================================================================
void SISGameUI::Go(EScreen S)
{
	Screen = S;
	bDirty = true;
	UpdateScene();
}

AISPlayerController* SISGameUI::IPC() const { return Cast<AISPlayerController>(PC.Get()); }
AISHomeWorld* SISGameUI::HW() const { AISPlayerController* P = IPC(); return P ? P->World : nullptr; }
bool SISGameUI::InWorld() const { AISPlayerController* P = IPC(); return P && P->GetMode() != EISMode::Menu; }
void SISGameUI::ShowOrders() { Screen = EScreen::Missions; bDirty = true; }
void SISGameUI::ShowPause() { Screen = EScreen::Pause; bDirty = true; }
void SISGameUI::ShowTitle() { Go(EScreen::Title); }
void SISGameUI::ShowCanSatDebrief()
{
	if (AISHomeWorld* W = HW()) MissionIdx = W->MissionIdx;
	QuizAnswers.Init(-1, Mission().Quiz.Num());
	Screen = EScreen::CanDebrief;
	bDirty = true;
}

void SISGameUI::UpdateScene()
{
	if (!Scene.IsValid() || InWorld()) return;
	switch (Screen)
	{
	case EScreen::Title: case EScreen::Missions: case EScreen::Help: Scene->ShowTitle(); Scene->SetBitMood(0); break;
	case EScreen::Design: case EScreen::Launch: Scene->ShowWorkshop(Design); Scene->SetBitMood(Eval.bCanLaunch ? 1 : 2); break;
	case EScreen::Ops:
		Scene->ShowOrbit(Eval.Dest, Eval.AltKm, Mission().Id == "sandbox" ? 51.6f : Mission().IncDeg, Design, Mission().Id == "m4" ? Design.Fleet : 1);
		Scene->SetBitMood(0);
		break;
	case EScreen::Debrief: Scene->ShowCompare(Design, Mission().RealMesh, Mission().RealName); break;
	case EScreen::Play: case EScreen::Bench: case EScreen::CanDebrief: case EScreen::Pause: break;
	case EScreen::Cosmos: Scene->ShowCosmos(Design); Scene->SetBitMood(1); break;
	case EScreen::Pedia:
		Scene->ShowModelOnly(MeshOf(Pedia[FMath::Clamp(PediaSel, 0, PediaCount - 1)].Mesh), Pedia[FMath::Clamp(PediaSel, 0, PediaCount - 1)].Name);
		break;
	}
}

void SISGameUI::Recompute(bool bPoke)
{
	Eval = ISSim::Evaluate(Design, Mission());
	if (Scene.IsValid() && (Screen == EScreen::Design || Screen == EScreen::Launch))
	{
		Scene->ShowWorkshop(Design);
		Scene->SetBitMood(Eval.bCanLaunch ? 1 : 2);
		if (bPoke) Scene->PokeBit();
	}
	bDirty = true;
}

void SISGameUI::StarterDesign(const FISMission& M)
{
	// Deliberately imperfect starting points: the player fixes them by reading the budgets.
	auto Set = [this](std::initializer_list<const char*> Ids, const char* LV)
	{
		for (const char* Id : Ids) Design.Parts.Add(FName(Id));
		Design.Vehicle = FName(LV);
	};
	Design = FISDesign();
	const FName Id = M.Id;
	if (Id == "m0") Set({ "bus_can", "pwr_none", "bat_aa", "adcs_none", "com_433", "prop_none", "cpu_ard", "thm_passive", "gnd_yard" }, "lv_hobby");
	else if (Id == "m1") Set({ "bus_med", "pwr_s", "bat_s", "adcs_mtq", "com_s", "prop_none", "cpu_cube", "thm_passive", "gnd_own", "pl_clock" }, "lv_f9");
	else if (Id == "m2") Set({ "bus_small", "pwr_s", "bat_s", "adcs_mtq", "com_s", "prop_none", "cpu_cube", "thm_passive", "gnd_own", "pl_mss" }, "lv_electron");
	else if (Id == "m3") Set({ "bus_geo", "pwr_m", "bat_m", "adcs_rw", "com_x", "prop_hyd_l", "cpu_rh", "thm_heat", "gnd_own", "pl_wx" }, "lv_a6");
	else if (Id == "m4") Set({ "bus_small", "pwr_s", "bat_s", "adcs_rw", "com_ka", "prop_none", "cpu_cube", "thm_passive", "gnd_net", "pl_xpdr" }, "lv_f9");
	else if (Id == "m5") Set({ "bus_med", "pwr_m", "bat_m", "adcs_rw", "com_x", "prop_hyd_l", "cpu_rh", "thm_heat", "gnd_dsn", "pl_hires" }, "lv_f9");
	else if (Id == "m6") Set({ "bus_deep", "pwr_m", "bat_m", "adcs_spin", "com_hga", "prop_biprop", "cpu_rh", "thm_heat", "gnd_dsn", "pl_fields", "pl_mwr" }, "lv_fh");
	else if (Id == "m7") Set({ "bus_deep", "pwr_mmrtg", "bat_s", "adcs_spin", "com_hga", "prop_hyd_s", "cpu_rh", "thm_heat", "gnd_dsn", "pl_fields" }, "lv_fh");
	else Set({ "bus_small", "pwr_s", "bat_s", "adcs_rw", "com_s", "prop_hyd_s", "cpu_cube", "thm_passive", "gnd_net" }, "lv_f9");
	Design.Fleet = FMath::Max(1, M.Fleet);
	Design.Dest = M.Dest;
	Design.AltKm = M.AltKm;
}

void SISGameUI::StartMission(int32 Index)
{
	MissionIdx = Index;
	StarterDesign(Mission());
	Cat = EISCat::Bus;
	InfoPart = NAME_None;
	InfoLine.Reset();
	PartsScroll = BudgetScroll = 0.f;
	Eval = ISSim::Evaluate(Design, Mission());
	ActivePlayer = 0;
	Ready.Init(false, TeamSize);
	Go(EScreen::Design);
}

void SISGameUI::StartOps()
{
	Eval = ISSim::Evaluate(Design, Mission());
	Ops.Start(Design, Mission(), Eval, RunSeed++);
	bAckFinish = false; bRecorded = false; LastReportPath.Reset();
	Warp = 100.f;
	Warp = 1.f;
	QuizAnswers.Init(-1, Mission().Quiz.Num());
	Go(EScreen::Ops);
	if (Scene.IsValid())
	{
		// orbital launchers fly for real (ISRocket physics); the hobby rocket and balloon keep the short scripted launch
		const float Inc = Mission().Id == "sandbox" ? 51.6f : Mission().IncDeg;
		const bool bFlying = Scene->StartFlight(Design.Vehicle, Eval.TotalKg, Eval.AltKm, Inc);
		float AutoT = -1.f;
		if (bFlying && FParse::Value(FCommandLine::Get(), TEXT("ISAutoLaunch="), AutoT) && Scene->Flight()) Scene->Flight()->StartCountdown(AutoT);   // dev: screenshots mid-flight
		if (!bFlying)
		{
			const FISVehicle* V = ISData::FindVehicle(Design.Vehicle);
			Scene->StartLaunch(V && V->CostM >= 100.f);
		}
	}
}

// =====================================================================================================
//  rebuild
// =====================================================================================================
void SISGameUI::Rebuild()
{
	bDirty = false;
	if (PartsBox.IsValid()) PartsScroll = PartsBox->GetScrollOffset();
	if (CosmosBox.IsValid()) CosmosScroll = CosmosBox->GetScrollOffset();
	CosmosBox.Reset();
	if (BudgetBox.IsValid()) BudgetScroll = BudgetBox->GetScrollOffset();
	PartsBox.Reset(); BudgetBox.Reset();

	TSharedRef<SWidget> W = SNullWidget::NullWidget;
	switch (Screen)
	{
	case EScreen::Title: W = BuildTitle(); break;
	case EScreen::Missions: W = BuildMissions(); break;
	case EScreen::Design: W = BuildDesign(); break;
	case EScreen::Launch: W = BuildLaunch(); break;
	case EScreen::Ops: W = BuildOps(); break;
	case EScreen::Debrief: W = BuildDebrief(); break;
	case EScreen::Pedia: W = BuildPedia(); break;
	case EScreen::Cosmos: W = BuildCosmos(); break;
	case EScreen::Help: W = BuildHelp(); break;
	case EScreen::Play: W = BuildPlay(); break;
	case EScreen::Bench: W = BuildBench(); break;
	case EScreen::CanDebrief: W = BuildCanDebrief(); break;
	case EScreen::Pause: W = BuildPause(); break;
	}
	Root->SetContent(W);
	if (PartsBox.IsValid()) PartsBox->SetScrollOffset(PartsScroll);
	if (CosmosBox.IsValid()) CosmosBox->SetScrollOffset(CosmosScroll);
	if (BudgetBox.IsValid()) BudgetBox->SetScrollOffset(BudgetScroll);
}

TSharedRef<SWidget> SISGameUI::BuildTopBar(const FString& Title)
{
	return Panel(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)[Txt(TEXT("INFINISAT"), 18, CAccent, true)]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Txt(Title, 14, CText, true)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[Btn(TEXT("Sat-o-pedia"), [this]() { Go(EScreen::Pedia); })]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[Btn(TEXT("How to play"), [this]() { Go(EScreen::Help); })]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[InWorld() ? Btn(TEXT("Back to the world"), [this]() { if (IPC()) IPC()->ClosePanel(); }, 1) : SNullWidget::NullWidget]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[Btn(TEXT("Main menu"), [this]() { if (IPC() && InWorld()) IPC()->EnterMenu(); Go(EScreen::Title); })],
		10.f);
}

TSharedRef<SWidget> SISGameUI::BuildBitTip(const FString& Text)
{
	return Panel(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
		[
			SNew(SBox).WidthOverride(90).HeightOverride(56)[SNew(SImage).Image(&BitBrush)]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("BIT says:"), 12, CAccent, true)]
			+ SVerticalBox::Slot().AutoHeight()[Txt(Text, 13, CText)]
		], 10.f, 1);
}

// ----------------------------------------------------------------------------------------------- TITLE
TSharedRef<SWidget> SISGameUI::BuildTitle()
{
	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(60, 40)
		[
			SNew(SBox).WidthOverride(560)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[SNew(SImage).Image(&LogoBrush)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)[Txt(TEXT("INFINISAT"), 44, CAccent, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)[Txt(TEXT("Design it. Launch it. Fly it. Learn why it works.\nFrom a back-yard CanSat to interstellar space."), 15, CText)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Start campaign (enter the world)"), [this]() { if (IPC()) IPC()->EnterWorld(); }, 1, 16)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Order board (jump to any chapter)"), [this]() { Go(EScreen::Missions); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Sandbox: design anything"), [this]() { StartMission(ISData::Missions().Num() - 1); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Cosmos explorer: fly the universe"), [this]() { Go(EScreen::Cosmos); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Sat-o-pedia: real spacecraft"), [this]() { Go(EScreen::Pedia); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("How to play"), [this]() { Go(EScreen::Help); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Settings (graphics, mouse)"), [this]() { Screen = EScreen::Pause; bDirty = true; }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Quit"), [this]() { if (PC.IsValid()) PC->ConsoleCommand(TEXT("quit")); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bJudgeMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bJudgeMode = S == ECheckBoxState::Checked; bDirty = true; })
						[Txt(TEXT("Judge / teacher mode: all chapters and parts unlocked"), 12, CText)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
					[Txt(TEXT("NASA Space Apps 2026 - Space Mission Design Game.  3D models & planet textures: NASA 3D Resources (public domain). Data: NASA, USGS, NASA Earthdata, planet4589.org. Hackathon prototype, not affiliated with NASA."), 12, CDim)],
					24.f)
			]
		];
}

// ----------------------------------------------------------------------------------------------- MISSIONS
TSharedRef<SWidget> SISGameUI::BuildMissions()
{
	const TArray<FISMission>& Ms = ISData::Missions();
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < Ms.Num(); ++i)
	{
		const FISMission& M = Ms[i];
		const bool bOpen = bJudgeMode || M.Chapter <= MaxChapter || M.Id == "sandbox";
		FString Stars;
		for (int32 s = 0; s < 3; ++s) Stars += (BestStars.IsValidIndex(i) && BestStars[i] > s) ? TEXT("*") : TEXT("-");
		List->AddSlot().AutoHeight().Padding(0, 3)
		[
			Btn(FString::Printf(TEXT("%s   %s%s"), *M.Title, M.Id == "sandbox" ? TEXT("") : *Stars, bOpen ? TEXT("") : TEXT("   (locked)")),
				[this, i]() { MissionIdx = i; bDirty = true; }, i == MissionIdx ? 2 : 0, 13, bOpen)
		];
	}
	const FISMission& M = Mission();
	TSharedRef<SVerticalBox> Learn = SNew(SVerticalBox);
	for (const FString& L : M.Learn) Learn->AddSlot().AutoHeight().Padding(8, 1)[Txt(TEXT("- ") + L, 12, CText)];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("Order board: choose a mission"))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(420)[Panel(SNew(SScrollBox) + SScrollBox::Slot()[List])]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(560)
				[
					Panel(
						SNew(SScrollBox)
						+ SScrollBox::Slot()[Txt(M.Title, 20, CAccent, true)]
						+ SScrollBox::Slot().Padding(0, 4)[Txt(TEXT("Client: ") + M.Client, 12, CDim)]
						+ SScrollBox::Slot().Padding(0, 8)[Txt(M.Brief, 14, CText)]
						+ SScrollBox::Slot().Padding(0, 4)[Txt(FString::Printf(TEXT("Destination: %s   |   Budget: %s   |   Life: %s"),
							*ISData::DestName(M.Dest), M.Id == "sandbox" ? TEXT("unlimited") : *ISSim::FmtMoney(M.BudgetM), *ISSim::FmtDays(M.LifeYears * 365.25)), 12, CGold)]
						+ SScrollBox::Slot().Padding(0, 12, 0, 2)[Txt(TEXT("You will learn:"), 13, CAccent, true)]
						+ SScrollBox::Slot()[Learn]
						+ SScrollBox::Slot().Padding(0, 12, 0, 2)[Txt(M.RealName.IsEmpty() ? FString() : TEXT("Real-world reference: ") + M.RealName, 13, CAccent, true)]
						+ SScrollBox::Slot()[Txt(M.RealFacts, 12, CDim)]
						+ SScrollBox::Slot().Padding(0, 12, 0, 4)[Txt(TEXT("DIFFICULTY"), 13, CAccent, true)]
						+ SScrollBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("Easy"), [this]() { ISSim::Difficulty = 0; bDirty = true; }, ISSim::Difficulty == 0 ? 2 : 0, 13)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("Normal"), [this]() { ISSim::Difficulty = 1; bDirty = true; }, ISSim::Difficulty == 1 ? 2 : 0, 13)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("Hard"), [this]() { ISSim::Difficulty = 2; bDirty = true; }, ISSim::Difficulty == 2 ? 2 : 0, 13)]
						]
						+ SScrollBox::Slot()[Txt(ISSim::Difficulty == 0 ? TEXT("Easy: 50% more budget, relaxed pointing and data targets, fewer failures.") : ISSim::Difficulty == 2 ? TEXT("Hard: 20% less budget and twice as many component failures. Scores x1.25.") : TEXT("Normal: realistic budget and failure rates."), 12, CDim)]
						+ SScrollBox::Slot().Padding(0, 10, 0, 4)[Txt(TEXT("PLAYERS (same screen, take turns)"), 13, CAccent, true)]
						+ SScrollBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("Solo"), [this]() { TeamSize = 1; bDirty = true; }, TeamSize == 1 ? 2 : 0, 13)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("2 players"), [this]() { TeamSize = 2; bDirty = true; }, TeamSize == 2 ? 2 : 0, 13)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("3 players"), [this]() { TeamSize = 3; bDirty = true; }, TeamSize == 3 ? 2 : 0, 13)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("4 players"), [this]() { TeamSize = 4; bDirty = true; }, TeamSize == 4 ? 2 : 0, 13)]
						]
						+ SScrollBox::Slot()[Txt([this]() { FString S; const TArray<FISRole> R = Roles(); for (int32 i = 0; i < R.Num(); ++i) S += FString::Printf(TEXT("Player %d: %s  -  goal: %s\n"), i + 1, *R[i].Name, *R[i].Goal); return TeamSize > 1 ? S : FString(TEXT("Solo: you make every decision.")); }(), 12, CDim)]
						+ SScrollBox::Slot().Padding(0, 6)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Txt(TEXT("Team / player name:"), 13, CText)]
							+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SEditableTextBox).Text(FText::FromString(TeamName)).OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type) { TeamName = T.ToString(); }).OnTextChanged_Lambda([this](const FText& T) { TeamName = T.ToString(); })]
						]
						+ SScrollBox::Slot().Padding(0, 14, 0, 0)[Btn(TEXT("Accept order  >"), [this]() {
							if (Mission().Id == "m0" && HW() && IPC()) { HW()->StartCanSatOrder(MissionIdx); IPC()->EnterWorld(); }
							else { if (IPC() && InWorld()) IPC()->EnterMenu(); StartMission(MissionIdx); }
						}, 1, 16)]
						+ SScrollBox::Slot().Padding(0, 6)[Txt(M.Id == "m0" ? TEXT("Hands-on: you build it on the garage workbench, launch it in the field and track it down.") : TEXT("Engineering view: design in the satellite workshop (clean room + spaceport scenes are next in development)."), 12, CDim)],
						18.f)
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- DESIGN
TSharedRef<SWidget> SISGameUI::BuildBudgets(bool bCompact)
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (const FISLine& L : Eval.Lines)
	{
		const bool bSel = InfoLine == L.Label;
		const FString Label = L.Label;
		Rows->AddSlot().AutoHeight().Padding(0, 2)
		[
			SNew(SButton)
			.ButtonStyle(bSel ? &StyleSelected : &StyleNormal)
			.OnClicked_Lambda([this, Label]() { InfoLine = (InfoLine == Label) ? FString() : Label; bDirty = true; return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Txt(L.Status == EISStatus::Ok ? TEXT("OK") : L.Status == EISStatus::Fail ? TEXT("X") : L.Status == EISStatus::Warn ? TEXT("!") : TEXT("i"), 12, StatusColor(L.Status), true)]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(L.Label)).Font(ISFont("Regular", 12)).ColorAndOpacity(CText)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6, 0)[SNew(STextBlock).Text(FText::FromString(L.Value)).Font(ISFont("Bold", 12)).ColorAndOpacity(StatusColor(L.Status))]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(L.Limit.IsEmpty() ? FString() : TEXT("/ ") + L.Limit)).Font(ISFont("Regular", 12)).ColorAndOpacity(CDim)]
			]
		];
		if (bSel)
			Rows->AddSlot().AutoHeight().Padding(6, 2, 0, 6)[Panel(Txt(TEXT("WHY?  ") + L.Why, 12, CText), 10.f, 1)];
	}
	return Rows;
}

TSharedRef<SWidget> SISGameUI::BuildObjectives(const TArray<FISObjective>& Objs)
{
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	for (const FISObjective& O : Objs)
	{
		V->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[Txt(O.bMet ? TEXT("[x]") : TEXT("[ ]"), 12, O.bMet ? COk : (O.bBonus ? CDim : CFail), true)]
			+ SHorizontalBox::Slot().FillWidth(1.f)[Txt(O.Text + (O.bMet ? FString() : TEXT("  - ") + O.Why), 12, O.bBonus ? CGold : CText)]
		];
	}
	return V;
}

TSharedRef<SWidget> SISGameUI::BuildDesign()
{
	const FISMission& M = Mission();
	const bool bMulti = Cat == EISCat::Payload || Cat == EISCat::Extra;

	// category tabs
	TSharedRef<SWrapBox> Tabs = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	for (int32 c = 0; c < (int32)EISCat::Count; ++c)
	{
		const EISCat C = (EISCat)c;
		bool bHas = false;
		for (const FName& Id : Design.Parts) if (const FISPart* P = ISData::FindPart(Id)) if (P->Cat == C) bHas = true;
		const bool bNeeded = !(C == EISCat::Payload || C == EISCat::Extra);
		Tabs->AddSlot()[Btn(ISData::CatName(C) + ((bNeeded && !bHas) ? TEXT(" (!)") : TEXT("")), [this, C]() { Cat = C; InfoPart = NAME_None; PartsScroll = 0; bDirty = true; }, C == Cat ? 2 : 0, 11)];
	}

	// parts list
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const FISPart& P : ISData::Parts())
	{
		if (P.Cat != Cat) continue;
		const bool bSel = Design.Has(P.Id);
		const bool bOpen = IsUnlocked(P) && OwnsCat(P.Cat);
		FString Stats = ISSim::FmtMass(P.MassKg) + TEXT("  |  ") + ISSim::FmtMoney(P.CostM);
		if (P.PowerW > 0) Stats += FString::Printf(TEXT("  |  uses %s W"), *ISSim::Fmt(P.PowerW, 1));
		if (P.SolarW > 0) Stats += FString::Printf(TEXT("  |  +%s W"), *ISSim::Fmt(P.SolarW, 0));
		if (P.RtgW > 0) Stats += FString::Printf(TEXT("  |  +%s W nuclear"), *ISSim::Fmt(P.RtgW, 0));
		if (P.BatteryWh > 0) Stats += FString::Printf(TEXT("  |  %s Wh"), *ISSim::Fmt(P.BatteryWh, 0));
		if (P.MaxTotalKg > 0) Stats += FString::Printf(TEXT("  |  max %s"), *ISSim::FmtMass(P.MaxTotalKg));
		if (P.PointingDeg > 0) Stats += FString::Printf(TEXT("  |  %s deg"), *ISSim::Fmt(P.PointingDeg, 2));
		if (P.RadioMbps > 0) Stats += FString::Printf(TEXT("  |  %s Mbps"), *ISSim::Fmt(P.RadioMbps, 3));
		if (P.PropKg > 0) Stats += FString::Printf(TEXT("  |  %s fuel, Isp %s s"), *ISSim::FmtMass(P.PropKg), *ISSim::Fmt(P.IspS, 0));
		if (P.SailM2 > 0) Stats += FString::Printf(TEXT("  |  %s m^2"), *ISSim::Fmt(P.SailM2, 2));
		if (P.ResolutionM > 0) Stats += FString::Printf(TEXT("  |  %s m @ %s km"), *ISSim::Fmt(P.ResolutionM, 1), *ISSim::Fmt(P.RefAltKm, 0));
		if (P.DataGbDay > 0) Stats += FString::Printf(TEXT("  |  %s Gb/day"), *ISSim::Fmt(P.DataGbDay, 3));
		if (P.RadTolKrad > 0) Stats += FString::Printf(TEXT("  |  %s krad"), *ISSim::Fmt(P.RadTolKrad, 0));
		const FName Id = P.Id;
		const EISCat C = P.Cat;
		List->AddSlot().AutoHeight().Padding(0, 2)
		[
			SNew(SButton)
			.ButtonStyle(!bOpen ? &StyleDisabled : bSel ? &StyleSelected : &StyleNormal)
			.IsEnabled(bOpen)
			.OnClicked_Lambda([this, Id, C, bMulti]()
			{
				if (bMulti) Design.Toggle(Id); else Design.SetSingle(C, Id);
				InfoPart = Id;
				Recompute(true);
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Txt((bSel ? TEXT("> ") : TEXT("")) + P.Name + (bOpen ? FString() : (!OwnsCat(P.Cat) ? FString(TEXT("  (another player decides)")) : FString::Printf(TEXT("  (unlocks Ch.%d)"), P.Unlock))), 12, CText, bSel)]
				+ SVerticalBox::Slot().AutoHeight()[Txt(Stats, 12, CDim)]
			]
		];
	}

	const FISPart* Info = ISData::FindPart(InfoPart);
	const FString InfoText = Info ? FString::Printf(TEXT("%s\n%s"), *Info->Name, *Info->Edu) : ISData::CatHelp(Cat) + (bMulti ? TEXT("\n(You can pick several.)") : TEXT("\n(Pick one.)"));

	// right column extras: sandbox destination / fleet
	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
	Right->AddSlot().AutoHeight()[Txt(TEXT("BUDGETS  (click a line: WHY?)"), 13, CAccent, true)];
	if (M.Id == "sandbox")
	{
		TSharedRef<SWrapBox> Dests = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(3, 3));
		for (int32 d = 0; d < (int32)EISDest::Count; ++d)
		{
			const EISDest Dd = (EISDest)d;
			Dests->AddSlot()[Btn(ISData::DestName(Dd), [this, Dd]() { Design.Dest = Dd; Recompute(); }, Design.Dest == Dd ? 2 : 0, 10)];
		}
		Right->AddSlot().AutoHeight().Padding(0, 4)[Dests];
	}
	if (M.Id == "m4")
	{
		Right->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Txt(FString::Printf(TEXT("Fleet size: %d satellites"), Design.Fleet), 13, CGold, true)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("-10"), [this]() { Design.Fleet = FMath::Max(1, Design.Fleet - 10); Recompute(); })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)[Btn(TEXT("+10"), [this]() { Design.Fleet = FMath::Min(1000, Design.Fleet + 10); Recompute(); })]
		];
	}
	Right->AddSlot().FillHeight(1.f)
	[
		SAssignNew(BudgetBox, SScrollBox)
		+ SScrollBox::Slot()[BuildBudgets(false)]
		+ SScrollBox::Slot().Padding(0, 10, 0, 2)[Txt(M.Id == "sandbox" ? FString() : TEXT("CLIENT OBJECTIVES (predicted)"), 13, CAccent, true)]
		+ SScrollBox::Slot()[BuildObjectives(Eval.Objectives)]
	];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("DESIGN  -  ") + M.Title)]
		+ SVerticalBox::Slot().AutoHeight().Padding(12, 0, 12, 8)[BuildTeamBar()]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(440)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Tabs]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)[Txt(ISData::CatName(Cat), 15, CAccent, true)]
						+ SVerticalBox::Slot().FillHeight(1.f)[SAssignNew(PartsBox, SScrollBox) + SScrollBox::Slot()[List]]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
						[
							SNew(SBox).MaxDesiredHeight(190)
							[
								Panel(SNew(SScrollBox) + SScrollBox::Slot()[Txt(InfoText, 12, CText)], 10.f, 1)
							]
						])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(480)[Panel(Right)]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[BuildBitTip(Eval.Hint)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
			[
				SNew(SBox).WidthOverride(240)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Mission briefing"), [this]() { Go(EScreen::Missions); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Copy design code (share)"), [this]() { FPlatformApplicationMisc::ClipboardCopy(*DesignCode()); Notice = TEXT("Design code copied: paste it to a teammate or another team to compare."); bDirty = true; }, 0, 12)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Paste design code"), [this]() { FString C; FPlatformApplicationMisc::ClipboardPaste(C); Notice = ApplyDesignCode(C) ? TEXT("Design loaded from the clipboard.") : TEXT("The clipboard does not contain a design code for this mission."); Recompute(); }, 0, 12)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Next: choose rocket  >"), [this]() { Go(EScreen::Launch); }, 1, 14)]
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- LAUNCH
TSharedRef<SWidget> SISGameUI::BuildLaunch()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	const EISDest Dest = Eval.Dest;
	const bool bSail = Design.HasTag(ISTag::SolarSail);
	for (const FISVehicle& V : ISData::Vehicles())
	{
		float Cap = V.Capacity[(int32)Dest];
		if (Dest == EISDest::Interstellar && bSail) Cap = V.Capacity[(int32)EISDest::Moon];
		const bool bFits = Cap > 0 && Eval.TotalKg <= Cap;
		const FName Id = V.Id;
		const FString Price = V.CostPerKgM > 0 ? FString::Printf(TEXT("%s per kg (min %s)"), *ISSim::FmtMoney(V.CostPerKgM), *ISSim::FmtMoney(V.CostM)) : ISSim::FmtMoney(V.CostM);
		List->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(SButton)
			.ButtonStyle(Design.Vehicle == Id ? &StyleSelected : &StyleNormal)
			.IsEnabled(OwnsLaunch())
			.OnClicked_Lambda([this, Id]() { Design.Vehicle = Id; Recompute(); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Txt(V.Name, 13, CText, true)]
				+ SVerticalBox::Slot().AutoHeight()[Txt(FString::Printf(TEXT("%s  |  %s  |  success %s%%"), *V.Site, *Price, *ISSim::Fmt(V.Reliability * 100, 0)), 12, CDim)]
				+ SVerticalBox::Slot().AutoHeight()[Txt(Cap > 0 ? FString::Printf(TEXT("Can lift %s to %s  ->  %s"), *ISSim::FmtMass(Cap), *ISData::DestName(Dest), bFits ? TEXT("FITS") : TEXT("too heavy")) : FString::Printf(TEXT("Cannot reach %s"), *ISData::DestName(Dest)), 12, bFits ? COk : CFail)]
			]
		];
	}
	const FISVehicle* LV = ISData::FindVehicle(Design.Vehicle);

	TSharedRef<SVerticalBox> Block = SNew(SVerticalBox);
	for (const FString& B : Eval.Blockers) Block->AddSlot().AutoHeight()[Txt(TEXT("X  ") + B, 12, CFail)];
	if (Eval.Blockers.Num() == 0)
	{
		int32 Fails = 0; for (const FISLine& L : Eval.Lines) if (L.Status == EISStatus::Fail) ++Fails;
		Block->AddSlot().AutoHeight()[Txt(Fails ? FString::Printf(TEXT("Ready to launch, but %d budget(s) still fail. You may launch anyway and see what happens: failing is a great teacher!"), Fails) : FString(TEXT("All systems go!")), 13, Fails ? CWarn : COk, true)];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("LAUNCH  -  choose your ride to ") + ISData::DestName(Dest))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(470)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(FString::Printf(TEXT("Your spacecraft: %s"), *ISSim::FmtMass(Eval.TotalKg)), 15, CAccent, true)]
						+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 6)[SNew(SScrollBox) + SScrollBox::Slot()[List]]
						+ SVerticalBox::Slot().AutoHeight()[Panel(Txt(LV ? LV->Edu : FString(TEXT("Pick a launcher.")), 12, CText), 10.f, 1)])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(480)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.f)[SAssignNew(BudgetBox, SScrollBox) + SScrollBox::Slot()[BuildBudgets(true)]]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[Block]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("<  Back to design"), [this]() { Go(EScreen::Design); })]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[BuildTeamBar()]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("LAUNCH!"), [this]() { StartOps(); }, 1, 20, Eval.bCanLaunch && (TeamSize <= 1 || !Ready.Contains(false)))])
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- OPERATIONS
TSharedRef<SWidget> SISGameUI::BuildOps()
{
	const FISMission& M = Mission();
	TSharedRef<SVerticalBox> Log = SNew(SVerticalBox);
	for (int32 i = Ops.Log.Num() - 1; i >= 0; --i)
	{
		const FISEvent& E = Ops.Log[i];
		const FLinearColor C = E.Type == EISEvt::Good ? COk : E.Type == EISEvt::Warn ? CWarn : (E.Type == EISEvt::Fatal || E.Type == EISEvt::Anomaly) ? CFail : E.Type == EISEvt::End ? CGold : CText;
		Log->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Txt(FString::Printf(TEXT("T+%s   %s"), *ISSim::FmtDays(E.Day), *E.Text), 12, C, E.Type != EISEvt::Info)]
			+ SVerticalBox::Slot().AutoHeight().Padding(14, 0, 0, 0)[Txt(E.Edu, 12, CDim)]
		];
	}

	auto WarpBtn = [this](const TCHAR* L, float W) { return Btn(L, [this, W]() { Warp = W; bDirty = true; }, Warp == W ? 2 : 0, 12); };

	TSharedRef<SOverlay> O = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("MISSION CONTROL  -  ") + M.Title)]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(380)
					[
						Panel(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("MISSION CLOCK"), 12, CAccent, true)]
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(ISFont("Bold", 22)).ColorAndOpacity(CText)
								.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("T+ %s"), *ISSim::FmtDays(Ops.Day))); })]
							+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("of ") + ISSim::FmtDays(Ops.EndDay) + TEXT(" planned"), 12, CDim)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(1)[WarpBtn(TEXT("||"), 0.f)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(1)[WarpBtn(TEXT("1x"), 1.f)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(1)[WarpBtn(TEXT("100x"), 100.f)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(1)[WarpBtn(TEXT("10k"), 1.0e4f)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(1)[WarpBtn(TEXT("1M"), 1.0e6f)]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								Btn(TEXT("Warp to next event  >>"), [this]() { const double Nx = Ops.NextEventDay(); if (Nx > Ops.Day) { Ops.Advance(Nx - Ops.Day + 1e-6); bDirty = true; } }, 0, 12)
							]
							+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("Time runs in real time at 1x (like KSP). One orbit of a low satellite takes ~95 minutes: try 100x."), 12, CDim)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[Txt(TEXT("Battery"), 12, CDim)]
							+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(14)[SNew(SProgressBar).Percent_Lambda([this]() { return TOptional<float>((float)Ops.Battery); }).FillColorAndOpacity(COk)]]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[Txt(TEXT("Spacecraft health"), 12, CDim)]
							+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(14)[SNew(SProgressBar).Percent_Lambda([this]() { return TOptional<float>(Ops.bMissionLost ? 0.f : (float)Ops.Health); }).FillColorAndOpacity(CAccent)]]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[Txt(TEXT("Mission progress"), 12, CDim)]
							+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(14)[SNew(SProgressBar).Percent_Lambda([this]() { return TOptional<float>((float)(Ops.Day / FMath::Max(1e-9, Ops.EndDay))); }).FillColorAndOpacity(CGold)]]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
							[SNew(STextBlock).Font(ISFont("Regular", 12)).ColorAndOpacity(CText).AutoWrapText(true)
								.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Data downloaded: %s Gb\nLink: %s kbps  |  contact %s min/day\nPower avg: %s W for %s W load"),
									*ISSim::Fmt(Ops.DataGb, 2), *ISSim::Fmt(Eval.LinkKbps, 1), *ISSim::Fmt(Eval.ContactMin, 0), *ISSim::Fmt(Eval.AvgGenW, 0), *ISSim::Fmt(Eval.LoadW, 0))); })]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 2)[Txt(TEXT("Tip: drag with the mouse to rotate, wheel to zoom. The satellite is drawn much bigger than real size so you can see it."), 12, CDim)]
							+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SSpacer)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Toggle follow camera / orbit map"), [this]() { if (Scene.IsValid()) Scene->ToggleMap(); }, 0, 13)]
							+ SVerticalBox::Slot().AutoHeight()[Ops.IsFinished() ? Btn(TEXT("Debrief  >"), [this]() { Go(EScreen::Debrief); }, 1, 16) : Btn(TEXT("Abort and redesign"), [this]() { Go(EScreen::Design); })])
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(500)
					[
						Panel(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("EVENT LOG (newest first)"), 12, CAccent, true)]
							+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[Log]])
					]
				]
			]
		];

	// launch commentary (top centre) - only for the old scripted launch (non-orbital vehicles)
	O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 90, 0, 0)
	[
		SNew(SBorder).BorderImage(&PanelBrush).BorderBackgroundColor(CPanel2).Padding(14)
		.Visibility_Lambda([this]() { return Scene.IsValid() && Scene->IsLaunching() && !Scene->IsFlightActive() ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(ISFont("Bold", 20)).ColorAndOpacity(CGold).Text_Lambda([this]() { return FText::FromString(Scene.IsValid() ? Scene->LaunchCaption() : FString()); })]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8, 0, 0)[Btn(TEXT("Skip launch"), [this]() { if (Scene.IsValid()) Scene->SkipLaunch(); }, 0, 12)]
		]
	];
	// FLIGHT HUD: you fly the real launch (ISRocket physics). Keys: SPACE launch/stage, T autopilot, SHIFT/CTRL throttle,
	// A/D pitch, X cut engines, comma/period time warp (slow motion down to 0.05x)
	{
		TWeakObjectPtr<AISSpaceScene> WS = Scene;
		auto Val = [WS](TFunction<FString(const FISRocketSim&)> F)
		{
			return TAttribute<FText>::CreateLambda([WS, F]() { const FISRocketSim* S = WS.IsValid() ? WS->Flight() : nullptr; return FText::FromString(S ? F(*S) : FString()); });
		};
		auto Cell = [this, Val](const TCHAR* Label, TFunction<FString(const FISRocketSim&)> F) -> TSharedRef<SWidget>
		{
			return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Txt(Label, 10, CDim)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(ISFont("Bold", 17)).ColorAndOpacity(CText).Text(Val(F))];
		};
		auto Clock = [](double T)
		{
			const double A = FMath::Abs(T);
			const int32 H = (int32)(A / 3600.0), M = (int32)(FMath::Fmod(A, 3600.0) / 60.0);
			return FString::Printf(TEXT("T%s%02d:%02d:%06.3f"), T < 0 ? TEXT("-") : TEXT("+"), H, M, FMath::Fmod(A, 60.0));
		};
		TSharedRef<SHorizontalBox> Warps = SNew(SHorizontalBox);
		for (float W : { 0.05f, 0.25f, 1.f, 2.f, 10.f, 50.f })
			Warps->AddSlot().AutoWidth().Padding(0, 0, 4, 0)[Btn(W < 1.f ? FString::Printf(TEXT("%.2gx"), W) : FString::Printf(TEXT("%.0fx"), W), [this, W]() { if (Scene.IsValid()) Scene->SetFlightWarp(W); }, 0, 11)];
		O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 14)
		[
			SNew(SBox).WidthOverride(980)
			.Visibility_Lambda([this]() { return Scene.IsValid() && Scene->IsFlightActive() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				Panel(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Font(ISFont("Bold", 22)).ColorAndOpacity(CGold).Text(Val([Clock](const FISRocketSim& S) { return Clock(S.T); }))]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(16, 0)[SNew(STextBlock).Font(ISFont("Bold", 13)).ColorAndOpacity(CAccent).Text(Val([](const FISRocketSim& S)
						{
							const TCHAR* Ph = S.Phase == EISFlightPhase::OnPad ? TEXT("ON THE PAD") : S.Phase == EISFlightPhase::Countdown ? TEXT("COUNTDOWN") : S.Phase == EISFlightPhase::Ascent ? TEXT("POWERED ASCENT")
								: S.Phase == EISFlightPhase::Coast ? TEXT("COASTING") : S.Phase == EISFlightPhase::Orbit ? TEXT("IN ORBIT") : TEXT("FAILED");
							return FString::Printf(TEXT("%s  |  %s  |  %s"), *S.L->Name.ToUpper(), *S.L->Stages[FMath::Clamp(S.StageIdx, 0, S.L->Stages.Num() - 1)].Name, Ph);
						}))]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Font(ISFont("Bold", 13)).ColorAndOpacity(CGold).Text(Val([](const FISRocketSim& S) { return S.bAutopilot ? FString(TEXT("AUTOPILOT")) : FString(TEXT("MANUAL CONTROL")); }))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 6)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("ALTITUDE"), [](const FISRocketSim& S) { return S.AltM < 10000 ? FString::Printf(TEXT("%.0f m"), S.AltM) : FString::Printf(TEXT("%.1f km"), S.AltM / 1000.0); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("SPEED  orbital / air"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.0f / %.0f m/s"), S.Speed, S.SurfSpeed); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("APOAPSIS x PERIAPSIS"), [](const FISRocketSim& S) { return S.ApoKm > 1e8 ? FString(TEXT("escape!")) : FString::Printf(TEXT("%.0f x %.0f km"), FMath::Max(S.ApoKm, 0.0), S.PeriKm); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("AIR LOAD q / MACH"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.1f kPa / %.1f"), S.QPa / 1000.0, S.Mach); })]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("THROTTLE / THRUST"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.0f%% / %.0f kN"), S.Throttle * 100.0, S.ThrustN / 1000.0); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("PITCH FROM VERTICAL / AoA"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.1f / %.1f deg"), S.PitchDeg, S.AoADeg); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("MASS / THRUST-TO-WEIGHT"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.1f t / %.2f"), S.MassKg / 1000.0, S.TWR); })]
						+ SHorizontalBox::Slot().FillWidth(1.f)[Cell(TEXT("DELTA-V LEFT (vacuum)"), [](const FISRocketSim& S) { return FString::Printf(TEXT("%.0f m/s"), S.DeltaVLeft()); })]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)[Btn(TEXT("LAUNCH / STAGE  (Space)"), [this]() { if (Scene.IsValid() && Scene->Flight()) Scene->Flight()->Stage(); }, 1, 12)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)[Btn(TEXT("Autopilot (T)"), [this]() { if (Scene.IsValid() && Scene->Flight()) Scene->Flight()->bAutopilot = !Scene->Flight()->bAutopilot; }, 0, 12)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)[Btn(TEXT("Skip to orbit"), [this]() { if (Scene.IsValid()) Scene->SkipLaunch(); }, 0, 12)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[Txt(TEXT("Time:"), 11, CDim)]
						+ SHorizontalBox::Slot().AutoWidth()[Warps]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[SNew(STextBlock).Font(ISFont("Regular", 12)).ColorAndOpacity(CText).AutoWrapText(true).Text(Val([Clock](const FISRocketSim& S)
					{
						FString Out;
						for (int32 i = FMath::Max(0, S.Events.Num() - 3); i < S.Events.Num(); ++i) Out += Clock(S.Events[i].T) + TEXT("  ") + S.Events[i].Text + TEXT("\n");
						if (S.Phase == EISFlightPhase::OnPad)
							Out += TEXT("Press SPACE (or LAUNCH) to start the 10 s countdown. The autopilot flies a real ascent; press T to fly it yourself:\nSHIFT / CTRL throttle, A / D pitch (lean east to build sideways speed), X cut engines, SPACE stage, comma / period time warp.");
						return Out;
					}))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
					[
						SNew(SHorizontalBox)
						.Visibility_Lambda([this]() { const FISRocketSim* S = Scene.IsValid() ? Scene->Flight() : nullptr; return S && S->Phase == EISFlightPhase::Failed ? EVisibility::Visible : EVisibility::Collapsed; })
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[Btn(TEXT("Retry launch"), [this]() { if (Scene.IsValid()) Scene->RetryFlight(); }, 1, 13)]
						+ SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("Back to the design"), [this]() { if (Scene.IsValid()) Scene->EndFlight(); Go(EScreen::Design); }, 0, 13)]
					], 14.f)
			]
		];
	}
	// big, unmissable end-of-mission panel (friend feedback: failures were easy to miss)
	if (Ops.IsFinished() && !bAckFinish)
	{
		FString Why;
		for (int32 i = Ops.Log.Num() - 1; i >= 0; --i)
			if (Ops.Log[i].Type == EISEvt::Fatal) { Why = Ops.Log[i].Text + TEXT("\n\n") + Ops.Log[i].Edu; break; }
		const bool bLost = Ops.bMissionLost || Ops.bLaunchFailed;
		O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(680)
			[
				Panel(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Txt(bLost ? TEXT("MISSION LOST") : TEXT("MISSION COMPLETE"), 26, bLost ? CFail : COk, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[Txt(FString::Printf(TEXT("After %s of a planned %s."), *ISSim::FmtDays(bLost ? FMath::Max(0.0, Ops.LostDay) : Ops.EndDay), *ISSim::FmtDays(Ops.EndDay)), 15, CText)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Txt(bLost ? (Why.IsEmpty() ? Ops.Debrief : Why) : Ops.Debrief, 15, CText)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[Btn(TEXT("See the debrief: what happened and how to fix it  >"), [this]() { bAckFinish = true; Go(EScreen::Debrief); }, 1, 16)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Read the event log first"), [this]() { bAckFinish = true; bDirty = true; }, 0, 13)],
					22.f, 1)
			]
		];
	}

	if (Ops.AwaitingChoice())
	{
		const FISEvent& E = Ops.PendingEvent();
		TSharedRef<SVerticalBox> Choices = SNew(SVerticalBox);
		for (int32 i = 0; i < Ops.Choices().Num(); ++i)
			Choices->AddSlot().AutoHeight().Padding(0, 4)[Btn(Ops.Choices()[i].Label, [this, i]() { Ops.Choose(i); bDirty = true; }, i == 0 ? 1 : 0, 14)];
		O->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(620)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("DECISION NEEDED"), 18, CWarn, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(E.Text, 15, CText, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(E.Edu, 12, CDim)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(TEXT("What do you do, flight director?"), 13, CAccent)]
					+ SVerticalBox::Slot().AutoHeight()[Choices],
					20.f, 1)
			]
		];
	}
	return O;
}

// ----------------------------------------------------------------------------------------------- DEBRIEF
TSharedRef<SWidget> SISGameUI::BuildDebrief()
{
	const FISMission& M = Mission();
	if (BestStars.IsValidIndex(MissionIdx) && Ops.IsFinished())
	{
		BestStars[MissionIdx] = FMath::Max(BestStars[MissionIdx], Ops.Stars);
		if (Ops.Stars >= 2) MaxChapter = FMath::Max(MaxChapter, M.Chapter + 1);
	}
	FString Stars;
	for (int32 s = 0; s < 3; ++s) Stars += s < Ops.Stars ? TEXT("* ") : TEXT("- ");

	// quiz
	TSharedRef<SVerticalBox> Quiz = SNew(SVerticalBox);
	for (int32 q = 0; q < M.Quiz.Num(); ++q)
	{
		const FISQuiz& Q = M.Quiz[q];
		Quiz->AddSlot().AutoHeight().Padding(0, 8, 0, 2)[Txt(FString::Printf(TEXT("Q%d. %s"), q + 1, *Q.Q), 13, CText, true)];
		TSharedRef<SWrapBox> Ans = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
		const int32 Picked = QuizAnswers.IsValidIndex(q) ? QuizAnswers[q] : -1;
		for (int32 a = 0; a < Q.A.Num(); ++a)
		{
			const int32 Kind = Picked < 0 ? 0 : (a == Q.Correct ? 1 : (a == Picked ? 2 : 0));
			Ans->AddSlot()[Btn(Q.A[a], [this, q, a]() { if (QuizAnswers.IsValidIndex(q) && QuizAnswers[q] < 0) { QuizAnswers[q] = a; bDirty = true; } }, Kind, 12)];
		}
		Quiz->AddSlot().AutoHeight()[Ans];
		if (Picked >= 0)
			Quiz->AddSlot().AutoHeight().Padding(0, 2)[Txt((Picked == Q.Correct ? TEXT("Correct! ") : TEXT("Not quite. ")) + Q.Why, 12, Picked == Q.Correct ? COk : CWarn)];
	}

	// score breakdown, team goals, leaderboard
	const FISScore Sc = ComputeScore();
	if (!bRecorded && Ops.IsFinished()) { bRecorded = true; RecordScore(Sc); }
	FString ScoreText = FString::Printf(TEXT("SCORE %d  (%s)\nMission success %d/50   Science value %d/20\nResource efficiency %d/15   Decisions %d/15"),
		FMath::RoundToInt(Sc.Total), *ISSim::DifficultyName(ISSim::Difficulty), FMath::RoundToInt(Sc.Success), FMath::RoundToInt(Sc.Science), FMath::RoundToInt(Sc.Efficiency), FMath::RoundToInt(Sc.Decisions));
	if (TeamSize > 1)
	{
		ScoreText += FString::Printf(TEXT("   Team coordination %d/20\n\nTEAM GOALS (competing priorities):"), FMath::RoundToInt(Sc.Team));
		for (const FISRole& R : Roles()) ScoreText += FString::Printf(TEXT("\n[%s] %s: %s"), RoleGoalMet(R.GoalId) ? TEXT("x") : TEXT(" "), *R.Name, *R.Goal);
	}
	FString Board = TEXT("LEADERBOARD (this mission)\n");
	{
		const TArray<FString> L = Leaderboard();
		for (int32 i = 0; i < L.Num(); ++i) Board += FString::Printf(TEXT("%d. %s\n"), i + 1, *L[i]);
	}

	// compare table
	const FString YourLife = Ops.bMissionLost ? ISSim::FmtDays(FMath::Max(0.0, Ops.LostDay)) : ISSim::FmtDays(Ops.EndDay);
	const FString Compare = FString::Printf(TEXT("YOUR DESIGN\nMass: %s   |   Power: %s W avg   |   Cost: %s\nOperated: %s   |   Data returned: %s Gb\n\nREAL MISSION: %s\n%s"),
		*ISSim::FmtMass(Eval.TotalKg), *ISSim::Fmt(Eval.AvgGenW, 0), *ISSim::FmtMoney(Eval.TotalCostM), *YourLife, *ISSim::Fmt(Ops.DataGb, 2),
		M.RealName.IsEmpty() ? TEXT("-") : *M.RealName, *M.RealFacts);

	const bool bHasNext = MissionIdx + 1 < ISData::Missions().Num() - 1;
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("DEBRIEF  -  ") + M.Title)]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(520)
				[
					Panel(
						SNew(SScrollBox)
						+ SScrollBox::Slot()[Txt(Stars + FString::Printf(TEXT("   %s%% of objectives"), *ISSim::Fmt(Ops.ScorePct, 0)), 26, CGold, true)]
						+ SScrollBox::Slot().Padding(0, 6)[Txt(Ops.Debrief, 14, CText)]
							+ SScrollBox::Slot().Padding(0, 6)[Panel(Txt(ScoreText, 14, CGold), 10.f, 1)]
						+ SScrollBox::Slot().Padding(0, 12, 0, 2)[Txt(TEXT("OBJECTIVES"), 13, CAccent, true)]
						+ SScrollBox::Slot()[BuildObjectives(Ops.FinalObjectives)]
						+ SScrollBox::Slot().Padding(0, 12, 0, 2)[Txt(TEXT("QUICK CHECK"), 13, CAccent, true)]
						+ SScrollBox::Slot()[Quiz])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(460)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("COMPARE WITH REALITY"), 13, CAccent, true)]
						+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 6)[SNew(SScrollBox) + SScrollBox::Slot()[Txt(Compare, 12, CText)] + SScrollBox::Slot().Padding(0, 10)[Txt(Board, 13, CText)]]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(LastReportPath.IsEmpty() ? TEXT("Save mission report") : TEXT("Report saved - open folder"), [this, Sc]() { if (LastReportPath.IsEmpty()) LastReportPath = WriteReport(Sc); FPlatformProcess::ExploreFolder(*FPaths::GetPath(LastReportPath)); bDirty = true; }, 0, 13)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(TEXT("Improve the design"), [this]() { Go(EScreen::Design); }, 0, 13)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(TEXT("Fly the same design again"), [this]() { StartOps(); }, 0, 13)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(bHasNext ? TEXT("Next chapter  >") : TEXT("Order board"), [this, bHasNext]()
							{ if (bHasNext && (bJudgeMode || Ops.Stars >= 2)) StartMission(MissionIdx + 1); else Go(EScreen::Missions); }, 1, 14)])
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- PEDIA
TSharedRef<SWidget> SISGameUI::BuildPedia()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 i = 0; i < PediaCount; ++i)
		List->AddSlot().AutoHeight().Padding(0, 2)[Btn(Pedia[i].Name, [this, i]() { PediaSel = i; UpdateScene(); bDirty = true; }, i == PediaSel ? 2 : 0, 12)];
	const FPediaEntry& E = Pedia[FMath::Clamp(PediaSel, 0, PediaCount - 1)];
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("SAT-O-PEDIA  -  real spacecraft (NASA public-domain 3D models)"))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(340)[Panel(SNew(SScrollBox) + SScrollBox::Slot()[List])]]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(500)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(E.Name, 18, CAccent, true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(E.Facts, 13, CText)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(TEXT("Drag to rotate, mouse wheel to zoom."), 12, CDim)], 16.f)
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- HELP
TSharedRef<SWidget> SISGameUI::BuildHelp()
{
	const FString Help =
		TEXT("THE LOOP\n")
		TEXT("1. ORDER BOARD: pick a client order. Each chapter teaches a new idea, from a CanSat to an interstellar probe.\n")
		TEXT("2. DESIGN: choose one part per subsystem (structure, power, battery, attitude control, radio, propulsion, computer, thermal, ground network) plus instruments and extras. ")
		TEXT("Every choice changes mass, power, data, delta-v, radiation, lifetime and cost. Budgets turn red when they don't close. CLICK ANY BUDGET LINE to see the formula and the numbers behind it.\n")
		TEXT("3. LAUNCH: buy a ride. Bigger rockets lift more but cost more. Some cannot reach your orbit at all.\n")
		TEXT("4. MISSION CONTROL: time runs fast. Watch power, data and health. When something breaks, you decide what to do, like a real flight director.\n")
		TEXT("5. DEBRIEF: see which objectives you met, the ONE thing that limited you, how your design compares with the real mission, and answer a quick check.\n\n")
		TEXT("TIPS\n")
		TEXT("- You can launch a design with red budgets: watch exactly how and when it fails. Then improve it.\n")
		TEXT("- Use existing networks: rent ground stations, relay satellites or the Deep Space Network instead of building your own; GNSS receivers use GPS/Galileo for free.\n")
		TEXT("- Redundant electronics cost mass and money but let you recover from failures.\n")
		TEXT("- Judge/teacher mode (main menu) unlocks every chapter and part.\n\n")
		TEXT("CONTROLS\nMouse: click buttons. Drag in the 3D view to rotate, mouse wheel to zoom.\n\n")
		TEXT("THE NUMBERS\nSimplified but real physics: Kepler's 3rd law, eclipse geometry, inverse-square sunlight, the rocket equation, Friis link budget scaled from Voyager, exponential reliability, radiation dose, drag lifetime, solar sail lightness number. ")
		TEXT("Spacecraft facts from NASA, USGS, NASA Earthdata; constellation numbers from planet4589.org (13 Nov 2026).");
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("HOW TO PLAY"))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12).HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride(760)
			[
				Panel(SNew(SScrollBox) + SScrollBox::Slot()[Txt(Help, 14, CText)]
					+ SScrollBox::Slot().Padding(0, 12)[Btn(TEXT("Start campaign  >"), [this]() { Go(EScreen::Missions); }, 1, 15)], 20.f)
			]
		];
}

// ----------------------------------------------------------------------------------------------- IN-WORLD HUD
TSharedRef<SWidget> SISGameUI::BuildPlay()
{
	TWeakObjectPtr<AISHomeWorld> WW = HW();
	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Txt(TEXT("+"), 18, FLinearColor(1, 1, 1, 0.8f), true)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 40, 0, 0)
			[
				SNew(STextBlock).Font(ISFont("Bold", 16)).ColorAndOpacity(CGold).ShadowOffset(FVector2D(1, 1))
				.Text_Lambda([this]() { return FText::FromString(IPC() ? IPC()->Prompt() : FString()); })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(16)
		[
			SNew(SBox).WidthOverride(430)
			[
				Panel(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("OBJECTIVE"), 12, CAccent, true)]
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(ISFont("Regular", 14)).ColorAndOpacity(CText).AutoWrapText(true)
						.Text_Lambda([WW]() { return FText::FromString(WW.IsValid() ? WW->Objective() : FString()); })], 10.f)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(16)
		[
			Panel(Txt(TEXT("WASD move   Mouse look   SHIFT float fast   SPACE hover up\nE interact   F chase camera (in flight)   ESC menu"), 12, CDim), 8.f)
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(16)
		[
			SNew(SBox).WidthOverride(560)
			[
				SNew(SBorder).BorderImage(&PanelBrush).BorderBackgroundColor(CPanel2).Padding(10)
				.Visibility_Lambda([this]() { return (ToastLog.Num() && UITime - ToastLog.Last().Value < 14.0) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)[SNew(SBox).WidthOverride(70).HeightOverride(44)[SNew(SImage).Image(&BitBrush)]]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(ISFont("Regular", 13)).ColorAndOpacity(CText).AutoWrapText(true)
						.Text_Lambda([this]()
						{
							FString S;
							for (int32 i = FMath::Max(0, ToastLog.Num() - 2); i < ToastLog.Num(); ++i)
								if (UITime - ToastLog[i].Value < 14.0) S += (S.IsEmpty() ? TEXT("") : TEXT("\n")) + ToastLog[i].Key;
							return FText::FromString(S);
						})
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(16)
		[
			SNew(SBox).WidthOverride(390)
			[
				SNew(SBorder).BorderImage(&PanelBrush).BorderBackgroundColor(CPanel).Padding(12)
				.Visibility_Lambda([WW]() { return WW.IsValid() && (WW->IsTracking() || WW->Stage == EISStage::Countdown) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("FLIGHT TELEMETRY  (cyan = altitude, orange = temperature)"), 12, CAccent, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						SNew(STextBlock).Font(ISFont("Mono", 12)).ColorAndOpacity(CText)
						.Text_Lambda([WW]()
						{
							if (!WW.IsValid()) return FText();
							const AISHomeWorld* W = WW.Get();
							const FISTelemetry* L = W->Telemetry.Num() ? &W->Telemetry.Last() : nullptr;
							return FText::FromString(FString::Printf(TEXT("Alt %6.0f m   Temp %5.1f C\nPress %6.1f hPa   Packets %d\nWind %.1f m/s"),
								L ? L->AltM : 0.f, L ? L->TempC : 15.f, L ? L->PressHPa : 1013.f, W->Telemetry.Num(), W->WindMs.Size()));
						})
					]
					+ SVerticalBox::Slot().AutoHeight()[SNew(SISTelemetryGraph).World(WW)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 2)[Txt(TEXT("HANDHELD RECEIVER 433 MHz (Yagi antenna)"), 12, CAccent, true)]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(ISFont("Mono", 13)).ColorAndOpacity(CText).AutoWrapText(true)
						.Text_Lambda([this, WW]()
						{
							if (!WW.IsValid()) return FText();
							const AISHomeWorld* W = WW.Get();
							const float Margin = W->TrackRssi - W->TrackSens;
							const int32 Bars = W->bTrackSignal ? FMath::Clamp((int32)(Margin / 6.f) + 1, 1, 10) : 0;
							FString Bar;
							for (int32 i = 0; i < 10; ++i) Bar += i < Bars ? TEXT("#") : TEXT(".");
							FString S = W->bTrackSignal ? FString::Printf(TEXT("[%s] %.0f dBm\n(receiver limit %.0f dBm)\nTurn slowly: the signal is strongest\nwhen you face the CanSat."), *Bar, W->TrackRssi, W->TrackSens)
								: FString(TEXT("[..........] no signal\nOut of range, beacon dead, or\nfacing away. Turn around / walk."));
							if (W->bHasGnss && W->bTrackSignal)
							{
								float Rel = 0.f;
								if (IPC() && IPC()->Bit) Rel = FMath::FindDeltaAngleDegrees(IPC()->GetControlRotation().Yaw, W->TrackDirWorld.Rotation().Yaw);
								S += FString::Printf(TEXT("\nGNSS fix: %.0f m away, %s %.0f deg"), W->TrackDistM, Rel >= 0 ? TEXT("right") : TEXT("left"), FMath::Abs(Rel));
							}
							if (W->Stage == EISStage::Landed)
								S += FString::Printf(TEXT("\nBeacon battery: %s"), W->BeaconLeftS > 0 ? *FString::Printf(TEXT("%.0f s (1 s = 1 game min)"), W->BeaconLeftS) : TEXT("EMPTY"));
							return FText::FromString(S);
						})
					]
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- BENCH
TSharedRef<SWidget> SISGameUI::BuildBench()
{
	AISHomeWorld* W = HW();
	if (!W) return SNullWidget::NullWidget;
	Eval = W->Eval;
	TWeakObjectPtr<AISHomeWorld> WW = W;
	TSharedRef<SVerticalBox> Installed = SNew(SVerticalBox);
	for (const FName& Id : W->Design.Parts)
	{
		const FISPart* P = ISData::FindPart(Id);
		if (!P || P->Cat == EISCat::Bus || P->Cat == EISCat::Ground || Id.ToString().EndsWith(TEXT("_none")) || Id == "thm_passive") continue;
		Installed->AddSlot().AutoHeight().Padding(0, 2)[Btn(FString::Printf(TEXT("x  %s   (%s, %s W)"), *P->Name, *ISSim::FmtMass(P->MassKg), *ISSim::Fmt(P->PowerW, 2)), [WW, Id]() { if (WW.IsValid()) WW->BenchToggle(Id); }, 0, 11)];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("WORKBENCH  -  build your CanSat (see-through can)"))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(400)
				[
					Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("CLICK a part on the bench to put it into the can. Click it in this list to take it out again. Parts stack inside: watch the height!"), 12, CText)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)[Txt(TEXT("INSIDE THE CAN"), 12, CAccent, true)]
						+ SVerticalBox::Slot().AutoHeight()[Installed]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 2)[Txt(TEXT("PART INFO (hover a part)"), 12, CAccent, true)]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SNew(SScrollBox) + SScrollBox::Slot()
							[
								SNew(STextBlock).Font(ISFont("Regular", 12)).ColorAndOpacity(CText).AutoWrapText(true)
								.Text_Lambda([WW]()
								{
									const FISPart* P = WW.IsValid() ? ISData::FindPart(WW->BenchInfoPart) : nullptr;
									if (!P) return FText::FromString(TEXT("Hover over a part on the bench."));
									return FText::FromString(FString::Printf(TEXT("%s\nMass %s  |  Power %s W  |  Cost %s\n\n%s"), *P->Name, *ISSim::FmtMass(P->MassKg), *ISSim::Fmt(P->PowerW, 2), *ISSim::FmtMoney(P->CostM), *P->Edu));
								})
							]
						])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(440)
				[
					Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("BUDGETS  (click a line: WHY?)"), 13, CAccent, true)]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SAssignNew(BudgetBox, SScrollBox)
							+ SScrollBox::Slot()[BuildBudgets(false)]
							+ SScrollBox::Slot().Padding(0, 8, 0, 2)[Txt(TEXT("CLIENT OBJECTIVES (predicted)"), 13, CAccent, true)]
							+ SScrollBox::Slot()[BuildObjectives(Eval.Objectives)]
						])
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[BuildBitTip(Eval.Hint)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
			[
				SNew(SBox).WidthOverride(240)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Seal the can (finished)"), [this, WW]() { if (WW.IsValid()) { WW->BenchSeal(); if (WW->Stage == EISStage::Sealed && IPC()) IPC()->LeaveBench(); } }, 1, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[Btn(TEXT("Leave the bench (E)"), [this]() { if (IPC()) IPC()->LeaveBench(); })]
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- CANSAT DEBRIEF
TSharedRef<SWidget> SISGameUI::BuildCanDebrief()
{
	AISHomeWorld* W = HW();
	if (!W) return SNullWidget::NullWidget;
	const FISFlightReport& R = W->Report;
	const FISMission& M = Mission();
	Eval = W->Eval;
	TArray<FISObjective> O;
	auto Add = [&O](const FString& T, bool b, const FString& Why, bool bBonus = false) { FISObjective X; X.Text = T; X.bMet = b; X.Why = Why; X.bBonus = bBonus; O.Add(X); };
	Add(TEXT("Mass 350 g or less"), Eval.TotalKg <= 0.35f, FString::Printf(TEXT("Yours: %s"), *ISSim::FmtMass(Eval.TotalKg)));
	Add(TEXT("Reach about 1 km"), R.ApogeeM >= 700.f, FString::Printf(TEXT("Apogee %.0f m (a heavier CanSat flies lower)"), R.ApogeeM));
	Add(TEXT("Carry a temperature sensor"), W->Design.HasTag(ISTag::TempSensor), TEXT("Required by the client"));
	Add(TEXT("Receive telemetry for most of the flight (60%+)"), R.TelemetryPct >= 60.f, FString::Printf(TEXT("%.0f%% of packets received: radio range and antenna pointing matter"), R.TelemetryPct));
	Add(TEXT("Land under 10 m/s"), R.LandingSpeed <= 10.f, FString::Printf(TEXT("Landed at %.1f m/s"), R.LandingSpeed));
	Add(TEXT("Survive the landing"), !R.bBroken, R.bBroken ? TEXT("Impact over 15 m/s broke it") : TEXT("Intact"));
	Add(TEXT("Beacon still transmitting when found (2 h battery)"), R.bBeaconAlive, FString::Printf(TEXT("Battery runtime %.1f h"), Eval.RuntimeH));
	Add(TEXT("Stay within budget"), Eval.TotalCostM <= M.BudgetM, ISSim::FmtMoney(Eval.TotalCostM));
	Add(TEXT("BONUS: GNSS receiver (uses GPS/Galileo)"), W->Design.HasTag(ISTag::GNSSRx), TEXT("Makes finding it much easier"), true);
	Add(TEXT("BONUS: camera"), W->Design.HasTag(ISTag::Camera), TEXT("Pictures from the sky!"), true);
	int32 Req = 0, Met = 0, Bonus = 0;
	FString FirstFail;
	for (const FISObjective& X : O) { if (X.bBonus) { Bonus += X.bMet; continue; } ++Req; if (X.bMet) ++Met; else if (FirstFail.IsEmpty()) FirstFail = X.Text + TEXT(": ") + X.Why; }
	const int32 Stars = (Met == Req && Bonus > 0) ? 3 : (Met == Req) ? 2 : (Met * 2 >= Req ? 1 : 0);
	if (BestStars.IsValidIndex(MissionIdx)) BestStars[MissionIdx] = FMath::Max(BestStars[MissionIdx], Stars);
	if (Stars >= 2) MaxChapter = FMath::Max(MaxChapter, 1);

	// lapse rate from the player's own data (linear regression of temperature against altitude)
	double Sx = 0, Sy = 0, Sxx = 0, Sxy = 0; int32 N = 0;
	for (const FISTelemetry& T : W->Telemetry) { const double X = T.AltM / 1000.0; Sx += X; Sy += T.TempC; Sxx += X * X; Sxy += X * T.TempC; ++N; }
	const double Den = N * Sxx - Sx * Sx;
	const FString Science = (N >= 4 && FMath::Abs(Den) > 1e-6)
		? FString::Printf(TEXT("YOUR SCIENCE: from %d data packets, temperature changed by %.1f C per km of height. The standard atmosphere says -6.5 C/km (the 'lapse rate'). Your CanSat measured real atmospheric physics!"), N, (N * Sxy - Sx * Sy) / Den)
		: FString(TEXT("YOUR SCIENCE: too few telemetry packets to measure the lapse rate (-6.5 C per km). A longer-range radio or staying closer would help."));

	FString StarStr;
	for (int32 i = 0; i < 3; ++i) StarStr += i < Stars ? TEXT("* ") : TEXT("- ");
	TSharedRef<SVerticalBox> Quiz = SNew(SVerticalBox);
	for (int32 q = 0; q < M.Quiz.Num(); ++q)
	{
		const FISQuiz& Q = M.Quiz[q];
		Quiz->AddSlot().AutoHeight().Padding(0, 8, 0, 2)[Txt(FString::Printf(TEXT("Q%d. %s"), q + 1, *Q.Q), 13, CText, true)];
		TSharedRef<SWrapBox> Ans = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
		const int32 Picked = QuizAnswers.IsValidIndex(q) ? QuizAnswers[q] : -1;
		for (int32 a = 0; a < Q.A.Num(); ++a)
			Ans->AddSlot()[Btn(Q.A[a], [this, q, a]() { if (QuizAnswers.IsValidIndex(q) && QuizAnswers[q] < 0) { QuizAnswers[q] = a; bDirty = true; } }, Picked < 0 ? 0 : (a == Q.Correct ? 1 : (a == Picked ? 2 : 0)), 12)];
		Quiz->AddSlot().AutoHeight()[Ans];
		if (Picked >= 0) Quiz->AddSlot().AutoHeight()[Txt((Picked == Q.Correct ? TEXT("Correct! ") : TEXT("Not quite. ")) + Q.Why, 12, Picked == Q.Correct ? COk : CWarn)];
	}
	TWeakObjectPtr<AISHomeWorld> WW = W;
	const FString Stats = FString::Printf(TEXT("Apogee %.0f m  |  top speed %.0f m/s  |  landing %.1f m/s\nDrift %.0f m with %.1f m/s wind  |  flight %.0f s  |  search %.0f s\nTelemetry packets %d (%.0f%%)"),
		R.ApogeeM, R.MaxSpeed, R.LandingSpeed, R.DriftM, W->WindMs.Size(), R.FlightS, R.SearchS, R.Samples, R.TelemetryPct);
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("DEBRIEF  -  ") + M.Title)]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(560)
				[
					Panel(SNew(SScrollBox)
						+ SScrollBox::Slot()[Txt(StarStr + FString::Printf(TEXT("  %d / %d objectives"), Met, Req), 26, CGold, true)]
						+ SScrollBox::Slot().Padding(0, 4)[Txt(FirstFail.IsEmpty() ? TEXT("Perfect flight! ") + M.Lesson : TEXT("Limiting factor -> ") + FirstFail + TEXT("\n") + M.Lesson, 13, CText)]
						+ SScrollBox::Slot().Padding(0, 6)[Txt(Stats, 12, CDim)]
						+ SScrollBox::Slot().Padding(0, 8, 0, 2)[Txt(TEXT("OBJECTIVES"), 13, CAccent, true)]
						+ SScrollBox::Slot()[BuildObjectives(O)]
						+ SScrollBox::Slot().Padding(0, 8, 0, 2)[Txt(TEXT("QUICK CHECK"), 13, CAccent, true)]
						+ SScrollBox::Slot()[Quiz])
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(440)
				[
					Panel(SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("YOUR FLIGHT DATA"), 13, CAccent, true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[SNew(SISTelemetryGraph).World(WW)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(Science, 12, CText)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(TEXT("REAL WORLD: ") + M.RealFacts, 12, CDim)]
						+ SVerticalBox::Slot().FillHeight(1.f)[SNew(SSpacer)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(TEXT("Build a better CanSat (try again)"), [this, WW]() { if (WW.IsValid()) WW->StartCanSatOrder(WW->MissionIdx); if (IPC()) IPC()->ClosePanel(); }, 0, 13)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Btn(TEXT("Next order (order board)  >"), [this]() { MissionIdx = FMath::Min(MissionIdx + 1, ISData::Missions().Num() - 1); ShowOrders(); }, 1, 14)])
				]
			]
		];
}

// ----------------------------------------------------------------------------------------------- PAUSE / SETTINGS
TSharedRef<SWidget> SISGameUI::BuildPause()
{
	AISPlayerController* P = IPC();
	if (!P) return SNullWidget::NullWidget;
	auto Row = [this](const FString& Label, TFunction<FString()> Value, TFunction<void()> Minus, TFunction<void()> Plus)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Txt(Label, 15, CText)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4)[Btn(TEXT("-"), Minus, 0, 15)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0)
			[
				SNew(SBox).WidthOverride(70).HAlign(HAlign_Center)
				[SNew(STextBlock).Font(ISFont("Bold", 15)).ColorAndOpacity(CAccent).Text_Lambda([Value]() { return FText::FromString(Value()); })]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4)[Btn(TEXT("+"), Plus, 0, 15)];
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(&WhiteBrush).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.55f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(520)
			[
				Panel(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)[Txt(TEXT("PAUSED"), 28, CText, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Row(TEXT("Mouse sensitivity"), [P]() { return ISSim::Fmt(P->MouseSens, 2); },
						[P]() { P->MouseSens = FMath::Max(0.1f, P->MouseSens - 0.1f); P->SaveSettings(); }, [P]() { P->MouseSens = FMath::Min(6.f, P->MouseSens + 0.1f); P->SaveSettings(); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Row(TEXT("Field of view"), [P]() { return ISSim::Fmt(P->Fov, 0); },
						[P]() { P->Fov = FMath::Max(60.f, P->Fov - 5.f); P->SaveSettings(); }, [P]() { P->Fov = FMath::Min(120.f, P->Fov + 5.f); P->SaveSettings(); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Row(TEXT("Graphics"), [P]() { return AISPlayerController::GraphicsName(P->Graphics); },
						[P]() { P->Graphics = FMath::Max(0, P->Graphics - 1); P->ApplyGraphics(); P->SaveSettings(); }, [P]() { P->Graphics = FMath::Min(3, P->Graphics + 1); P->ApplyGraphics(); P->SaveSettings(); })]
					+ SVerticalBox::Slot().AutoHeight()[Txt(TEXT("Low (low-poly) runs on laptops without a graphics card. Trees and grass density apply after re-entering the world."), 12, CDim)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([P]() { return P->bInvertY ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([P](ECheckBoxState S) { P->bInvertY = S == ECheckBoxState::Checked; P->SaveSettings(); })
						[Txt(TEXT("Invert mouse Y"), 15, CText)]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 4)[Btn(TEXT("Resume"), [this]() { if (InWorld()) { if (IPC()) IPC()->ClosePanel(); } else Go(EScreen::Title); }, 1, 16)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Order board"), [this]() { ShowOrders(); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Btn(TEXT("Main menu"), [this]() { if (IPC()) IPC()->EnterMenu(); Go(EScreen::Title); }, 0, 14)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[Txt(TEXT("WASD move  |  Mouse look  |  SHIFT faster  |  SPACE hover up  |  CTRL/C hover low  |  E interact  |  F chase cam  |  ESC pause"), 12, CDim)],
					24.f)
			]
		];
}

// =====================================================================================================
//  team play, scoring, leaderboard, reports, shareable design codes
// =====================================================================================================
TArray<SISGameUI::FISRole> SISGameUI::Roles() const
{
	using C = EISCat;
	auto R = [](const TCHAR* N, TArray<EISCat> Cats, bool bLaunch, const TCHAR* Goal, int32 Id) { FISRole X; X.Name = N; X.Cats = Cats; X.bLaunch = bLaunch; X.Goal = Goal; X.GoalId = Id; return X; };
	TArray<FISRole> L;
	switch (TeamSize)
	{
	case 2:
		L.Add(R(TEXT("Mission Planner & Spacecraft Engineer"), { C::Bus, C::Power, C::Battery, C::ADCS, C::Propulsion, C::Computer, C::Thermal }, true, TEXT("keep total cost under 85% of the budget"), 0));
		L.Add(R(TEXT("Science & Communications Lead"), { C::Payload, C::Extra, C::Comms, C::Ground }, false, TEXT("return 50% more data than required, or fly a bonus instrument"), 2));
		break;
	case 3:
		L.Add(R(TEXT("Mission Planner"), { C::Propulsion, C::Ground }, true, TEXT("keep total cost under 85% of the budget"), 0));
		L.Add(R(TEXT("Spacecraft Engineer"), { C::Bus, C::Power, C::Battery, C::ADCS, C::Computer, C::Thermal }, false, TEXT("at least 75% chance that every part survives"), 1));
		L.Add(R(TEXT("Science & Comms Lead"), { C::Payload, C::Extra, C::Comms }, false, TEXT("return 50% more data than required, or fly a bonus instrument"), 2));
		break;
	case 4:
		L.Add(R(TEXT("Mission Planner"), { C::Propulsion }, true, TEXT("keep total cost under 85% of the budget"), 0));
		L.Add(R(TEXT("Spacecraft Engineer"), { C::Bus, C::Power, C::Battery, C::ADCS, C::Computer, C::Thermal }, false, TEXT("at least 75% chance that every part survives"), 1));
		L.Add(R(TEXT("Science Lead"), { C::Payload, C::Extra }, false, TEXT("return 50% more data than required, or fly a bonus instrument"), 2));
		L.Add(R(TEXT("Comms & Resources Manager"), { C::Comms, C::Ground }, false, TEXT("15% mass margin on the rocket and 10% spare power"), 3));
		break;
	default:
		break;
	}
	return L;
}

bool SISGameUI::OwnsCat(EISCat C) const
{
	if (TeamSize <= 1) return true;
	const TArray<FISRole> L = Roles();
	return L.IsValidIndex(ActivePlayer) && L[ActivePlayer].Cats.Contains(C);
}

bool SISGameUI::OwnsLaunch() const
{
	if (TeamSize <= 1) return true;
	const TArray<FISRole> L = Roles();
	return L.IsValidIndex(ActivePlayer) && L[ActivePlayer].bLaunch;
}

bool SISGameUI::RoleGoalMet(int32 GoalId) const
{
	const FISMission M = ISSim::ApplyDifficulty(Mission());
	switch (GoalId)
	{
	case 0: return Eval.TotalCostM <= 0.85f * M.BudgetM;
	case 1: return Eval.RelSat >= 0.75f;
	case 2:
	{
		bool bBonus = false;
		for (FName T : M.Bonus) if (Design.HasTag(T)) bBonus = true;
		return bBonus || (M.MinDataGbDay > 0 && Eval.DataDownGb >= 1.5f * M.MinDataGbDay);
	}
	case 3: return Eval.VehicleCap > 0 && Eval.TotalKg <= 0.85f * Eval.VehicleCap && Eval.AvgGenW >= 1.1f * Eval.LoadW / 0.85f;
	}
	return false;
}

TSharedRef<SWidget> SISGameUI::BuildTeamBar()
{
	if (TeamSize <= 1)
		return Panel(Txt(Notice.IsEmpty() ? FString::Printf(TEXT("Solo mission  |  %s difficulty  |  every decision is yours. Watch the budgets: every part costs mass, power, money and data."), *ISSim::DifficultyName(ISSim::Difficulty)) : Notice, 13, CText), 8.f, 1);
	const TArray<FISRole> L = Roles();
	if (Ready.Num() != TeamSize) const_cast<SISGameUI*>(this)->Ready.Init(false, TeamSize);
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	const FISRole& Me = L[FMath::Clamp(ActivePlayer, 0, L.Num() - 1)];
	FString Cats;
	for (EISCat C : Me.Cats) Cats += (Cats.IsEmpty() ? TEXT("") : TEXT(", ")) + ISData::CatName(C);
	if (Me.bLaunch) Cats += TEXT(", Launch vehicle");
	Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
	[
		Txt(FString::Printf(TEXT("PLAYER %d: %s  -  you decide: %s.\nYour goal: %s. Talk to your team: other players see the same budgets."), ActivePlayer + 1, *Me.Name, *Cats, *Me.Goal), 14, CGold)
	];
	for (int32 i = 0; i < TeamSize; ++i)
		Row->AddSlot().AutoWidth().Padding(3).VAlign(VAlign_Center)
		[
			Btn(FString::Printf(TEXT("P%d %s"), i + 1, Ready.IsValidIndex(i) && Ready[i] ? TEXT("agrees") : TEXT("...")), [this, i]() { if (Ready.IsValidIndex(i)) Ready[i] = !Ready[i]; bDirty = true; }, Ready.IsValidIndex(i) && Ready[i] ? 1 : 0, 12)
		];
	Row->AddSlot().AutoWidth().Padding(6, 3).VAlign(VAlign_Center)
	[
		Btn(TEXT("Hand over to next player  >"), [this]() { ActivePlayer = (ActivePlayer + 1) % FMath::Max(1, TeamSize); Notice.Reset(); bDirty = true; }, 1, 13)
	];
	return Panel(Row, 8.f, 1);
}

SISGameUI::FISScore SISGameUI::ComputeScore() const
{
	FISScore S;
	const FISMission M = ISSim::ApplyDifficulty(Mission());
	S.Success = 50.f * Ops.ScorePct / 100.f;
	int32 BonusN = 0, BonusMet = 0;
	for (const FISObjective& O : Ops.FinalObjectives) if (O.bBonus) { ++BonusN; if (O.bMet) ++BonusMet; }
	const float BonusFrac = BonusN ? (float)BonusMet / BonusN : 0.f;
	const double NeedData = M.MinDataGbDay * Ops.EndDay;
	const float DataFrac = NeedData > 0 ? FMath::Clamp((float)(Ops.DataGb / NeedData), 0.f, 1.f) : (Ops.bMissionLost ? 0.3f : 1.f);
	S.Science = 20.f * (0.5f * BonusFrac + 0.5f * DataFrac);
	const float BudgetLeft = M.BudgetM > 0 ? FMath::Clamp(1.f - Eval.TotalCostM / M.BudgetM, 0.f, 0.5f) / 0.5f : 0.f;
	const float MassLeft = Eval.VehicleCap > 0 ? FMath::Clamp(1.f - Eval.TotalKg / Eval.VehicleCap, 0.f, 0.5f) / 0.5f : 0.f;
	S.Efficiency = 15.f * (0.6f * BudgetLeft + 0.4f * MassLeft);
	S.Decisions = 15.f * (Ops.TotalChoices > 0 ? (float)Ops.GoodChoices / Ops.TotalChoices : 1.f);
	if (TeamSize > 1)
	{
		int32 Met = 0; const TArray<FISRole> L = Roles();
		for (const FISRole& R : L) if (RoleGoalMet(R.GoalId)) ++Met;
		S.Team = 20.f * Met / FMath::Max(1, L.Num());
	}
	const float Mult = ISSim::Difficulty == 0 ? 0.8f : ISSim::Difficulty == 2 ? 1.25f : 1.f;
	S.Total = (S.Success + S.Science + S.Efficiency + S.Decisions + S.Team) * Mult;
	return S;
}

static FString BoardFile() { return FPaths::ProjectSavedDir() / TEXT("Infinisat") / TEXT("leaderboard.csv"); }

void SISGameUI::RecordScore(const FISScore& S)
{
	const FString Line = FString::Printf(TEXT("%s;%s;%d;%s;%d;%s\n"), *Mission().Id.ToString(), *TeamName.Replace(TEXT(";"), TEXT(",")), FMath::RoundToInt(S.Total),
		*ISSim::DifficultyName(ISSim::Difficulty), TeamSize, *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M")));
	FFileHelper::SaveStringToFile(Line, *BoardFile(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

TArray<FString> SISGameUI::Leaderboard() const
{
	TArray<FString> Lines, Out;
	FFileHelper::LoadFileToStringArray(Lines, *BoardFile());
	struct FE { int32 Score; FString Text; };
	TArray<FE> E;
	for (const FString& L : Lines)
	{
		TArray<FString> P; L.ParseIntoArray(P, TEXT(";"), false);
		if (P.Num() >= 6 && P[0] == Mission().Id.ToString())
			E.Add({ FCString::Atoi(*P[2]), FString::Printf(TEXT("%s  -  %s pts  (%s, %s player%s, %s)"), *P[1], *P[2], *P[3], *P[4], P[4] == TEXT("1") ? TEXT("") : TEXT("s"), *P[5]) });
	}
	E.Sort([](const FE& A, const FE& B) { return A.Score > B.Score; });
	for (int32 i = 0; i < FMath::Min(10, E.Num()); ++i) Out.Add(E[i].Text);
	if (Out.Num() == 0) Out.Add(TEXT("(no scores yet)"));
	return Out;
}

FString SISGameUI::DesignCode() const
{
	FString Parts;
	for (const FName& P : Design.Parts) Parts += (Parts.IsEmpty() ? TEXT("") : TEXT(",")) + P.ToString();
	return FString::Printf(TEXT("INFINISAT1|%s|%s|%s|%d|%d"), *Mission().Id.ToString(), *Parts, *Design.Vehicle.ToString(), Design.Fleet, (int32)Design.Dest);
}

bool SISGameUI::ApplyDesignCode(const FString& Code)
{
	TArray<FString> P; Code.TrimStartAndEnd().ParseIntoArray(P, TEXT("|"), false);
	if (P.Num() < 6 || P[0] != TEXT("INFINISAT1") || P[1] != Mission().Id.ToString()) return false;
	TArray<FString> Ids; P[2].ParseIntoArray(Ids, TEXT(","));
	FISDesign D = Design;
	D.Parts.Reset();
	for (const FString& Id : Ids) if (ISData::FindPart(FName(*Id))) D.Parts.Add(FName(*Id));
	D.Vehicle = FName(*P[3]);
	D.Fleet = FMath::Max(1, FCString::Atoi(*P[4]));
	D.Dest = (EISDest)FMath::Clamp(FCString::Atoi(*P[5]), 0, (int32)EISDest::Count - 1);
	Design = D;
	return true;
}

FString SISGameUI::WriteReport(const FISScore& S)
{
	const FISMission& M = Mission();
	auto Esc = [](FString X) { return X.Replace(TEXT("&"), TEXT("&amp;")).Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT("\n"), TEXT("<br>")); };
	FString H = TEXT("<!doctype html><meta charset='utf-8'><title>Infinisat mission report</title><style>body{font-family:Segoe UI,Arial;max-width:900px;margin:30px auto;color:#123}td,th{border-bottom:1px solid #ccd;padding:6px;text-align:left;vertical-align:top}h1{color:#0a5}.x{color:#c33}.ok{color:#183}</style>");
	H += FString::Printf(TEXT("<h1>%s</h1><p><b>%s</b> | %s difficulty | %d player(s) | %s</p>"), *Esc(M.Title), *Esc(TeamName), *ISSim::DifficultyName(ISSim::Difficulty), TeamSize, *FDateTime::Now().ToString());
	H += FString::Printf(TEXT("<h2>Score: %d</h2><p>Mission success %d/50, science value %d/20, resource efficiency %d/15, decisions %d/15%s</p>"), FMath::RoundToInt(S.Total),
		FMath::RoundToInt(S.Success), FMath::RoundToInt(S.Science), FMath::RoundToInt(S.Efficiency), FMath::RoundToInt(S.Decisions), TeamSize > 1 ? *FString::Printf(TEXT(", team coordination %d/20"), FMath::RoundToInt(S.Team)) : TEXT(""));
	H += TEXT("<h2>Why it went this way</h2><p>") + Esc(Ops.Debrief) + TEXT("</p><h2>Objectives</h2><table>");
	for (const FISObjective& O : Ops.FinalObjectives)
		H += FString::Printf(TEXT("<tr><td class='%s'>%s</td><td>%s</td><td>%s</td></tr>"), O.bMet ? TEXT("ok") : TEXT("x"), O.bMet ? TEXT("met") : TEXT("missed"), *Esc(O.Text), *Esc(O.Why));
	H += TEXT("</table><h2>Design</h2><ul>");
	for (const FName& Id : Design.Parts) if (const FISPart* P = ISData::FindPart(Id)) H += TEXT("<li>") + Esc(ISData::CatName(P->Cat) + TEXT(": ") + P->Name) + TEXT("</li>");
	if (const FISVehicle* V = ISData::FindVehicle(Design.Vehicle)) H += TEXT("<li>Launch: ") + Esc(V->Name + TEXT(" from ") + V->Site) + TEXT("</li>");
	H += TEXT("</ul><p>Design code (paste in the game to load it): <code>") + Esc(DesignCode()) + TEXT("</code></p><h2>Budgets and the physics behind them</h2><table>");
	for (const FISLine& L : Eval.Lines)
		H += FString::Printf(TEXT("<tr><td><b>%s</b></td><td>%s / %s</td><td>%s</td></tr>"), *Esc(L.Label), *Esc(L.Value), *Esc(L.Limit), *Esc(L.Why));
	H += TEXT("</table><h2>Mission timeline</h2><table>");
	for (const FISEvent& E : Ops.Log)
		H += FString::Printf(TEXT("<tr><td>T+%s</td><td>%s</td><td><i>%s</i></td></tr>"), *ISSim::FmtDays(E.Day), *Esc(E.Text), *Esc(E.Edu));
	H += TEXT("</table><h2>Real-world reference</h2><p>") + Esc(M.RealName + TEXT(": ") + M.RealFacts) + TEXT("</p>");
	const FString Dir = FPaths::Combine(FPlatformProcess::UserDir(), TEXT("Infinisat Reports"));
	const FString File = FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s.html"), *M.Id.ToString(), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
	FFileHelper::SaveStringToFile(H, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return File;
}

// ----------------------------------------------------------------------------------------------- COSMOS EXPLORER
namespace
{
	// Screen-space body labels + markers for the cosmos (crisp at any distance, decluttered like the KSP map).
	// Click a label to fly there; clicks elsewhere fall through to the camera controls.
	class SISCosmosOverlay : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SISCosmosOverlay) {}
		SLATE_END_ARGS()
		TWeakObjectPtr<AISSpaceScene> Scene;
		TWeakObjectPtr<APlayerController> PC;
		TFunction<void(int32)> OnPick;
		void Construct(const FArguments&, TWeakObjectPtr<AISSpaceScene> InScene, TWeakObjectPtr<APlayerController> InPC, TFunction<void(int32)> InPick)
		{
			Scene = InScene; PC = InPC; OnPick = MoveTemp(InPick);
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100, 100); }
		virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
		{
			const FVector2f L = FVector2f(G.AbsoluteToLocal(E.GetScreenSpacePosition()));
			for (const TPair<int32, FSlateRect>& H : Hits)
				if (H.Value.ContainsPoint(FVector2D(L))) { if (OnPick) OnPick(H.Key); return FReply::Handled(); }
			return FReply::Unhandled();
		}
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			Hits.Reset();
			AISSpaceScene* S = Scene.Get();
			APlayerController* P = PC.Get();
			if (!S || !P || !S->CosmosLabelsOn() || !GEngine || !GEngine->GameViewport) return Layer;
			FVector2D VP; GEngine->GameViewport->GetViewportSize(VP);
			if (VP.X < 1 || VP.Y < 1) return Layer;
			const FVector2f K = FVector2f(G.GetLocalSize()) / FVector2f(VP);
			const FVector Cam = S->GetCameraLocation();
			const int32 N = S->CosmosCount();
			TArray<FVector2f> Scr; Scr.SetNum(N);
			TArray<bool> On; On.Init(false, N);
			for (int32 i = 0; i < N; ++i)
			{
				FVector2D Px;
				if (P->ProjectWorldLocationToScreen(S->CosmosPos(i), Px, true)) { Scr[i] = FVector2f(Px) * K; On[i] = true; }
			}
			const FSlateFontInfo Font = ISFont("Bold", 11);
			const TSharedRef<FSlateFontMeasure> FM = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const int32 Sel = S->CosmosFocusIdx();
			// priority: selected body, then stars, planets, moons, spacecraft (tree depth); lower ones yield on overlap
			TArray<int32> Order;
			for (int32 i = 0; i < N; ++i) Order.Add(i);
			Order.Sort([S, Sel](int32 A, int32 B)
			{
				const int32 DA = A == Sel ? -1 : S->CosmosDepth(A), DB = B == Sel ? -1 : S->CosmosDepth(B);
				return DA != DB ? DA < DB : A < B;
			});
			TArray<FSlateRect> Taken;
			for (const int32 i : Order)
			{
				const FString Text = S->CosmosLabelText(i);
				if (!On[i] || Text.IsEmpty()) continue;
				const int32 Par = S->CosmosParent(i);
				if (Par >= 0)
				{
					// hidden behind its parent planet (seen through it)?
					const FVector Pp = S->CosmosPos(Par), Bp = S->CosmosPos(i);
					const float PR = S->CosmosRadius(Par);
					const FVector D = (Bp - Cam).GetSafeNormal();
					const double T = FVector::DotProduct(Pp - Cam, D);
					if (T > 0.0 && T < FVector::Dist(Cam, Bp) && FVector::DistSquared(Cam + D * T, Pp) < (double)PR * PR) continue;
				}
				// hide moons and spacecraft until their parent is far enough apart on screen
				if (Par >= 0 && S->CosmosParent(Par) >= 0 && On[Par] && FVector2f::Distance(Scr[i], Scr[Par]) < 46.f && i != Sel) continue;
				if (Par >= 0 && S->CosmosParent(Par) < 0 && On[Par] && FVector2f::Distance(Scr[i], Scr[Par]) < 22.f && i != Sel) continue;
				const float Dist = FMath::Max(1.f, (float)FVector::Dist(Cam, S->CosmosPos(i)));
				const float Rpx = S->CosmosRadius(i) / Dist * (float)VP.X * 1.072f * K.X;   // focal length for a 50 deg horizontal FOV
				const FLinearColor C = FLinearColor(S->CosmosLabelColor(i)) * (i == Sel ? 1.f : 0.85f);
				if (Rpx < 5.f)
				{
					TArray<FVector2f> Ring;
					for (int32 k = 0; k <= 16; ++k) { const float A = 2.f * PI * k / 16; Ring.Add(Scr[i] + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 6.f); }
					FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Ring, ESlateDrawEffect::None, C, true, i == Sel ? 2.f : 1.2f);
				}
				const FVector2f Sz = FVector2f(FM->Measure(Text, Font));
				const FVector2f At = Scr[i] - FVector2f(Sz.X * 0.5f, FMath::Max(Rpx, 6.f) + 6.f + Sz.Y);
				const FSlateRect Box(At.X - 3, At.Y - 2, At.X + Sz.X + 3, At.Y + Sz.Y + 2);
				bool bOverlap = false;
				for (const FSlateRect& Rc : Taken) if (FSlateRect::DoRectanglesIntersect(Rc, Box)) { bOverlap = true; break; }
				if (bOverlap) continue;
				Taken.Add(Box);
				FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(Sz, FSlateLayoutTransform(At + FVector2f(1.f, 1.f))), Text, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.85f));
				FSlateDrawElement::MakeText(Out, Layer + 2, G.ToPaintGeometry(Sz, FSlateLayoutTransform(At)), Text, Font, ESlateDrawEffect::None, C);
				Hits.Add(TPair<int32, FSlateRect>(i, FSlateRect(At.X - 4, At.Y - 2, At.X + Sz.X + 4, Scr[i].Y + 8)));
			}
			return Layer + 3;
		}
	private:
		mutable TArray<TPair<int32, FSlateRect>> Hits;
	};
}

TSharedRef<SWidget> SISGameUI::BuildCosmos()
{
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	const int32 N = Scene.IsValid() ? Scene->CosmosCount() : 0;
	const int32 Sel = Scene.IsValid() ? Scene->CosmosFocusIdx() : 0;
	// tree order: every body directly followed by its moons / spacecraft
	TArray<int32> Order;
	{
		TFunction<void(int32)> Visit = [&](int32 P)
		{
			for (int32 c = 0; c < N; ++c)
				if (Scene->CosmosParent(c) == P) { Order.Add(c); Visit(c); }
		};
		Visit(-1);
	}
	FString LastGroup;
	for (const int32 i : Order)
	{
		const FString Grp = Scene->CosmosGroup(i);
		if (Grp != LastGroup)
		{
			List->AddSlot().AutoHeight().Padding(0, LastGroup.IsEmpty() ? 0 : 10, 0, 3)[Txt(Grp.ToUpper(), 13, CAccent, true)];
			LastGroup = Grp;
		}
		List->AddSlot().AutoHeight().Padding(FMath::Min(3, Scene->CosmosDepth(i)) * 14.f, 1, 0, 1)
			[Btn(Scene->CosmosName(i), [this, i]() { if (Scene.IsValid()) Scene->FocusCosmos(i); bDirty = true; }, i == Sel ? 2 : 0, 12)];
	}
	const float Rate = Scene.IsValid() ? Scene->GetCosmosRate() : 1.f;
	TSharedRef<SHorizontalBox> Speeds = SNew(SHorizontalBox);
	struct FSpd { const TCHAR* L; float R; } Spd[] = { { TEXT("||"), 0.f }, { TEXT("1 day/s"), 1.f }, { TEXT("1 month/s"), 30.f }, { TEXT("1 year/s"), 365.f } };
	for (const FSpd& S : Spd)
	{
		const float R = S.R;
		Speeds->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Btn(S.L, [this, R]() { if (Scene.IsValid()) Scene->SetCosmosRate(R); bDirty = true; }, FMath::IsNearlyEqual(Rate, R) ? 2 : 0, 12)];
	}
	Speeds->AddSlot().AutoWidth()[Btn(Scene.IsValid() && Scene->CosmosLabelsOn() ? TEXT("Labels: on") : TEXT("Labels: off"), [this]() { if (Scene.IsValid()) Scene->ToggleCosmosLabels(); bDirty = true; }, 0, 12)];
	const FString Name = Scene.IsValid() ? Scene->CosmosName(Sel) : FString();
	const FString Facts = Scene.IsValid() ? Scene->CosmosFacts(Sel) : FString();
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SISCosmosOverlay, Scene, PC, [this](int32 i) { if (Scene.IsValid()) Scene->FocusCosmos(i); bDirty = true; })]
		+ SOverlay::Slot()[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12)[BuildTopBar(TEXT("COSMOS EXPLORER  -  click a world to fly there"))]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 0, 12, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(320)[Panel(SAssignNew(CosmosBox, SScrollBox) + SScrollBox::Slot()[List])]]
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
			[
				SNew(SBox).WidthOverride(520)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[Txt(Name, 18, CAccent, true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)[Txt(Facts, 13, CText)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)[Txt(TEXT("Time speed"), 12, CDim)]
						+ SVerticalBox::Slot().AutoHeight()[Speeds]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[Txt(TEXT("Drag to look around, mouse wheel to zoom. Distances are squeezed so everything fits; sizes, tilts, spins and periods of real worlds are true to life. Worlds marked FICTIONAL are invented but follow real physics."), 11, CDim)], 16.f)
				]
			]
		]];
}
