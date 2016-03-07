

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

	UFUNCTION(BlueprintPure, Category = "HitpointSystem")
	int32 GetHP() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitpointSystem", Meta = (BlueprintProtected = "true"))
	int32 m_hitPoint;
};
