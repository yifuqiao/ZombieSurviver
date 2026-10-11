
#include "ZombieSurviver.h"
#include "AIBrain.h"
#include "AIPathFollowingComponent.h"
#include "HealthComponent.h"
#include "WoodWall.h"
#include "ZombieSurviverGameMode.h"
#include "SurvivorFeedback.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"



AAIBrain::AAIBrain(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAIPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	m_targetPlayer = nullptr;
	DoorAttackCooldown = 0.f;
	StuckTime = 0.f;
	
	UHealthComponent* hpComponent = CreateDefaultSubobject<UHealthComponent>(FName("HealthComponent"));
	GetOwner()->AddComponent(FName("HealthComponent"), false, FTransform(FVector()), hpComponent);
}

void AAIBrain::BeginPlay()
{
	Super::BeginPlay();
	AcquireTarget();
}

void AAIBrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Pawn = GetPawn();
	if (!Pawn || !GetWorld())
	{
		return;
	}

	UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return;
	}

	if (!m_targetPlayer || m_targetPlayer->IsPendingKill())
	{
		AcquireTarget();
	}
	if (!m_targetPlayer)
	{
		return;
	}

	UNavigationSystem* NavSystem = GetWorld()->GetNavigationSystem();
	if (!NavSystem)
	{
		return;
	}

	DoorAttackCooldown = FMath::Max(0.f, DoorAttackCooldown - DeltaTime);

	FVector Destination = m_targetPlayer->GetActorLocation();
	AWoodWall* Door = nullptr;
	if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(GetWorld()->GetAuthGameMode()))
	{
		Mode->GetZombieOrder(Pawn, m_targetPlayer, Destination, Door);
		if (Door && !Door->IsBreached())
		{
			const float DoorDistSq = FVector::DistSquared(Pawn->GetActorLocation(), Door->GetDoorLocation());
			const float GoalDistSq = FVector::DistSquared(Pawn->GetActorLocation(), Destination);
			const bool bInReach = GoalDistSq < 150.f * 150.f || DoorDistSq < 260.f * 260.f;
			if (!bInReach && Pawn->GetVelocity().SizeSquared() < 1600.f && DoorDistSq < 460.f * 460.f)
			{
				StuckTime += DeltaTime;
			}
			else
			{
				StuckTime = 0.f;
			}
			const bool bPressedAgainstDoor = StuckTime > 0.8f;
			if (!bInReach && !bPressedAgainstDoor)
			{
				NavSystem->SimpleMoveToLocation(this, Destination);
				return;
			}
			StuckTime = 0.f;

			StopMovement();
			SetFocalPoint(Door->GetDoorLocation());
			const FVector ToDoor = (Door->GetDoorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
			if (!ToDoor.IsNearlyZero())
			{
				SetControlRotation(FRotator(0.f, ToDoor.Rotation().Yaw, 0.f));
			}

			if (DoorAttackCooldown <= 0.f)
			{
				DoorAttackCooldown = FMath::FRandRange(0.72f, 1.05f);
				Door->TakeDamage(Mode->GetBarricadeDamage());
				if (ACharacter* Zombie = Cast<ACharacter>(Pawn))
				{
					SurvivorFeedback::PlayZombieBite(Zombie->GetMesh());
				}
			}
			return;
		}
	}

	StuckTime = 0.f;
	NavSystem->SimpleMoveToLocation(this, Destination);
}

void AAIBrain::SetTarget(AActor* player)
{
	m_targetPlayer = player;
}

void AAIBrain::AcquireTarget()
{
	if (!GetWorld())
	{
		return;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		m_targetPlayer = PC->GetPawn();
	}
}
