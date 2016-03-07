// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombieSurviver.h"
#include "SpawnVolume.h"
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

void ASpawnVolume::Spawn()
{
	// If we have set something to spawn:
	if (WhatToSpawn != NULL)
	{
		// Check for a valid World: 
		UWorld* const World = GetWorld();
		if (World)
		{
			// Set the spawn parameters
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Instigator;

			// Get a random location to spawn at
			FVector SpawnLocation = GetRandomPointInVolume();

			// Get a random rotation for the spawned item
			FRotator SpawnRotation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorRotation();
			//SpawnRotation.Yaw = FMath::FRand() * 360.0f;
			//SpawnRotation.Pitch = FMath::FRand() * 360.0f;
			//SpawnRotation.Roll = FMath::FRand() * 360.0f;

			// spawn the pickup
			AActor* const SpawnedPickup = World->SpawnActor<AActor>(WhatToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

			SpawnDelay = FMath::FRandRange(SpawnDelayRangeLow, SpawnDelayRangeHigh);
			GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::Spawn, SpawnDelay, false);
			AAIBrain* const brainScript = Cast<AAIBrain>(SpawnedPickup->GetInstigatorController());

			if (brainScript != nullptr)
			{
				brainScript->SetTarget(GetWorld()->GetFirstPlayerController()->GetPawn());
			}
		}
	}

}
