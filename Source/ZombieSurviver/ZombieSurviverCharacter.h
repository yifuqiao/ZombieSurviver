// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"
#include "ZombieSurviverCharacter.generated.h"

class UInputComponent;
class AWoodWall;

UCLASS(config=Game)
class AZombieSurviverCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Light, meta = (AllowPrivateAccess = "true"))
	class USpotLightComponent* Flashlight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Light, meta = (AllowPrivateAccess = "true"))
	class UPointLightComponent* MuzzleLight;

public:
	AZombieSurviverCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AZombieSurviverProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Survivor)
	int32 MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Survivor)
	float FireInterval;

	UFUNCTION(BlueprintCallable, Category = Survivor)
	void ApplySurvivorDamage(int32 Amount, FVector FromLocation);

	void AddCameraTrauma(float Amount);
	void ConfirmHit();

	UFUNCTION(BlueprintPure, Category = Survivor)
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = Survivor)
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = Survivor)
	float GetCrosshairSpread() const;

	UFUNCTION(BlueprintPure, Category = Survivor)
	float GetDamageFlash() const { return DamageFlash; }

	UFUNCTION(BlueprintPure, Category = Survivor)
	float GetHitConfirm() const { return HitConfirm; }

	UFUNCTION(BlueprintPure, Category = Survivor)
	bool IsSurvivorDead() const { return bDead; }

	class AWoodWall* GetFocusedEntrance() const { return FocusedEntrance; }
	bool WantsHammerPrompt() const;

protected:
	
	/** Fires a projectile. */
	void OnFire();

	void OnMelee();
	void OnHammerPressed();
	void OnHammerReleased();
	void SwingHammer(bool bAllowMelee);
	void UpdateRepairAim();
	void StartSprint();
	void StopSprint();
	void ToggleFlashlight();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

	void HandleDeath();
	void UpdateFeel(float DeltaSeconds);
	void UpdateThreat(float DeltaSeconds);
	void UpdatePostProcess(float DeltaSeconds);

public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

private:
	int32 Health;
	float Stamina;
	float StaminaRegenDelay;
	float FireCooldown;
	float MeleeCooldown;
	float DamageFlash;
	float HitConfirm;
	float Trauma;
	float RecoilPitch;
	float AppliedRoll;
	float BobTime;
	float FootstepClock;
	float ThreatClock;
	float HealBank;
	float TimeSinceDamage;
	float VulnerableTime;
	float WalkSpeed;
	float SprintSpeed;
	float MuzzleIntensity;
	bool bWantsSprint;
	bool bDead;
	bool bFlashlightOn;
	bool bViewCached;
	bool bHammerHeld;
	class AWoodWall* FocusedEntrance;
	FVector CameraBaseRelLocation;
	FVector ArmsBaseRelLocation;
	TMap<AActor*, float> BiteTimes;
};
