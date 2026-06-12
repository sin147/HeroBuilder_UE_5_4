// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HB_CharacterHelper.h"

bool AHB_CharacterHelper::IsInteracting(EPlayerCharacterState State) const
{
	//交互三段：前摇 / 进行中 / 后摇
	return State == EPCS_PreInteract || State == EPCS_Interact || State == EPCS_PostInteract;
}
