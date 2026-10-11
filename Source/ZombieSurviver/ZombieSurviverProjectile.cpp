// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverProjectile.h"
#include "HealthComponent.h"
#include "SurvivorFeedback.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PointLightComponent.h"

AZombieSurviverProjectile::AZombieSurviverProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AZombieSurviverProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	TracerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TracerLight"));
	TracerLight->AttachParent = CollisionComp;
	TracerLight->SetMobility(EComponentMobility::Movable);
	TracerLight->SetIntensity(3500.f);
	TracerLight->SetLightColor(FLinearColor(1.f, 0.78f, 0.35f));
	TracerLight->SetAttenuationRadius(160.f);
	TracerLight->CastShadows = false;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

void AZombieSurviverProjectile::OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if ((OtherActor == NULL) || (OtherActor == this) || (OtherComp == NULL))
	{
		return;
	}

	const FVector Impact = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : GetActorLocation();
	const FVector Normal = Hit.ImpactNormal.IsNearlyZero() ? -GetVelocity().GetSafeNormal() : FVector(Hit.ImpactNormal);

	if (UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>())
	{
		Health->NotifyHitFrom(GetActorLocation());
	}
	else
	{
		if (OtherComp->IsSimulatingPhysics())
		{
			OtherComp->AddImpulseAtLocation(GetVelocity() * 80.0f, Impact);
		}
		SurvivorFeedback::Burst(GetWorld(), Impact, SurvivorFeedback::Sparks(), 0.28f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::SparkSound(), Impact, 0.35f, FMath::FRandRange(0.8f, 1.3f));
		SurvivorFeedback::SpawnShards(GetWorld(), Impact, Normal, 2, SurvivorFeedback::Cube(), nullptr, FLinearColor(0.55f, 0.52f, 0.45f), 0.035f, 140.f);
	}

	Destroy();
}