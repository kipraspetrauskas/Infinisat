#include "ISParts3D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	template <typename T> T* Rooted(const TCHAR* Path)
	{
		T* O = LoadObject<T>(nullptr, Path);
		if (O && !O->IsRooted()) O->AddToRoot();
		return O;
	}
	const FLinearColor PCBGreen(0.02f, 0.25f, 0.08f), PCBBlue(0.02f, 0.08f, 0.35f), Chip(0.02f, 0.02f, 0.025f),
		Gold(0.85f, 0.65f, 0.2f), Silver(0.8f, 0.8f, 0.82f), Copper(0.75f, 0.38f, 0.18f), Orange(1.f, 0.35f, 0.05f), White(0.92f, 0.92f, 0.94f);
}

int32 ISQuality::Level = 2;

bool ISDemo::bDemo = false;
int32 ISDemo::Look = 0;
FString ISDemo::Map;

void ISDemo::Init(const UObject* Ctx)
{
	Map = Ctx ? UGameplayStatics::GetCurrentLevelName(Ctx, true) : FString();
	bDemo = Map.StartsWith(TEXT("Demo_")) || FParse::Param(FCommandLine::Get(), TEXT("ISDemo"));
	Look = bDemo ? 2 : 0;   // demo gameplay default: B bright morning (cinematics use A via -ISLook=1)
	FParse::Value(FCommandLine::Get(), TEXT("ISLook="), Look);
	Look = FMath::Clamp(Look, 0, 3);
}

namespace ISMat
{
	UMaterialInterface* Color() { static UMaterialInterface* M = Rooted<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Color.M_IS_Color")); return M; }
	UMaterialInterface* Unlit() { static UMaterialInterface* M = Rooted<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Unlit.M_IS_Unlit")); return M; }
	UMaterialInterface* Glass() { static UMaterialInterface* M = Rooted<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_Glass.M_IS_Glass")); return M ? M : Color(); }
	UMaterialInterface* World() { static UMaterialInterface* M = Rooted<UMaterialInterface>(TEXT("/Game/Infinisat/Materials/M_IS_World.M_IS_World")); return M ? M : Color(); }
	UStaticMesh* Cube() { static UStaticMesh* M = Rooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")); return M; }
	UStaticMesh* Sphere() { static UStaticMesh* M = Rooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere")); return M; }
	UStaticMesh* Cyl() { static UStaticMesh* M = Rooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); return M; }
	UStaticMesh* Cone() { static UStaticMesh* M = Rooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone")); return M; }
	UStaticMesh* Mesh(const FString& Path) { return LoadObject<UStaticMesh>(nullptr, *Path); }
	FString PH(const TCHAR* Id) { return FString::Printf(TEXT("/Game/Infinisat/PH/%s/SM_%s.SM_%s"), Id, Id, Id); }
}

void FISModel::Destroy()
{
	for (UStaticMeshComponent* C : Parts) if (IsValid(C)) C->DestroyComponent();
	Parts.Reset();
}

void FISModel::SetVisible(bool b)
{
	for (UStaticMeshComponent* C : Parts) if (IsValid(C)) C->SetVisibility(b);
}

namespace ISBuild
{
	static UStaticMeshComponent* Make(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& Scale, const FRotator& Rot, FISModel* Into)
	{
		if (!Mesh || !Parent) return nullptr;
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(Parent);
		C->SetRelativeTransform(FTransform(Rot, Pos, Scale));
		C->RegisterComponent();
		if (Into) Into->Parts.Add(C);
		return C;
	}

	UStaticMeshComponent* Prim(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, FISModel* Into, const FRotator& Rot, float Metal, float Rough, float Emissive)
	{
		UStaticMeshComponent* C = Make(Owner, Parent, Mesh, Pos, SizeCm / 100.f, Rot, Into);
		if (!C) return nullptr;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ISMat::Color(), C);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metal);
		MID->SetScalarParameterValue(TEXT("Roughness"), Rough);
		MID->SetScalarParameterValue(TEXT("Emissive"), Emissive);
		C->SetMaterial(0, MID);
		return C;
	}

	UStaticMeshComponent* Unlit(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, float Intensity, FISModel* Into, const FRotator& Rot)
	{
		UStaticMeshComponent* C = Make(Owner, Parent, Mesh, Pos, SizeCm / 100.f, Rot, Into);
		if (!C) return nullptr;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ISMat::Unlit(), C);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		C->SetMaterial(0, MID);
		C->CastShadow = false;
		return C;
	}

	UStaticMeshComponent* Glass(UObject* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Pos, const FVector& SizeCm,
		const FLinearColor& Color, float Opacity, FISModel* Into, const FRotator& Rot)
	{
		UStaticMeshComponent* C = Make(Owner, Parent, Mesh, Pos, SizeCm / 100.f, Rot, Into);
		if (!C) return nullptr;
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ISMat::Glass(), C);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		C->SetMaterial(0, MID);
		C->CastShadow = false;
		return C;
	}

	UStaticMeshComponent* Asset(UObject* Owner, USceneComponent* Parent, const FString& Path, const FVector& Pos, float Scale, FISModel* Into, const FRotator& Rot, bool bCollide)
	{
		UStaticMeshComponent* C = Make(Owner, Parent, ISMat::Mesh(Path), Pos, FVector(Scale), Rot, Into);
		if (C && bCollide)
		{
			C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			C->SetCollisionResponseToAllChannels(ECR_Block);
		}
		return C;
	}

	// ------------------------------------------------------------------------------------------ CanSat parts
	static void Pins(UObject* O, USceneComponent* P, const FVector& Start, const FVector& Step, int32 N, FISModel* M)
	{
		for (int32 i = 0; i < N; ++i)
			Prim(O, P, ISMat::Cube(), Start + Step * i, FVector(0.08f, 0.08f, 0.6f), Gold, M, FRotator::ZeroRotator, 1.f, 0.25f);
	}

	float CanSatPart(UObject* O, USceneComponent* P, FName Id, const FVector& B, FISModel* M)
	{
		const FString S = Id.ToString();
		// real component models (KiCad library, converted in Autodesk Fusion) when available
		static const TMap<FString, FString> Cad = { { TEXT("cpu_ard"), TEXT("esp32_wroom32") }, { TEXT("com_433"), TEXT("radio_zeta433") },
			{ TEXT("pl_temp"), TEXT("dht11") }, { TEXT("bat_aa"), TEXT("battery_18650") }, { TEXT("ex_wires"), TEXT("header_1x15") }, { TEXT("ex_buzzer"), TEXT("buzzer") } };
		if (const FString* C = Cad.Find(S))
		{
			const FString Path = FString::Printf(TEXT("/Game/Infinisat/CAD/%s/SM_%s.SM_%s"), **C, **C, **C);
			if (UStaticMesh* Mesh = ISMat::Mesh(Path))
			{
				const bool bEsp = S == TEXT("cpu_ard"), bRadio = S == TEXT("com_433");
				if (bEsp || bRadio)   // modules sit on a small carrier board
					Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.08f), FVector(bEsp ? 2.8f : 2.4f, bEsp ? 5.2f : 2.4f, 0.16f), bEsp ? PCBBlue : PCBGreen, M);
				Asset(O, P, Path, B + FVector(0, 0, 0.16f), 1.f, M);
				float H = Mesh->GetBoundingBox().GetSize().Z + 0.2f;
				// lay tall parts flat (an 18650 cell or a pin header stands 4-7 cm tall but is mounted lying down,
				// otherwise the stack cannot fit the 11.5 cm can), then rest the part on the stack
				if (M && M->Parts.Num())
					if (UStaticMeshComponent* Pc = M->Parts.Last())
					{
						const FVector Sz = Mesh->GetBoundingBox().GetSize();
						if (Sz.Z > FMath::Max(Sz.X, Sz.Y) && Sz.Z > 2.f)
						{
							Pc->SetRelativeRotation(Sz.X <= Sz.Y ? FRotator(90.f, 0.f, 0.f) : FRotator(0.f, 0.f, 90.f));
							Pc->UpdateComponentToWorld();
							const FBox Bx = Pc->Bounds.GetBox();
							const float Base = P ? (float)P->GetComponentTransform().TransformPosition(B + FVector(0, 0, 0.16f)).Z : (float)Bx.Min.Z;
							Pc->AddWorldOffset(FVector(0, 0, Base - Bx.Min.Z));
							H = (float)Bx.GetSize().Z / FMath::Max(0.01f, P ? (float)P->GetComponentScale().Z : 1.f) + 0.2f;
						}
					}
				if (bRadio)   // quarter-wave whip: 17.3 cm at 433 MHz
					Prim(O, P, ISMat::Cyl(), B + FVector(1.f, 0, 1.6f + 1.4f), FVector(0.45f, 0.45f, 2.8f), Copper, M, FRotator::ZeroRotator, 1.f, 0.35f);   // helical 433 MHz coil antenna (the usual one on hobby LoRa modules)
				return H;
			}
		}
		if (S == TEXT("bat_aa"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.15f), FVector(5.4f, 3.1f, 0.3f), Chip, M);
			for (int32 Row = 0; Row < 2; ++Row) for (int32 i = 0; i < 2; ++i)
			{
				const FVector C = B + FVector(0, -0.75f + i * 1.5f, 1.05f + Row * 1.45f);
				Prim(O, P, ISMat::Cyl(), C, FVector(1.42f, 1.42f, 4.6f), Row ? FLinearColor(0.1f, 0.12f, 0.12f) : Copper, M, FRotator(90, 0, 0), 0.6f, 0.35f);
				Prim(O, P, ISMat::Cyl(), C + FVector(2.4f, 0, 0), FVector(0.6f, 0.6f, 0.2f), Silver, M, FRotator(90, 0, 0), 1.f, 0.2f);
			}
			Prim(O, P, ISMat::Cyl(), B + FVector(-2.6f, 1.2f, 1.6f), FVector(0.15f, 0.15f, 3.f), FLinearColor(0.8f, 0.05f, 0.05f), M);
			Prim(O, P, ISMat::Cyl(), B + FVector(-2.6f, 0.9f, 1.6f), FVector(0.15f, 0.15f, 3.f), Chip, M);
			return 3.3f;
		}
		if (S == TEXT("bat_cube"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.45f), FVector(5.f, 3.4f, 0.9f), FLinearColor(0.15f, 0.3f, 0.7f), M, FRotator::ZeroRotator, 0.4f, 0.3f);
			Prim(O, P, ISMat::Cube(), B + FVector(2.6f, 0, 0.45f), FVector(0.3f, 1.2f, 0.4f), Silver, M, FRotator::ZeroRotator, 1.f);
			return 1.1f;
		}
		if (S == TEXT("cpu_ard") || S == TEXT("cpu_cube"))
		{
			const bool bNano = S == TEXT("cpu_ard");
			const FVector Sz = bNano ? FVector(4.3f, 1.8f, 0.16f) : FVector(5.f, 5.f, 0.16f);
			for (int32 x : { -1, 1 }) for (int32 y : { -1, 1 })
				Prim(O, P, ISMat::Cyl(), B + FVector(x * (Sz.X * 0.45f), y * (Sz.Y * 0.4f), 0.3f), FVector(0.3f, 0.3f, 0.6f), Gold, M, FRotator::ZeroRotator, 1.f);
			const FVector T = B + FVector(0, 0, 0.7f);
			Prim(O, P, ISMat::Cube(), T, Sz, bNano ? PCBBlue : PCBGreen, M, FRotator::ZeroRotator, 0.1f, 0.6f);
			Prim(O, P, ISMat::Cube(), T + FVector(0.2f, 0, 0.15f), FVector(0.8f, 0.8f, 0.12f), Chip, M, FRotator(0, 45, 0));
			Prim(O, P, ISMat::Cube(), T + FVector(-1.3f, 0, 0.12f), FVector(0.5f, 0.3f, 0.1f), Chip, M);
			Prim(O, P, ISMat::Cube(), T + FVector(Sz.X * 0.5f - 0.3f, 0, 0.25f), FVector(0.7f, 0.8f, 0.35f), Silver, M, FRotator::ZeroRotator, 1.f, 0.2f);
			Unlit(O, P, ISMat::Cube(), T + FVector(1.1f, 0.5f, 0.12f), FVector(0.15f, 0.1f, 0.06f), FLinearColor(0.f, 1.f, 0.2f), 8.f, M);
			Pins(O, P, T + FVector(-Sz.X * 0.42f, Sz.Y * 0.42f, 0.35f), FVector(0.254f, 0, 0), bNano ? 15 : 18, M);
			Pins(O, P, T + FVector(-Sz.X * 0.42f, -Sz.Y * 0.42f, 0.35f), FVector(0.254f, 0, 0), bNano ? 15 : 18, M);
			if (!bNano)
			{
				Prim(O, P, ISMat::Cube(), T + FVector(-1.f, 1.f, 0.15f), FVector(1.f, 1.f, 0.12f), Chip, M);
				Prim(O, P, ISMat::Cube(), T + FVector(1.2f, -1.2f, 0.15f), FVector(0.6f, 1.2f, 0.12f), Chip, M);
			}
			return 1.3f;
		}
		if (S == TEXT("com_433") || S == TEXT("com_uhf"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.4f), FVector(2.6f, 1.6f, 0.16f), PCBGreen, M);
			Prim(O, P, ISMat::Cube(), B + FVector(-0.3f, 0, 0.55f), FVector(1.f, 1.f, 0.2f), Silver, M, FRotator::ZeroRotator, 1.f, 0.2f);
			for (int32 i = 0; i < 6; ++i)
				Prim(O, P, ISMat::Cyl(), B + FVector(1.f, 0, 0.6f + i * 0.18f), FVector(0.5f, 0.5f, 0.06f), Copper, M, FRotator::ZeroRotator, 1.f, 0.3f);
			// quarter-wave whip antenna: 17.3 cm at 433 MHz (lambda/4 = c / f / 4)
			Prim(O, P, ISMat::Cyl(), B + FVector(1.f, 0, 1.6f + 1.4f), FVector(0.45f, 0.45f, 2.8f), Copper, M, FRotator::ZeroRotator, 1.f, 0.35f);   // helical 433 MHz coil antenna (the usual one on hobby LoRa modules)
			return 1.6f;
		}
		if (S == TEXT("pl_temp"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.3f), FVector(1.3f, 1.0f, 0.12f), FLinearColor(0.35f, 0.05f, 0.45f), M);
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.42f), FVector(0.25f, 0.25f, 0.1f), Silver, M, FRotator::ZeroRotator, 1.f);
			Pins(O, P, B + FVector(-0.5f, -0.4f, 0.55f), FVector(0.254f, 0, 0), 5, M);
			return 0.8f;
		}
		if (S == TEXT("pl_cam_mini"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.75f), FVector(1.5f, 1.5f, 1.5f), Chip, M);
			Prim(O, P, ISMat::Cyl(), B + FVector(0.9f, 0, 0.75f), FVector(0.9f, 0.9f, 0.6f), FLinearColor(0.05f, 0.05f, 0.06f), M, FRotator(90, 0, 0), 0.5f, 0.15f);
			Prim(O, P, ISMat::Cyl(), B + FVector(1.21f, 0, 0.75f), FVector(0.6f, 0.6f, 0.02f), FLinearColor(0.1f, 0.2f, 0.5f), M, FRotator(90, 0, 0), 0.9f, 0.05f);
			return 1.6f;
		}
		if (S == TEXT("pl_gnss") || S == TEXT("pl_gnss_mini"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.3f), FVector(2.5f, 2.5f, 0.16f), PCBBlue, M);
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.6f), FVector(2.2f, 2.2f, 0.45f), FLinearColor(0.85f, 0.75f, 0.55f), M);
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.84f), FVector(1.5f, 1.5f, 0.02f), Silver, M, FRotator::ZeroRotator, 1.f, 0.2f);
			Prim(O, P, ISMat::Cyl(), B + FVector(0.3f, 0.3f, 0.88f), FVector(0.2f, 0.2f, 0.1f), Gold, M, FRotator::ZeroRotator, 1.f);
			return 1.f;
		}
		if (S == TEXT("ex_chute_s") || S == TEXT("ex_chute_l"))
		{
			const float D = S == TEXT("ex_chute_l") ? 4.6f : 3.4f;
			Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, 1.3f), FVector(D, D, 2.6f), Orange, M, FRotator::ZeroRotator, 0.f, 0.9f);
			Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, 2.65f), FVector(D * 0.6f, D * 0.6f, 0.1f), FLinearColor(0.1f, 0.1f, 0.1f), M);
			return 2.7f;
		}
		if (S == TEXT("adcs_mag"))
		{
			Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.3f), FVector(3.f, 0.6f, 0.6f), FLinearColor(0.5f, 0.5f, 0.55f), M, FRotator::ZeroRotator, 0.9f);
			Prim(O, P, ISMat::Cube(), B + FVector(1.f, 0, 0.3f), FVector(1.f, 0.62f, 0.62f), FLinearColor(0.7f, 0.1f, 0.1f), M);
			return 0.6f;
		}
		if (S == TEXT("adcs_mtq"))
		{
			Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, 0.5f), FVector(1.f, 1.f, 3.5f), Copper, M, FRotator(90, 0, 0), 1.f, 0.3f);
			return 1.f;
		}
		if (S == TEXT("pwr_body") || S == TEXT("pwr_cube"))
		{
			for (int32 i = 0; i < 6; ++i)
			{
				const float A = i * 60.f;
				const FVector Dir = FRotator(0, A, 0).Vector();
				Prim(O, P, ISMat::Cube(), FVector(B.X, B.Y, 5.75f) + Dir * 3.42f, FVector(0.05f, 2.f, 6.f), FLinearColor(0.04f, 0.07f, 0.3f), M, FRotator(0, A, 0), 0.6f, 0.15f);
			}
			return 0.f;
		}
		// generic module (larger satellites / unknown): a labelled box
		Prim(O, P, ISMat::Cube(), B + FVector(0, 0, 0.5f), FVector(2.f, 2.f, 1.f), FLinearColor(0.6f, 0.6f, 0.62f), M);
		return 1.f;
	}

	void CanShell(UObject* O, USceneComponent* P, const FVector& B, bool bOpen, FISModel* M)
	{
		const float D = 6.6f, H = 11.5f;
		if (bOpen) Glass(O, P, ISMat::Cyl(), B + FVector(0, 0, H * 0.5f), FVector(D, D, H), FLinearColor(0.8f, 0.85f, 0.9f), 0.18f, M);
		else Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, H * 0.5f), FVector(D, D, H), FLinearColor(0.82f, 0.84f, 0.88f), M, FRotator::ZeroRotator, 1.f, 0.25f);
		Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, 0.15f), FVector(D + 0.1f, D + 0.1f, 0.3f), Silver, M, FRotator::ZeroRotator, 1.f, 0.25f);
		Prim(O, P, ISMat::Cyl(), B + FVector(0, 0, H - 0.15f), FVector(D + 0.1f, D + 0.1f, 0.3f), Silver, M, FRotator::ZeroRotator, 1.f, 0.25f);
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector Dir = FRotator(0, i * 120.f, 0).Vector();
			Prim(O, P, ISMat::Cyl(), B + Dir * 2.9f + FVector(0, 0, H * 0.5f), FVector(0.3f, 0.3f, H), Silver, M, FRotator::ZeroRotator, 1.f, 0.3f);
		}
	}

	void HobbyRocket(UObject* O, USceneComponent* P, FISModel* M)
	{
		const float D = 7.6f;
		Prim(O, P, ISMat::Cyl(), FVector(0, 0, 5), FVector(3.f, 3.f, 10.f), FLinearColor(0.15f, 0.15f, 0.15f), M);           // motor
		Prim(O, P, ISMat::Cyl(), FVector(0, 0, 48), FVector(D, D, 76.f), White, M, FRotator::ZeroRotator, 0.1f, 0.4f);        // booster tube
		Prim(O, P, ISMat::Cyl(), FVector(0, 0, 87), FVector(D + 0.1f, D + 0.1f, 2.f), Orange, M);                               // coupler band
		Glass(O, P, ISMat::Cyl(), FVector(0, 0, 97), FVector(D, D, 18.f), FLinearColor(0.9f, 0.9f, 1.f), 0.25f, M);           // clear payload bay
		Prim(O, P, ISMat::Cone(), FVector(0, 0, 118), FVector(D, D, 24.f), Orange, M, FRotator::ZeroRotator, 0.1f, 0.4f);    // nose cone
		for (int32 i = 0; i < 3; ++i)
		{
			const float A = i * 120.f;
			const FVector Dir = FRotator(0, A, 0).Vector();
			Prim(O, P, ISMat::Cube(), Dir * (D * 0.5f + 7.f) + FVector(0, 0, 14), FVector(14.f, 0.4f, 20.f), Orange, M, FRotator(0, A, 0));
		}
		Prim(O, P, ISMat::Cyl(), FVector(D * 0.5f + 0.6f, 0, 40), FVector(1.f, 1.f, 3.f), Chip, M);                          // rail button
	}

	void LaunchPad(UObject* O, USceneComponent* P, FISModel* M)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			const float A = i * 120.f + 60.f;
			const FVector Dir = FRotator(0, A, 0).Vector();
			Prim(O, P, ISMat::Cyl(), Dir * 35.f + FVector(0, 0, 18), FVector(2.5f, 2.5f, 80.f), FLinearColor(0.25f, 0.25f, 0.27f), M, FRotator(-60, A, 0), 0.8f, 0.4f);
		}
		Prim(O, P, ISMat::Cyl(), FVector(0, 0, 38), FVector(30.f, 30.f, 1.f), FLinearColor(0.5f, 0.5f, 0.52f), M, FRotator::ZeroRotator, 0.9f, 0.4f);   // blast plate
		Prim(O, P, ISMat::Cyl(), FVector(4.4f, 0, 38 + 90), FVector(1.2f, 1.2f, 180.f), Silver, M, FRotator::ZeroRotator, 1.f, 0.25f);                 // rail
		Prim(O, P, ISMat::Cube(), FVector(-250, 120, 8), FVector(20.f, 14.f, 8.f), FLinearColor(0.85f, 0.75f, 0.1f), M);                                // controller
		Unlit(O, P, ISMat::Cube(), FVector(-250, 120, 12.2f), FVector(2.f, 2.f, 0.5f), FLinearColor(1.f, 0.1f, 0.05f), 6.f, M);
		Prim(O, P, ISMat::Cyl(), FVector(-125, 60, 1), FVector(0.5f, 0.5f, 280.f), Chip, M, FRotator(90, 25.6f, 0));                                    // wire
	}

	void Parachute(UObject* O, USceneComponent* P, float AreaM2, FISModel* M)
	{
		const float Dcm = FMath::Sqrt(4.f * AreaM2 / PI) * 100.f;
		Prim(O, P, ISMat::Sphere(), FVector(0, 0, Dcm * 0.9f), FVector(Dcm, Dcm, Dcm * 0.45f), Orange, M, FRotator::ZeroRotator, 0.f, 0.9f);
		Prim(O, P, ISMat::Cyl(), FVector(0, 0, Dcm * 0.9f + Dcm * 0.2f), FVector(Dcm * 0.15f, Dcm * 0.15f, 1.f), White, M);
		for (int32 i = 0; i < 8; ++i)
		{
			const float A = i * 45.f;
			const FVector Rim = FRotator(0, A, 0).Vector() * Dcm * 0.48f + FVector(0, 0, Dcm * 0.9f);
			const FVector Anchor(0, 0, 11.f);
			const FVector Mid = (Rim + Anchor) * 0.5f;
			const FRotator R = FRotationMatrix::MakeFromZ((Rim - Anchor).GetSafeNormal()).Rotator();
			Prim(O, P, ISMat::Cyl(), Mid, FVector(0.15f, 0.15f, (Rim - Anchor).Size()), White, M, R);
		}
	}

	void Hand(UObject* O, USceneComponent* P, float S, FISModel* M)
	{
		const FLinearColor Shell(0.86f, 0.87f, 0.9f);
		Prim(O, P, ISMat::Sphere(), FVector::ZeroVector, FVector(14.f, 14.f, 12.f) * S, Shell, M, FRotator::ZeroRotator, 0.1f, 0.35f);
		for (int32 f = 0; f < 5; ++f)
		{
			const float A = FMath::DegreesToRadians(-70.f + f * 35.f);
			Prim(O, P, ISMat::Sphere(), FVector(5.f, 11.f * FMath::Sin(A), 10.f * FMath::Cos(A) + 2.f) * S, FVector(4.f) * S, Shell, M, FRotator::ZeroRotator, 0.1f, 0.35f);
		}
		Unlit(O, P, ISMat::Sphere(), FVector(-6.5f, 0, 0) * S, FVector(3.f) * S, FLinearColor(0.1f, 0.85f, 1.f), 3.f, M);
	}
}
