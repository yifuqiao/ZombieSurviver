// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombieSurviver.h"
#include "WoodWall.h"
#include "WoodPlank.h"
#include "ZombieSurviverCharacter.h"
#include "ZombieSurviverGameMode.h"
#include "SurvivorFeedback.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"

AWoodWall::AWoodWall()
{
	PrimaryActorTick.bCanEverTick = true;
	m_hitPoint = 0;
	m_maxHitPoint = 0;
	ShakeRemaining = 0.f;
	LastImpactSoundTime = -1.f;
	DoorLocation = FVector::ZeroVector;
	DoorNormal = FVector::ForwardVector;
	DoorHalfWidth = 70.f;
	DoorHalfHeight = 120.f;
	EntranceIndex = 0;
	bBreached = false;
	bBreachLatched = false;
	DoorSeal = nullptr;
}

void AWoodWall::BeginPlay()
{
	Super::BeginPlay();
	BaseRotation = GetActorRotation();
	DoorLocation = GetActorLocation();
	DoorNormal = GetActorForwardVector().GetSafeNormal2D();
	if (DoorNormal.IsNearlyZero())
	{
		DoorNormal = FVector::ForwardVector;
	}
	SetActorTickEnabled(false);

	if (m_woodPrefab != nullptr)
	{
		for (int32 i = 0; i < m_leftPoints.Num(); ++i)
		{
			if (!m_rightPoints.IsValidIndex(i) || m_leftPoints[i] == nullptr || m_rightPoints[i] == nullptr)
			{
				continue;
			}

			FActorSpawnParameters Parameters;
			Parameters.Template = m_woodPrefab;

			AWoodPlank* Plank = GetWorld()->SpawnActor<AWoodPlank>(m_woodPrefab->GetClass(), Parameters);
			if (!Plank)
			{
				continue;
			}
			Plank->AttachRootComponentToActor(this);
			Plank->Init(m_leftPoints[i]->GetActorLocation(), m_rightPoints[i]->GetActorLocation(), GetActorRotation(), this);
			Plank->SetOwner(this);
			m_allWoodPlanks.Add(Plank);
		}
	}

	CacheDoor();
	EnsureSeal();
	UpdateTotals();
	RefreshSeal();

	if (UWorld* World = GetWorld())
	{
		if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode()))
		{
			Mode->RegisterEntrance(this);
		}
	}
}

void AWoodWall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ShakeRemaining <= 0.f)
	{
		SetActorRotation(BaseRotation);
		SetActorTickEnabled(false);
		return;
	}

	ShakeRemaining = FMath::Max(0.f, ShakeRemaining - DeltaSeconds);
	const float Magnitude = ShakeRemaining * 14.f;
	FRotator Shaken = BaseRotation;
	Shaken.Roll += FMath::Sin(ShakeRemaining * 58.f) * Magnitude;
	Shaken.Pitch += FMath::Cos(ShakeRemaining * 41.f) * Magnitude * 0.3f;
	SetActorRotation(Shaken);
}

void AWoodWall::TakeDamage(int32 deltaDamange)
{
	if (deltaDamange <= 0 || bBreached)
	{
		return;
	}

	ShakeRemaining = 0.38f;
	SetActorTickEnabled(true);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastImpactSoundTime > 0.16f)
	{
		LastImpactSoundTime = Now;
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), DoorLocation, 0.4f, FMath::FRandRange(0.9f, 1.3f));
	}

	int32 Remaining = deltaDamange;
	bool bBrokeOne = false;
	while (Remaining > 0)
	{
		AWoodPlank* Target = FindOutermostIntact();
		if (!Target)
		{
			break;
		}

		const FVector HitLocation = Target->GetActorLocation();
		const int32 Spent = FMath::Min(Target->GetLife(), Remaining);
		const bool bBroke = Target->ApplyHit(Spent);
		Remaining -= Spent;
		SurvivorFeedback::Burst(GetWorld(), HitLocation, SurvivorFeedback::Dust(), 0.22f);
		if (!bBroke)
		{
			break;
		}

		BreakPlank(Target);
		bBrokeOne = true;
	}

	UpdateTotals();
	RefreshSeal();

	if (!bBrokeOne)
	{
		return;
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (FVector::DistSquared(Pawn->GetActorLocation(), DoorLocation) < 900.f * 900.f)
			{
				if (AZombieSurviverCharacter* Survivor = Cast<AZombieSurviverCharacter>(Pawn))
				{
					Survivor->AddCameraTrauma(0.5f);
				}
			}
		}
	}
}

int32 AWoodWall::RepairWithHammer(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	AWoodPlank* Target = nullptr;
	for (int32 Index = m_allWoodPlanks.Num() - 1; Index >= 0; --Index)
	{
		AWoodPlank* Plank = m_allWoodPlanks[Index];
		if (Plank && Plank->IsBroken())
		{
			Target = Plank;
			break;
		}
	}

	if (!Target)
	{
		int32 Lowest = MAX_int32;
		for (AWoodPlank* Plank : m_allWoodPlanks)
		{
			if (Plank && !Plank->IsBroken() && Plank->GetLife() < Plank->GetMaxLife() && Plank->GetLife() < Lowest)
			{
				Lowest = Plank->GetLife();
				Target = Plank;
			}
		}
	}

	if (!Target)
	{
		return 0;
	}

	const int32 Before = Target->GetLife();
	const int32 MaxBefore = Target->GetMaxLife();
	if (!Target->Nail(Amount))
	{
		return 0;
	}

	const bool bFinishedBoard = Target->GetLife() >= Target->GetMaxLife() && Before < MaxBefore;
	const FVector Loc = Target->GetActorLocation();
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Click(), Loc, 0.9f, FMath::FRandRange(0.46f, 0.62f));
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), Loc, 0.32f, FMath::FRandRange(1.2f, 1.45f));
	SurvivorFeedback::Burst(GetWorld(), Loc, SurvivorFeedback::Dust(), 0.32f);
	SurvivorFeedback::SpawnShards(GetWorld(), Loc, DoorNormal, 3, SurvivorFeedback::Cube(), nullptr, FLinearColor(0.42f, 0.28f, 0.12f), 0.045f, 160.f);

	UpdateTotals();
	RefreshSeal();

	if (bFinishedBoard)
	{
		if (AZombieSurviverGameMode* Mode = GetWorld() ? Cast<AZombieSurviverGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			const bool bFrail = Target->GetGeneration() >= 3;
			Mode->AddFloater(Loc + FVector(0.f, 0.f, 24.f), bFrail ? FString(TEXT("WORN")) : FString(TEXT("BOARDED")), bFrail ? FLinearColor(0.85f, 0.55f, 0.22f) : FLinearColor(0.76f, 0.92f, 0.42f), 1.25f);
		}
		return 2;
	}
	return 1;
}

bool AWoodWall::CanRepair() const
{
	for (AWoodPlank* Plank : m_allWoodPlanks)
	{
		if (Plank && Plank->GetLife() < Plank->GetMaxLife())
		{
			return true;
		}
	}
	return false;
}

float AWoodWall::GetIntegrity() const
{
	return m_maxHitPoint > 0 ? FMath::Clamp(float(m_hitPoint) / float(m_maxHitPoint), 0.f, 1.f) : 0.f;
}

int32 AWoodWall::GetIntactPlanks() const
{
	int32 Intact = 0;
	for (AWoodPlank* Plank : m_allWoodPlanks)
	{
		if (Plank && !Plank->IsBroken() && Plank->GetLife() > 0)
		{
			++Intact;
		}
	}
	return Intact;
}

float AWoodWall::GetWearAlpha() const
{
	if (m_allWoodPlanks.Num() == 0)
	{
		return 0.f;
	}

	int32 GenerationSum = 0;
	for (AWoodPlank* Plank : m_allWoodPlanks)
	{
		if (Plank)
		{
			GenerationSum += Plank->GetGeneration();
		}
	}
	return FMath::Clamp(float(GenerationSum) / float(m_allWoodPlanks.Num() * 4), 0.f, 1.f);
}

FString AWoodWall::GetEntranceLabel() const
{
	return EntranceIndex > 0 ? FString::Printf(TEXT("DOOR %d"), EntranceIndex) : FString(TEXT("DOOR"));
}

bool AWoodWall::Separates(const FVector& A, const FVector& B) const
{
	const float SideA = FVector::DotProduct(A - DoorLocation, DoorNormal);
	const float SideB = FVector::DotProduct(B - DoorLocation, DoorNormal);
	return (SideA > 40.f && SideB < -40.f) || (SideA < -40.f && SideB > 40.f);
}

FVector AWoodWall::GetAttackPoint(const AActor* Attacker) const
{
	const FVector From = Attacker ? Attacker->GetActorLocation() : DoorLocation + DoorNormal * 200.f;
	const float Side = FMath::Sign(FVector::DotProduct(From - DoorLocation, DoorNormal));
	const float Signed = FMath::IsNearlyZero(Side) ? 1.f : Side;
	const FVector Sideways = FVector::CrossProduct(DoorNormal, FVector::UpVector);
	float Slot = 0.f;
	if (Attacker)
	{
		Slot = float(int32(Attacker->GetUniqueID() % 5) - 2) * 32.f;
		Slot = FMath::Clamp(Slot, -(DoorHalfWidth - 20.f), DoorHalfWidth - 20.f);
	}
	return DoorLocation + DoorNormal * Signed * 150.f + Sideways * Slot;
}

FVector AWoodWall::GetInsidePoint(const FVector& InsideLocation) const
{
	const float Side = FMath::Sign(FVector::DotProduct(InsideLocation - DoorLocation, DoorNormal));
	const float Signed = FMath::IsNearlyZero(Side) ? -1.f : Side;
	return DoorLocation + DoorNormal * Signed * 150.f;
}

void AWoodWall::SetEntranceIndex(int32 Index)
{
	EntranceIndex = Index;
}

void AWoodWall::BreakPlank(AWoodPlank* Plank)
{
	if (!Plank || Plank->IsHidden())
	{
		return;
	}

	const FVector Loc = Plank->GetActorLocation();
	UStaticMesh* Mesh = nullptr;
	UMaterialInterface* Material = nullptr;
	if (UStaticMeshComponent* PlankMesh = Plank->GetMesh())
	{
		Mesh = PlankMesh->GetStaticMesh();
		Material = PlankMesh->GetMaterial(0);
	}
	SurvivorFeedback::Burst(GetWorld(), Loc, SurvivorFeedback::Sparks(), 0.55f);
	SurvivorFeedback::Burst(GetWorld(), Loc, SurvivorFeedback::Smoke(), 0.4f);
	SurvivorFeedback::SpawnShards(GetWorld(), Loc, FVector::UpVector, 6, Mesh ? Mesh : SurvivorFeedback::Cube(), Material, FLinearColor(0.35f, 0.22f, 0.1f), Mesh ? 0.12f : 0.06f, 220.f);
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), Loc, 0.8f, FMath::FRandRange(0.7f, 1.05f));
	Plank->BreakApart();

	if (AZombieSurviverGameMode* Mode = GetWorld() ? Cast<AZombieSurviverGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		Mode->AddFloater(Loc + FVector(0.f, 0.f, 20.f), FString(TEXT("CRACK")), FLinearColor(0.92f, 0.45f, 0.18f), 1.15f);
	}
}

void AWoodWall::CacheDoor()
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	float Width = 0.f;

	const int32 PointCount = FMath::Min(m_leftPoints.Num(), m_rightPoints.Num());
	for (int32 Index = 0; Index < PointCount; ++Index)
	{
		if (!m_leftPoints[Index] || !m_rightPoints[Index])
		{
			continue;
		}
		const FVector Left = m_leftPoints[Index]->GetActorLocation();
		const FVector Right = m_rightPoints[Index]->GetActorLocation();
		Sum += (Left + Right) * 0.5f;
		MinZ = FMath::Min(MinZ, FMath::Min(Left.Z, Right.Z));
		MaxZ = FMath::Max(MaxZ, FMath::Max(Left.Z, Right.Z));
		Width = FMath::Max(Width, FVector::Dist2D(Left, Right));
		++Count;

		if (Index == 0)
		{
			FVector Span = Right - Left;
			Span.Z = 0.f;
			const FVector Normal = FVector::CrossProduct(Span.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
			if (!Normal.IsNearlyZero())
			{
				DoorNormal = Normal;
			}
		}
	}

	if (Count > 0)
	{
		DoorLocation = Sum / float(Count);
		DoorHalfWidth = FMath::Clamp(Width * 0.5f + 12.f, 46.f, 170.f);
		const float SpanZ = MaxZ > MinZ ? (MaxZ - MinZ) * 0.5f : 0.f;
		DoorHalfHeight = FMath::Clamp(FMath::Max(120.f, SpanZ + 48.f), 120.f, 180.f);
	}
	else
	{
		DoorLocation = GetActorLocation();
	}
}

void AWoodWall::EnsureSeal()
{
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Component);
		if (Prim && Prim->GetName().Contains(TEXT("WallVolume")))
		{
			DoorSeal = Prim;
			break;
		}
	}

	if (!DoorSeal)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(this, TEXT("DoorSeal"));
		Box->SetMobility(EComponentMobility::Movable);
		if (!RootComponent)
		{
			RootComponent = Box;
		}
		Box->RegisterComponent();
		if (Box != RootComponent)
		{
			Box->AttachTo(RootComponent, NAME_None, EAttachLocation::KeepWorldPosition);
		}
		Box->SetBoxExtent(FVector(28.f, DoorHalfWidth, DoorHalfHeight));
		Box->SetWorldLocation(DoorLocation);
		Box->SetWorldRotation(DoorNormal.Rotation());
		Box->SetHiddenInGame(true);
		DoorSeal = Box;
	}

	if (DoorSeal)
	{
		DoorSeal->CanCharacterStepUpOn = ECB_No;
	}
}

void AWoodWall::SetSealBlocks(bool bBlock)
{
	if (!DoorSeal)
	{
		return;
	}

	DoorSeal->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorSeal->SetCollisionObjectType(ECC_WorldDynamic);
	DoorSeal->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorSeal->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DoorSeal->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	DoorSeal->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	if (bBlock)
	{
		DoorSeal->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}

void AWoodWall::RefreshSeal()
{
	const bool bNowOpen = GetIntactPlanks() <= 0;
	SetSealBlocks(!bNowOpen);
	if (!bBreachLatched)
	{
		bBreached = bNowOpen;
		bBreachLatched = true;
		return;
	}
	if (bNowOpen == bBreached)
	{
		return;
	}

	bBreached = bNowOpen;
	if (bBreached)
	{
		SurvivorFeedback::Burst(GetWorld(), DoorLocation, SurvivorFeedback::Smoke(), 0.9f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Boom(), DoorLocation, 0.45f, 0.8f);
	}
	else
	{
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), DoorLocation, 0.7f, 1.35f);
	}

	if (AZombieSurviverGameMode* Mode = GetWorld() ? Cast<AZombieSurviverGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (bBreached)
		{
			Mode->NotifyEntranceBreached(this);
		}
		else
		{
			Mode->NotifyEntranceSealed(this);
		}
	}
}

void AWoodWall::UpdateTotals()
{
	m_hitPoint = 0;
	m_maxHitPoint = 0;
	for (AWoodPlank* Plank : m_allWoodPlanks)
	{
		if (!Plank)
		{
			continue;
		}
		m_maxHitPoint += Plank->GetMaxLife();
		m_hitPoint += FMath::Max(0, Plank->GetLife());
	}
}

AWoodPlank* AWoodWall::FindOutermostIntact() const
{
	for (int32 Index = m_allWoodPlanks.Num() - 1; Index >= 0; --Index)
	{
		AWoodPlank* Plank = m_allWoodPlanks[Index];
		if (Plank && !Plank->IsBroken() && Plank->GetLife() > 0)
		{
			return Plank;
		}
	}
	return nullptr;
}
