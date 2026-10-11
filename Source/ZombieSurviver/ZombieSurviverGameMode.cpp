// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverGameMode.h"
#include "ZombieSurviverHUD.h"
#include "ZombieSurviverCharacter.h"
#include "SpawnVolume.h"
#include "WoodWall.h"
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
	bPendingIntro = false;
	bHouseOverrun = false;
	SpawnLow = 1.0f;
	SpawnHigh = 4.5f;
	BannerAlpha = 0.f;
	BannerHold = 0.f;
	SurvivalTime = 0.f;
}

void AZombieSurviverGameMode::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ASpawnVolume> It(GetWorld()); It; ++It)
	{
		SpawnVolumes.Add(*It);
	}

	bPendingIntro = true;
	GetWorldTimerManager().SetTimer(AmbienceTimer, this, &AZombieSurviverGameMode::StartAmbience, 0.6f, false);
}

void AZombieSurviverGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPendingIntro)
	{
		bPendingIntro = false;
		AnnounceDoors();
	}

	if (!bGameOver)
	{
		SurvivalTime += DeltaSeconds;
	}

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

void AZombieSurviverGameMode::PushBanner(const FString& Text, float Hold)
{
	Banner = Text;
	BannerAlpha = 1.f;
	BannerHold = Hold;
}

FString AZombieSurviverGameMode::GetSurvivalText() const
{
	const int32 Total = FMath::Max(0, FMath::FloorToInt(SurvivalTime));
	return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
}

int32 AZombieSurviverGameMode::GetBarricadeDamage() const
{
	return 12 + FMath::Max(0, Wave - 1) * 5;
}

void AZombieSurviverGameMode::RegisterEntrance(AWoodWall* Wall)
{
	if (!Wall || Entrances.Contains(Wall))
	{
		return;
	}
	Entrances.Add(Wall);
	SortEntrances();
}

void AZombieSurviverGameMode::SortEntrances()
{
	FVector Center = FVector::ZeroVector;
	int32 Count = 0;
	for (AWoodWall* Wall : Entrances)
	{
		if (!Wall)
		{
			continue;
		}
		Center += Wall->GetDoorLocation();
		++Count;
	}
	if (Count > 0)
	{
		Center /= float(Count);
	}

	for (int32 Index = 1; Index < Entrances.Num(); ++Index)
	{
		AWoodWall* Key = Entrances[Index];
		const float KeyAngle = Key ? FMath::Atan2(Key->GetDoorLocation().Y - Center.Y, Key->GetDoorLocation().X - Center.X) : 0.f;
		int32 Slot = Index - 1;
		while (Slot >= 0)
		{
			AWoodWall* Other = Entrances[Slot];
			const float OtherAngle = Other ? FMath::Atan2(Other->GetDoorLocation().Y - Center.Y, Other->GetDoorLocation().X - Center.X) : 0.f;
			if (OtherAngle <= KeyAngle)
			{
				break;
			}
			Entrances[Slot + 1] = Other;
			--Slot;
		}
		Entrances[Slot + 1] = Key;
	}

	for (int32 Index = 0; Index < Entrances.Num(); ++Index)
	{
		if (Entrances[Index])
		{
			Entrances[Index]->SetEntranceIndex(Index + 1);
		}
	}
}

void AZombieSurviverGameMode::AnnounceDoors()
{
	if (Entrances.Num() == 0)
	{
		for (TActorIterator<AWoodWall> It(GetWorld()); It; ++It)
		{
			RegisterEntrance(*It);
		}
	}

	if (Entrances.Num() > 0)
	{
		PushBanner(FString::Printf(TEXT("HAMMER THE %d DOORS"), Entrances.Num()), 2.6f);
	}
	else
	{
		PushBanner(TEXT("THEY ARE COMING"), 2.2f);
	}
}

void AZombieSurviverGameMode::NotifyEntranceBreached(AWoodWall* Wall)
{
	if (bGameOver || !Wall)
	{
		return;
	}

	PushBanner(FString::Printf(TEXT("%s IS OPEN"), *Wall->GetEntranceLabel()), 1.6f);
	AddFloater(Wall->GetDoorLocation() + FVector(0.f, 0.f, 40.f), FString(TEXT("OPEN")), FLinearColor(0.9f, 0.2f, 0.12f), 1.5f);

	bool bAnySealed = false;
	for (AWoodWall* Entrance : Entrances)
	{
		if (Entrance && !Entrance->IsBreached())
		{
			bAnySealed = true;
			break;
		}
	}
	if (!bAnySealed && !bHouseOverrun)
	{
		bHouseOverrun = true;
		PushBanner(TEXT("THEY'RE INSIDE"), 2.f);
	}
}

void AZombieSurviverGameMode::NotifyEntranceSealed(AWoodWall* Wall)
{
	if (bGameOver || !Wall)
	{
		return;
	}
	bHouseOverrun = false;
	PushBanner(FString::Printf(TEXT("%s BOARDED"), *Wall->GetEntranceLabel()), 1.2f);
}

void AZombieSurviverGameMode::GetZombieOrder(AActor* Zombie, AActor* Player, FVector& OutDestination, AWoodWall*& OutDoorToBash) const
{
	OutDoorToBash = nullptr;
	OutDestination = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	if (!Zombie || !Player || Entrances.Num() == 0)
	{
		return;
	}

	const FVector ZombieLocation = Zombie->GetActorLocation();
	const FVector PlayerLocation = Player->GetActorLocation();
	AWoodWall* NearestOpen = nullptr;
	AWoodWall* NearestSealed = nullptr;
	float OpenDist = TNumericLimits<float>::Max();
	float SealedDist = TNumericLimits<float>::Max();

	for (AWoodWall* Wall : Entrances)
	{
		if (!Wall || Wall->IsPendingKill())
		{
			continue;
		}

		const float Dist = FVector::Dist(ZombieLocation, Wall->GetDoorLocation());
		const bool bSeparates = Wall->Separates(ZombieLocation, PlayerLocation);
		const bool bCloseOutside = Dist < 220.f && FVector::Dist(PlayerLocation, Wall->GetDoorLocation()) > Dist + 80.f;
		if (!bSeparates && !bCloseOutside)
		{
			continue;
		}
		if (bSeparates && Dist > 1800.f && !bCloseOutside)
		{
			continue;
		}

		if (Wall->IsBreached())
		{
			if (Dist < OpenDist)
			{
				OpenDist = Dist;
				NearestOpen = Wall;
			}
		}
		else if (Dist < SealedDist)
		{
			SealedDist = Dist;
			NearestSealed = Wall;
		}
	}

	if (NearestOpen && !(NearestSealed && SealedDist + 180.f < OpenDist))
	{
		const FVector Inside = NearestOpen->GetInsidePoint(PlayerLocation);
		if (!NearestOpen->Separates(ZombieLocation, PlayerLocation) || FVector::DistSquared(ZombieLocation, Inside) < 170.f * 170.f)
		{
			OutDestination = PlayerLocation;
		}
		else
		{
			OutDestination = Inside;
		}
		return;
	}

	if (NearestSealed)
	{
		OutDoorToBash = NearestSealed;
		OutDestination = NearestSealed->GetAttackPoint(Zombie);
	}
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
