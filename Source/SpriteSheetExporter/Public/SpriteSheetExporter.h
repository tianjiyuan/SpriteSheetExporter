// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "CoreMisc.h"


class FSpriteSheetExporterModule : public IModuleInterface, public FSelfRegisteringExec
{
	// intercept Exec console commands
	bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
