// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombieSurviver.h"
#include "WoodWall.h"
#include "WoodPlank.h"
#include "ZombieSurviverCharacter.h"
#include "SurvivorFeedback.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWoodWall::AWoodWall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//m_woodPrefab = CreateDefaultSubobject<AWoodPlank>(TEXT("wood"));
	m_hitPoint = 100;
	m_maxHitPoint = 100;
	m_brokenCount = 0;
	ShakeRemaining = 0.f;
	LastImpactSoundTime = -1.f;
}

// Called when the game starts or when spawned
void AWoodWall::BeginPlay()
{
	Super::BeginPlay();
	BaseRotation = GetActorRotation();
	SetActorTickEnabled(false);

	if (m_woodPrefab == nullptr)
		return;

	for (int32 i = 0; i < m_leftPoints.Num(); ++i)
	{
		if (!m_rightPoints.IsValidIndex(i) || m_leftPoints[i] == nullptr || m_rightPoints[i] == nullptr)
		{
			continue;
		}

		FActorSpawnParameters Parameters;
		Parameters.Template = m_woodPrefab;
		
		AWoodPlank* plank = GetWorld()->SpawnActor<AWoodPlank>(m_woodPrefab->GetClass(), Parameters);
		if (!plank)
		{
			continue;
		}
		// this order is important
		plank->AttachRootComponentToActor(this);
		plank->Init(m_leftPoints[i]->GetActorLocation(), m_rightPoints[i]->GetActorLocation(), this->GetActorRotation());
		plank->SetOwner(GetOwner());
		m_allWoodPlanks.Add(plank);
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
	if (deltaDamange <= 0)
	{
		return;
	}

	m_hitPoint = FMath::Max(0, m_hitPoint - deltaDamange);
	ShakeRemaining = 0.38f;
	SetActorTickEnabled(true);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastImpactSoundTime > 0.16f)
	{
		LastImpactSoundTime = Now;
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), GetActorLocation(), 0.4f, FMath::FRandRange(0.9f, 1.3f));
	}

	const int32 PlankCount = m_allWoodPlanks.Num();
	int32 BrokenTarget = 0;
	if (PlankCount > 0 && m_maxHitPoint > 0 && m_hitPoint < m_maxHitPoint)
	{
		const int32 Band = FMath::Max(1, m_maxHitPoint / PlankCount);
		BrokenTarget = FMath::Clamp((m_maxHitPoint - m_hitPoint) / Band, 0, PlankCount);
	}

	bool bBrokeOne = false;
	while (m_brokenCount < BrokenTarget)
	{
		const int32 Index = PlankCount - 1 - m_brokenCount;
		if (m_allWoodPlanks.IsValidIndex(Index))
		{
			BreakPlank(m_allWoodPlanks[Index]);
			bBrokeOne = true;
		}
		++m_brokenCount;
	}

	if (bBrokeOne)
	{
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) < 900.f * 900.f)
				{
					if (AZombieSurviverCharacter* Survivor = Cast<AZombieSurviverCharacter>(Pawn))
					{
						Survivor->AddCameraTrauma(0.5f);
					}
				}
			}
		}
	}

	if (m_hitPoint <= 0)
	{
		SurvivorFeedback::Burst(GetWorld(), GetActorLocation(), SurvivorFeedback::Smoke(), 0.9f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Boom(), GetActorLocation(), 0.45f, 0.8f);
	}
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

	Plank->SetActorHiddenInGame(true);
	Plank->SetActorEnableCollision(false);
	Plank->SetActorTickEnabled(false);
}

