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

enum class EISMode : uint8 { Menu, Walk, Bench, Panel };

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
	void OpenPanel();          // world stays, cursor for a UI panel (orders, debrief)
	void ClosePanel();         // back to walking
	EISMode GetMode() const { return Mode; }
	const FString& Prompt() const { return CurrentPrompt; }

	UPROPERTY() AISSpaceScene* Scene = nullptr;
	UPROPERTY() AISHomeWorld* World = nullptr;
	UPROPERTY() AISBitCharacter* Bit = nullptr;
	TSharedPtr<SISGameUI> UI;

private:
	void SetCursorMode(bool bCursor);
	void TickWalk(float Dt);
	void TickBench(float Dt);
	TSharedPtr<class SWidget> UIContainer;
	FVector2D LastMouse = FVector2D::ZeroVector;
	bool bDragging = false;
	EISMode Mode = EISMode::Menu;
	FString CurrentPrompt;
	FName LookAction;
};
