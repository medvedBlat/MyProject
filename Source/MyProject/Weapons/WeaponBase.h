// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class UDamageType;

//Information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class MYPROJECT_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* ImpactEffect;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UAnimMontage* ReloadingMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 MaxMagCapacity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	int32 Capacity;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	bool bCanFire;
	
	FTimerHandle TimerHandle_TimeBetweenShots;

	float LastTimeFired;

	float TimeBetweenShots;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerFire();

	//Playing FX
	UFUNCTION()
	void OnRep_HitScanTrace();
	
	virtual void Fire();

	void PlayFireEffect(FVector TraceEnd);

	void PlayImpactEffect(FVector ImpactPoint);

public:	

	void StartFire();

	void StopFire();

	void Reload();
};
