// // Zombie Surviver Prototype \n Written by Yifu Qiao

#pragma once

#include "GameFramework/Actor.h"
#include "WoodPlank.generated.h"

UCLASS()
class ZOMBIESURVIVER_API AWoodPlank : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWoodPlank();

	void Init(FVector point1, FVector point2, FRotator rot);
	/** Return the mesh for the pickup */
	FORCEINLINE class UStaticMeshComponent* GetMesh() const { return m_woodPlankMesh; }
private: 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* m_woodPlankMesh;
};
