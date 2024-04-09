// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBaseWeapon.h"

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
	}
}
