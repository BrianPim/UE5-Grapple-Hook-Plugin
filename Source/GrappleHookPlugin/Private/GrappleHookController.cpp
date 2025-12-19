// Copyright (c) 2025 Brian Pimentel


#include "GrappleHookController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GrappleHookLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

//Definitions
class UEnhancedInputLocalPlayerSubsystem;

UGrappleHookController::UGrappleHookController()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UGrappleHookController::BeginPlay()
{
	Super::BeginPlay();

	//Initial Grapple Hook setup.
	SetupGrappleHook();
}


// Called every frame
void UGrappleHookController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (GrapplePoint)
	{
		if (FVector::Dist(PlayerCharacter->GetActorTransform().GetLocation(), GrapplePoint->GetActorTransform().GetLocation()) > ReleaseRange)
		{
			FVector GrapplePointPosition = GrapplePoint->GetActorLocation();
			FVector PlayerPosition = PlayerCharacter->GetActorLocation();
			FVector Direction = (GrapplePointPosition - PlayerPosition).GetSafeNormal();

			//Setting the velocity of the Player's MoveComponent as long as they have an active GrapplePoint.
			MovementComponent->Velocity = Direction * CurrentSpeed;

			//Accelerates from the initial speed to the maximum speed.
			if (SpeedLerpElapsed < SpeedLerpDuration)
			{
				SpeedLerpElapsed = FMath::Clamp(SpeedLerpElapsed + DeltaTime, 0.0f, SpeedLerpDuration);
				CurrentSpeed = FMath::Lerp(InitialSpeed, MaxSpeed, SpeedLerpElapsed / SpeedLerpDuration);
			}

			//If the path is blocked, cancel the Grapple action.
			if (CancelIfBlocked && CheckGrappleBlocked(Direction))
			{
				CancelGrapple();
			}
		}
		else
		{
			//Cancel the Grapple action if we've reached the destination.
			CancelGrapple();
		}
	}
}

//Checks to see if the Player is aiming at a valid Grapple target; and if they are, initiates the Grapple action.
void UGrappleHookController::UseGrappleHook()
{
	//Return if there's already a point being Grappled towards.
	if (GrapplePoint)
	{
		CancelGrapple();
		return;
	}

	if (TOptional<FHitResult> HitResult = GrappleHookLineTrace())
	{
		CurrentSpeed = InitialSpeed;
		SpeedLerpElapsed = 0.0f;

		//Caches the current Gravity Scale so that we can reset it after the Grapple is done.
		PreviousGravityScale = MovementComponent->GravityScale;

		//Ignoring Move Input b/c we don't want the player to be able to move during the Grapple (may revisit this).
		PlayerController->SetIgnoreMoveInput(true);
		MovementComponent->SetMovementMode(MOVE_Flying);
		MovementComponent->GravityScale = 0.0f;

		PlayerCharacter->SetActorRotation((HitResult->ImpactPoint - PlayerCharacter->GetActorLocation()).Rotation());

		//Caches current Controller Rotation Yaw value and then sets to false so that the character doesn't rotate away
		//from the grapple point mid-action.
		PreviousYawBool = PlayerCharacter->bUseControllerRotationYaw;
		PlayerCharacter->bUseControllerRotationYaw = false;
			
		SetupGrapplePointActor(HitResult->ImpactPoint, HitResult->GetComponent());

		//Further functionality handled via Blueprint that references OnGrappleStart delegate.
		OnGrappleStart.Broadcast();
	}
	else
	{
		UE_LOG(GrappleHookLog, Error, TEXT("No Grapple Hit!"));
	}
}

//Spawns an Actor to act as the end of the Grapple Hook. We use an Actor and attach it to the provided SceneComponent in order to actively track
//the surface hit by the Grapple Hook (in case it's a moving object).
void UGrappleHookController::SetupGrapplePointActor(FVector ImpactPoint, USceneComponent* HitComponent)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GrapplePoint = GetWorld()->SpawnActor<AActor>(
		GrappleEndPointActor != nullptr ? GrappleEndPointActor : AActor::StaticClass(),
		ImpactPoint,
		FRotator::ZeroRotator,
		SpawnParams);

	//Currently un-used, but may be useful to have in some cases in the future.
	GrapplePoint->Tags.Add(FName("Grapple Point"));
	
	if (!GrapplePoint->GetRootComponent())
	{
		USceneComponent* Root = NewObject<USceneComponent>(GrapplePoint, TEXT("Root"));
		Root->RegisterComponent();
		GrapplePoint->SetRootComponent(Root);
	}

	//Setting the Grapple's location here, as setting it before it has a registered Root doesn't actually do anything.
	GrapplePoint->SetActorLocation(ImpactPoint);

	//Attaching the GrapplePoint to the provided SceneComponent so that it actively tracks where the Grapple Hook hit.
	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
	GrapplePoint->AttachToComponent(HitComponent, AttachRules);
}

//Draws a Line Trace to check if the Player is aiming towards a valid Object to Grapple to.
TOptional<FHitResult> UGrappleHookController::GrappleHookLineTrace() const
{
	FVector CameraLocation;
	FRotator CameraRotation;

	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector Normal = CameraRotation.Vector().GetSafeNormal();
		
	FHitResult HitResult;

	FVector CastStart = CameraLocation;
	FVector CastEnd = CameraLocation + Normal * MaxGrappleRange;

	bool GrappleHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CastStart,
		CastEnd,
		ECC_Visibility,
		GrappleCollisionQueryParams
	);
	
	if (GrappleHit)
	{
		return HitResult;
	}
	
	return {};
}

//Draws a Box Trace in the provided direction, returns true if anything is detected within the box.
bool UGrappleHookController::CheckGrappleBlocked(FVector Direction) const
{
	//Divided by 2 because MakeBox creates the shape from center to edge, instead of edge to edge.
	FCollisionShape MyBox = FCollisionShape::MakeBox(CancelIfBlockedTraceDimensions / 2);

	FHitResult HitResult;
	
	bool Hit = GetWorld()->SweepSingleByChannel(
		HitResult,
		PlayerCharacter->GetActorLocation(),
		PlayerCharacter->GetActorLocation() + Direction * CancelIfBlockedTraceOffset,
		FQuat::Identity,
		ECC_Visibility,
		MyBox,
		GrappleCollisionQueryParams
	);

	return Hit;
}

//Returns whether whatever the player is aiming at is a valid Grapple Target or not.
bool UGrappleHookController::HasValidGrappleTarget() const
{
	return !IsValid(GrapplePoint) && GrappleHookLineTrace().IsSet();
}

//Returns if GrapplePoint is valid.
bool UGrappleHookController::IsGrappling() const
{
	return IsValid(GrapplePoint);
}

//Returns GrapplePoint.
AActor* UGrappleHookController::GetGrappleEndPointActor() const
{
	return GrapplePoint;
}

//Cancels the Grapple action and resets values.
void UGrappleHookController::CancelGrapple()
{
	if (!GrapplePoint)
	{
		return;
	}

	FVector GrapplePointPosition = GrapplePoint->GetActorLocation();
	FVector PlayerPosition = PlayerCharacter->GetActorLocation();
	FVector Direction = (GrapplePointPosition - PlayerPosition).GetSafeNormal();

	//Reset values.
	GrapplePoint->Destroy();
	GrapplePoint = nullptr;

	PlayerController->SetIgnoreMoveInput(false);
	PlayerCharacter->bUseControllerRotationYaw = PreviousYawBool ? PreviousYawBool : false;

	MovementComponent->SetMovementMode(MOVE_Falling);
	MovementComponent->GravityScale = PreviousGravityScale ? PreviousGravityScale : 1.0f;

	//Adding an impulse here to simulate the Player continuing to move along the Grapple path for a short time.
	MovementComponent->AddImpulse(Direction * CurrentSpeed * ReleaseVelocityMultiplier,true);

	//Further functionality handled via Blueprint that references OnGrappleEnd delegate.
	OnGrappleEnd.Broadcast();
}

//Initial Grapple Hook Setup.
void UGrappleHookController::SetupGrappleHook()
{
	//Assignments and checks
	PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	checkf(PlayerCharacter, TEXT("Unable to get reference to the Local Player's Character"));

	PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	checkf(PlayerController, TEXT("Unable to get reference to the Local Player's PlayerController"));

	MovementComponent = PlayerCharacter->GetCharacterMovement();
	checkf(MovementComponent, TEXT("Unable to get reference to the Local Player's CharacterMovementComponent"));

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	checkf(EnhancedInputComponent, TEXT("Unable to get reference to the EnhancedInputComponent"));
	
	TObjectPtr<ULocalPlayer> LocalPlayer = PlayerController->GetLocalPlayer();

	//Adding the Grapple InputMappingContext to the Player.
	TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	checkf(InputSubsystem, TEXT("Unable to get reference to the EnhancedInputLocalPlayerSubsystem"));
	
	checkf(InputMappingContext, TEXT("InputMappingContext was not specified"));
	InputSubsystem->AddMappingContext(InputMappingContext, 0);

	GrappleCollisionQueryParams.AddIgnoredActor(PlayerCharacter);

	//Bind input action, only attempt to bind if valid value was provided.
	if (ActionGrappleHook)
	{
		EnhancedInputComponent->BindAction(ActionGrappleHook, ETriggerEvent::Triggered, this, &UGrappleHookController::UseGrappleHook);
	}
	else
	{
		UE_LOG(GrappleHookLog, Error, TEXT("Grapple Input Missing!"));
	}

	UE_LOG(GrappleHookLog, Display, TEXT("Grapple Setup Complete!"));
}





