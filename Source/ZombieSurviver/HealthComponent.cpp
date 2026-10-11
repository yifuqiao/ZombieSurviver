#include "ZombieSurviver.h"
#include "HealthComponent.h"
#include "ZombieSurviverCharacter.h"
#include "ZombieSurviverGameMode.h"
#include "SurvivorFeedback.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimSequence.h"

UHealthComponent::UHealthComponent()
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	m_hitPoint = 100;
	m_maxHitPoint = 100;
	bDead = false;
	bHasHitOrigin = false;
	bRising = false;
	bLunging = false;
	BaseWalkSpeed = 0.f;
	RiseTime = 0.f;
	LungeTime = 0.f;
	LungeCooldown = 1.5f;
	HitStopUntil = 0.f;
	BaseMeshScale = FVector(1.f);
	BaseMeshRelativeLocation = FVector::ZeroVector;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	m_maxHitPoint = FMath::Max(1, m_hitPoint);
	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsA(AZombieSurviverCharacter::StaticClass()))
	{
		SetComponentTickEnabled(false);
		return;
	}

	ACharacter* Zombie = Cast<ACharacter>(Owner);
	if (Zombie && Zombie->GetCharacterMovement())
	{
		const float Current = Zombie->GetCharacterMovement()->MaxWalkSpeed;
		BaseWalkSpeed = FMath::Max(80.f, Current) * FMath::FRandRange(0.78f, 1.24f);
		Zombie->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		Zombie->GetCharacterMovement()->RotationRate.Yaw = FMath::FRandRange(160.f, 420.f);

		if (USkeletalMeshComponent* Mesh = Zombie->GetMesh())
		{
			BaseMeshRelativeLocation = Mesh->GetRelativeTransform().GetLocation();
			BaseMeshScale = Mesh->GetRelativeTransform().GetScale3D();
			Mesh->SetRelativeScale3D(BaseMeshScale * 0.72f);
			Mesh->SetRelativeLocation(BaseMeshRelativeLocation + FVector(0.f, 0.f, -48.f));
			bRising = true;
			RiseTime = 0.f;
		}
	}

	if (UWorld* World = GetWorld())
	{
		const FVector Loc = Owner->GetActorLocation();
		SurvivorFeedback::Burst(World, Loc, SurvivorFeedback::Smoke(), 0.45f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Hiss(), Loc, 0.22f, FMath::FRandRange(0.55f, 0.85f));

		if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode()))
		{
			Mode->RegisterHostile();
		}

		World->GetTimerManager().SetTimer(
			MoanTimer,
			this,
			&UHealthComponent::Moan,
			FMath::FRandRange(3.5f, 7.5f),
			true,
			FMath::FRandRange(0.4f, 4.f));
	}
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDead || !GetOwner())
	{
		return;
	}

	if (HitStopUntil > 0.f && GetWorld() && GetWorld()->GetRealTimeSeconds() >= HitStopUntil)
	{
		GetOwner()->CustomTimeDilation = 1.f;
		HitStopUntil = 0.f;
	}

	UpdateRise(DeltaTime);
	UpdateLunge(DeltaTime);
}

void UHealthComponent::NotifyHitFrom(FVector Origin)
{
	LastHitOrigin = Origin;
	bHasHitOrigin = true;
}

void UHealthComponent::TakeDamage(int32 deltaDamange)
{
	if (bDead || deltaDamange <= 0)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	m_hitPoint -= deltaDamange;

	FVector HitOrigin = LastHitOrigin;
	if (!bHasHitOrigin)
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					HitOrigin = Pawn->GetActorLocation();
				}
			}
		}
	}
	bHasHitOrigin = false;

	const FVector FromOwner = (Owner->GetActorLocation() - HitOrigin).GetSafeNormal();
	const FVector Shove = FromOwner.IsNearlyZero() ? Owner->GetActorForwardVector() * -1.f : FromOwner;
	const bool bKill = m_hitPoint <= 0;
	const FVector Chest = Owner->GetActorLocation() + FVector(0.f, 0.f, 55.f);

	if (UWorld* World = GetWorld())
	{
		SurvivorFeedback::Burst(World, Chest, SurvivorFeedback::Sparks(), bKill ? 0.85f : 0.45f);
		SurvivorFeedback::SpawnShards(
			World,
			Chest,
			Shove,
			bKill ? 8 : 3,
			SurvivorFeedback::Sphere(),
			nullptr,
			FLinearColor(0.45f, 0.02f, 0.02f),
			bKill ? 0.09f : 0.045f,
			bKill ? 280.f : 160.f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), Chest, bKill ? 0.85f : 0.45f, FMath::FRandRange(0.75f, 1.25f));

		if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode()))
		{
			Mode->AddFloater(Chest + FVector(0.f, 0.f, 20.f), bKill ? FString(TEXT("DOWN")) : FString::FromInt(deltaDamange), bKill ? FLinearColor(1.f, 0.35f, 0.2f) : FLinearColor(0.95f, 0.9f, 0.75f), bKill ? 1.6f : 1.f);
		}

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AZombieSurviverCharacter* Survivor = Cast<AZombieSurviverCharacter>(PC->GetPawn()))
			{
				Survivor->ConfirmHit();
				Survivor->AddCameraTrauma(bKill ? 0.55f : 0.18f);
			}
		}
	}

	if (bKill)
	{
		Die(HitOrigin);
		return;
	}

	if (ACharacter* Zombie = Cast<ACharacter>(Owner))
	{
		Zombie->LaunchCharacter(Shove * FMath::Clamp(deltaDamange * 8.f, 120.f, 360.f) + FVector(0.f, 0.f, 40.f), true, false);
		SurvivorFeedback::PlayZombieHit(Zombie->GetMesh(), HitOrigin, deltaDamange >= 40);
		Owner->CustomTimeDilation = 0.25f;
		if (UWorld* World = GetWorld())
		{
			HitStopUntil = World->GetRealTimeSeconds() + 0.045f;
		}
	}
}

void UHealthComponent::Die(const FVector& HitOrigin)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	m_hitPoint = 0;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MoanTimer);
		const FVector Loc = Owner->GetActorLocation() + FVector(0.f, 0.f, 40.f);
		SurvivorFeedback::Burst(World, Loc, SurvivorFeedback::Explosion(), 0.28f);
		SurvivorFeedback::Burst(World, Loc, SurvivorFeedback::Smoke(), 0.7f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Boom(), Loc, 0.55f, FMath::FRandRange(0.7f, 1.05f));

		if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode()))
		{
			Mode->RegisterKill(Loc);
		}
	}

	Owner->CustomTimeDilation = 1.f;
	Owner->SetActorTickEnabled(false);

	if (ACharacter* Zombie = Cast<ACharacter>(Owner))
	{
		if (AController* Controller = Zombie->GetController())
		{
			Controller->StopMovement();
			Controller->UnPossess();
			Controller->SetLifeSpan(0.2f);
		}

		if (UCharacterMovementComponent* Move = Zombie->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->DisableMovement();
		}
		Zombie->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (USkeletalMeshComponent* Mesh = Zombie->GetMesh())
		{
			const FVector Shove = (Owner->GetActorLocation() - HitOrigin).GetSafeNormal();
			if (Mesh->GetPhysicsAsset())
			{
				Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
				Mesh->SetAllBodiesSimulatePhysics(true);
				Mesh->SetSimulatePhysics(true);
				Mesh->WakeAllRigidBodies();
				Mesh->bBlendPhysics = true;
				Mesh->AddImpulse((Shove.IsNearlyZero() ? FVector::UpVector : Shove) * 420.f + FVector(0.f, 0.f, 160.f), NAME_None, true);
			}
			else if (UAnimSequence* Death = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Character/LandOfTheDead_Zombie/anim/DeathF.DeathF")))
			{
				Mesh->PlayAnimation(Death, false);
			}
		}
	}
	else
	{
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);
	}

	Owner->SetLifeSpan(12.f);
	SetComponentTickEnabled(false);
}

void UHealthComponent::UpdateRise(float DeltaTime)
{
	if (!bRising)
	{
		return;
	}

	ACharacter* Zombie = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Zombie ? Zombie->GetMesh() : nullptr;
	if (!Mesh)
	{
		bRising = false;
		return;
	}

	RiseTime += DeltaTime;
	const float Alpha = FMath::Clamp(RiseTime / 0.45f, 0.f, 1.f);
	const float Eased = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	Mesh->SetRelativeScale3D(BaseMeshScale * FMath::Lerp(0.72f, 1.f, Eased));
	Mesh->SetRelativeLocation(BaseMeshRelativeLocation + FVector(0.f, 0.f, FMath::Lerp(-48.f, 0.f, Eased)));
	if (Alpha >= 1.f)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale);
		Mesh->SetRelativeLocation(BaseMeshRelativeLocation);
		bRising = false;
	}
}

void UHealthComponent::UpdateLunge(float DeltaTime)
{
	ACharacter* Zombie = Cast<ACharacter>(GetOwner());
	if (!Zombie || !Zombie->GetCharacterMovement() || BaseWalkSpeed <= 0.f || bRising)
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* Player = (World && World->GetFirstPlayerController()) ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}

	const float DistSq = FVector::DistSquared(Zombie->GetActorLocation(), Player->GetActorLocation());
	if (!bLunging && DistSq < 620.f * 620.f)
	{
		LungeCooldown -= DeltaTime;
		if (LungeCooldown <= 0.f)
		{
			bLunging = true;
			LungeTime = FMath::FRandRange(0.35f, 0.55f);
			LungeCooldown = FMath::FRandRange(2.4f, 4.4f);
			Zombie->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * FMath::FRandRange(1.6f, 2.15f);
			SurvivorFeedback::Burst(World, Zombie->GetActorLocation(), SurvivorFeedback::Smoke(), 0.3f);
		}
	}

	if (bLunging)
	{
		LungeTime -= DeltaTime;
		if (LungeTime <= 0.f)
		{
			bLunging = false;
			Zombie->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		}
	}
}

void UHealthComponent::Moan()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || bDead)
	{
		return;
	}

	APawn* Player = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (Player && FVector::DistSquared(Owner->GetActorLocation(), Player->GetActorLocation()) > 1600.f * 1600.f)
	{
		return;
	}

	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Hiss(), Owner->GetActorLocation() + FVector(0.f, 0.f, 70.f), 0.16f, FMath::FRandRange(0.35f, 0.6f));
}

int UHealthComponent::GetHP() const
{
	return m_hitPoint;
}

int32 UHealthComponent::GetMaxHP() const
{
	return m_maxHitPoint;
}

bool UHealthComponent::IsDead() const
{
	return bDead;
}
