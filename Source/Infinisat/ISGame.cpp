#include "ISGame.h"
#include "Infinisat.h"
#include "ISScene.h"
#include "ISWorld.h"
#include "ISBit.h"
#include "SISGameUI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Camera/CameraActor.h"
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

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World = GetWorld()->SpawnActor<AISHomeWorld>(AISHomeWorld::StaticClass(), FTransform::Identity, SP);
	Scene = GetWorld()->SpawnActor<AISSpaceScene>(AISSpaceScene::StaticClass(), FTransform::Identity, SP);

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
	const bool bWorldArg = FParse::Param(FCommandLine::Get(), TEXT("ISWorld"));
	UE_LOG(LogInfinisat, Log, TEXT("PC BeginPlay: world=%d scene=%d ui=%d arg=%d"), World != nullptr, Scene != nullptr, UI.IsValid(), bWorldArg);
	if (bWorldArg) EnterWorld();
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
	if (Bit) Bit->SetActorHiddenInGame(true);
	if (Scene) { Scene->SetActive(true); if (Scene->GetCamera()) SetViewTarget(Scene->GetCamera()); }
	SetCursorMode(true);
}

void AISPlayerController::EnterWorld()
{
	if (Scene) Scene->SetActive(false);
	if (World) World->SetActive(true);
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
			if (bThrUp || bThrDn || bCut || Pitch != 0.f) F->bAutopilot = false;   // touching the controls takes over from the autopilot
			if (bThrUp) F->Throttle = FMath::Min(1.0, F->Throttle + Dt * 0.6);
			if (bThrDn) F->Throttle = FMath::Max(0.0, F->Throttle - Dt * 0.6);
			if (bCut) F->Throttle = 0.0;
			if (Pitch != 0.f) F->PitchDeg = FMath::Clamp(F->PitchDeg + Pitch * Dt * 6.0, -10.0, 120.0);
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
	if (UPrimitiveComponent* C = Bit->TraceView(320.f))
	{
		LookAction = World->ActionFor(C);
		CurrentPrompt = World->PromptFor(LookAction);
	}
	if (WasInputKeyJustPressed(EKeys::Q)) World->Scan();
	if (WasInputKeyJustPressed(EKeys::E) && !LookAction.IsNone())
	{
		if (LookAction == "laptop") { OpenPanel(); if (UI) UI->ShowOrders(); }
		else if (LookAction == "bench" && World->Stage == EISStage::Build) { Bit->Reach(); EnterBench(); }
		else World->Interact(LookAction, Bit);
	}

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
	FHitResult H;
	if (GetHitResultUnderCursor(ECC_Visibility, false, H))
	{
		const FName Part = World->PartUnderCursor(H.GetComponent());
		if (!Part.IsNone()) World->BenchInfoPart = Part;
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton) && !Part.IsNone()) World->BenchToggle(Part);
	}
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Escape)) LeaveBench();
}
