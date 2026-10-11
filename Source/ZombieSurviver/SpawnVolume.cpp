// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombieSurviver.h"
#include "SpawnVolume.h"
#include "ZombieSurviverGameMode.h"
#include "SurvivorFeedback.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIBrain.h"

// Sets default values
ASpawnVolume::ASpawnVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create the Box Component to represent the spawn volume
	WhereToSpawn = CreateDefaultSubobject<UBoxComponent>(TEXT("WhereToSpawn"));
	RootComponent = WhereToSpawn;

	//Set the spawn delay range
	SpawnDelayRangeLow = 1.0f;
	SpawnDelayRangeHigh = 4.5f;

}

// Called when the game starts or when spawned
void ASpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	SetSpawningActive(true);


}

// Called every frame
void ASpawnVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector ASpawnVolume::GetRandomPointInVolume()
{
	FVector SpawnOrigin = WhereToSpawn->Bounds.Origin;
	FVector SpawnExtent = WhereToSpawn->Bounds.BoxExtent;

	return UKismetMathLibrary::RandomPointInBoundingBox(SpawnOrigin, SpawnExtent);

}

void ASpawnVolume::SetSpawningActive(bool bShouldSpawn)
{
	if (bShouldSpawn)
	{
		// Set the timer on Spawn Pickup
		SpawnDelay = FMath::FRandRange(SpawnDelayRangeLow, SpawnDelayRangeHigh);
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::Spawn, SpawnDelay, false);
	}
	else
	{
		// clear the timer on Spawn Pickup
		GetWorldTimerManager().ClearTimer(SpawnTimer);
	}
}

void ASpawnVolume::SetSpawnWindow(float Low, float High)
{
	SpawnDelayRangeLow = FMath::Max(0.2f, Low);
	SpawnDelayRangeHigh = FMath::Max(SpawnDelayRangeLow, High);
}

void ASpawnVolume::Spawn()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode());
	if (Mode && !Mode->CanSpawnAnother())
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::Spawn, 1.25f, false);
		return;
	}

	// If we have set something to spawn:
	if (WhatToSpawn != NULL)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!PlayerPawn)
		{
			GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::Spawn, 0.5f, false);
			return;
		}

		// Set the spawn parameters
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;

		// Get a random location to spawn at
		FVector SpawnLocation = GetRandomPointInVolume();

		FRotator SpawnRotation = (PlayerPawn->GetActorLocation() - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.f;
		SpawnRotation.Roll = 0.f;

		SurvivorFeedback::Burst(World, SpawnLocation, SurvivorFeedback::Smoke(), 0.55f);
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::Hiss(), SpawnLocation, 0.28f, FMath::FRandRange(0.5f, 0.8f));

		// spawn the pickup
		AActor* const SpawnedPickup = World->SpawnActor<AActor>(WhatToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

		SpawnDelay = FMath::FRandRange(SpawnDelayRangeLow, SpawnDelayRangeHigh);
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::Spawn, SpawnDelay, false);

		if (SpawnedPickup)
		{
			AAIBrain* const brainScript = Cast<AAIBrain>(SpawnedPickup->GetInstigatorController());
			if (brainScript != nullptr)
			{
				brainScript->SetTarget(PlayerPawn);
			}
		}
	}
}
