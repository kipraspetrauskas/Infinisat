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
	}
	if (Bit) { Bit->SetActorHiddenInGame(false); SetViewTarget(Bit); }
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
	Bit->LookInput(MX * 0.6f, -MY * 0.6f);
	if (WasInputKeyJustPressed(EKeys::SpaceBar)) Bit->Hover();
	if (WasInputKeyJustReleased(EKeys::SpaceBar)) Bit->StopJumping();

	// what are we looking at?
	LookAction = NAME_None;
	CurrentPrompt.Reset();
	if (UPrimitiveComponent* C = Bit->TraceView(320.f))
	{
		LookAction = World->ActionFor(C);
		CurrentPrompt = World->PromptFor(LookAction);
	}
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
	if (WasInputKeyJustPressed(EKeys::Escape)) { EnterMenu(); if (UI) UI->ShowTitle(); }
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
