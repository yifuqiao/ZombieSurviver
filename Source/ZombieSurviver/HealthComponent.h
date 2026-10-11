#pragma once

#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIESURVIVER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION(BlueprintCallable, Category = "HitpointSystem")
	void TakeDamage(int32 deltaDamange);

	/** Where the latest hit came from, so the following TakeDamage can shove and bleed that way. */
	UFUNCTION(BlueprintCallable, Category = "HitpointSystem")
	void NotifyHitFrom(FVector Origin);

	UFUNCTION(BlueprintPure, Category = "HitpointSystem")
	int32 GetHP() const;

	UFUNCTION(BlueprintPure, Category = "HitpointSystem")
	int32 GetMaxHP() const;

	UFUNCTION(BlueprintPure, Category = "HitpointSystem")
	bool IsDead() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitpointSystem", Meta = (BlueprintProtected = "true"))
	int32 m_hitPoint;

	UFUNCTION()
	void Moan();

private:
	void Die(const FVector& HitOrigin);
	void UpdateRise(float DeltaTime);
	void UpdateLunge(float DeltaTime);

	int32 m_maxHitPoint;
	bool bDead;
	bool bHasHitOrigin;
	bool bRising;
	bool bLunging;
	FVector LastHitOrigin;
	FVector BaseMeshScale;
	FVector BaseMeshRelativeLocation;
	float BaseWalkSpeed;
	float RiseTime;
	float LungeTime;
	float LungeCooldown;
	float HitStopUntil;
	FTimerHandle MoanTimer;
};
