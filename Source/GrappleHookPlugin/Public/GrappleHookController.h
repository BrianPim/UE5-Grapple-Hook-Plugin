// Copyright (c) 2025 Brian Pimentel

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleHookController.generated.h"


class UInputMappingContext;
class UInputAction;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GRAPPLEHOOKPLUGIN_API UGrappleHookController : public UActorComponent
{
	GENERATED_BODY()

public:

	//Input Action to map to grapple hook
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputAction> ActionGrappleHook = nullptr;
	
	// Sets default values for this component's properties
	UGrappleHookController();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	//Input Mapping Context to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Input")
	TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;
	
	void SetupGrappleHookInput();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void HandleUseGrappleHook();

	FHitResult* GrappleHookLineTrace();

private:

	//Default
	static constexpr float BaseMaxGrappleRange = 10000.0f;

	//Grapple Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxGrappleRange = BaseMaxGrappleRange;

	//Used to store a reference to the InputComponent cast to an EnhancedInputComponent
	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

	//Used to store a reference to the pawn we are controlling
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController = nullptr;
	
	//Used to store a reference to the pawn we are controlling
	UPROPERTY()
	TObjectPtr<APawn> PlayerPawn = nullptr;

	//Used to store a reference to where the Grapple hit. An actor because we want to dynamically track the destination if we've grappled to a moving object
	UPROPERTY()
	TObjectPtr<AActor> GrapplePoint = nullptr;
};
