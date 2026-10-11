// // Zombie Surviver Prototype \n Written by Yifu Qiao

#include "ZombieSurviver.h"
#include "WoodPlank.h"
#include "WoodWall.h"
#include "Components/StaticMeshComponent.h"

AWoodPlank::AWoodPlank()
{
	PrimaryActorTick.bCanEverTick = true;
	m_woodPlankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("wood"));
	RootComponent = m_woodPlankMesh;
	Barricade = nullptr;
	MaxLife = 96;
	Life = 96;
	Generation = 0;
	bBroken = false;
	Shake = 0.f;
	BaseScale = FVector(1.f);
	BaseRotation = FRotator::ZeroRotator;
	SetActorTickEnabled(false);
}

void AWoodPlank::Init(FVector point1, FVector point2, FRotator rot, AWoodWall* Wall)
{
	Barricade = Wall;
	m_woodPlankMesh->SetWorldLocation((point1 + point2) * 0.5f);
	m_woodPlankMesh->SetWorldRotation(rot);
	BaseRotation = rot;
	BaseScale = m_woodPlankMesh->GetComponentScale();
	if (BaseScale.IsNearlyZero())
	{
		BaseScale = FVector(1.f);
	}

	MaxLife = 96;
	Life = MaxLife;
	Generation = 0;
	bBroken = false;
	Shake = 0.f;

	m_woodPlankMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	m_woodPlankMesh->SetCollisionObjectType(ECC_WorldDynamic);
	m_woodPlankMesh->SetCollisionResponseToAllChannels(ECR_Block);
	m_woodPlankMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	m_woodPlankMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	m_woodPlankMesh->CanCharacterStepUpOn = ECB_No;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	RefreshPose();
	SetActorTickEnabled(false);
}

bool AWoodPlank::Nail(int32 Amount)
{
	if (Amount <= 0 || (!bBroken && Life >= MaxLife))
	{
		return false;
	}

	if (bBroken)
	{
		++Generation;
		MaxLife = FMath::Max(48, 96 - Generation * 12);
		Life = 0;
		bBroken = false;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
	}

	Life = FMath::Min(MaxLife, Life + Amount);
	Shake = 0.28f;
	SetActorTickEnabled(true);
	RefreshPose();
	return true;
}

bool AWoodPlank::ApplyHit(int32 Amount)
{
	if (bBroken || Amount <= 0)
	{
		return false;
	}

	Life = FMath::Max(0, Life - Amount);
	Shake = 0.34f;
	SetActorTickEnabled(true);
	RefreshPose();
	if (Life > 0)
	{
		return false;
	}

	bBroken = true;
	return true;
}

void AWoodPlank::BreakApart()
{
	bBroken = true;
	Life = 0;
	Shake = 0.f;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AWoodPlank::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBroken)
	{
		SetActorTickEnabled(false);
		return;
	}

	Shake = FMath::Max(0.f, Shake - DeltaSeconds);
	RefreshPose();
	if (Shake <= 0.f)
	{
		SetActorTickEnabled(false);
	}
}

void AWoodPlank::RefreshPose()
{
	const float Intact = MaxLife > 0 ? FMath::Clamp(float(Life) / float(MaxLife), 0.f, 1.f) : 0.f;
	FRotator Posed = BaseRotation;
	const float Loose = (1.f - Intact) * 9.f;
	Posed.Roll += Loose;
	Posed.Pitch += Loose * 0.4f;
	if (Shake > 0.f)
	{
		Posed.Roll += FMath::Sin(Shake * 78.f) * Shake * 24.f;
		Posed.Pitch += FMath::Cos(Shake * 54.f) * Shake * 8.f;
	}
	SetActorRotation(Posed);

	if (m_woodPlankMesh)
	{
		FVector Scale = BaseScale;
		Scale.Z *= FMath::Lerp(0.58f, 1.f, Intact);
		Scale.Y *= FMath::Lerp(0.86f, 1.f, Intact);
		m_woodPlankMesh->SetWorldScale3D(Scale);
	}
}
