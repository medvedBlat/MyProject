#pragma once

#include "CoreMinimal.h"
#include "../Weapons/WeaponBase.h"
#include "ProjectileBaseWeapon.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API AProjectileBaseWeapon : public AWeaponBase
{
	GENERATED_BODY()
	
public:
	
	AProjectileBaseWeapon();
	
protected:
	
	virtual void Fire() override;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> Projectile;
};
