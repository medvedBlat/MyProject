// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBaseWeapon.h"

#include "Kismet/GameplayStatics.h"

AProjectileBaseWeapon::AProjectileBaseWeapon()
{
	RootComponent = WeaponMesh;
	BaseDamage = 20.0f;
	MuzzleSocket = "MuzzleFlash";
	FireRate = 30;
	MaxMagCapacity = 0;
	bCanFire = true;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void AProjectileBaseWeapon::Fire()
{
	AActor* MyOwner = GetOwner();
	if(MyOwner && Projectile)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocket);

		FActorSpawnParameters SpawnParam;
		SpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		GetWorld()->SpawnActor<AActor>(Projectile, MuzzleLocation, EyeRotation, SpawnParam);
		
		LastTimeFired = GetWorld()->TimeSeconds;

		if(FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,WeaponMesh->GetSocketLocation("MuzzleFlash"));
		}
		--Capacity;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"),Capacity));
		if(Capacity <= 0)
		{
			StopFire();
			bCanFire = false;
		}
	}
	
}
