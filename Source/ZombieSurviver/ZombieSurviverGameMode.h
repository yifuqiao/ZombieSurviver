// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/GameMode.h"
#include "WoodWall.h"
#include "ZombieSurviverGameMode.generated.h"

class ASpawnVolume;

struct FCombatFloater
{
	FVector Location;
	FString Text;
	FLinearColor Color;
	float Age;
	float Scale;
};

UCLASS(minimalapi)
class AZombieSurviverGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AZombieSurviverGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void RegisterHostile();
	void RegisterKill(const FVector& Location);
	void NotifySurvivorDown();
	void AddFloater(const FVector& Location, const FString& Text, const FLinearColor& Color, float Scale);
	bool CanSpawnAnother() const;

	void RegisterEntrance(AWoodWall* Wall);
	void NotifyEntranceBreached(AWoodWall* Wall);
	void NotifyEntranceSealed(AWoodWall* Wall);
	void GetZombieOrder(AActor* Zombie, AActor* Player, FVector& OutDestination, AWoodWall*& OutDoorToBash) const;
	int32 GetBarricadeDamage() const;

	int32 GetWave() const { return Wave; }
	int32 GetKills() const { return Kills; }
	int32 GetScore() const { return Score; }
	int32 GetAlive() const { return Alive; }
	bool IsGameOver() const { return bGameOver; }
	float GetSurvivalTime() const { return SurvivalTime; }
	FString GetSurvivalText() const;
	const FString& GetBanner() const { return Banner; }
	float GetBannerAlpha() const { return BannerAlpha; }
	const TArray<FCombatFloater>& GetFloaters() const { return Floaters; }
	const TArray<AWoodWall*>& GetEntrances() const { return Entrances; }

private:
	UFUNCTION()
	void StartAmbience();

	UFUNCTION()
	void RestartRound();

	void PushBanner(const FString& Text, float Hold = 1.35f);
	void AdvanceWave();
	void SortEntrances();
	void AnnounceDoors();

	UPROPERTY(EditAnywhere, Category = "Horde")
	int32 MaxAlive;

	UPROPERTY(EditAnywhere, Category = "Horde")
	int32 KillsPerWave;

	int32 Wave;
	int32 Kills;
	int32 Score;
	int32 Alive;
	bool bGameOver;
	bool bPendingIntro;
	bool bHouseOverrun;
	float SpawnLow;
	float SpawnHigh;
	float BannerAlpha;
	float BannerHold;
	float SurvivalTime;
	FString Banner;
	TArray<FCombatFloater> Floaters;
	TArray<ASpawnVolume*> SpawnVolumes;

	UPROPERTY()
	TArray<AWoodWall*> Entrances;
	FTimerHandle RestartTimer;
	FTimerHandle AmbienceTimer;
};
