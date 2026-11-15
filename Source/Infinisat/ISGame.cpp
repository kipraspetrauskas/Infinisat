#include "ISGame.h"
#include "ISCareer.h"
#include "ISEarth.h"
#include "Kismet/GameplayStatics.h"
#include "Infinisat.h"
#include "ISScene.h"
#include "ISWorld.h"
#include "ISBit.h"
#include "SISGameUI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Camera/CameraActor.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/PlayerCameraManager.h"
#include "Widgets/SWeakWidget.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "ISParts3D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

AISGameMode::AISGameMode()
{
	PlayerControllerClass = AISPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

AISPlayerController::AISPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bAutoManageActiveCameraTarget = false;
}

void AISPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	ISDemo::Init(this);   // before the world is spawned: its sky reads the demo look

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World = GetWorld()->SpawnActor<AISHomeWorld>(AISHomeWorld::StaticClass(), FTransform::Identity, SP);
	Scene = GetWorld()->SpawnActor<AISSpaceScene>(AISSpaceScene::StaticClass(), FTransform::Identity, SP);
	Earth = GetWorld()->SpawnActor<AISEarth>(AISEarth::StaticClass(), FTransform::Identity, SP);
	if (Scene) Scene->Earth = Earth;
	if (World) World->Earth = Earth;

	if (GEngine && GEngine->GameViewport)
	{
		UI = SNew(SISGameUI).Scene(Scene).PC(this);
		UIContainer = SNew(SWeakWidget).PossiblyNullContent(UI.ToSharedRef());
		GEngine->GameViewport->AddViewportWidgetContent(UIContainer.ToSharedRef(), 10);
	}
	if (GConfig)
	{
		GConfig->GetFloat(TEXT("Infinisat"), TEXT("MouseSens"), MouseSens, GGameUserSettingsIni);
		GConfig->GetBool(TEXT("Infinisat"), TEXT("InvertY"), bInvertY, GGameUserSettingsIni);
		GConfig->GetFloat(TEXT("Infinisat"), TEXT("Fov"), Fov, GGameUserSettingsIni);
		GConfig->GetInt(TEXT("Infinisat"), TEXT("Graphics"), Graphics, GGameUserSettingsIni);
		// settings v2: Epic used to be auto-picked on fast GPUs and is far heavier than the look needs -> move to High once
		int32 Ver = 1;
		GConfig->GetInt(TEXT("Infinisat"), TEXT("SettingsVersion"), Ver, GGameUserSettingsIni);
		if (Ver < 2)
		{
			if (Graphics > 2) Graphics = 2;
			GConfig->SetInt(TEXT("Infinisat"), TEXT("SettingsVersion"), 2, GGameUserSettingsIni);
			GConfig->SetInt(TEXT("Infinisat"), TEXT("Graphics"), Graphics, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("ISGraphics="), Graphics)) {}
	if (Graphics < 0)
	{
		// first launch: measure this PC and choose a preset (integrated GPUs land on Low)
		if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			GS->RunHardwareBenchmark();
			Graphics = FMath::Clamp(GS->GetRecommendedResolutionScale() < 60.f ? 0 : (int32)GS->ScalabilityQuality.GetSingleQualityLevel(), 0, 2);   // Epic is opt-in
		}
		else Graphics = 2;
	}
	ISQuality::Level = Graphics;
	ApplyGraphics();
	EnterMenu();
	const bool bWorldArg = FParse::Param(FCommandLine::Get(), TEXT("ISWorld")) || ISDemo::Map == TEXT("Demo_Backyard");
	UE_LOG(LogInfinisat, Log, TEXT("PC BeginPlay: world=%d scene=%d ui=%d arg=%d"), World != nullptr, Scene != nullptr, UI.IsValid(), bWorldArg);
	if (bWorldArg || ISCareer::bEnterWorldOnLoad) { ISCareer::bEnterWorldOnLoad = false; EnterWorld(); }
	double OvA = 0.0;   // dev: -ISOverview=metres starts with the camera lifted above the site
	if (bWorldArg && FParse::Value(FCommandLine::Get(), TEXT("ISOverview="), OvA)) EnterOverview(OvA);
	int32 DevJob = -1;   // dev: -ISJob=N starts orbital order N as a job in the world
	if (bWorldArg && UI && FParse::Value(FCommandLine::Get(), TEXT("ISJob="), DevJob)) UI->DevStartJob(DevJob);
}

void AISPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GEngine && GEngine->GameViewport && UIContainer.IsValid())
		GEngine->GameViewport->RemoveViewportWidgetContent(UIContainer.ToSharedRef());
	UI.Reset();
	UIContainer.Reset();
	Super::EndPlay(Reason);
}

void AISPlayerController::SetCursorMode(bool bCursor)
{
	bShowMouseCursor = bCursor;
	if (bCursor)
	{
		FInputModeGameAndUI M;
		M.SetHideCursorDuringCapture(false);
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(M);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AISPlayerController::EnterMenu()
{
	UE_LOG(LogInfinisat, Log, TEXT("EnterMenu"));
	Mode = EISMode::Menu;
	if (World) World->SetActive(false);
	if (Earth) Earth->SetVisible(false);   // the space views switch it back on where the real Earth is in view
	if (Bit) Bit->SetActorHiddenInGame(true);
	if (Scene) { Scene->SetActive(true); if (Scene->GetCamera()) SetViewTarget(Scene->GetCamera()); }
	SetCursorMode(true);
}

void AISPlayerController::EnterWorld()
{
	if (Scene) Scene->SetActive(false);
	if (World) World->SetActive(true);
	if (Earth) { Earth->UseSiteOrigin(); Earth->SetVisible(true); }
	if (!Bit && World)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Bit = GetWorld()->SpawnActor<AISBitCharacter>(AISBitCharacter::StaticClass(), World->SpawnPoint(), SP);
		Possess(Bit);
		SetControlRotation(World->SpawnPoint().Rotator());
		// dev: -ISPos=x,y,z -ISYaw=deg -ISPitch=deg for screenshots
		FString PosS;
		if (FParse::Value(FCommandLine::Get(), TEXT("ISPos="), PosS, false))
		{
			TArray<FString> P; PosS.ParseIntoArray(P, TEXT(","));
			if (P.Num() == 3) Bit->SetActorLocation(FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2])), false, nullptr, ETeleportType::TeleportPhysics);
		}
		float Yaw = 0, Pitch = 0;
		FRotator CR = GetControlRotation();
		if (FParse::Value(FCommandLine::Get(), TEXT("ISYaw="), Yaw)) CR.Yaw = Yaw;
		if (FParse::Value(FCommandLine::Get(), TEXT("ISPitch="), Pitch)) CR.Pitch = Pitch;
		SetControlRotation(CR);
		if (FParse::Param(FCommandLine::Get(), TEXT("ISFly")))
		{
			Bit->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			Bit->GetCharacterMovement()->GravityScale = 0.f;
			Bit->SetActorEnableCollision(false);
		}
	}
	if (Bit) { Bit->SetActorHiddenInGame(false); SetViewTarget(Bit); }
	ApplySettings();
	// dev: -ISTop=x,y,z looks straight down from that point (layout checks)
	FString TopS;
	if (FParse::Value(FCommandLine::Get(), TEXT("ISTop="), TopS, false))
	{
		TArray<FString> P; TopS.ParseIntoArray(P, TEXT(","));
		if (P.Num() == 3)
		{
			FActorSpawnParameters SP2;
			ACameraActor* Top = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(FRotator(-90, 90, 0), FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]))), SP2);
			Top->GetCameraComponent()->SetFieldOfView(70.f);
			Top->GetCameraComponent()->bConstrainAspectRatio = false;
			SetViewTarget(Top);
			Mode = EISMode::Panel;
		}
	}
	UE_LOG(LogInfinisat, Log, TEXT("EnterWorld: bit=%d"), Bit != nullptr);
	Mode = EISMode::Walk;
	SetCursorMode(false);
	// dev: -ISCam=x,y,z,pitch,yaw[,fov] fixed camera for look comparisons (same frame every run)
	FString CamS;
	if (FParse::Value(FCommandLine::Get(), TEXT("ISCam="), CamS, false))
	{
		TArray<FString> P; CamS.ParseIntoArray(P, TEXT(","));
		if (P.Num() >= 5)
		{
			const FVector L(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]));
			ACameraActor* Fixed = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(FRotator(FCString::Atof(*P[3]), FCString::Atof(*P[4]), 0), L));
			Fixed->GetCameraComponent()->SetFieldOfView(P.Num() >= 6 ? FCString::Atof(*P[5]) : 70.f);
			Fixed->GetCameraComponent()->bConstrainAspectRatio = false;
			SetViewTarget(Fixed);
		}
	}
	if (PlayerCameraManager) PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.2f, FLinearColor::Black, false, false);
}

void AISPlayerController::EnterBench()
{
	if (!World || !World->BenchCamera()) return;
	Mode = EISMode::Bench;
	SetViewTargetWithBlend(World->BenchCamera(), 0.6f, VTBlend_EaseInOut, 2.f);
	SetCursorMode(true);
}

void AISPlayerController::LeaveBench()
{
	Mode = EISMode::Walk;
	if (Bit) SetViewTargetWithBlend(Bit, 0.5f, VTBlend_EaseInOut, 2.f);
	SetCursorMode(false);
}

void AISPlayerController::OpenPanel()
{
	Mode = EISMode::Panel;
	SetCursorMode(true);
}

void AISPlayerController::OpenPause()
{
	OpenPanel();
	if (UI) UI->ShowPause();
}

void AISPlayerController::ApplySettings()
{
	if (Bit && Bit->Cam) Bit->Cam->SetFieldOfView(Fov);
}

void AISPlayerController::SaveSettings()
{
	ApplySettings();
	if (!GConfig) return;
	GConfig->SetFloat(TEXT("Infinisat"), TEXT("MouseSens"), MouseSens, GGameUserSettingsIni);
	GConfig->SetBool(TEXT("Infinisat"), TEXT("InvertY"), bInvertY, GGameUserSettingsIni);
	GConfig->SetFloat(TEXT("Infinisat"), TEXT("Fov"), Fov, GGameUserSettingsIni);
	GConfig->SetInt(TEXT("Infinisat"), TEXT("Graphics"), Graphics, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

FString AISPlayerController::GraphicsName(int32 L)
{
	switch (L)
	{
	case 0: return TEXT("Low (low-poly)");
	case 1: return TEXT("Medium");
	case 2: return TEXT("High");
	default: return TEXT("Epic");
	}
}

void AISPlayerController::ApplyGraphics()
{
	Graphics = FMath::Clamp(Graphics, 0, 3);
	ISQuality::Level = Graphics;
	if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GS->SetOverallScalabilityLevel(Graphics);
		GS->ApplySettings(false);
	}
	auto CV = [](const TCHAR* Name, float V)
	{
		if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) C->Set(V, ECVF_SetByGameSetting);
	};
	// Presets sized for this game's look (simple shapes, flat colours): the expensive UE5 features (Lumen, virtual
	// shadow maps) only on Epic. Measured on an RTX 4070 at 1440p in the house: Epic was 21.6 ms GPU for a low-poly room.
	const bool bLow = Graphics == 0, bEpic = Graphics >= 3;
	CV(TEXT("r.Nanite"), bLow ? 0.f : 1.f);                       // Nanite off = simplified fallback meshes (the low-poly look)
	CV(TEXT("r.MipMapLODBias"), bLow ? 2.f : 0.f);                 // chunkier textures, far less memory
	CV(TEXT("r.DynamicGlobalIlluminationMethod"), bEpic ? 1.f : 0.f);   // Lumen GI only on Epic
	CV(TEXT("r.SSGI.Enable"), (Graphics == 1 || Graphics == 2) ? 1.f : 0.f);   // cheap screen-space bounce light instead
	CV(TEXT("r.ReflectionMethod"), bEpic ? 1.f : (bLow ? 0.f : 2.f)); // Lumen reflections on Epic, screen-space otherwise
	CV(TEXT("r.Shadow.Virtual.Enable"), bEpic ? 1.f : 0.f);       // virtual shadow maps only on Epic
	CV(TEXT("r.ScreenPercentage"), bLow ? 70.f : (Graphics == 1 ? 80.f : (Graphics == 2 ? 85.f : 100.f)));   // TSR upscales the rest
	CV(TEXT("r.ViewDistanceScale"), bLow ? 0.6f : 1.f);
	CV(TEXT("r.VolumetricCloud"), Graphics >= 2 ? 1.f : 0.f);
	CV(TEXT("r.Fog"), 1.f);
	CV(TEXT("t.MaxFPS"), 120.f);                                   // no point burning the GPU at 300 fps in a menu
	if (World) World->ApplyLightQuality();
}

void AISPlayerController::ClosePanel()
{
	Mode = EISMode::Walk;
	if (Bit) SetViewTarget(Bit);
	SetCursorMode(false);
}

void AISPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	switch (Mode)
	{
	case EISMode::Walk: TickWalk(DeltaTime); break;
	case EISMode::Bench: TickBench(DeltaTime); break;
	case EISMode::Overview: TickOverview(DeltaTime); break;
	case EISMode::Menu:
	{
		if (!Scene) break;
		if (Scene->GetCamera() && GetViewTarget() != Scene->GetCamera()) SetViewTarget(Scene->GetCamera());
		const bool bDown = IsInputKeyDown(EKeys::LeftMouseButton) || IsInputKeyDown(EKeys::RightMouseButton);
		float X = 0, Y = 0;
		if (bDown && GetMousePosition(X, Y))
		{
			const FVector2D Now(X, Y);
			if (bDragging) { const FVector2D D = Now - LastMouse; Scene->OrbitCamera(D.X * 0.25f, -D.Y * 0.25f); }
			LastMouse = Now; bDragging = true;
		}
		else bDragging = false;
		if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) Scene->ZoomCamera(1.f);
		if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) Scene->ZoomCamera(-1.f);
		// flying the launch: SPACE launch/stage, T autopilot, SHIFT/CTRL throttle, A/D pitch, X cut, comma/period warp
		if (FISRocketSim* F = Scene->Flight())
		{
			if (WasInputKeyJustPressed(EKeys::SpaceBar)) F->Stage();
			if (WasInputKeyJustPressed(EKeys::T)) F->bAutopilot = !F->bAutopilot;
			const float Dt = DeltaTime;
			const bool bThrUp = IsInputKeyDown(EKeys::LeftShift), bThrDn = IsInputKeyDown(EKeys::LeftControl), bCut = WasInputKeyJustPressed(EKeys::X);
			const float Pitch = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
			if (bThrUp || bThrDn || bCut) F->bManualThrottle = true;   // throttle keys take only the throttle
			if (Pitch != 0.f && F->bAutopilot) { F->bAutopilot = false; F->ManualOffsetDeg = 0.0; }   // steering keys: stability assist keeps the nose near the flight path
			if (bThrUp) F->Throttle = FMath::Min(1.0, F->Throttle + Dt * 0.6);
			if (bThrDn) F->Throttle = FMath::Max(0.0, F->Throttle - Dt * 0.6);
			if (bCut) F->Throttle = 0.0;
			if (Pitch != 0.f) F->ManualOffsetDeg = FMath::Clamp(F->ManualOffsetDeg + Pitch * Dt * 8.0, -30.0, 30.0);
			if (WasInputKeyJustPressed(EKeys::Period)) Scene->SetFlightWarp(Scene->GetFlightWarp() * 2.f);
			if (WasInputKeyJustPressed(EKeys::Comma)) Scene->SetFlightWarp(Scene->GetFlightWarp() * 0.5f);
		}
		break;
	}
	default: break;
	}
}

void AISPlayerController::TickWalk(float Dt)
{
	if (!Bit || !World) return;
	if (CanTestStep >= 0 || (CanTestStep == -1 && FParse::Param(FCommandLine::Get(), TEXT("ISCanTest")))) { TickCanTest(Dt); }
	const float F = (IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float R = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	Bit->MoveInput(F, R, IsInputKeyDown(EKeys::LeftShift));
	float MX = 0, MY = 0;
	GetInputMouseDelta(MX, MY);
	Bit->LookInput(MX * MouseSens, MY * MouseSens * (bInvertY ? -1.f : 1.f));
	if (WasInputKeyJustPressed(EKeys::SpaceBar)) Bit->Hover();
	if (WasInputKeyJustReleased(EKeys::SpaceBar)) Bit->StopJumping();
	const bool bLow = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::C);
	if (bLow && !Bit->bIsCrouched) Bit->Crouch();
	if (!bLow && Bit->bIsCrouched) Bit->UnCrouch();

	// what are we looking at?
	LookAction = NAME_None;
	CurrentPrompt.Reset();
	UPrimitiveComponent* LookComp = Bit->TraceView(320.f);
	if (LookComp)
	{
		LookAction = World->ActionFor(LookComp);
		CurrentPrompt = World->PromptFor(LookAction);
	}
	World->SetHighlight(LookAction, LookAction.IsNone() ? nullptr : LookComp);   // yellow outline on the real object
	if (WasInputKeyJustPressed(EKeys::Q)) World->Scan();
	// mouse wheel: rise above the site (and on up to orbit)
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown) && !World->IsTracking()) { EnterOverview(80.0); return; }
	if (WasInputKeyJustPressed(EKeys::E) && !LookAction.IsNone())
	{
		if (LookAction == "sc_orders" && World->JobPhase == 3) { World->JobPhase = 0; World->Toast(FString::Printf(TEXT("Report filed for %s. The customer has the data; new orders are on the board."), *World->JobTitle)); }
		if (LookAction == "laptop" || LookAction == "sc_orders") { OpenPanel(); if (UI) UI->ShowOrders(); }
		else if (LookAction == "sc_design" || LookAction == "sc_lab" || LookAction == "sc_track") { Bit->Reach(); EnterMenu(); if (UI) UI->OpenBuilding(LookAction); }
		else if (LookAction == "bench" && World->Stage == EISStage::Build) { Bit->Reach(); EnterBench(); }
		else World->Interact(LookAction, Bit);
	}

	// time control while the CanSat flies: comma slower (down to 1/8), period faster (up to 8x); normal again after landing
	if (World->IsTracking() || World->Stage == EISStage::Countdown)
	{
		const float Cur = UGameplayStatics::GetGlobalTimeDilation(this);
		if (WasInputKeyJustPressed(EKeys::Period)) UGameplayStatics::SetGlobalTimeDilation(this, FMath::Min(8.f, Cur * 2.f));
		if (WasInputKeyJustPressed(EKeys::Comma)) UGameplayStatics::SetGlobalTimeDilation(this, FMath::Max(0.125f, Cur * 0.5f));
		// TAB held: fast-forward 8x;  N: skip straight to the landing (the flight is still simulated step by step)
		if (WasInputKeyJustPressed(EKeys::Tab)) UGameplayStatics::SetGlobalTimeDilation(this, 8.f);
		if (WasInputKeyJustReleased(EKeys::Tab)) UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		if (WasInputKeyJustPressed(EKeys::N) && World->Stage <= EISStage::Descent) World->SkipFlight();
	}
	else if (!FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(this), 1.f) && !FParse::Param(FCommandLine::Get(), TEXT("ISCanTest"))) UGameplayStatics::SetGlobalTimeDilation(this, 1.f);

	// chase camera while the CanSat flies
	if (WasInputKeyJustPressed(EKeys::F) && World->IsTracking())
	{
		World->bChaseCam = !World->bChaseCam;
		SetViewTargetWithBlend(World->bChaseCam ? (AActor*)World->ChaseCamera() : (AActor*)Bit, 0.4f);
	}
	if (World->bChaseCam && !World->IsTracking()) { World->bChaseCam = false; SetViewTarget(Bit); }
	if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::P)) OpenPause();
	if (World->bDebriefReady) { World->bDebriefReady = false; OpenPanel(); if (UI) UI->ShowCanSatDebrief(); }
}

void AISPlayerController::TickBench(float Dt)
{
	if (!World) return;
	if (World->bUIDrag) { if (WasInputKeyJustPressed(EKeys::Escape)) { World->bUIDrag = false; World->EndDrag(false); } return; }   // the UI drives drags started from a part card
	const bool bLmb = IsInputKeyDown(EKeys::LeftMouseButton);
	if (World->DragPart.IsNone())
	{
		FHitResult H;
		if (GetHitResultUnderCursor(ECC_Visibility, false, H))
		{
			const FName Part = World->PartUnderCursor(H.GetComponent());
			if (!Part.IsNone()) World->BenchInfoPart = Part;
			// press on a part on the bench (or in the can) picks it up
			if (WasInputKeyJustPressed(EKeys::LeftMouseButton) && !Part.IsNone()) { World->BeginDrag(Part); DragT = 0.f; }
		}
	}
	else
	{
		DragT += Dt;
		FVector S, D;
		if (DeprojectMousePositionToWorld(S, D)) World->UpdateDrag(S, D);
		if (!bLmb)
		{
			// a quick click (no drag) on a bench part = put it where it usually goes
			const FName Id = World->DragPart;
			const bool bClick = DragT < 0.25f && World->HoverSlot < 0 && !World->Placed.Contains(Id);
			if (bClick) { World->EndDrag(false); World->BenchPlace(Id, AISHomeWorld::IdealSlot(Id)); }
			else World->EndDrag(true);
		}
		if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::RightMouseButton)) World->EndDrag(false);
		return;
	}
	if (WasInputKeyJustPressed(EKeys::G)) World->bGuide = !World->bGuide;
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Escape)) LeaveBench();
}

// Dev check of the full CanSat chain: order -> find parts -> build -> seal -> carry -> load -> launch -> land -> recover.
void AISPlayerController::TickCanTest(float Dt)
{
	if (CanTestStep == -1) CanTestStep = 0;
	if (CanTestStep < 0 || !World || !Bit) return;
	CanTestT += Dt;
	auto Log = [this](const FString& S) { UE_LOG(LogTemp, Warning, TEXT("[CANTEST] t=%.1f stage=%d  %s"), CanTestT, (int32)World->Stage, *S); };
	switch (CanTestStep)
	{
	case 0:
		if (CanTestT < 2.f) return;
		World->StartCanSatOrder(0);
		for (const FISHuntItem& H : TArray<FISHuntItem>(World->Hunt)) World->Interact(FName(*(TEXT("item:") + H.Part.ToString())), Bit);
		Log(FString::Printf(TEXT("order taken, parts found %d/%d"), World->FoundCount(true), World->RequiredCount()));
		CanTestStep = 1; return;
	case 1:
		if (FParse::Param(FCommandLine::Get(), TEXT("ISBenchEmpty"))) { EnterBench(); CanTestStep = 99; return; }   // dev: all parts found, empty can, play the bench by hand
		for (const char* Id : { "bat_aa", "cpu_ard", "com_433", "pl_temp", "ex_wires", "ex_chute_l" }) World->BenchToggle(FName(Id));
		Log(FString::Printf(TEXT("built: %.0f g, stack %.1f cm"), World->Eval.TotalKg * 1000.f, World->StackHeightCm));
		if (FParse::Param(FCommandLine::Get(), TEXT("ISCanTestBench"))) { EnterBench(); CanTestStep = 99; return; }   // stay at the bench for a screenshot
		World->BenchSeal();
		Log(World->Stage == EISStage::Sealed ? TEXT("sealed OK") : TEXT("SEAL FAILED"));
		CanTestStep = 2; return;
	case 2:
		World->Interact("cansat", Bit);
		Log(World->Stage == EISStage::Carry ? TEXT("picked up OK") : TEXT("PICK-UP FAILED"));
		Bit->SetActorLocation(World->PadWorld() + FVector(-250, 0, 120), false, nullptr, ETeleportType::TeleportPhysics);
		World->Interact("pad", Bit);
		Log(World->Stage == EISStage::OnPad ? TEXT("loaded into the rocket OK") : TEXT("LOADING FAILED"));
		World->Interact("pad", Bit);
		Log(World->Stage == EISStage::Countdown ? TEXT("countdown OK") : TEXT("COUNTDOWN FAILED"));
		CanTestStep = 3; return;
	case 3:
		// -ISCanTestStay: skip the flight to the landing and stay on the debrief (screenshot of the career payment)
		if (FParse::Param(FCommandLine::Get(), TEXT("ISCanTestStay")) && World->Stage >= EISStage::Boost && World->Stage <= EISStage::Descent) World->SkipFlight();
		if (World->Stage == EISStage::Landed)
		{
			Log(FString::Printf(TEXT("landed: apogee %.0f m, landing %.1f m/s, drift %.0f m, broken %d"), World->Report.ApogeeM, World->Report.LandingSpeed, World->Report.DriftM, (int32)World->Report.bBroken));
			World->Interact("landed", Bit);
			Log(World->Stage == EISStage::Recovered ? TEXT("PASS: recovered") : TEXT("RECOVERY FAILED"));
			CanTestStep = 4;
		}
		else if (CanTestT > 900.f) { Log(TEXT("FAIL: never landed")); CanTestStep = 4; }
		return;
	case 4:
		if (FParse::Param(FCommandLine::Get(), TEXT("ISCanTestStay"))) { Log(TEXT("career: ") + ISCareer::StatusLine()); CanTestStep = 5; return; }
		ConsoleCommand(TEXT("quit"));
		CanTestStep = 5; return;
	default: return;
	}
}

// ------------------------------------------------------------------------------------------------ overview
void AISPlayerController::EnterOverview(double AltM)
{
	if (!Bit) return;
	if (!OverCam)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		OverCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
		OverCam->GetCameraComponent()->bConstrainAspectRatio = false;
		OverCam->GetCameraComponent()->SetFieldOfView(60.f);
	}
	OverAnchor = Bit->GetActorLocation();
	OverYaw = GetControlRotation().Yaw;
	OverAlt = FMath::Max(AltM * 0.5, 20.0);
	OverAltGoal = AltM;
	Mode = EISMode::Overview;
	TickOverview(0.f);
	SetViewTargetWithBlend(OverCam, AltM > 1000.0 ? 0.f : 0.5f, VTBlend_EaseInOut, 2.f);
	SetCursorMode(false);
}

void AISPlayerController::TickOverview(float Dt)
{
	if (!OverCam) return;
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) OverAltGoal = FMath::Min(OverAltGoal * 1.8, 400000.0);
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) OverAltGoal = OverAltGoal / 1.8;
	float MX = 0, MY = 0;
	GetInputMouseDelta(MX, MY);
	OverYaw += MX * MouseSens * 0.5f;
	// smooth in log space: every notch feels the same from 50 m to 150 km
	OverAlt = FMath::Exp(FMath::FInterpTo(FMath::Loge(FMath::Max(OverAlt, 1.0)), FMath::Loge(FMath::Max(OverAltGoal, 1.0)), (double)Dt, 4.0));
	// back on the ground
	if (OverAltGoal < 35.0 || WasInputKeyJustPressed(EKeys::Escape))
	{
		Mode = EISMode::Walk;
		if (Bit) SetViewTargetWithBlend(Bit, 0.5f, VTBlend_EaseInOut, 2.f);
		return;
	}
	// low: looking at the site at an angle; high: straight down, like a satellite
	const float T = FMath::Clamp((float)FMath::Loge(OverAlt / 60.0) / FMath::Loge(150000.0 / 60.0), 0.f, 1.f);
	const float Pitch = FMath::Lerp(-35.f, -89.f, FMath::SmoothStep(0.f, 1.f, T));
	const FRotator R(Pitch, OverYaw, 0.f);
	const double Dist = OverAlt * 100.0 / FMath::Max(0.05, FMath::Sin(FMath::DegreesToRadians(-Pitch)));
	OverCam->SetActorLocationAndRotation(OverAnchor - R.Vector() * Dist, R);
	// above ~150 km the globe view (same real Earth, Earth-centred) takes over; the solar system is further out
	if (OverAlt > 140000.0 && OverAltGoal > 140000.0)
	{
		EnterMenu();
		if (UI) UI->OpenGlobe(OverAlt);
	}
}
