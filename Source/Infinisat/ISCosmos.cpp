// Infinisat - Cosmos explorer: the solar system with real NASA textures, three fictional exoplanet systems, a black hole,
// nebulae and galaxies. Every non-NASA texture is generated here at runtime (procedural noise, all CPU cores), so the
// explorer costs no downloads and no licences. Distances are compressed so everything fits in one flyable scene;
// periods, tilts and spin directions are real for solar-system bodies.
#include "ISScene.h"
#include "Engine/Engine.h"
#include "ISSim.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Async/ParallelFor.h"
#include "Components/LineBatchComponent.h"
#include "ISOrbit.h"

namespace ISCos
{
	FORCEINLINE float Hash3(int32 X, int32 Y, int32 Z, uint32 S)
	{
		uint32 H = (uint32)X * 0x8da6b343u ^ (uint32)Y * 0xd8163841u ^ (uint32)Z * 0xcb1ab31fu ^ (S + 1u) * 0x165667b1u;
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15; H *= 0x27d4eb2du; H ^= H >> 16;
		return (H & 0xffffffu) / 16777215.f;
	}
	float Noise(const FVector3f& P, uint32 S)
	{
		const int32 X = FMath::FloorToInt(P.X), Y = FMath::FloorToInt(P.Y), Z = FMath::FloorToInt(P.Z);
		float FX = P.X - X, FY = P.Y - Y, FZ = P.Z - Z;
		FX = FX * FX * (3.f - 2.f * FX); FY = FY * FY * (3.f - 2.f * FY); FZ = FZ * FZ * (3.f - 2.f * FZ);
		const float A = FMath::Lerp(Hash3(X, Y, Z, S), Hash3(X + 1, Y, Z, S), FX);
		const float B = FMath::Lerp(Hash3(X, Y + 1, Z, S), Hash3(X + 1, Y + 1, Z, S), FX);
		const float C = FMath::Lerp(Hash3(X, Y, Z + 1, S), Hash3(X + 1, Y, Z + 1, S), FX);
		const float D = FMath::Lerp(Hash3(X, Y + 1, Z + 1, S), Hash3(X + 1, Y + 1, Z + 1, S), FX);
		return FMath::Lerp(FMath::Lerp(A, B, FY), FMath::Lerp(C, D, FY), FZ);
	}
	float Fbm(FVector3f P, uint32 S, int32 Oct = 6, float Gain = 0.5f)
	{
		float A = 0.5f, Sum = 0.f, Norm = 0.f;
		for (int32 i = 0; i < Oct; ++i) { Sum += A * Noise(P, S + i * 101u); Norm += A; P = P * 2.03f + FVector3f(17.1f, 3.7f, 9.3f); A *= Gain; }
		return Sum / Norm;
	}
	float Ridged(FVector3f P, uint32 S, int32 Oct = 5)
	{
		float A = 0.5f, Sum = 0.f, Norm = 0.f;
		for (int32 i = 0; i < Oct; ++i) { const float N = 1.f - FMath::Abs(2.f * Noise(P, S + i * 131u) - 1.f); Sum += A * N * N; Norm += A; P *= 2.07f; A *= 0.5f; }
		return Sum / Norm;
	}
	FVector3f Dir(float U, float V)
	{
		const float Lon = U * 2.f * PI, Lat = (0.5f - V) * PI;
		return FVector3f(FMath::Cos(Lat) * FMath::Cos(Lon), FMath::Cos(Lat) * FMath::Sin(Lon), FMath::Sin(Lat));
	}
	float Smooth(float A, float B, float X) { return FMath::SmoothStep(A, B, X); }
	struct FKey { float T; FLinearColor C; };
	FLinearColor Ramp(float T, std::initializer_list<FKey> K)
	{
		const FKey* Prev = K.begin();
		if (T <= Prev->T) return Prev->C;
		for (const FKey* It = K.begin() + 1; It != K.end(); ++It)
		{
			if (T <= It->T) return FMath::Lerp(Prev->C, It->C, (T - Prev->T) / FMath::Max(1e-4f, It->T - Prev->T));
			Prev = It;
		}
		return Prev->C;
	}
	FLinearColor L(float R, float G, float B) { return FLinearColor(R, G, B, 1.f); }

	enum class EWorld : uint8 { Terran, Ocean, Desert, Lava, Ice, Gas, IceGiant, Rock, Io, Europa, Titan };

	// One generator for every kind of world: U,V equirectangular -> linear colour. Seamless (sampled on the sphere).
	FLinearColor World(EWorld Kind, uint32 S, float U, float V, const FLinearColor& Tint)
	{
		const FVector3f D = Dir(U, V);
		const float Lat = D.Z;
		switch (Kind)
		{
		case EWorld::Terran: case EWorld::Ocean:
		{
			const float Sea = Kind == EWorld::Ocean ? 0.66f : 0.52f;
			float H = Fbm(D * 2.f, S, 7) + 0.18f * (Ridged(D * 3.5f, S + 3, 4) - 0.5f);
			FLinearColor C;
			if (H < Sea) C = FMath::Lerp(L(0.004f, 0.02f, 0.08f), L(0.02f, 0.11f, 0.22f), Smooth(Sea - 0.12f, Sea, H));
			else
			{
				const float E = (H - Sea) / (1.f - Sea);
				const float Dry = Smooth(0.45f, 0.7f, Fbm(D * 3.f, S + 9, 4) + (0.35f - FMath::Abs(Lat)) * 0.6f);
				C = Ramp(E, { {0.f, L(0.05f, 0.11f, 0.03f)}, {0.25f, L(0.09f, 0.13f, 0.04f)}, {0.5f, L(0.2f, 0.16f, 0.08f)}, {0.75f, L(0.3f, 0.27f, 0.24f)}, {0.9f, L(0.85f, 0.85f, 0.9f)} });
				C = FMath::Lerp(C, L(0.55f, 0.38f, 0.2f) * (0.8f + 0.3f * E), Dry * 0.85f);
			}
			if (FMath::Abs(Lat) > 0.82f - 0.12f * Fbm(D * 4.f, S + 5, 3)) C = L(0.85f, 0.9f, 0.95f);
			const FVector3f W(Fbm(D * 2.f, S + 31, 3), Fbm(D * 2.f, S + 37, 3), Fbm(D * 2.f, S + 41, 3));
			const float Cl = Smooth(0.5f, 0.72f, Fbm(D * 3.5f + W * 2.5f, S + 20, 6));
			return FMath::Lerp(C, L(0.9f, 0.92f, 0.95f), Cl * (Kind == EWorld::Ocean ? 0.95f : 0.8f));
		}
		case EWorld::Desert:
		{
			const float N = Fbm(D * 3.f, S, 6), Dune = Ridged(FVector3f(D.X * 14.f, D.Y * 14.f, D.Z * 4.f), S + 7, 3);
			FLinearColor C = Ramp(N, { {0.3f, L(0.25f, 0.1f, 0.04f)}, {0.5f, L(0.6f, 0.32f, 0.12f)}, {0.7f, L(0.8f, 0.55f, 0.3f)} }) * Tint;
			C = C * (0.85f + 0.25f * Dune);
			if (FMath::Abs(Lat) > 0.9f - 0.06f * N) C = L(0.85f, 0.8f, 0.78f);
			return C;
		}
		case EWorld::Lava:
		{
			const float Crust = 0.03f + 0.07f * Fbm(D * 6.f, S, 5);
			const float Crack = Smooth(0.78f, 0.95f, Ridged(D * 3.f, S + 2, 5));
			const float Pool = Smooth(0.68f, 0.78f, Fbm(D * 2.f, S + 8, 5));
			const float Hot = FMath::Max(Crack, Pool);
			return FMath::Lerp(L(Crust, Crust * 0.9f, Crust * 0.85f), Ramp(Hot, { {0.f, L(0.4f, 0.04f, 0.f)}, {0.6f, L(1.f, 0.25f, 0.02f)}, {1.f, L(1.f, 0.75f, 0.25f)} }), Hot);
		}
		case EWorld::Ice: case EWorld::Europa:
		{
			const float N = Fbm(D * 3.f, S, 6);
			FLinearColor C = (Kind == EWorld::Europa ? L(0.78f, 0.72f, 0.62f) : L(0.7f, 0.82f, 0.95f)) * (0.75f + 0.3f * N);
			const float Line = Smooth(0.86f, 0.95f, Ridged(D * 4.f, S + 4, 4));
			C = FMath::Lerp(C, Kind == EWorld::Europa ? L(0.35f, 0.18f, 0.08f) : L(0.15f, 0.35f, 0.6f), Line * 0.8f);
			return C;
		}
		case EWorld::Gas: case EWorld::IceGiant:
		{
			const bool bIce = Kind == EWorld::IceGiant;
			const FVector3f P(D.X * 2.5f, D.Y * 2.5f, D.Z * 10.f);
			const float T = Lat * (bIce ? 6.f : 14.f) + 1.6f * (Fbm(P, S, 6) - 0.5f) + 0.5f * (Fbm(D * 18.f, S + 7, 3) - 0.5f);
			const float Band = 0.5f + 0.5f * FMath::Sin(T * PI);
			FLinearColor C = bIce ? FMath::Lerp(Tint * 0.7f, Tint, Band) : Ramp(Band, { {0.f, Tint * L(0.5f, 0.35f, 0.25f)}, {0.5f, Tint * L(0.95f, 0.85f, 0.7f)}, {1.f, Tint * L(1.f, 0.97f, 0.9f)} });
			const FVector3f SC = FVector3f(0.8f, 0.3f, -0.35f).GetSafeNormal();
			const float Sd = FVector3f::Dist(D, SC);
			if (!bIce && Sd < 0.16f)
			{
				const float K = Smooth(0.16f, 0.06f, Sd) * (0.7f + 0.3f * Noise(D * 40.f, S + 50));
				C = FMath::Lerp(C, L(0.75f, 0.28f, 0.12f), K);
			}
			return C;
		}
		case EWorld::Io:
		{
			const float N = Fbm(D * 4.f, S, 6);
			FLinearColor C = Ramp(N, { {0.3f, L(0.7f, 0.45f, 0.08f)}, {0.5f, L(0.85f, 0.75f, 0.25f)}, {0.65f, L(0.9f, 0.88f, 0.7f)}, {0.8f, L(0.6f, 0.2f, 0.05f)} });
			if (Noise(D * 30.f, S + 9) > 0.86f) C = L(0.05f, 0.03f, 0.02f);
			return C;
		}
		case EWorld::Titan:
			return L(0.55f, 0.35f, 0.12f) * (0.85f + 0.2f * Fbm(D * 3.f, S, 4));
		case EWorld::Rock: default:
		{
			const float N = Fbm(D * 4.f, S, 7), R = Ridged(D * 8.f, S + 3, 4);
			return Tint * (0.25f + 0.55f * N + 0.2f * R);
		}
		}
	}

	FLinearColor SaturnRing(float U)
	{
		const float R = 1.24f + U * 1.03f;        // planet radii
		float A = 0.f; FLinearColor C = L(0.72f, 0.64f, 0.5f);
		if (R < 1.53f) { A = 0.12f + 0.1f * (R - 1.24f) / 0.29f; C = L(0.45f, 0.42f, 0.38f); }       // C ring
		else if (R < 1.95f) { A = 0.75f + 0.2f * FMath::Sin(R * 60.f); C = L(0.85f, 0.76f, 0.6f); }  // B ring
		else if (R < 2.03f) A = 0.04f;                                                               // Cassini division
		else if (R < 2.27f) { A = 0.55f; if (FMath::Abs(R - 2.214f) < 0.006f) A = 0.f; }              // A ring + Encke gap
		A *= 0.75f + 0.25f * Noise(FVector3f(R * 300.f, 0.f, 0.f), 3);
		return FLinearColor(C.R, C.G, C.B, FMath::Clamp(A, 0.f, 1.f));
	}
}

using namespace ISCos;

// ------------------------------------------------------------------------------------------------ building blocks
UTexture2D* AISSpaceScene::CosmosTex(const FString& Key, int32 W, int32 H, TFunctionRef<FLinearColor(float U, float V)> F)
{
	if (UTexture2D** Found = CosmosTexCache.Find(Key)) return *Found;
	UTexture2D* T = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!T) return nullptr;
	T->SRGB = true;
	T->NeverStream = true;
	T->AddressX = TA_Wrap;
	T->AddressY = TA_Clamp;
	T->Filter = TF_Bilinear;
	FTexture2DMipMap& Mip = T->GetPlatformData()->Mips[0];
	FColor* Px = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	ParallelFor(H, [&](int32 Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const FLinearColor C = F((X + 0.5f) / W, (Y + 0.5f) / H);
			FColor O = FLinearColor(FMath::Clamp(C.R, 0.f, 1.f), FMath::Clamp(C.G, 0.f, 1.f), FMath::Clamp(C.B, 0.f, 1.f), 1.f).ToFColor(true);
			O.A = (uint8)FMath::Clamp(FMath::RoundToInt(C.A * 255.f), 0, 255);
			Px[Y * W + X] = O;
		}
	});
	Mip.BulkData.Unlock();
	T->UpdateResource();
	CosmosTexCache.Add(Key, T);
	return T;
}

USceneComponent* AISSpaceScene::CosmosNode(USceneComponent* Parent)
{
	USceneComponent* N = NewObject<USceneComponent>(this);
	N->SetupAttachment(Parent ? Parent : SceneRoot);
	N->RegisterComponent();
	CosmosComps.Add(N);
	return N;
}

UStaticMeshComponent* AISSpaceScene::CosmosSphere(USceneComponent* Parent, float Radius, UMaterialInterface* Mat, bool bHi)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	UStaticMesh* Hi = bHi ? LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Infinisat/Meshes/SM_SphereHi.SM_SphereHi")) : nullptr;
	C->SetStaticMesh(Hi ? Hi : MSphere);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetupAttachment(Parent);
	C->SetRelativeScale3D(FVector(Radius / 50.f));
	C->CastShadow = false;
	C->RegisterComponent();
	if (Mat) C->SetMaterial(0, Mat);
	CosmosComps.Add(C);
	return C;
}

int32 AISSpaceScene::AddCosmosBody(const FString& Name, const FString& Group, int32 Parent, float OrbitR, float PeriodDays, float Radius, float SpinDegPerDay, const FString& Facts)
{
	FCosmosBody B;
	B.Name = Name; B.Group = Group; B.Parent = Parent; B.OrbitR = OrbitR; B.PeriodDays = PeriodDays; B.Radius = Radius;
	B.SpinDegPerDay = SpinDegPerDay; B.Facts = Facts;
	B.Phase = FMath::FRandRange(0.f, 2.f * PI);
	B.Angle = B.Phase;
	B.Star = Parent >= 0 ? (CosmosBodies[Parent].Star >= 0 ? CosmosBodies[Parent].Star : Parent) : -1;
	if (Parent >= 0) B.Center = CosmosBodies[Parent].Center;
	B.Pivot = CosmosNode(SceneRoot);
	B.Spinner = CosmosNode(B.Pivot);
	return CosmosBodies.Add(B);
}

int32 AISSpaceScene::CosmosDepth(int32 I) const
{
	int32 D = 0;
	while (CosmosBodies.IsValidIndex(I) && CosmosBodies[I].Parent >= 0) { I = CosmosBodies[I].Parent; ++D; }
	return D;
}

void AISSpaceScene::CosmosPlanetTex(int32 Body, UTexture2D* Tex, float Tilt, float Ambient)
{
	FCosmosBody& B = CosmosBodies[Body];
	B.Tilt = Tilt;
	if (Tex) B.Thumb = Tex;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatPlanet, this);
	if (Tex) M->SetTextureParameterValue(TEXT("Tex"), Tex);
	M->SetScalarParameterValue(TEXT("Ambient"), Ambient);
	CosmosSphere(B.Spinner, B.Radius, M);
}

void AISSpaceScene::CosmosAtmo(int32 Body, const FLinearColor& C, float Scale, float Intensity)
{
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatAtmo, this);
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	CosmosSphere(CosmosBodies[Body].Pivot, CosmosBodies[Body].Radius * Scale, M);
}

void AISSpaceScene::CosmosRing(USceneComponent* Parent, float R0, float R1, UTexture2D* Tex, bool bGlow, float Brightness, float Tilt)
{
	USceneComponent* N = CosmosNode(Parent);
	N->SetRelativeRotation(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Tilt)));
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this);
	P->SetupAttachment(N);
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->CastShadow = false;
	TArray<FVector> Vx; TArray<int32> Tri; TArray<FVector> Nrm; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	const int32 Seg = 256;
	for (int32 i = 0; i <= Seg; ++i)
	{
		const float A = 2.f * PI * i / Seg, Cs = FMath::Cos(A), Sn = FMath::Sin(A);
		Vx.Add(FVector(R0 * Cs, R0 * Sn, 0)); UV.Add(FVector2D(0.f, (float)i / Seg));
		Vx.Add(FVector(R1 * Cs, R1 * Sn, 0)); UV.Add(FVector2D(1.f, (float)i / Seg));
		Nrm.Add(FVector::UpVector); Nrm.Add(FVector::UpVector);
		if (i < Seg) { const int32 B = i * 2; Tri.Append({ B, B + 1, B + 2, B + 1, B + 3, B + 2 }); }
	}
	P->CreateMeshSection_LinearColor(0, Vx, Tri, Nrm, UV, Col, Tan, false);
	P->RegisterComponent();
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(bGlow ? MatGlow : MatRing, this);
	if (Tex) M->SetTextureParameterValue(TEXT("Tex"), Tex);
	M->SetScalarParameterValue(bGlow ? TEXT("Intensity") : TEXT("Brightness"), Brightness);
	P->SetMaterial(0, M);
	CosmosComps.Add(P);
}

UStaticMeshComponent* AISSpaceScene::CosmosSprite(USceneComponent* Parent, const FVector& Loc, float Size, UTexture2D* Tex, const FLinearColor& C, float Intensity, bool bFaceCamera)
{
	UStaticMeshComponent* S = NewObject<UStaticMeshComponent>(this);
	S->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	S->CastShadow = false;
	S->SetupAttachment(Parent);
	S->SetRelativeLocation(Loc);
	S->SetRelativeScale3D(FVector(Size / 100.f));
	if (!bFaceCamera) S->SetRelativeRotation(FRotationMatrix::MakeFromZ(-Loc.GetSafeNormal()).Rotator());
	S->RegisterComponent();
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, this);
	if (Tex) M->SetTextureParameterValue(TEXT("Tex"), Tex);
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	S->SetMaterial(0, M);
	CosmosComps.Add(S);
	if (bFaceCamera) CosmosBillboards.Add(S);
	return S;
}

void AISSpaceScene::CosmosOrbitDots(int32 Body, const FLinearColor& C)
{
	if (CosmosBodies.IsValidIndex(Body) && CosmosBodies[Body].Parent >= 0) CosmosBodies[Body].OrbitColor = C;
}

void AISSpaceScene::CosmosLabel(int32 Body, const FString& Text, const FColor& C)
{
	// drawn by the UI as crisp screen-space text (SISCosmosOverlay), not as 3D text
	CosmosBodies[Body].Label = Text;
	CosmosBodies[Body].LabelColor = C;
}

FVector AISSpaceScene::CosmosPos(int32 I) const
{
	return CosmosBodies.IsValidIndex(I) && CosmosBodies[I].Pivot ? CosmosBodies[I].Pivot->GetComponentLocation() : FVector::ZeroVector;
}

FVector AISSpaceScene::GetCameraLocation() const
{
	return Camera ? Camera->GetActorLocation() : FVector::ZeroVector;
}

int32 AISSpaceScene::CosmosStar(const FString& Name, const FVector& Center, float Radius, const FLinearColor& C, float Light, const FString& Facts)
{
	const int32 I = AddCosmosBody(Name, Name, -1, 0.f, 1.f, Radius, 14.f, Facts);
	FCosmosBody& B = CosmosBodies[I];
	B.Center = Center;
	B.Pivot->SetWorldLocation(Center);
	// photosphere: granulation + limb darkening is in the texture; additive so it glows and blooms
	UTexture2D* Surf = CosmosTex(TEXT("star_surface"), 512, 256, [](float U, float V)
	{
		const FVector3f D = Dir(U, V);
		const float G = 0.75f + 0.35f * Fbm(D * 24.f, 77, 4) + 0.15f * Fbm(D * 4.f, 78, 3);
		return L(G, G, G);
	});
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, this);
	M->SetTextureParameterValue(TEXT("Tex"), Surf);
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetScalarParameterValue(TEXT("Intensity"), 0.95f / FMath::Max3(C.R, C.G, C.B));   // keep the hue: brightest channel ~1
	CosmosSphere(B.Spinner, Radius, M);
	UTexture2D* Corona = CosmosTex(TEXT("corona"), 256, 256, [](float U, float V)
	{
		const float X = U * 2.f - 1.f, Y = V * 2.f - 1.f, R = FMath::Sqrt(X * X + Y * Y);
		const float Ang = FMath::Atan2(Y, X);
		const float Streams = 0.7f + 0.6f * Fbm(FVector3f(FMath::Cos(Ang) * 3.f, FMath::Sin(Ang) * 3.f, R * 2.f), 21, 4);
		const float Gl = (FMath::Exp(-(R - 0.28f) * 9.f) * Streams + 0.35f * FMath::Exp(-R * 3.f)) * (1.f - Smooth(0.75f, 1.f, R));
		return L(Gl, Gl, Gl);
	});
	CosmosSprite(B.Pivot, FVector::ZeroVector, Radius * 7.f, Corona, C, 1.1f, true);
	UPointLightComponent* PL = NewObject<UPointLightComponent>(this);
	PL->bUseInverseSquaredFalloff = false;
	PL->LightFalloffExponent = 0.25f;
	PL->SetupAttachment(B.Pivot);
	PL->SetIntensityUnits(ELightUnits::Unitless);
	PL->SetIntensity(9.f);
	PL->SetAttenuationRadius(Light);
	PL->SetLightColor(FMath::Lerp(C, FLinearColor::White, 0.6f));
	PL->SetCastShadows(false);
	PL->RegisterComponent();
	CosmosComps.Add(PL);
	CosmosLabel(I, Name, C.ToFColor(true));
	return I;
}

// ------------------------------------------------------------------------------------------------ the scene
void AISSpaceScene::ShowCosmos(const FISDesign& D)
{
	ClearView();
	View = EISView::Cosmos;
	// close-ups of planet surfaces and spacecraft: the default 10 cm near plane would cut them away at this scale
	if (GEngine && GetWorld()) GEngine->Exec(GetWorld(), TEXT("r.SetNearClipPlane 0.5"));
	if (!MatGlow) MatGlow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Glow.M_IS_Glow"));
	if (!MatRing) MatRing = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Ring.M_IS_Ring"));
	SunLight->SetVisibility(false);
	FillLight->SetIntensity(0.06f);
	if (UMaterialInstanceDynamic* SkyM = UMaterialInstanceDynamic::Create(MatSky, Sky))
	{
		if (UTexture* MW = LoadObject<UTexture>(nullptr, TEXT("/Game/Infinisat/Textures/T_MilkyWay.T_MilkyWay"))) SkyM->SetTextureParameterValue(TEXT("Tex"), MW);
		SkyM->SetScalarParameterValue(TEXT("Brightness"), 0.9f);
		Sky->SetMaterial(0, SkyM);
	}
	CosmosLines = NewObject<ULineBatchComponent>(this);
	CosmosLines->SetupAttachment(SceneRoot);
	CosmosLines->RegisterComponent();
	CosmosComps.Add(CosmosLines);
	FMath::RandInit(42);
	BuildCosmosBackdrop();
	BuildSolarSystem(D);
	BuildExoSystem(11, TEXT("Aurelia"), FVector(240000, 110000, 26000), FLinearColor(1.f, 0.45f, 0.25f), 900.f,
		TEXT("FICTIONAL red dwarf (M type), 0.12 Sun masses, 2,600 K. Like the real TRAPPIST-1, its planets huddle so close that a 'year' lasts days and the planets are tidally locked: one side always faces the star. Red dwarfs live trillions of years, longer than the Universe is old."));
	BuildExoSystem(23, TEXT("Vesper"), FVector(-210000, 220000, -38000), FLinearColor(0.65f, 0.78f, 1.f), 3400.f,
		TEXT("FICTIONAL blue-white giant (B type), 6 Sun masses, 15,000 K, about 800 times brighter than the Sun. Hot stars burn out in tens of millions of years, so life would have little time. Its habitable zone is far out, where only giant planets formed."));
	BuildExoSystem(37, TEXT("Halcyon"), FVector(-110000, -290000, 52000), FLinearColor(1.f, 0.72f, 0.42f), 1700.f,
		TEXT("FICTIONAL orange dwarf (K type), 0.8 Sun masses, 4,600 K. Astronomers think K stars are among the best hosts for life: calm, long-lived (15-30 billion years) and bright enough for a wide habitable zone."));
	BuildBlackHole(FVector(360000, -170000, -18000));
	CosmosBodyBase = CosmosBodies.Num();
	CosmosCompBase = CosmosComps.Num();
	SeedCosmosSim();
	for (int32 i = 0; i < CosmosBodies.Num(); ++i) if (CosmosBodies[i].Name == TEXT("Earth")) { FocusCosmos(i); break; }
	CamYaw = 150.f; CamPitch = -18.f;
	CamFocus = CamFocusGoal; CamDist = CamDistGoal;
}

void AISSpaceScene::FocusCosmos(int32 Idx)
{
	if (!CosmosBodies.IsValidIndex(Idx)) return;
	CosmosSel = Idx;
	const FCosmosBody& B = CosmosBodies[Idx];
	const bool bStar = B.Parent < 0;
	CamDistGoal = B.Radius * (bStar ? 9.f : 5.f);
	CamMin = B.Radius * 1.02f;   // close enough to see the surface (zoom works on the height above it, see ZoomCamera)
	CamMax = 1.4e6f;
}

void AISSpaceScene::BuildSolarSystem(const FISDesign& D)
{
	const FString G = TEXT("Solar System");
	const int32 SunI = CosmosStar(TEXT("Sun"), FVector::ZeroVector, 2200.f, FLinearColor(1.f, 0.82f, 0.55f), 130000.f,
		TEXT("Our star: a G-type main-sequence star, 4.6 billion years old. 1.39 million km across (109 Earths), 99.86% of the Solar System's mass. Surface 5,800 K, core 15 million K where 600 million tonnes of hydrogen fuse into helium every second. Light takes 8 min 20 s to reach Earth."));
	CosmosBodies[SunI].Group = G;
	struct FPl { const TCHAR* N; const TCHAR* Tex; float R, Orbit, Period, Spin, Tilt, Inc; FLinearColor Atmo; float AtmoI; const TCHAR* Facts; };
	const FPl Pl[] = {
		{ TEXT("Mercury"), TEXT("T_Mercury"), 90.f, 7000.f, 87.97f, 6.1f, 0.03f, 7.f, FLinearColor(0, 0, 0, 0), 0.f,
			TEXT("Smallest planet, 4,879 km. 0.39 AU from the Sun. Day side 430 C, night side -180 C: almost no atmosphere to hold heat. Its day (176 Earth days) is longer than its year (88 days). MESSENGER and BepiColombo mapped it.") },
		{ TEXT("Venus"), TEXT("T_Venus"), 205.f, 9500.f, 224.7f, -1.48f, 177.4f, 3.4f, FLinearColor(1.f, 0.8f, 0.45f), 0.9f,
			TEXT("Earth's twin in size (12,104 km) but a runaway greenhouse: CO2 air 92 times thicker than ours, 465 C at the surface, sulfuric-acid clouds. It spins backwards, very slowly (243-day day). Soviet Venera landers survived up to 2 hours.") },
		{ TEXT("Earth"), nullptr, 210.f, 12500.f, 365.25f, 360.98f, 23.44f, 0.f, FLinearColor(0.3f, 0.55f, 1.f), 0.7f,
			TEXT("Home. 12,742 km, 1 AU (150 million km) from the Sun, 30 km/s around it. 71% ocean. The only place we know life exists. ~10,000 active satellites orbit it today: most in LEO below 2,000 km, GPS at 20,200 km, weather and TV satellites in GEO at 35,786 km.") },
		{ TEXT("Mars"), TEXT("T_Mars"), 120.f, 16000.f, 686.98f, 350.9f, 25.19f, 1.85f, FLinearColor(0.9f, 0.5f, 0.3f), 0.35f,
			TEXT("The red planet: iron-oxide dust. 6,779 km. Olympus Mons (22 km tall) and Valles Marineris (4,000 km long). Thin CO2 air (0.6% of Earth's). Ancient rivers and lakes; Perseverance is collecting rock samples to look for signs of past life.") },
		{ TEXT("Jupiter"), TEXT("T_Jupiter2"), 900.f, 26000.f, 4332.6f, 870.5f, 3.13f, 1.3f, FLinearColor(0.9f, 0.75f, 0.55f), 0.3f,
			TEXT("Largest planet: 139,820 km, 318 Earth masses, a 10-hour day. Hydrogen and helium with ammonia clouds; the Great Red Spot is a storm bigger than Earth, observed since 1830. Its strong magnetic field traps deadly radiation belts (Juno's electronics hide in a titanium vault). 95 known moons.") },
		{ TEXT("Saturn"), TEXT("T_Saturn"), 760.f, 38000.f, 10759.f, 810.8f, 26.73f, 2.49f, FLinearColor(0.95f, 0.85f, 0.6f), 0.25f,
			TEXT("116,460 km, so light it would float on water. Its rings are 280,000 km wide but mostly only ~10 m thick: billions of chunks of water ice. Cassini (1997-2017) flew between the rings and the planet before diving in. 146 known moons.") },
		{ TEXT("Uranus"), TEXT("T_Uranus"), 400.f, 52000.f, 30687.f, -501.2f, 97.77f, 0.77f, FLinearColor(0.55f, 0.9f, 1.f), 0.45f,
			TEXT("Ice giant, 50,724 km, rolling on its side (98 deg tilt), maybe after a giant impact. Each pole gets 42 years of sunlight then 42 years of darkness. Methane makes it cyan. Only Voyager 2 has visited (1986).") },
		{ TEXT("Neptune"), TEXT("T_Neptune"), 390.f, 64000.f, 60190.f, 536.3f, 28.32f, 1.77f, FLinearColor(0.3f, 0.45f, 1.f), 0.5f,
			TEXT("Farthest planet, 30 AU: sunlight takes 4 hours. 49,244 km. Fastest winds in the Solar System, 2,100 km/h. Found in 1846 by mathematics (Le Verrier predicted it from Uranus's wobble) before anyone saw it.") },
	};
	TMap<FString, int32> Idx;
	for (const FPl& P : Pl)
	{
		const int32 I = AddCosmosBody(P.N, G, SunI, P.Orbit, P.Period, P.R, P.Spin, P.Facts);
		CosmosBodies[I].Inc = P.Inc;
		CosmosBodies[I].Node = FMath::FRandRange(0.f, 360.f);
		Idx.Add(P.N, I);
		if (P.Tex)
		{
			UTexture2D* T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Infinisat/Textures/%s.%s"), P.Tex, P.Tex));
			CosmosPlanetTex(I, T, P.Tilt, 0.02f);
		}
		else
		{
			CosmosBodies[I].Tilt = P.Tilt;
			UMaterialInterface* EM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Earth.M_IS_Earth"));
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(EM ? EM : MatPlanet, this);
			CosmosSphere(CosmosBodies[I].Spinner, P.R, M);
			CosmosBodies[I].SunDirMID = M;
			if (UMaterialInterface* CM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Infinisat/Materials/M_IS_Clouds.M_IS_Clouds"))) CosmosSphere(CosmosBodies[I].Spinner, P.R * 1.008f, CM);
		}
		if (P.AtmoI > 0.f) CosmosAtmo(I, P.Atmo, 1.035f, P.AtmoI);
		CosmosOrbitDots(I, FLinearColor(0.35f, 0.75f, 1.f) * 0.5f);
		CosmosLabel(I, P.N, FColor(200, 230, 255));
	}
	// rings
	UTexture2D* SatRing = CosmosTex(TEXT("ring_saturn"), 1024, 4, [](float U, float) { return SaturnRing(U); });
	CosmosRing(CosmosBodies[Idx[TEXT("Saturn")]].Pivot, 760.f * 1.24f, 760.f * 2.27f, SatRing, false, 1.1f, 26.73f);
	UTexture2D* URing = CosmosTex(TEXT("ring_uranus"), 1024, 4, [](float U, float)
	{
		float A = 0.f;
		for (float C : { 0.15f, 0.3f, 0.42f, 0.55f, 0.9f }) A += FMath::Exp(-FMath::Square((U - C) / 0.006f));
		return FLinearColor(0.4f, 0.42f, 0.45f, FMath::Clamp(A * 0.6f, 0.f, 1.f));
	});
	CosmosRing(CosmosBodies[Idx[TEXT("Uranus")]].Pivot, 400.f * 1.6f, 400.f * 2.05f, URing, false, 0.8f, 97.77f);

	// moons
	struct FMoon { const TCHAR* N; const TCHAR* P; float R, Orbit, Period; EWorld K; FLinearColor Tint; const TCHAR* Tex; const TCHAR* Facts; };
	const FMoon Moons[] = {
		{ TEXT("Moon"), TEXT("Earth"), 58.f, 720.f, 27.32f, EWorld::Rock, L(0.6f, 0.6f, 0.6f), TEXT("T_Moon2"),
			TEXT("3,474 km, 384,400 km away (1.3 light-seconds). Probably formed from debris when a Mars-sized body hit the young Earth. Tidally locked: we always see the same face. 12 people walked on it (Apollo, 1969-72); Artemis plans to return.") },
		{ TEXT("Io"), TEXT("Jupiter"), 58.f, 1500.f, 1.77f, EWorld::Io, L(1, 1, 1), nullptr,
			TEXT("The most volcanic place known: 400 active volcanoes, lava fountains hundreds of km high. Heated by tides as Jupiter and the other moons squeeze it.") },
		{ TEXT("Europa"), TEXT("Jupiter"), 50.f, 1900.f, 3.55f, EWorld::Europa, L(1, 1, 1), nullptr,
			TEXT("An icy shell over a salty ocean holding twice the water of Earth's oceans. One of the best places to look for life. NASA's Europa Clipper arrives in 2030.") },
		{ TEXT("Ganymede"), TEXT("Jupiter"), 84.f, 2500.f, 7.15f, EWorld::Rock, L(0.7f, 0.65f, 0.58f), nullptr,
			TEXT("The largest moon in the Solar System (5,268 km), bigger than Mercury, and the only moon with its own magnetic field. ESA's JUICE will orbit it in 2034.") },
		{ TEXT("Callisto"), TEXT("Jupiter"), 78.f, 3300.f, 16.7f, EWorld::Rock, L(0.45f, 0.4f, 0.36f), nullptr,
			TEXT("The most heavily cratered surface known: almost unchanged for 4 billion years. Outside Jupiter's worst radiation, so a candidate for a future human base.") },
		{ TEXT("Titan"), TEXT("Saturn"), 82.f, 2600.f, 15.95f, EWorld::Titan, L(1, 1, 1), nullptr,
			TEXT("Thick nitrogen atmosphere (1.5 bar), lakes and rain of liquid methane at -179 C. Huygens landed in 2005; the Dragonfly drone is due in 2034.") },
		{ TEXT("Triton"), TEXT("Neptune"), 44.f, 900.f, -5.88f, EWorld::Ice, L(1, 1, 1), nullptr,
			TEXT("Orbits backwards: probably a captured Kuiper-belt world. Nitrogen geysers erupt from its frozen surface (seen by Voyager 2).") },
	};
	for (const FMoon& Mn : Moons)
	{
		const int32 P = Idx[Mn.P];
		const int32 I = AddCosmosBody(Mn.N, G, P, Mn.Orbit, FMath::Abs(Mn.Period), Mn.R, 360.f / FMath::Abs(Mn.Period), Mn.Facts);
		CosmosBodies[I].Inc = CosmosBodies[P].Tilt + (Mn.Period < 0 ? 157.f : 0.f);
		if (FString(Mn.N) == TEXT("Moon")) CosmosBodies[I].Inc = 5.1f;
		UTexture2D* T = Mn.Tex ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Infinisat/Textures/%s.%s"), Mn.Tex, Mn.Tex)) : nullptr;
		if (!T)
		{
			const EWorld K = Mn.K; const FLinearColor Tint = Mn.Tint; const uint32 S = GetTypeHash(FString(Mn.N));
			T = CosmosTex(FString(TEXT("moon_")) + Mn.N, 512, 256, [K, Tint, S](float U, float V) { return World(K, S, U, V, Tint); });
		}
		CosmosPlanetTex(I, T, 0.f, 0.02f);
		if (Mn.K == EWorld::Titan) CosmosAtmo(I, FLinearColor(1.f, 0.6f, 0.2f), 1.08f, 1.2f);
		CosmosOrbitDots(I, FLinearColor(0.35f, 0.75f, 1.f) * 0.35f);
		CosmosLabel(I, Mn.N, FColor(170, 200, 225));
	}

	// spacecraft: the player's own design plus NASA's real ones (same models as the Sat-o-pedia)
	{
		const int32 E = Idx[TEXT("Earth")];
		const int32 I = AddCosmosBody(TEXT("Your satellite"), G, E, 210.f * 1.35f, 0.066f, 14.f, 0.f,
			TEXT("The spacecraft you designed in the clean room, in low Earth orbit (drawn ~1,000 times bigger than real so you can see it). At 500 km it circles Earth every ~95 minutes at 7.6 km/s."));
		CosmosBodies[I].Inc = 51.6f;
		SatRoot->AttachToComponent(CosmosBodies[I].Pivot, FAttachmentTransformRules::KeepRelativeTransform);
		SatRoot->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(14.f / 150.f)));
		BuildSatellite(SatRoot, D, SatParts);
		SatRoot->SetVisibility(true, true);
		CosmosLabel(I, TEXT("YOUR SATELLITE"), FColor(255, 170, 60));
	}
	struct FCraft { const TCHAR* N; const TCHAR* Model; const TCHAR* Parent; float Orbit, Period, Size, Inc; const TCHAR* Facts; };
	const FCraft Craft[] = {
		{ TEXT("ISS"), TEXT("ISS"), TEXT("Earth"), 210.f * 1.2f, 0.064f, 16.f, 51.6f, TEXT("International Space Station: 420 km up, 92 minutes per orbit, crewed since November 2000. As big as a football field.") },
		{ TEXT("GOES (GEO)"), TEXT("GOES"), TEXT("Earth"), 210.f * 2.6f, 1.f, 12.f, 0.f, TEXT("Weather satellite in geostationary orbit, 35,786 km up: one orbit per day, so it hovers over the same spot and watches storms grow.") },
		{ TEXT("Lunar Reconnaissance Orbiter"), TEXT("LRO"), TEXT("Moon"), 58.f * 1.4f, 0.08f, 8.f, 90.f, TEXT("Polar lunar orbit since 2009, mapping the Moon at 0.5 m per pixel for Artemis landing sites.") },
		{ TEXT("Juno"), TEXT("Juno"), TEXT("Jupiter"), 900.f * 1.5f, 53.f, 30.f, 90.f, TEXT("Solar-powered, in a long polar orbit of Jupiter since 2016, diving under the radiation belts every 53 days.") },
		{ TEXT("Parker Solar Probe"), TEXT("Parker"), TEXT("Sun"), 2200.f * 1.7f, 88.f, 40.f, 3.4f, TEXT("Touches the Sun's corona behind a carbon heat shield; at 192 km/s the fastest object humans have built.") },
	};
	Idx.Add(TEXT("Sun"), SunI);
	for (int32 i = 0; i < CosmosBodies.Num(); ++i) if (CosmosBodies[i].Name == TEXT("Moon")) Idx.Add(TEXT("Moon"), i);
	auto AddModel = [this](int32 Body, const TCHAR* Model, float Size)
	{
		UStaticMesh* SM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Infinisat/Models/%s/SM_%s.SM_%s"), Model, Model, Model));
		if (!SM) return;
		const float BR = FMath::Max(1.f, (float)SM->GetBounds().SphereRadius);
		AddMesh(CosmosBodies[Body].Spinner, SM, FTransform(FRotator::ZeroRotator, -SM->GetBounds().Origin * (Size / BR), FVector(Size / BR)), FLinearColor(0, 0, 0, 0), false, 0.f, nullptr);
	};
	for (const FCraft& C : Craft)
	{
		const int32 I = AddCosmosBody(C.N, G, Idx[C.Parent], C.Orbit, C.Period, C.Size, 0.f, C.Facts);
		CosmosBodies[I].Inc = C.Inc;
		AddModel(I, C.Model, C.Size);
		CosmosLabel(I, C.N, FColor(140, 220, 255));
	}
	{
		const int32 I = AddCosmosBody(TEXT("Voyager 1"), G, -1, 0.f, 1.f, 60.f, 0.f,
			TEXT("Launched 1977, now ~170 AU away (24 billion km: light needs over 22 hours). In interstellar space since 2012, still sending data on 4 W of power to the 70 m Deep Space Network dishes. Shown here at the edge of the scene."));
		CosmosBodies[I].Center = FVector(-84000, 36000, 15000);
		CosmosBodies[I].Pivot->SetWorldLocation(CosmosBodies[I].Center);
		AddModel(I, TEXT("Voyager"), 60.f);
		CosmosLabel(I, TEXT("Voyager 1 (interstellar space)"), FColor(200, 150, 255));
	}
	// asteroid belt between Mars and Jupiter + Kuiper belt beyond Neptune (instanced rocks)
	for (int32 Belt = 0; Belt < 2; ++Belt)
	{
		UInstancedStaticMeshComponent* I = AddDots(MSphere, Belt ? FLinearColor(0.55f, 0.65f, 0.8f) : FLinearColor(0.5f, 0.45f, 0.4f), 0.25f);
		const int32 N = Belt ? 2500 : 3000;
		const float R0 = Belt ? 70000.f : 19500.f, R1 = Belt ? 82000.f : 23000.f;
		for (int32 k = 0; k < N; ++k)
		{
			const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(R0, R1);
			I->AddInstance(FTransform(FRotator::ZeroRotator, FVector(R * FMath::Cos(A), R * FMath::Sin(A), FMath::FRandRange(-1.f, 1.f) * (R1 - R0) * 0.08f), FVector(FMath::FRandRange(0.04f, 0.16f) * (Belt ? 1.6f : 1.f))));
		}
	}
}

void AISSpaceScene::BuildExoSystem(int32 Seed, const FString& StarName, const FVector& Center, const FLinearColor& StarC, float StarR, const FString& StarFacts)
{
	FRandomStream R(Seed);
	const int32 Star = CosmosStar(StarName, Center, StarR, StarC, 60000.f, StarFacts);
	CosmosBodies[Star].Group = StarName + TEXT(" system");
	// a hand-picked lineup per system so each one shows something different
	TArray<EWorld> Kinds;
	if (Seed == 11) Kinds = { EWorld::Lava, EWorld::Desert, EWorld::Terran, EWorld::Ocean, EWorld::Ice };
	else if (Seed == 23) Kinds = { EWorld::Lava, EWorld::Gas, EWorld::IceGiant };
	else Kinds = { EWorld::Desert, EWorld::Terran, EWorld::Gas, EWorld::Ice };
	const TCHAR* Letters = TEXT("bcdefgh");
	float Orbit = StarR * 3.2f;
	for (int32 k = 0; k < Kinds.Num(); ++k)
	{
		const EWorld K = Kinds[k];
		const bool bGiant = K == EWorld::Gas || K == EWorld::IceGiant;
		const float Rad = bGiant ? R.FRandRange(500.f, 800.f) : R.FRandRange(130.f, 260.f);
		Orbit += (bGiant ? 5500.f : 2400.f) + R.FRandRange(0.f, 1500.f);
		const float AU = Orbit / 12500.f;
		const float Period = 365.25f * FMath::Pow(AU, 1.5f) * (Seed == 11 ? 0.03f : 1.f);
		const float EarthR = Rad / 210.f;
		const FLinearColor Tint = K == EWorld::Gas ? (Seed == 23 ? L(0.7f, 0.85f, 1.f) : L(1.f, 0.85f, 0.65f)) : K == EWorld::IceGiant ? L(0.35f, 0.5f, 0.95f) : K == EWorld::Desert ? L(1.f, 0.85f, 0.8f) : L(1, 1, 1);
		static const TCHAR* KindName[] = { TEXT("temperate rocky world"), TEXT("ocean world"), TEXT("desert world"), TEXT("lava world"), TEXT("ice world"), TEXT("gas giant"), TEXT("ice giant"), TEXT("rock"), TEXT("volcanic moon"), TEXT("ice moon"), TEXT("hazy moon") };
		static const TCHAR* KindWhy[] = {
			TEXT("Liquid water can exist here: it sits in the star's habitable zone. Green and brown land, white ice caps and swirling clouds."),
			TEXT("No continents at all: a global ocean hundreds of km deep over high-pressure ice. Real candidates include K2-18 b and TOI-270 d."),
			TEXT("Dry and dusty: the water was lost to space when the young star was violent. Like a bigger Mars."),
			TEXT("So close to its star that the rock melts: glowing magma oceans and cracks. Real example: 55 Cancri e (~2,000 C)."),
			TEXT("Far from its star: a frozen shell cut by blue cracks, maybe a hidden ocean below, like Europa."),
			TEXT("Hydrogen and helium with banded clouds and a giant storm. Gas giants form beyond the 'snow line' where ice helps build big cores fast."),
			TEXT("Water, ammonia and methane ices under a hydrogen sky: methane absorbs red light, so it looks blue. The most common planet size in the galaxy is between Earth and Neptune."),
			TEXT(""), TEXT(""), TEXT(""), TEXT("") };
		const FString Name = FString::Printf(TEXT("%s %c"), *StarName, Letters[k]);
		const FString Facts = FString::Printf(TEXT("FICTIONAL %s. Radius %s Earths, orbits at %s AU, one year = %s Earth days. %s"),
			KindName[(int32)K], *ISSim::Fmt(EarthR * (bGiant ? 4.f : 1.f), 1), *ISSim::Fmt(AU * (Seed == 11 ? 0.04f : 1.f), 2), *ISSim::Fmt(Period, Period < 10 ? 1 : 0), KindWhy[(int32)K]);
		const int32 I = AddCosmosBody(Name, StarName + TEXT(" system"), Star, Orbit, Period, Rad, bGiant ? 700.f : 300.f, Facts);
		CosmosBodies[I].Inc = R.FRandRange(-4.f, 4.f);
		const uint32 S = (uint32)(Seed * 977 + k * 131);
		UTexture2D* T = CosmosTex(FString::Printf(TEXT("exo_%d_%d"), Seed, k), 1024, 512, [K, S, Tint](float U, float V) { return World(K, S, U, V, Tint); });
		CosmosPlanetTex(I, T, R.FRandRange(0.f, 30.f), K == EWorld::Lava ? 0.9f : 0.02f);
		if (K == EWorld::Terran || K == EWorld::Ocean) CosmosAtmo(I, FLinearColor(0.35f, 0.6f, 1.f), 1.04f, 0.8f);
		else if (K == EWorld::Lava) CosmosAtmo(I, FLinearColor(1.f, 0.35f, 0.1f), 1.05f, 0.6f);
		else if (bGiant) CosmosAtmo(I, Tint, 1.03f, 0.35f);
		if (K == EWorld::Gas && Seed != 37)
		{
			UTexture2D* RT = CosmosTex(FString::Printf(TEXT("ring_exo_%d"), Seed), 1024, 4, [S](float U, float)
			{
				const float A = (0.25f + 0.6f * Noise(FVector3f(U * 40.f, 0.f, 0.f), S)) * Smooth(0.f, 0.1f, U) * (1.f - Smooth(0.85f, 1.f, U));
				return FLinearColor(0.6f, 0.75f, 0.9f, A);
			});
			CosmosRing(CosmosBodies[I].Pivot, Rad * 1.4f, Rad * 2.8f, RT, false, 1.f, CosmosBodies[I].Tilt);
		}
		if (K == EWorld::Gas && Seed == 37)
		{
			// a habitable moon around the gas giant (think Pandora)
			const uint32 MS = S + 7;
			const int32 M = AddCosmosBody(Name + TEXT(" I (moon)"), StarName + TEXT(" system"), I, Rad * 2.4f, 6.f, 95.f, 60.f,
				TEXT("FICTIONAL habitable moon of a gas giant: Earth-like air and oceans, warmed by its star and by tides. Its sky is dominated by the giant planet. Exomoons are hard to detect; none is confirmed yet."));
			UTexture2D* MT = CosmosTex(TEXT("exo_moon_halcyon"), 1024, 512, [MS](float U, float V) { return World(EWorld::Terran, MS, U, V, FLinearColor::White); });
			CosmosPlanetTex(M, MT, 5.f, 0.02f);
			CosmosAtmo(M, FLinearColor(0.35f, 0.65f, 1.f), 1.05f, 0.9f);
			CosmosOrbitDots(M, FLinearColor(0.6f, 0.8f, 0.6f) * 0.4f);
			CosmosLabel(M, Name + TEXT(" I"), FColor(190, 255, 200));
		}
		CosmosOrbitDots(I, StarC * 0.45f);
		CosmosLabel(I, Name, FColor(255, 225, 180));
	}
}

void AISSpaceScene::BuildBlackHole(const FVector& Center)
{
	const float R = 1400.f;
	const int32 I = AddCosmosBody(TEXT("Abyss (black hole)"), TEXT("Abyss"), -1, 0.f, 1.f, R, 0.f,
		TEXT("FICTIONAL stellar black hole, 20 Sun masses, event horizon 118 km across (drawn much bigger). Nothing, not even light, escapes from inside. Gas spiralling in forms a white-hot accretion disk at millions of degrees; the bright ring is light bent all the way around the hole. Real ones were imaged by the Event Horizon Telescope: M87* (2019) and our galaxy's Sgr A* (2022)."));
	FCosmosBody& B = CosmosBodies[I];
	B.Center = Center;
	B.Pivot->SetWorldLocation(Center);
	{ UMaterialInstanceDynamic* BM = UMaterialInstanceDynamic::Create(MatUnlit, this); BM->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black); BM->SetScalarParameterValue(TEXT("Intensity"), 0.f); CosmosSphere(B.Pivot, R, BM); }
	UTexture2D* Disk = CosmosTex(TEXT("accretion"), 512, 256, [](float U, float V)
	{
		const float A = V * 2.f * PI;
		const float Turb = Fbm(FVector3f(U * 22.f, FMath::Cos(A) * 2.5f, FMath::Sin(A) * 2.5f), 91, 5);
		const float Hot = FMath::Pow(1.f - U, 1.6f) * (0.55f + 0.9f * Turb) * Smooth(0.f, 0.04f, U) * (1.f - Smooth(0.75f, 1.f, U));
		const FLinearColor C = Ramp(1.f - U, { {0.f, L(0.5f, 0.08f, 0.02f)}, {0.5f, L(1.f, 0.45f, 0.1f)}, {0.85f, L(1.f, 0.85f, 0.6f)}, {1.f, L(0.85f, 0.9f, 1.f)} });
		return C * Hot;
	});
	CosmosRing(B.Spinner, R * 1.5f, R * 6.f, Disk, true, 0.55f, 72.f);
	B.SpinDegPerDay = 40.f;
	UTexture2D* Photon = CosmosTex(TEXT("photon_ring"), 256, 256, [](float U, float V)
	{
		const float X = U * 2.f - 1.f, Y = V * 2.f - 1.f, Rr = FMath::Sqrt(X * X + Y * Y);
		const float Pr = 1.6f * FMath::Exp(-FMath::Square((Rr - 0.5f) / 0.025f)) + 0.45f * FMath::Exp(-FMath::Square((Rr - 0.52f) / 0.12f));
		return L(Pr, Pr * 0.75f, Pr * 0.5f);
	});
	CosmosSprite(B.Pivot, FVector::ZeroVector, R * 4.2f, Photon, FLinearColor(1.f, 0.85f, 0.65f), 2.f, true);
	CosmosLabel(I, TEXT("Abyss (black hole)"), FColor(255, 160, 90));
	UPointLightComponent* PL = NewObject<UPointLightComponent>(this);
	PL->bUseInverseSquaredFalloff = false;
	PL->SetupAttachment(B.Pivot);
	PL->SetIntensityUnits(ELightUnits::Unitless);
	PL->SetIntensity(4.f);
	PL->SetAttenuationRadius(R * 12.f);
	PL->SetLightColor(FLinearColor(1.f, 0.6f, 0.3f));
	PL->SetCastShadows(false);
	PL->RegisterComponent();
	CosmosComps.Add(PL);
}

void AISSpaceScene::BuildCosmosBackdrop()
{
	CosmosBackdrop = CosmosNode(SceneRoot);
	// stars at "infinity" (the backdrop follows the camera): spectral classes with real colours, few bright, many faint
	struct FCls { FLinearColor C; float Share, Glow; } Cls[] = {   // real star colours are pale: only a hint of blue or orange
		{ FLinearColor(0.82f, 0.88f, 1.f), 0.08f, 5.f }, { FLinearColor(0.93f, 0.95f, 1.f), 0.17f, 4.f }, { FLinearColor(1.f, 0.99f, 0.95f), 0.25f, 3.5f },
		{ FLinearColor(1.f, 0.93f, 0.8f), 0.25f, 3.f }, { FLinearColor(1.f, 0.82f, 0.66f), 0.25f, 2.6f } };
	FRandomStream R(5);
	const FQuat Plane(FVector::ForwardVector, FMath::DegreesToRadians(60.f));   // Milky Way band, matches the sky texture tilt
	for (const FCls& C : Cls)
	{
		UInstancedStaticMeshComponent* I = AddDots(MSphere, C.C, C.Glow);
		I->AttachToComponent(CosmosBackdrop, FAttachmentTransformRules::KeepRelativeTransform);
		const int32 N = FMath::RoundToInt(9000 * C.Share);
		for (int32 k = 0; k < N; ++k)
		{
			FVector D = R.VRand();
			if (R.FRand() < 0.45f) { D.Z *= 0.12f; D = Plane.RotateVector(D.GetSafeNormal()); }
			const float Mag = FMath::Pow(R.FRand(), 7.f);
			I->AddInstance(FTransform(FRotator::ZeroRotator, D * 1.6e6f, FVector(2.2f + 9.f * Mag)));   // points, not blobs
		}
	}
	// no painted nebulae or cartoon galaxies: the real Milky Way panorama (sky texture) is the background, like Universe Sandbox
}

void AISSpaceScene::ClearCosmos()
{
	if (SatRoot && SatRoot->GetAttachParent() != SceneRoot) SatRoot->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	for (USceneComponent* C : CosmosComps) if (C) C->DestroyComponent();
	CosmosComps.Reset();
	CosmosBillboards.Reset();
	CosmosBodies.Reset();
	NBody = FISNBodySim();
	SimToCosmos.Reset();
	CosmosScale.Reset();
	CosmosBodyBase = CosmosCompBase = 0;
	bCosmosMission = false;
	CosmosProbe = -1;
	CosmosBackdrop = nullptr;
	if (GEngine && GetWorld() && View == EISView::Cosmos) GEngine->Exec(GetWorld(), TEXT("r.SetNearClipPlane 10"));
	if (SunLight) SunLight->SetVisibility(true);
	if (FillLight) FillLight->SetIntensity(1.2f);
}

// ------------------------------------------------------------------------------------------------ a mission in the Cosmos view
UTexture2D* AISSpaceScene::MoonTexture(const FString& Name)
{
	// same generated surfaces as the explorer's moons (shared cache key)
	struct FM { const TCHAR* N; EWorld K; FLinearColor Tint; };
	const FM Ms[] = { { TEXT("Io"), EWorld::Io, L(1, 1, 1) }, { TEXT("Europa"), EWorld::Europa, L(1, 1, 1) },
		{ TEXT("Ganymede"), EWorld::Rock, L(0.7f, 0.65f, 0.58f) }, { TEXT("Callisto"), EWorld::Rock, L(0.45f, 0.4f, 0.36f) } };
	for (const FM& M : Ms)
	{
		if (Name != M.N) continue;
		const EWorld K = M.K; const FLinearColor Tint = M.Tint; const uint32 S = GetTypeHash(Name);
		return CosmosTex(FString(TEXT("moon_")) + Name, 512, 256, [K, Tint, S](float U, float V) { return World(K, S, U, V, Tint); });
	}
	return nullptr;
}

// The solar-system view during a deep-space cruise: planets where they really are on the mission's day, the clock locked to
// the mission, the probe and its whole route drawn on the compressed layout.
void AISSpaceScene::SetCosmosMission(bool bOn)
{
	bCosmosMission = bOn && View == EISView::Cosmos && bVoyage && Voy.bValid;
	if (!bCosmosMission) return;
	CosmosMissionT = MissionDay * 86400.0;
	SeedCosmosSim(Voy.Epoch + CosmosMissionT);
	CosmosDays = MissionDay;
	// an Earth mission's satellite on rails does not belong here
	const int32 Ys = CosmosFind(TEXT("Your satellite"));
	if (Ys >= 0) { CosmosBodies[Ys].Label.Reset(); CosmosBodies[Ys].Pivot->SetVisibility(false, true); }

	const FString Where = ViewDest == EISDest::Jupiter ? TEXT("Jupiter") : TEXT("interstellar space");
	const int32 I = AddCosmosBody(TEXT("Your probe"), TEXT("Solar System"), -1, 0.f, 1.f, 30.f, 0.f,
		FString::Printf(TEXT("Your spacecraft on its way to %s. The orange line is where it has been, the blue line where gravity takes it next: no engine needed, it coasts on a real orbit around the Sun."), *Where));
	if (MatGlow)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(MatGlow, this);
		M->SetTextureParameterValue(TEXT("Tex"), CosmosTex(TEXT("white"), 4, 4, [](float, float) { return FLinearColor::White; }));
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.6f, 0.2f));
		M->SetScalarParameterValue(TEXT("Intensity"), 3.f);
		CosmosSphere(CosmosBodies[I].Spinner, 30.f, M, false);
	}
	CosmosLabel(I, TEXT("YOUR PROBE"), FColor(255, 170, 60));
	CosmosProbe = I;
	// interstellar: name the two boundaries of the Sun's bubble (rings are drawn every frame)
	if (ViewDest == EISDest::Interstellar)
	{
		const int32 SunS = NBody.Find(TEXT("Sun"));
		const double Au = 1.495978707e8;
		struct FMark { double AuR; const TCHAR* Text; };
		const FMark Marks[] = { { 94.0, TEXT("Termination shock ~94 AU") }, { 120.0, TEXT("HELIOPAUSE ~120 AU: interstellar space") } };
		for (const FMark& Mk : Marks)
		{
			const int32 K = AddCosmosBody(Mk.Text, TEXT("Solar System"), -1, 0.f, 1.f, 1.f, 0.f, TEXT("The edge of the bubble the solar wind blows into the gas between the stars. Voyager 1 crossed the heliopause in 2012 at 121 AU, Voyager 2 in 2018."));
			CosmosBodies[K].Pivot->SetWorldLocation(FVector(0, CompressKm(SunS, Mk.AuR * Au, 0.f), 0));
			CosmosLabel(K, Mk.Text, FColor(200, 150, 255));
		}
	}
	CosmosBodyBase = CosmosBodies.Num();   // the sandbox's Reset keeps them
	CosmosCompBase = CosmosComps.Num();
	FocusCosmos(I);
	CamDistGoal = 12000.f;                 // the inner solar system with the route in view, not a close-up of a dot
}

void AISSpaceScene::TickCosmosMission(TArray<FVector>& SimPos, TArray<uint8>& SimDone)
{
	if (!bCosmosMission || !Voy.bValid || !CosmosBodies.IsValidIndex(CosmosProbe) || NBody.B.Num() == 0 || SimPos.Num() == 0) return;
	const double T = MissionDay * 86400.0;
	const int32 SunS = NBody.Find(TEXT("Sun"));
	const FVector SunC = CosmosSimPos(SunS, SimPos, SimDone);
	// heliocentric position on the compressed layout; never drawn inside Earth or Jupiter (their spheres are exaggerated)
	auto Disp = [&](double t)
	{
		const FVector3d H = Voy.HelioPos(t);
		FVector Out = SunC + FVector(H.GetSafeNormal()) * CompressKm(SunS, H.Size(), 0.f);
		for (const TCHAR* N : { TEXT("Earth"), TEXT("Jupiter") })
		{
			const int32 Ob = ISOrbit::FindBody(N), S = NBody.Find(N);
			if (Ob < 0 || S < 0 || !NBody.B[S].bAlive) continue;
			const FVector C = CosmosSimPos(S, SimPos, SimDone);
			const float Touch = CosmosDispR(SimToCosmos[S]) * 1.15f;
			const FVector D = Out - C;
			if (D.Size() < Touch) Out = C + (D.IsNearlyZero() ? FVector(H - ISOrbit::BodyPos(Ob, Voy.Epoch + t)).GetSafeNormal() : D.GetSafeNormal()) * Touch;
		}
		return Out;
	};
	CosmosBodies[CosmosProbe].Pivot->SetWorldLocation(Disp(T));
	if (!CosmosLines) return;
	const double A = Voy.DepartT, B = Voy.ArriveT;
	const int32 N = 300;
	FVector Prev = Disp(A);
	for (int32 i = 1; i <= N; ++i)
	{
		const double t = A + (B - A) * i / N;
		const FVector P = Disp(t);
		CosmosLines->DrawLine(Prev, P, t <= T ? FLinearColor(1.f, 0.55f, 0.15f) : FLinearColor(0.3f, 0.85f, 1.f), SDPG_World, 0.f, 0.f);
		Prev = P;
	}
	if (ViewDest == EISDest::Interstellar)
		for (const double Au : { 94.0, 120.0 })
		{
			const float Rr = CompressKm(SunS, Au * 1.495978707e8, 0.f);
			FVector P0 = SunC + FVector(Rr, 0, 0);
			for (int32 k = 1; k <= 256; ++k)
			{
				const float Ang = 2.f * PI * k / 256;
				const FVector P1 = SunC + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * Rr;
				CosmosLines->DrawLine(P0, P1, Au < 100.0 ? FLinearColor(0.4f, 0.8f, 1.f) * 0.6f : FLinearColor(0.8f, 0.4f, 1.f), SDPG_World, 0.f, 0.f);
				P0 = P1;
			}
		}
}

// ------------------------------------------------------------------------------------------------ sandbox (real gravity)
void AISSpaceScene::SeedCosmosSim(double T0)
{
	ISNBody::SeedSolarSystem(NBody, T0);
	SimToCosmos.Init(-1, NBody.B.Num());
	for (int32 c = 0; c < CosmosBodies.Num(); ++c)
	{
		FCosmosBody& B = CosmosBodies[c];
		B.Sim = -1;
		if (B.Pivot && !B.Pivot->IsVisible()) B.Pivot->SetVisibility(true, true);   // merged away in the last run
		if (B.Group != TEXT("Solar System")) continue;
		const int32 S = NBody.Find(B.Name);
		if (S < 0 || SimToCosmos[S] >= 0) continue;
		B.Sim = S;
		B.RealR0 = NBody.B[S].Radius;
		B.Pivot->SetRelativeScale3D(FVector::OneVector);
		SimToCosmos[S] = c;
	}
	// the hand-made layout as a real-km -> scene-units curve per parent (e.g. Sun: Mercury 57.9e6 km -> 7000 ... Neptune -> 64000)
	CosmosScale.Reset();
	for (const FCosmosBody& B : CosmosBodies)
	{
		if (B.Sim < 0 || B.Parent < 0 || CosmosBodies[B.Parent].Sim < 0) continue;
		const int32 P = CosmosBodies[B.Parent].Sim;
		CosmosScale.FindOrAdd(P).Add(FVector2D(FMath::Loge((NBody.B[B.Sim].R - NBody.B[P].R).Size()), FMath::Loge((double)B.OrbitR)));
	}
	for (TPair<int32, TArray<FVector2D>>& It : CosmosScale) It.Value.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
	CosmosEventT = 0.f;
}

float AISSpaceScene::CosmosDispR(int32 C) const
{
	if (!CosmosBodies.IsValidIndex(C)) return 0.f;
	const FCosmosBody& B = CosmosBodies[C];
	return B.Sim >= 0 ? B.Radius * (float)(NBody.B[B.Sim].Radius / B.RealR0) : B.Radius;
}

float AISSpaceScene::CompressKm(int32 SimP, double Km, float Touch) const
{
	double D;
	const double LK = FMath::Loge(FMath::Max(Km, 1.0));
	const TArray<FVector2D>* Pts = CosmosScale.Find(SimP);
	if (Pts && Pts->Num() > 0)
	{
		// log-log interpolation between the original children; proportional beyond the innermost / outermost
		const TArray<FVector2D>& P = *Pts;
		if (LK <= P[0].X) D = FMath::Exp(P[0].Y + LK - P[0].X);
		else if (LK >= P.Last().X) D = FMath::Exp(P.Last().Y + LK - P.Last().X);
		else
		{
			int32 k = 1;
			while (P[k].X < LK) ++k;
			D = FMath::Exp(FMath::Lerp(P[k - 1].Y, P[k].Y, (LK - P[k - 1].X) / (P[k].X - P[k - 1].X)));
		}
	}
	else
	{
		// nothing orbited it at the start: square-root squeeze relative to its radius (close to the hand-made moon spacing)
		const int32 C = SimToCosmos.IsValidIndex(SimP) ? SimToCosmos[SimP] : -1;
		D = (C >= 0 ? CosmosDispR(C) : 100.f) * 0.55 * FMath::Sqrt(Km / FMath::Max(NBody.B[SimP].Radius, 1.0));
	}
	return (float)FMath::Max(D, (double)Touch);   // never draw two bodies overlapping before they really touch
}

// scene position of a simulated body: its current primary's scene position + the squeezed real offset
FVector AISSpaceScene::CosmosSimPos(int32 S, TArray<FVector>& Memo, TArray<uint8>& Done) const
{
	if (Done[S]) return Memo[S];
	const int32 P = NBody.Primary(S);
	FVector Pos = FVector::ZeroVector;   // the heaviest body sits where the Sun was built
	if (P >= 0)
	{
		const FVector3d D = NBody.B[S].R - NBody.B[P].R;
		const double Km = D.Size();
		const float Touch = CosmosDispR(SimToCosmos[S]) + CosmosDispR(SimToCosmos[P]);
		Pos = CosmosSimPos(P, Memo, Done) + D / FMath::Max(Km, 1e-9) * CompressKm(P, Km, Touch);
	}
	Memo[S] = Pos;
	Done[S] = 1;
	return Pos;
}

void AISSpaceScene::CosmosScaleMass(int32 I, double F)
{
	if (!CosmosIsSim(I) || !CosmosAlive(I)) return;
	FISNBody& X = NBody.B[CosmosBodies[I].Sim];
	X.Mu *= F;
	X.Radius *= FMath::Pow(F, 1.0 / 3.0);   // same density: radius grows with the cube root of the mass
	NBody.Changed();
	CosmosNotice(FString::Printf(TEXT("%s now has %.3g Earth masses."), *X.Name, X.Mu / 398600.4418));
}

void AISSpaceScene::CosmosNudge(int32 I, double F)
{
	if (!CosmosIsSim(I) || !CosmosAlive(I)) return;
	const int32 S = CosmosBodies[I].Sim, P = NBody.Primary(S);
	FISNBody& X = NBody.B[S];
	const FVector3d Vp = P >= 0 ? NBody.B[P].V : FVector3d::ZeroVector;
	X.V = Vp + (X.V - Vp) * F;
	NBody.Changed();
	CosmosNotice(FString::Printf(TEXT("%s %s by %.0f%%."), *X.Name, F > 1.0 ? TEXT("sped up") : TEXT("slowed down"), FMath::Abs(F - 1.0) * 100.0));
}

void AISSpaceScene::CosmosAddBody(int32 Kind)
{
	if (NBody.B.Num() == 0) return;
	const int32 Sel = CosmosIsSim(CosmosSel) && CosmosAlive(CosmosSel) ? CosmosSel : SimToCosmos[NBody.Heaviest()];
	const FISNBody T = NBody.B[CosmosBodies[Sel].Sim];   // copy: Add may reallocate
	FRandomStream Rng(++CosmosAdded * 7919);
	const double Ang = Rng.FRandRange(0.f, 2.f * PI);
	const FVector3d Dir = FVector3d(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(0.2f, 0.8f)).GetSafeNormal();
	FString Name, Facts;
	double Mu, Rad;
	FVector3d R, V;
	float DispR;
	EWorld Look = EWorld::Rock;
	if (Kind == 0)
	{
		Name = FString::Printf(TEXT("New moon %d"), CosmosAdded);
		Mu = FMath::Min(4902.8, T.Mu * 0.01);
		Rad = 1737.4 * FMath::Pow(Mu / 4902.8, 1.0 / 3.0);
		const double A = 30.0 * T.Radius;
		const double Vc = ISOrbit::CircularSpeed(T.Mu + Mu, A);
		R = T.R + FVector3d(FMath::Cos(Ang), FMath::Sin(Ang), 0) * A;
		V = T.V + FVector3d(-FMath::Sin(Ang), FMath::Cos(Ang), 0) * Vc;
		DispR = FMath::Max(20.f, CosmosBodies[Sel].Radius * 0.27f);
		Facts = FString::Printf(TEXT("A moon you added on a circular orbit around %s, 30 of its radii out. Speed it up or slow it down to stretch the orbit."), *T.Name);
	}
	else if (Kind == 1)
	{
		Name = FString::Printf(TEXT("Rogue planet %d"), CosmosAdded);
		Mu = 126686534.0;
		Rad = 69911.0;
		Look = EWorld::Gas;
		const int32 H = NBody.Heaviest();
		const double D = 3.0 * (T.R - NBody.B[H].R).Size() + 4.5e8;   // starts ~3 AU further out than the target
		R = T.R + Dir * D;
		V = T.V - Dir * 20.0;
		DispR = 900.f;
		Facts = FString::Printf(TEXT("A Jupiter-mass planet thrown out of another star system, falling at 20 km/s towards %s. Real rogue planets exist: microlensing surveys suggest there are billions in our galaxy."), *T.Name);
	}
	else
	{
		Name = FString::Printf(TEXT("Impactor %d"), CosmosAdded);
		Mu = FMath::Min(T.Mu * 0.1, 42828.0);   // at most Mars-sized, like the Theia impact that probably made our Moon
		Rad = 3389.5 * FMath::Pow(Mu / 42828.0, 1.0 / 3.0);
		const double D = FMath::Max(60.0 * T.Radius, 1.0e6);
		R = T.R + Dir * D;
		V = T.V - Dir * 15.0;
		DispR = FMath::Max(18.f, CosmosBodies[Sel].Radius * 0.25f);
		Facts = FString::Printf(TEXT("A body on a collision course with %s at 15 km/s. When they touch they merge: mass and momentum are kept."), *T.Name);
	}
	const int32 S = NBody.Add(Name, Mu, Rad, R, V);
	const int32 I = AddCosmosBody(Name, TEXT("Solar System"), Sel, 0.f, 1.f, DispR, 20.f, Facts);
	const uint32 Seed = GetTypeHash(Name);
	UTexture2D* Tex = CosmosTex(Look == EWorld::Gas ? TEXT("added_gas") : TEXT("added_rock"), 512, 256,
		[Look, Seed](float U, float Vv) { return World(Look, Seed, U, Vv, Look == EWorld::Gas ? L(0.8f, 0.9f, 1.f) : L(0.6f, 0.55f, 0.5f)); });
	CosmosPlanetTex(I, Tex, 0.f, 0.02f);
	CosmosOrbitDots(I, FLinearColor(1.f, 0.6f, 0.2f) * 0.6f);
	CosmosLabel(I, Name, FColor(255, 190, 120));
	CosmosBodies[I].Sim = S;
	CosmosBodies[I].RealR0 = Rad;
	check(S == SimToCosmos.Num());
	SimToCosmos.Add(I);
	CosmosNotice(FString::Printf(TEXT("Added %s."), *Name));
}

void AISSpaceScene::CosmosResetSim()
{
	if (CosmosBodyBase <= 0) return;
	for (int32 i = CosmosCompBase; i < CosmosComps.Num(); ++i) if (CosmosComps[i]) CosmosComps[i]->DestroyComponent();
	CosmosComps.SetNum(CosmosCompBase);
	CosmosBodies.SetNum(CosmosBodyBase);
	if (!CosmosBodies.IsValidIndex(CosmosSel)) FocusCosmos(CosmosFind(TEXT("Earth")));
	SeedCosmosSim();
	CosmosDays = 0.0;
	CosmosNotice(TEXT("Back to the real Solar System."));
}

FString AISSpaceScene::CosmosSimInfo(int32 I) const
{
	if (!CosmosIsSim(I)) return FString();
	const int32 S = CosmosBodies[I].Sim;
	const FISNBody& X = NBody.B[S];
	if (!X.bAlive) return TEXT("Destroyed in a collision.");
	const double Me = X.Mu / 398600.4418;
	FString Out = Me >= 1000.0 ? FString::Printf(TEXT("%.0f Earth masses (%.3g Suns)"), Me, X.Mu / 1.32712440018e11) : FString::Printf(TEXT("%.3g Earth masses"), Me);
	const int32 P = NBody.Primary(S);
	if (P < 0) return Out + TEXT(". Everything else orbits it.");
	const FISNBody& Pb = NBody.B[P];
	const FVector3d R = X.R - Pb.R, V = X.V - Pb.V;
	const FISElements E = ISOrbit::Elements(Pb.Mu + X.Mu, Pb.Radius, R, V);
	const double Km = R.Size();
	Out += FString::Printf(TEXT(". Orbits %s at %.2f km/s, %s km away."), *Pb.Name, V.Size(),
		*(Km >= 1e6 ? FString::Printf(TEXT("%.1f million"), Km / 1e6) : FString::Printf(TEXT("%.0f"), Km)));
	if (E.E < 1.0)
	{
		const double Days = E.Period / 86400.0;
		Out += Days < 2.0 ? FString::Printf(TEXT(" Period %.1f h"), Days * 24.0) : Days < 730.0 ? FString::Printf(TEXT(" Period %.1f d"), Days) : FString::Printf(TEXT(" Period %.1f y"), Days / 365.25);
		Out += FString::Printf(TEXT(", eccentricity %.3f."), E.E);
	}
	else Out += TEXT(" On an escape path.");
	if (E.Periapsis < 0.0 && FVector3d::DotProduct(R, V) < 0.0) Out += TEXT(" Collision course!");
	return Out;
}

void AISSpaceScene::TickCosmos(float Dt, const FVector& CamLoc)
{
	CosmosDays += Dt * CosmosRate;
	CosmosEventT -= Dt;
	// the Solar System on real gravity; everything with Sim < 0 (spacecraft, other stars) stays on its rails below
	TArray<FVector> SimPos;
	TArray<uint8> SimDone;
	if (NBody.B.Num() > 0)
	{
		// a mission in progress drives the clock (the probe and the planets stay on the same day); otherwise the speed buttons
		double Step = (double)Dt * CosmosRate * 86400.0;
		if (bCosmosMission) { Step = FMath::Max(0.0, MissionDay * 86400.0 - CosmosMissionT); CosmosMissionT += Step; }
		NBody.Advance(Step);
		CosmosDays = bCosmosMission ? MissionDay : NBody.T / 86400.0;
		if (!NBody.LastEvent.IsEmpty()) { CosmosNotice(NBody.LastEvent); NBody.LastEvent.Reset(); }
		SimPos.SetNumZeroed(NBody.B.Num());
		SimDone.SetNumZeroed(NBody.B.Num());
		for (int32 s = 0; s < NBody.B.Num(); ++s)
		{
			const int32 c = SimToCosmos[s];
			if (c < 0) continue;
			FCosmosBody& B = CosmosBodies[c];
			if (!NBody.B[s].bAlive) { if (B.Pivot->IsVisible()) B.Pivot->SetVisibility(false, true); continue; }
			B.Pivot->SetWorldLocation(CosmosSimPos(s, SimPos, SimDone));
			B.Pivot->SetRelativeScale3D(FVector(NBody.B[s].Radius / B.RealR0));
		}
	}
	for (FCosmosBody& B : CosmosBodies)
	{
		if (B.Parent >= 0 && B.Sim < 0)
		{
			const FCosmosBody& P = CosmosBodies[B.Parent];
			if (!P.Pivot->IsVisible() && B.Pivot->IsVisible()) B.Pivot->SetVisibility(false, true);   // its planet was destroyed
			B.Angle += FMath::Min(2.f * PI / FMath::Max(0.01f, B.PeriodDays) * CosmosRate, 1.2f) * Dt;
			const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(B.Node)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(B.Inc));
			B.Pivot->SetWorldLocation(P.Pivot->GetComponentLocation() + Q.RotateVector(FVector(FMath::Cos(B.Angle), FMath::Sin(B.Angle), 0.f) * B.OrbitR));
		}
		B.SpinAngle += FMath::Clamp(B.SpinDegPerDay * CosmosRate, -45.f, 45.f) * Dt;
		B.Spinner->SetRelativeRotation(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(B.Tilt)) * FQuat(FVector::UpVector, FMath::DegreesToRadians(B.SpinAngle)));
		if (B.SunDirMID && B.Star >= 0)
			B.SunDirMID->SetVectorParameterValue(TEXT("SunDir"), FLinearColor((CosmosBodies[B.Star].Pivot->GetComponentLocation() - B.Pivot->GetComponentLocation()).GetSafeNormal()));
	}
	if (CosmosBodies.IsValidIndex(CosmosSel)) CamFocusGoal = CosmosBodies[CosmosSel].Pivot->GetComponentLocation();
	// orbits: 1-pixel screen-space lines, redrawn every frame so moon orbits follow their planet (KSP map style)
	if (CosmosLines)
	{
		CosmosLines->Flush();
		for (int32 i = 0; i < CosmosBodies.Num(); ++i)
		{
			const FCosmosBody& B = CosmosBodies[i];
			if (B.Parent < 0 || B.OrbitColor.A <= 0.f || !B.Pivot->IsVisible()) continue;
			if (B.Sim >= 0)
			{
				// the conic it is on right now around what it currently orbits (osculating orbit): mass edits reshape it at once
				const int32 S = B.Sim, Pi = NBody.Primary(S);
				if (Pi < 0) continue;
				const FISNBody& X = NBody.B[S];
				const FISNBody& Pb = NBody.B[Pi];
				FVector3d R = X.R - Pb.R, V = X.V - Pb.V;
				const double Mu = Pb.Mu + X.Mu;
				const FISElements E = ISOrbit::Elements(Mu, Pb.Radius, R, V);
				const bool bClosed = E.E < 1.0 && E.Period > 0.0;
				const int32 N = bClosed ? 192 : 96;
				const double Step = (bClosed ? E.Period : 4.0 * R.Size() / FMath::Max(V.Size(), 1e-6)) / N;
				const FVector C = CosmosSimPos(Pi, SimPos, SimDone);
				const float Touch = CosmosDispR(i) + CosmosDispR(SimToCosmos[Pi]);
				auto Map = [&](const FVector3d& Q) { const double K = Q.Size(); return C + Q / FMath::Max(K, 1e-9) * CompressKm(Pi, K, Touch); };
				FVector Prev = Map(R);
				for (int32 k = 1; k <= N; ++k)
				{
					ISOrbit::Kepler(Mu, R, V, Step);
					const FVector Pt = Map(R);
					CosmosLines->DrawLine(Prev, Pt, B.OrbitColor, SDPG_World, 0.f, 0.f);
					Prev = Pt;
				}
				continue;
			}
			const FVector C = CosmosBodies[B.Parent].Pivot->GetComponentLocation();
			const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(B.Node)) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(B.Inc));
			const int32 N = B.OrbitR > 20000.f ? 256 : 128;
			FVector Prev = C + Q.RotateVector(FVector(B.OrbitR, 0, 0));
			for (int32 k = 1; k <= N; ++k)
			{
				const float A = 2.f * PI * k / N;
				const FVector P = C + Q.RotateVector(FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * B.OrbitR);
				CosmosLines->DrawLine(Prev, P, B.OrbitColor, SDPG_World, 0.f, 0.f);
				Prev = P;
			}
		}
	}
	TickCosmosMission(SimPos, SimDone);
	if (CosmosBackdrop) CosmosBackdrop->SetWorldLocation(CamLoc);
	Sky->SetWorldLocation(CamLoc);
	for (USceneComponent* S : CosmosBillboards)
		if (S) S->SetWorldRotation(FRotationMatrix::MakeFromZ(CamLoc - S->GetComponentLocation()).Rotator());
}
