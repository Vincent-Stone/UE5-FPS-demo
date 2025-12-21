// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "ShooterPlayerController.h"
#include "ShooterPlayerState.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::OnRep_CurrentHP() 
{
	OnHealthUpdate();
}

void AShooterCharacter::OnHealthUpdate()
{
	const float LifePercent = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);

	//客户端特定的功能
	if (IsLocallyControlled())
	{
		//FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHP);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHP <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
		
		//更新HUD
		OnDamaged.Broadcast(LifePercent);
		UE_LOG(LogTemp, Warning, TEXT("OnRep_CurrentHP Client HP=%f"), CurrentHP);
	}

	//服务器特定的功能
	if (GetLocalRole() == ROLE_Authority)
	{
		//FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHP);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}

	//在所有机器上都执行的函数。
	/*
		因任何因伤害或死亡而产生的特殊功能都应放在这里。
	*/
}

void AShooterCharacter::Server_SwitchWeapon_Next_Implementation()
{
	if (OwnedWeapons.Num() <= 1 || !CurrentWeapon)
		return;

	int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);
	if (WeaponIndex == INDEX_NONE)
		return;

	WeaponIndex = (WeaponIndex + 1) % OwnedWeapons.Num();
	CurrentWeapon = OwnedWeapons[WeaponIndex];
}

void AShooterCharacter::Server_SwitchWeapon_Implementation(int32 Index)
{
	if (!OwnedWeapons.IsValidIndex(Index))
		return;

	CurrentWeapon = OwnedWeapons[Index];
}



void AShooterCharacter::OnRep_CurrentWeapon()
{
		if (LastWeapon)
		{
			LastWeapon->DeactivateWeapon();
		}

		if (CurrentWeapon)
		{
			CurrentWeapon->ActivateWeapon();
		}

		LastWeapon = CurrentWeapon;

}


//////////////////////////////////////////////////////////////////////////
	// 复制的属性

void AShooterCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//复制当前生命值。
	DOREPLIFETIME(AShooterCharacter, CurrentHP);
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterCharacter, bIsDead);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	if (HasAuthority())
	{
		CurrentHP = MaxHP;
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

}

void AShooterCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (IsLocallyControlled())
	{
		if (AShooterPlayerController* PC =
			Cast<AShooterPlayerController>(GetController()))
		{
			OnDamaged.AddDynamic(
				PC,
				&AShooterPlayerController::OnPawnDamaged
			);
		}
	}
}


float AShooterCharacter::TakeDamage(
	float Damage,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!HasAuthority() || bIsDead)
		return 0.f;

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.f, MaxHP);

	if (CurrentHP <= 0.f)
	{
		bIsDead = true;
		Die();
	}
	UE_LOG(LogTemp, Warning, TEXT("TakeDamage Server HP=%f"), CurrentHP);
	// 不在这里 Broadcast UI
	// UI 交给 OnRep_CurrentHP

	return Damage;
}


/*float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}
	if (GetLocalRole() == ROLE_Authority) {
		// Reduce HP
		CurrentHP -= Damage;
		OnHealthUpdate();
		if (CurrentHP <= 0.0f && !bIsDead)
		{
			bIsDead = true;   // 关键
			Die();            // Server 逻辑
		}
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}*/

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// Client 只负责“我要切下一把”
	Server_SwitchWeapon_Next();
}

void AShooterCharacter::DoSwitchWeapon(int32 Index)
{
	// Client 只负责“我要切到 Index”
	Server_SwitchWeapon(Index);
}


void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	if (!HasAuthority())
		return;

	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);
	if (OwnedWeapon)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AShooterWeapon* AddedWeapon =
		GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

	if (!AddedWeapon)
		return;

	OwnedWeapons.Add(AddedWeapon);

	// 只改数据，不做表现
	CurrentWeapon = AddedWeapon;
	// 不要 ActivateWeapon()
}


/*void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}*/

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

uint8 AShooterCharacter::GetTeam() const
{
	if (AShooterPlayerState* PS =
		GetPlayerState<AShooterPlayerState>())
	{
		return PS->GetTeam();
	}
	return -1;
}

void AShooterCharacter::Die_Implementation()
{
	// Server 上进行死亡逻辑，不需要表现
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// 停止角色运动
	GetCharacterMovement()->StopMovementImmediately();

	// 远程调用客户端表现
	// Server 执行，不会再做表现
		// 告诉客户端自己已经死亡，执行相应动画和表现
		//Client_Death();

	uint8 Team = GetTeam();

	// 通知游戏模式、队伍等其他功能
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(Team);
	}

	// 设定重生时间
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRep_IsDead()
{
	if (!bIsDead)
		return;

	// 停止输入（只影响本地玩家）
	if (IsLocallyControlled())
	{
		DisableInput(nullptr);
	}

	// 停止移动（所有客户端）
	GetCharacterMovement()->StopMovementImmediately();

	// 收起武器
	if (CurrentWeapon)
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// 清 HUD
	OnBulletCountUpdated.Broadcast(0, 0);

	// 播放死亡动画 / 蓝图逻辑
	BP_OnDeath();

	// 镜头抬高（官方模板常用做法）
	if (IsLocallyControlled())
	{
		FVector CamLoc = GetFirstPersonCameraComponent()->GetRelativeLocation();
		CamLoc.Z += 50.f;
		GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);
	}
}
/*
void AShooterCharacter::Client_Death_Implementation()
{
	// 停止所有输入
	DisableInput(nullptr);

	// 停止角色移动
	GetCharacterMovement()->StopMovementImmediately();

	// 执行死亡动画
	BP_OnDeath();

	// 镜头抬高
	if (IsLocallyControlled())
	{
		FVector CamLoc = GetFirstPersonCameraComponent()->GetRelativeLocation();
		CamLoc.Z += 50.f;
		GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);
	}

	// 更新HUD、其他客户端表现等
	OnBulletCountUpdated.Broadcast(0, 0); // 例如重置弹药数量
}*/


void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}
