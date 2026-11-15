// Infinisat - CanSat assembly on the workbench (KSP-style): the can has real mounting decks, parts are dragged in from
// picture cards (or from the bench), the camera zooms onto the deck being worked on. Any deck is allowed; the wrong one
// changes the flight: a parachute buried under the electronics never comes out, a sensor away from the vent holes
// measures the warm inside of the can, a GNSS receiver without a view of the sky gets no fix, a radio under the
// battery loses range.
#include "ISWorld.h"
#include "ISParts3D.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

const TArray<AISHomeWorld::FCanSlot>& AISHomeWorld::CanSlots()
{
	// heights in cm inside the 11.5 cm can (bottom plate 0.4 cm, lid 0.3 cm)
	static const TArray<FCanSlot> S = {
		{ TEXT("Battery bay (bottom)"), TEXT("The heaviest part goes lowest: the can falls straight and the battery is protected at landing."), 0.4f, 2.6f, FLinearColor(1.f, 0.55f, 0.15f) },
		{ TEXT("Main board"), TEXT("The computer sits in the middle, close to everything it has to wire to."), 2.6f, 4.4f, FLinearColor(0.3f, 0.55f, 1.f) },
		{ TEXT("Radio deck"), TEXT("The radio sits above the battery so its antenna can go out through the lid; metal below it blocks the signal."), 4.4f, 6.2f, FLinearColor(0.75f, 0.4f, 1.f) },
		{ TEXT("Sensor deck (vent holes)"), TEXT("Holes in the can wall here let outside air reach the sensors. Anywhere else they measure the warm air inside the can."), 6.2f, 7.8f, FLinearColor(0.3f, 0.95f, 0.45f) },
		{ TEXT("Sky deck (window)"), TEXT("Clear view up and out: a GNSS receiver needs to see the satellites, a camera needs a window."), 7.8f, 9.4f, FLinearColor(0.3f, 0.9f, 1.f) },
		{ TEXT("Parachute bay (under the lid)"), TEXT("The parachute must be packed on top: the lid pops off at apogee and the chute pulls out."), 9.4f, 11.2f, FLinearColor(1.f, 0.85f, 0.2f) },
	};
	return S;
}

int32 AISHomeWorld::IdealSlot(FName Id)
{
	const FISPart* P = ISData::FindPart(Id);
	if (!P) return 1;
	if (P->Cat == EISCat::Battery) return 0;
	if (P->Cat == EISCat::Computer) return 1;
	if (P->Cat == EISCat::Comms) return 2;
	if (P->Tag == ISTag::TempSensor || P->Tag == ISTag::Buzzer) return 3;
	if (P->Tag == ISTag::GNSSRx || P->Tag == ISTag::Camera) return 4;
	if (P->Tag == ISTag::Parachute) return 5;
	if (P->Tag == ISTag::Wiring) return 1;
	return 3;
}

bool AISHomeWorld::BenchPlace(FName Id, int32 Slot)
{
	const FISPart* P = ISData::FindPart(Id);
	if (!P || Stage != EISStage::Build || !CanSlots().IsValidIndex(Slot)) return false;
	// wires run along the whole stack (they do not take deck space)
	if (P->Tag != ISTag::Wiring)
	{
		int32 Used = 0;
		for (const auto& KV : Placed) if (KV.Value == Slot && KV.Key != Id) if (const FISPart* Q = ISData::FindPart(KV.Key)) if (Q->Tag != ISTag::Wiring) ++Used;
		if (Used >= 2) { Toast(FString::Printf(TEXT("The %s is full (2 parts per deck). Take something out first, or use another deck."), *CanSlots()[Slot].Name)); return false; }
	}
	// single-choice categories (battery, computer, radio): the new one replaces the old one
	if (!(P->Cat == EISCat::Payload || P->Cat == EISCat::Extra))
	{
		const FName Old = Design.GetSingle(P->Cat);
		if (!Old.IsNone() && Old != Id) Placed.Remove(Old);
		Design.SetSingle(P->Cat, Id);
	}
	else if (!Design.Has(Id)) Design.Toggle(Id);
	Placed.Add(Id, Slot);
	BenchInfoPart = Id;
	FocusSlot = Slot; BenchFocusT = 1.6f;
	Eval = ISSim::Evaluate(Design, ISData::Missions()[MissionIdx]);
	UpdatePlacementFlags();
	if (Slot != IdealSlot(Id) && P->Tag != ISTag::Wiring)
		Toast(FString::Printf(TEXT("%s placed in the %s. It fits, but the usual place is the %s: %s"), *P->Name, *CanSlots()[Slot].Name, *CanSlots()[IdealSlot(Id)].Name, *CanSlots()[IdealSlot(Id)].Why));
	RebuildCan(); RebuildTray();
	++DesignVersion;
	SaveProgress();
	return true;
}

void AISHomeWorld::BenchRemove(FName Id)
{
	const FISPart* P = ISData::FindPart(Id);
	if (!P || Stage != EISStage::Build) return;
	Placed.Remove(Id);
	if (P->Cat == EISCat::Payload || P->Cat == EISCat::Extra) { if (Design.Has(Id)) Design.Toggle(Id); }
	else if (Design.GetSingle(P->Cat) == Id) Design.SetSingle(P->Cat, P->Cat == EISCat::Power ? FName("pwr_none") : P->Cat == EISCat::ADCS ? FName("adcs_none") : NAME_None);
	BenchInfoPart = Id;
	Eval = ISSim::Evaluate(Design, ISData::Missions()[MissionIdx]);
	UpdatePlacementFlags();
	RebuildCan(); RebuildTray();
	++DesignVersion;
	SaveProgress();
}

void AISHomeWorld::UpdatePlacementFlags()
{
	bChuteBuried = bSensorSealed = bGnssBlind = bCamBlind = bRadioShielded = false;
	int32 BatSlot = -1;
	for (const auto& KV : Placed) if (const FISPart* P = ISData::FindPart(KV.Key)) if (P->Cat == EISCat::Battery) BatSlot = KV.Value;
	for (const auto& KV : Placed)
	{
		const FISPart* P = ISData::FindPart(KV.Key);
		if (!P) continue;
		if (P->Tag == ISTag::Parachute && KV.Value != 5) bChuteBuried = true;
		if (P->Tag == ISTag::TempSensor && KV.Value != 3) bSensorSealed = true;
		if (P->Tag == ISTag::GNSSRx && KV.Value < 4) bGnssBlind = true;
		if (P->Tag == ISTag::Camera && KV.Value != 4) bCamBlind = true;
		if (P->Cat == EISCat::Comms && BatSlot >= 0 && KV.Value < BatSlot) bRadioShielded = true;
	}
	// a temperature reading counts as long as at least one temperature sensor sits by the vents
	for (const auto& KV : Placed) if (const FISPart* P = ISData::FindPart(KV.Key)) if (P->Tag == ISTag::TempSensor && KV.Value == 3) bSensorSealed = false;
}

TArray<FString> AISHomeWorld::PlacementIssues() const
{
	TArray<FString> I;
	if (bChuteBuried) I.Add(TEXT("Parachute is not under the lid: it stays packed inside and the can falls at ~40 m/s."));
	if (bSensorSealed) I.Add(TEXT("Temperature sensor is not at the vent holes: it will measure the warm air inside the can, not the sky."));
	if (bGnssBlind) I.Add(TEXT("GNSS module is under other boards: no view of the satellites, no position fix."));
	if (bCamBlind) I.Add(TEXT("Camera is not at the window: the pictures will be black."));
	if (bRadioShielded) I.Add(TEXT("Radio is below the battery: the cell and its metal case block much of the signal (range about 1/3)."));
	return I;
}

// ------------------------------------------------------------------------------------------------ the can, part by part
void AISHomeWorld::RebuildCan()
{
	CanModel.Destroy();
	GuideModel.Destroy();
	for (USceneComponent* C : CanPartRoots) if (IsValid(C)) C->DestroyComponent();
	CanPartRoots.Reset();
	for (auto It = TrayHits.CreateIterator(); It; ++It) if (It.Key().IsValid() && It.Key()->ComponentHasTag("placed")) { It.Key()->DestroyComponent(); It.RemoveCurrent(); }
	if (Stage == EISStage::NoOrder) return;
	const bool bOpen = Stage == EISStage::Build;
	ISBuild::CanShell(this, CanRoot, FVector::ZeroVector, bOpen, &CanModel);
	const TArray<FCanSlot>& Slots = CanSlots();
	if (bOpen)
	{
		// the deck plates (thin standoff boards) so the levels are visible
		for (int32 s = 1; s < Slots.Num(); ++s)
			ISBuild::Glass(this, CanRoot, ISMat::Cyl(), FVector(0, 0, Slots[s].Z0), FVector(6.1f, 6.1f, 0.1f), FLinearColor(0.75f, 0.8f, 0.85f), 0.25f, &CanModel);
		// vent holes and window on the shell
		for (int32 k = 0; k < 6; ++k)
		{
			const FVector Dir = FRotator(0, k * 60.f + 30.f, 0).Vector();
			ISBuild::Prim(this, CanRoot, ISMat::Cyl(), Dir * 3.32f + FVector(0, 0, (Slots[3].Z0 + Slots[3].Z1) * 0.5f), FVector(0.45f, 0.45f, 0.05f), FLinearColor(0.02f, 0.02f, 0.02f), &CanModel, FRotator(90, k * 60.f + 30.f, 0));
		}
		ISBuild::Glass(this, CanRoot, ISMat::Cube(), FVector(0, -3.3f, (Slots[4].Z0 + Slots[4].Z1) * 0.5f), FVector(2.4f, 0.06f, 1.3f), FLinearColor(0.4f, 0.8f, 1.f), 0.5f, &CanModel);
	}
	// parts per deck: up to two side by side; each is fitted into its share of the deck (no overlaps, nothing sticks out)
	TMap<int32, TArray<FName>> ByDeck;
	for (const FName& Id : Design.Parts) if (const int32* S = Placed.Find(Id)) ByDeck.FindOrAdd(*S).Add(Id);
	float Top = 0.4f;
	for (auto& KV : ByDeck)
	{
		const FCanSlot& Sl = Slots[KV.Key];
		TArray<FName> Board;
		for (const FName& Id : KV.Value) { const FISPart* P = ISData::FindPart(Id); if (P && P->Tag != ISTag::Wiring) Board.Add(Id); }
		for (int32 i = 0; i < Board.Num(); ++i)
		{
			USceneComponent* R = NewObject<USceneComponent>(this);
			R->SetupAttachment(CanRoot);
			R->RegisterComponent();
			CanPartRoots.Add(R);
			FISModel Tmp;
			ISBuild::CanSatPart(this, R, Board[i], FVector::ZeroVector, &Tmp);
			// local bounds of what was built
			FBox B(ForceInit);
			const FTransform Inv = R->GetComponentTransform().Inverse();
			for (UStaticMeshComponent* C : Tmp.Parts) if (IsValid(C)) B += C->Bounds.GetBox().TransformBy(Inv);
			CanModel.Parts.Append(Tmp.Parts);
			if (!B.IsValid) continue;
			const FVector Ext = B.GetSize();
			const bool bTurn = Ext.X > Ext.Y;                 // long side along Y (front to back)
			const float Wx = bTurn ? Ext.Y : Ext.X, Wy = bTurn ? Ext.X : Ext.Y;
			const bool bTwo = Board.Num() > 1;
			const float MaxX = bTwo ? 2.7f : 4.6f, MaxY = bTwo ? 4.4f : 4.6f, MaxZ = Sl.Z1 - Sl.Z0 - 0.2f;
			const float Sc = FMath::Min(1.f, FMath::Min3(MaxX / FMath::Max(0.1f, Wx), MaxY / FMath::Max(0.1f, Wy), MaxZ / FMath::Max(0.1f, (float)Ext.Z)));
			const FRotator Rot(0, bTurn ? 90.f : 0.f, 0);
			const FVector C = Rot.RotateVector(B.GetCenter()) * Sc;
			const float X = bTwo ? (i == 0 ? -1.45f : 1.45f) : 0.f;
			R->SetRelativeScale3D(FVector(Sc));
			R->SetRelativeRotation(Rot);
			R->SetRelativeLocation(FVector(X - C.X, -C.Y, Sl.Z0 + 0.1f - B.Min.Z * Sc));
			Top = FMath::Max(Top, Sl.Z0 + 0.1f + (float)Ext.Z * Sc);
			if (bOpen)
			{
				// click a mounted part to pick it up again (drag it out of the can to remove it)
				UBoxComponent* Hb = NewObject<UBoxComponent>(this);
				Hb->SetupAttachment(CanRoot);
				Hb->SetRelativeLocation(FVector(X, 0, (Sl.Z0 + Sl.Z1) * 0.5f));
				Hb->SetBoxExtent(FVector(bTwo ? 1.4f : 2.6f, 2.6f, (Sl.Z1 - Sl.Z0) * 0.5f));
				Hb->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				Hb->SetCollisionResponseToAllChannels(ECR_Ignore);
				Hb->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				Hb->ComponentTags.Add("placed");
				Hb->RegisterComponent();
				TrayHits.Add(Hb, Board[i]);
			}
		}
		// wiring: coloured wires running up the side of the stack
		for (const FName& Id : KV.Value)
			if (const FISPart* P = ISData::FindPart(Id)) if (P->Tag == ISTag::Wiring)
				for (int32 w = 0; w < 4; ++w)
				{
					const FLinearColor Wc = w == 0 ? FLinearColor(0.8f, 0.1f, 0.1f) : w == 1 ? FLinearColor(0.05f, 0.05f, 0.05f) : w == 2 ? FLinearColor(0.9f, 0.8f, 0.1f) : FLinearColor(0.1f, 0.4f, 0.9f);
					ISBuild::Prim(this, CanRoot, ISMat::Cyl(), FVector(2.75f, -1.2f + w * 0.8f, 5.6f), FVector(0.12f, 0.12f, 10.f), Wc, &CanModel);
				}
	}
	StackHeightCm = Top;
	if (!bOpen)
	{
		// sealed can: lid, beacon LED and antenna stub visible outside
		ISBuild::Unlit(this, CanRoot, ISMat::Sphere(), FVector(0, 2.2f, 11.8f), FVector(0.6f), FLinearColor(1.f, 0.1f, 0.05f), 10.f, &CanModel);
	}
	if (IsValid(CanHit)) { Actions.Remove(CanHit); CanHit->DestroyComponent(); CanHit = nullptr; }
	if (Stage == EISStage::Sealed) CanHit = AddInteract(CanRoot->GetComponentLocation() + FVector(0, 0, 6), FVector(8, 8, 9), "cansat");
}

// ------------------------------------------------------------------------------------------------ aim highlight
void AISHomeWorld::SetHighlight(FName Action, UPrimitiveComponent* Hit)
{
	// steady outline: a one-frame miss of the view trace must not switch it off and on (that read as flashing)
	static double LastSeen = 0.0;
	const double Now = FPlatformTime::Seconds();
	if (Action == HighlightAction) { LastSeen = Now; return; }
	if (Action.IsNone() && !HighlightAction.IsNone() && Now - LastSeen < 0.25) return;
	LastSeen = Now;
	for (const TWeakObjectPtr<UPrimitiveComponent>& C : Highlighted) if (C.IsValid()) C->SetRenderCustomDepth(false);
	Highlighted.Reset();
	HighlightAction = Action;
	if (Action.IsNone() || !Hit) return;
	const FString A = Action.ToString();
	if (A.StartsWith(TEXT("item:")))
	{
		const FName Part(*A.RightChop(5));
		for (const FISHuntItem& H : Hunt) if (H.Part == Part) for (UStaticMeshComponent* C : H.Model.Parts) if (IsValid(C)) Highlighted.Add(C);
	}
	else
	{
		// the visible meshes that sit inside the interaction volume (laptop, workbench, can, rocket...)
		const FBox Zone = Hit->Bounds.GetBox().ExpandBy(4.f);
		const double ZoneVol = Zone.GetVolume();
		for (UPrimitiveComponent* C : AllComps)
		{
			if (!IsValid(C) || !C->IsVisible() || C == Hit) continue;
			const FBox B = C->Bounds.GetBox();
			if (!Zone.IsInside(B.GetCenter()) || B.GetVolume() > ZoneVol * 3.0) continue;
			Highlighted.Add(C);
		}
		if (Highlighted.Num() == 0 && Hit->IsVisible()) Highlighted.Add(Hit);
	}
	for (const TWeakObjectPtr<UPrimitiveComponent>& C : Highlighted) if (C.IsValid()) C->SetRenderCustomDepth(true);
}

FString AISHomeWorld::ModeTag() const
{
	switch (Stage)
	{
	case EISStage::NoOrder: return JobPhase == 1 ? TEXT("BUILD THE SATELLITE") : JobPhase == 3 ? TEXT("REPORT") : TEXT("NEW ORDER WAITING");
	case EISStage::Build: return UsesHunt() && FoundCount(true) < RequiredCount() ? FString::Printf(TEXT("FIND PARTS  %d/%d"), FoundCount(true), RequiredCount()) : FString(TEXT("BUILD THE CANSAT"));
	case EISStage::Sealed: case EISStage::Carry: return TEXT("CARRY TO THE PAD");
	case EISStage::OnPad: case EISStage::Countdown: return TEXT("LAUNCH");
	case EISStage::Boost: case EISStage::Coast: case EISStage::Descent: return TEXT("IN FLIGHT");
	case EISStage::Landed: return TEXT("FIND THE CANSAT");
	default: return TEXT("RECOVERED");
	}
}

FVector AISHomeWorld::CanRootLocation() const { return CanRoot ? CanRoot->GetComponentLocation() : FVector::ZeroVector; }

// ------------------------------------------------------------------------------------------------ drag & drop
void AISHomeWorld::BeginDrag(FName Id)
{
	if (Stage != EISStage::Build || !ISData::FindPart(Id)) return;
	DragPart = Id;
	HoverSlot = -1;
	FocusSlot = IdealSlot(Id);
	BenchInfoPart = Id;
	if (!DragRoot)
	{
		DragRoot = NewObject<USceneComponent>(this);
		DragRoot->SetupAttachment(Root);
		DragRoot->RegisterComponent();
	}
	DragModel.Destroy();
	ISBuild::CanSatPart(this, DragRoot, Id, FVector::ZeroVector, &DragModel);
	DragRoot->SetWorldLocation(CanRoot->GetComponentLocation() + FVector(0, -6, 14));
}

void AISHomeWorld::UpdateDrag(const FVector& RayStart, const FVector& RayDir)
{
	if (DragPart.IsNone() || !DragRoot) return;
	const FVector Can = CanRoot->GetComponentLocation();
	// vertical plane through the can axis, facing the camera
	const FVector N = FVector(RayDir.X, RayDir.Y, 0).GetSafeNormal();
	const float Den = FVector::DotProduct(RayDir, N);
	FVector Hit = RayStart + RayDir * 40.f;
	if (FMath::Abs(Den) > 1e-3f) Hit = RayStart + RayDir * (FVector::DotProduct(Can - RayStart, N) / Den);
	const FVector L = Hit - Can;
	const float Side = FVector(L.X, L.Y, 0).Size();
	HoverSlot = -1;
	if (Side < 9.f && L.Z > -3.f && L.Z < 15.f)
	{
		const float Z = FMath::Clamp((float)L.Z, 0.5f, 11.1f);
		const TArray<FCanSlot>& S = CanSlots();
		for (int32 i = 0; i < S.Num(); ++i) if (Z >= S[i].Z0 && Z < S[i].Z1) HoverSlot = i;
	}
	if (HoverSlot >= 0)
	{
		const FCanSlot& S = CanSlots()[HoverSlot];
		DragRoot->SetWorldLocation(Can + FVector(0, -9.f, S.Z0 + 0.3f));   // hovering just in front of the deck it will go into
		FocusSlot = HoverSlot;
	}
	else DragRoot->SetWorldLocation(Hit);
	// guide: ring on the deck the part belongs to (green) and on the deck under the cursor (yellow if it is not the usual one)
	GuideModel.Destroy();
	if (bGuide || HoverSlot >= 0)
	{
		const int32 Ideal = IdealSlot(DragPart);
		auto Ring = [this](int32 Sl, const FLinearColor& C)
		{
			const FCanSlot& S = CanSlots()[Sl];
			ISBuild::Unlit(this, CanRoot, ISMat::Cyl(), FVector(0, 0, S.Z0 + 0.05f), FVector(6.5f, 6.5f, 0.08f), C, 3.f, &GuideModel);
			ISBuild::Glass(this, CanRoot, ISMat::Cyl(), FVector(0, 0, (S.Z0 + S.Z1) * 0.5f), FVector(6.3f, 6.3f, S.Z1 - S.Z0 - 0.1f), C, 0.18f, &GuideModel);
		};
		if (bGuide) Ring(Ideal, FLinearColor(0.2f, 1.f, 0.4f));
		if (HoverSlot >= 0 && HoverSlot != Ideal) Ring(HoverSlot, FLinearColor(1.f, 0.8f, 0.2f));
	}
}

void AISHomeWorld::EndDrag(bool bDrop)
{
	if (DragPart.IsNone()) return;
	const FName Id = DragPart;
	DragPart = NAME_None;
	DragModel.Destroy();
	GuideModel.Destroy();
	if (bDrop && HoverSlot >= 0) BenchPlace(Id, HoverSlot);
	else if (bDrop && Placed.Contains(Id)) BenchRemove(Id);   // dragged out of the can: take it out
	HoverSlot = -1;
}

// ------------------------------------------------------------------------------------------------ camera
void AISHomeWorld::UpdateBenchCamera(float Dt)
{
	if (!BenchCam) return;
	BenchFocusT = FMath::Max(0.f, BenchFocusT - Dt);
	const FVector Can = CanRoot->GetComponentLocation();
	// home view: the open can big in the middle, the bench parts behind it
	BenchCamHome = Can + FVector(0, -36.f, 24.f);
	BenchCamHomeRot = ((Can + FVector(0, 6.f, 4.f)) - BenchCamHome).Rotation();
	FVector Want = BenchCamHome;
	FRotator WantRot = BenchCamHomeRot;
	if (Stage == EISStage::Build && (!DragPart.IsNone() || BenchFocusT > 0.f) && CanSlots().IsValidIndex(FocusSlot))
	{
		// zoom onto the deck being worked on
		const FCanSlot& S = CanSlots()[FocusSlot];
		const FVector Look = Can + FVector(0, 0, (S.Z0 + S.Z1) * 0.5f);
		Want = Look + FVector(0, -21.f, 9.f);
		WantRot = (Look - Want).Rotation();
	}
	BenchCam->SetActorLocation(FMath::VInterpTo(BenchCam->GetActorLocation(), Want, Dt, 5.f));
	BenchCam->SetActorRotation(FMath::RInterpTo(BenchCam->GetActorRotation(), WantRot, Dt, 5.f));
}

// ------------------------------------------------------------------------------------------------ part pictures
UTextureRenderTarget2D* AISHomeWorld::PartThumb(FName Id)
{
	if (UTextureRenderTarget2D** T = Thumbs.Find(Id)) return *T;
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = RTF_RGBA8;
	RT->ClearColor = FLinearColor(0.035f, 0.04f, 0.05f, 1.f);
	RT->InitAutoFormat(192, 192);
	RT->UpdateResourceImmediate(true);
	Thumbs.Add(Id, RT);
	ThumbQueue.Add(Id);   // photographed by ProcessThumbs(), one part at a time
	return RT;
}

// Photo booth 200 m above the house. One part per round: build it (frame 0), let the renderer pick it up, capture
// (frame 3), clear it (frame 5), next part. Only the part and a dark backdrop card are rendered (show-only list).
void AISHomeWorld::ProcessThumbs()
{
	if (ThumbQueue.Num() == 0) return;
	if (!ThumbStage)
	{
		ThumbStage = NewObject<USceneComponent>(this);
		ThumbStage->SetupAttachment(Root);
		ThumbStage->RegisterComponent();
		ThumbStage->SetWorldLocation(BenchTop + FVector(0, 0, 20000.f));
		ThumbCam = NewObject<USceneCaptureComponent2D>(this);
		ThumbCam->SetupAttachment(ThumbStage);
		ThumbCam->bCaptureEveryFrame = false;
		ThumbCam->bCaptureOnMovement = false;
		ThumbCam->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		ThumbCam->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		ThumbCam->FOVAngle = 30.f;
		ThumbCam->bOverride_CustomNearClippingPlane = true;   // tiny parts sit a few cm from the lens (default near clip ~10 cm cut them away)
		ThumbCam->CustomNearClippingPlane = 0.05f;
		ThumbCam->ShowFlags.SetAtmosphere(false);
		ThumbCam->ShowFlags.SetFog(false);
		ThumbCam->ShowFlags.SetVolumetricFog(false);
		ThumbCam->ShowFlags.SetCloud(false);
		ThumbCam->ShowFlags.SetEyeAdaptation(false);
		ThumbCam->PostProcessSettings.bOverride_AutoExposureMethod = true;
		ThumbCam->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		ThumbCam->PostProcessSettings.bOverride_AutoExposureBias = true;
		ThumbCam->PostProcessSettings.AutoExposureBias = 11.f;
		ThumbCam->RegisterComponent();
	}
	const FName Id = ThumbQueue[0];
	// photograph the part lying on the bench (already on screen, so it renders); parts not on the bench are built in the booth
	TArray<UStaticMeshComponent*> Subject;
	if (const FISModel* T = TrayModels.Find(Id)) { for (UStaticMeshComponent* C : T->Parts) if (IsValid(C)) Subject.Add(C); }
	if (ThumbFrames < 0)
	{
		ThumbModel.Destroy();
		if (Subject.Num() == 0) ISBuild::CanSatPart(this, ThumbStage, Id, FVector::ZeroVector, &ThumbModel);
		ThumbFrames = 0;
		return;
	}
	if (Subject.Num() == 0) for (UStaticMeshComponent* C : ThumbModel.Parts) if (IsValid(C)) Subject.Add(C);
	++ThumbFrames;
	if (ThumbFrames == 3)
	{
		double Big = 0;
		for (UStaticMeshComponent* C : Subject) Big = FMath::Max(Big, C->Bounds.GetBox().GetVolume());
		FBox B(ForceInit);
		for (UStaticMeshComponent* C : Subject) if (C->Bounds.GetBox().GetVolume() >= Big * 0.01) B += C->Bounds.GetBox();
		if (B.IsValid)
		{
			const FVector Ctr = B.GetCenter();
			const float Rad = FMath::Max(0.5f, (float)B.GetExtent().Size());
			const FVector Dir = FRotator(-35.f, 50.f, 0).Vector();
			// dark backdrop card, visible to the photo camera only
			UStaticMeshComponent* Back = ISBuild::Prim(this, ThumbStage, ISMat::Cube(), ThumbStage->GetComponentTransform().InverseTransformPosition(Ctr + Dir * Rad * 3.f),
				FVector(Rad * 14.f, Rad * 14.f, 1.f), FLinearColor(0.035f, 0.04f, 0.05f), &ThumbModel, Dir.Rotation() + FRotator(90, 0, 0), 0.f, 0.9f);
			if (Back) Back->SetVisibleInSceneCaptureOnly(true);
			ThumbCam->SetWorldLocation(Ctr - Dir * Rad / FMath::Tan(FMath::DegreesToRadians(15.f)) * 1.05f);
			ThumbCam->SetWorldRotation(Dir.Rotation());
		}
	}
	if (ThumbFrames == 5 || ThumbFrames == 30)
	{
		ThumbCam->ShowOnlyComponents.Reset();
		for (UStaticMeshComponent* C : Subject) ThumbCam->ShowOnlyComponents.Add(C);
		for (UStaticMeshComponent* C : ThumbModel.Parts) if (IsValid(C)) ThumbCam->ShowOnlyComponents.AddUnique(C);
		if (UTextureRenderTarget2D** RT = Thumbs.Find(Id)) { ThumbCam->TextureTarget = *RT; ThumbCam->CaptureScene(); }
	}
	if (ThumbFrames >= 32)
	{
		ThumbModel.Destroy();
		ThumbCam->ShowOnlyComponents.Reset();
		ThumbQueue.RemoveAt(0);
		ThumbFrames = -1;
	}
}

// N during the CanSat flight: run the rest of the flight physics at once (same result as watching it), land, then search.
void AISHomeWorld::SkipFlight()
{
	if (Stage == EISStage::Countdown) Countdown = 0.001f;
	if (Stage == EISStage::Countdown) Tick(0.01f);
	for (int32 i = 0; i < 120000 && Stage >= EISStage::Boost && Stage <= EISStage::Descent; ++i) StepFlight(0.01f);
}
