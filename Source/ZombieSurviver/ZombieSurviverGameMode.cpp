// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverGameMode.h"
#include "ZombieSurviverHUD.h"
#include "ZombieSurviverCharacter.h"
#include "SpawnVolume.h"
#include "SurvivorFeedback.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

AZombieSurviverGameMode::AZombieSurviverGameMode()
	: Super()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	HUDClass = AZombieSurviverHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	MaxAlive = 10;
	KillsPerWave = 5;
	Wave = 1;
	Kills = 0;
	Score = 0;
	Alive = 0;
	bGameOver = false;
	SpawnLow = 1.0f;
	SpawnHigh = 4.5f;
	BannerAlpha = 0.f;
	BannerHold = 0.f;
}

void AZombieSurviverGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ASpawnVolume> It(GetWorld()); It; ++It)
	{
		SpawnVolumes.Add(*It);
	}

	PushBanner(TEXT("THEY ARE COMING"));
	GetWorldTimerManager().SetTimer(AmbienceTimer, this, &AZombieSurviverGameMode::StartAmbience, 0.6f, false);
}

void AZombieSurviverGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (BannerHold > 0.f)
	{
		BannerHold -= DeltaSeconds;
		BannerAlpha = 1.f;
	}
	else if (BannerAlpha > 0.f)
	{
		BannerAlpha = FMath::Max(0.f, BannerAlpha - DeltaSeconds * 0.7f);
	}

	for (int32 Index = Floaters.Num() - 1; Index >= 0; --Index)
	{
		Floaters[Index].Age += DeltaSeconds;
		if (Floaters[Index].Age > 0.9f)
		{
			Floaters.RemoveAt(Index);
		}
	}
}

void AZombieSurviverGameMode::RegisterHostile()
{
	if (!bGameOver)
	{
		++Alive;
	}
}

void AZombieSurviverGameMode::RegisterKill(const FVector& Location)
{
	if (bGameOver)
	{
		return;
	}

	++Kills;
	Alive = FMath::Max(0, Alive - 1);
	Score += 10 + Wave * 5;
	AddFloater(Location + FVector(0.f, 0.f, 30.f), FString::Printf(TEXT("+%d"), 10 + Wave * 5), FLinearColor(1.f, 0.82f, 0.35f), 1.15f);

	if (Kills > 0 && Kills % KillsPerWave == 0)
	{
		AdvanceWave();
	}
}

void AZombieSurviverGameMode::NotifySurvivorDown()
{
	if (bGameOver)
	{
		return;
	}
	bGameOver = true;
	PushBanner(TEXT("YOU DIED"));
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Boom(), FVector::ZeroVector, 0.8f, 0.55f);
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AZombieSurviverGameMode::RestartRound, 3.4f, false);
}

void AZombieSurviverGameMode::RestartRound()
{
	RestartGame();
}

void AZombieSurviverGameMode::AddFloater(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale)
{
	FCombatFloater Floater;
	Floater.Location = Location;
	Floater.Text = Text;
	Floater.Color = Color;
	Floater.Age = 0.f;
	Floater.Scale = Scale;
	Floaters.Add(Floater);
	if (Floaters.Num() > 24)
	{
		Floaters.RemoveAt(0);
	}
}

bool AZombieSurviverGameMode::CanSpawnAnother() const
{
	return !bGameOver && Alive < MaxAlive;
}

void AZombieSurviverGameMode::StartAmbience()
{
	FVector Origin = FVector::ZeroVector;
	USceneComponent* AttachTo = nullptr;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Origin = Pawn->GetActorLocation();
			if (AZombieSurviverCharacter* Survivor = Cast<AZombieSurviverCharacter>(Pawn))
			{
				AttachTo = Survivor->GetFirstPersonCameraComponent();
			}
		}
	}

	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Wind(), Origin, 0.45f, 1.f);
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Music(), Origin, 0.28f, 1.f);
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Birds(), Origin, 0.18f, 1.f);

	if (AttachTo)
	{
		if (UParticleSystemComponent* Dust = UGameplayStatics::SpawnEmitterAttached(
			SurvivorFeedback::Dust(),
			AttachTo,
			NAME_None,
			FVector(140.f, 0.f, -8.f),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			false))
		{
			Dust->SetWorldScale3D(FVector(0.22f));
		}
	}
}

void AZombieSurviverGameMode::PushBanner(const FString& Text)
{
	Banner = Text;
	BannerAlpha = 1.f;
	BannerHold = 1.35f;
}

void AZombieSurviverGameMode::AdvanceWave()
{
	++Wave;
	MaxAlive = FMath::Min(20, MaxAlive + 2);
	SpawnLow = FMath::Max(0.32f, SpawnLow * 0.84f);
	SpawnHigh = FMath::Max(SpawnLow + 0.25f, SpawnHigh * 0.84f);
	for (ASpawnVolume* Volume : SpawnVolumes)
	{
		if (Volume)
		{
			Volume->SetSpawnWindow(SpawnLow, SpawnHigh);
		}
	}

	PushBanner(FString::Printf(TEXT("WAVE %d"), Wave));
	FVector Origin = FVector::ZeroVector;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Origin = Pawn->GetActorLocation();
		}
	}
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Boom(), Origin, 0.7f, 0.62f);
}
