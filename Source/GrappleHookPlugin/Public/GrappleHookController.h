// Copyright (c) 2025 Brian Pimentel

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleHookController.generated.h"

//Definitions
class UCharacterMovementComponent;
class UInputMappingContext;
class UInputAction;


//Delegate for Grapple events.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrappleEvent);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GRAPPLEHOOKPLUGIN_API UGrappleHookController : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UGrappleHookController();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	//Input Action to map to Grapple.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputAction> ActionGrappleHook = nullptr;
	
	//Input Mapping Context to use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

	//Returns whether whatever the player is aiming at is a valid Grapple Target or not.
	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Whether or not the Player is aiming at a valid target."))
	bool HasValidGrappleTarget() const;

	//Returns if GrapplePoint is valid.
	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Whether or not the Player is currently using the Grapple Hook."))
	bool IsGrappling() const;

	//Returns GrapplePoint.
	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Returns the point that the Player is grappling towards."))
	AActor* GetGrappleEndPointActor() const;

	//Initial Grapple Hook Setup.
	void SetupGrappleHookInput();

	//Delegates
	//Fires in HandleUseGrappleHook.
	UPROPERTY(BlueprintAssignable, Category = "Grapple Hook", meta = (ToolTip = "Hook up additional functionality to this."))
	FGrappleEvent OnGrappleStart;

	//Fires in CancelGrapple.
	UPROPERTY(BlueprintAssignable, Category = "Grapple Hook", meta = (ToolTip = "Hook up additional functionality to this."))
	FGrappleEvent OnGrappleEnd;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//Initiates the Grapple action.
	void HandleUseGrappleHook();

	//Cancels the Grapple action.
	UFUNCTION(BlueprintCallable, Category = "Grapple Hook")
	void CancelGrapple();

	//Spawns and attaches the end of the Grapple Hook to the destination object.
	void SetupGrapplePointActor(FVector ImpactPoint, USceneComponent* HitComponent);

	//Checks if the Player is aiming towards a valid Object to Grapple to.
	TOptional<FHitResult> GrappleHookLineTrace() const;

	//Checks if there's an object in the way of the Grapple path.
	bool CheckGrappleBlocked(FVector Direction) const;

private:

	//Defaults
	static constexpr float BaseMaxGrappleRange = 10000.0f;
	static constexpr float BaseMaxSpeed = 2000.0f;
	static constexpr float BaseInitialSpeed = 500.0f;
	static constexpr float BaseSpeedLerpDuration = 1.0f;
	static constexpr float BaseReleaseRange = 100.0f;

	static constexpr float BaseCancelIfBlockedX = 50.0f;
	static constexpr float BaseCancelIfBlockedY = 50.0f;
	static constexpr float BaseCancelIfBlockedZ = 100.0f;
	static constexpr float BaseCancelIfBlockedOffset = 100.0f;
	
	static constexpr bool BaseCancelIfBlocked = true;

	//Grapple Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxGrappleRange = BaseMaxGrappleRange;

	//Grapple Max Speed value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxSpeed = BaseMaxSpeed;

	//Grapple Initial Speed value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float InitialSpeed = BaseInitialSpeed;

	//Grapple Speed Lerp (Acceleration) value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float SpeedLerpDuration = BaseSpeedLerpDuration;

	//Grapple Release Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float ReleaseRange = BaseReleaseRange;

	//Whether or not the Grapple will auto-cancel if there's an object in front of the Player, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	bool CancelIfBlocked = BaseCancelIfBlocked;

	//Dimensions of the Box Trace used to check if the Grapple vector is blocked, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true", EditCondition = "CancelIfBlocked"))
	FVector CancelIfBlockedTraceDimensions = FVector(BaseCancelIfBlockedX, BaseCancelIfBlockedY, BaseCancelIfBlockedZ);

	//Offset of the Box Trace used to check if the Grapple vector is blocked, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true", EditCondition = "CancelIfBlocked"))
	float CancelIfBlockedTraceOffset = BaseCancelIfBlockedOffset;

	//Used to store a reference to the InputComponent cast to an EnhancedInputComponent
	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

	//Used to store a reference to the Player's PlayerController
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController = nullptr;

	//Used to store a reference to the Character we are controlling
	UPROPERTY()
	TObjectPtr<ACharacter> PlayerCharacter = nullptr;
	
	//Used to store a reference to the pawn we are controlling
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;

	//Used to store a reference to where the Grapple hit. An actor because we want to dynamically track the destination if we've grappled to a moving object
	UPROPERTY()
	TObjectPtr<AActor> GrapplePoint = nullptr;

	//CollisionQueryParams that determine valid objects to consider in valid Grapple target determination (currently only excludes PlayerCharacter)
	FCollisionQueryParams GrappleCollisionQueryParams;

	float CurrentSpeed = InitialSpeed;
	float SpeedLerpElapsed = 0.0f;
	
	float PreviousGravityScale = 0.0f;
	bool PreviousYawBool = false;
};
