// Infinisat - game mode and player controller. The controller owns the mode switching:
// Menu (space backdrop) <-> Walk (first-person BIT in the open world) <-> Bench (build view) <-> UI panels.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ISGame.generated.h"

class AISSpaceScene;
class AISHomeWorld;
class AISBitCharacter;
class SISGameUI;

enum class EISMode : uint8 { Menu, Walk, Bench, Panel, Overview };   // Overview: the camera rising above the site (wheel), up to the globe

UCLASS()
class AISGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AISGameMode();
};

UCLASS()
class AISPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AISPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float DeltaTime) override;

	void EnterMenu();          // space backdrop + Slate menus
	void EnterWorld();         // first-person BIT, fade in
	void EnterBench();
	void LeaveBench();
	float DragT = 0.f;   // how long the current bench drag has lasted (short = a click)
	void OpenPanel();          // world stays, cursor for a UI panel (orders, debrief)
	void ClosePanel();         // back to walking
	EISMode GetMode() const { return Mode; }
	const FString& Prompt() const { return CurrentPrompt; }
	void OpenPause();
	void EnterOverview(double AltM = 80.0);   // lift the camera above the site
	double OverviewAltM() const { return OverAlt; }
	void SaveSettings();
	void ApplySettings();

	// player settings (saved in GameUserSettings.ini)
	float MouseSens = 1.3f;
	bool bInvertY = false;
	float Fov = 90.f;
	int32 Graphics = -1;      // 0 Low (low-poly, no GPU needed), 1 Medium, 2 High, 3 Epic
	void ApplyGraphics();
	static FString GraphicsName(int32 L);

	UPROPERTY() AISSpaceScene* Scene = nullptr;
	UPROPERTY() class AISEarth* Earth = nullptr;   // the real globe (Cesium), shared by the walking world and the space views
	UPROPERTY() AISHomeWorld* World = nullptr;
	UPROPERTY() AISBitCharacter* Bit = nullptr;
	TSharedPtr<SISGameUI> UI;

private:
	void SetCursorMode(bool bCursor);
	void TickWalk(float Dt);
	void TickCanTest(float Dt);   // dev: -ISCanTest plays the whole CanSat chain and logs [CANTEST]
	int32 CanTestStep = -1;
	float CanTestT = 0.f;
	void TickBench(float Dt);
	// one zoom from your feet to orbit: the wheel lifts the camera above the site through the real terrain;
	// at ~150 km the globe view (same real Earth) takes over, and coming back down lands you where you were
	void TickOverview(float Dt);
	UPROPERTY() class ACameraActor* OverCam = nullptr;
	double OverAlt = 0.0, OverAltGoal = 0.0;   // metres above the site
	float OverYaw = 0.f;
	FVector OverAnchor = FVector::ZeroVector;
	TSharedPtr<class SWidget> UIContainer;
	FVector2D LastMouse = FVector2D::ZeroVector;
	bool bDragging = false;
	EISMode Mode = EISMode::Menu;
	FString CurrentPrompt;
	FName LookAction;
};
