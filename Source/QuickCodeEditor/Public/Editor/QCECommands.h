// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Styling/CoreStyle.h"

/**
 * Quick Code Editor commands for context menu and keyboard shortcuts
 */
class QUICKCODEEDITOR_API FQCECommands : public TCommands<FQCECommands>
{
public:
	FQCECommands();

	// Command declarations
	TSharedPtr<FUICommandInfo> FindAndReplace;
	TSharedPtr<FUICommandInfo> GenerateDefinition;
	TSharedPtr<FUICommandInfo> OpenInExplorer;
	TSharedPtr<FUICommandInfo> GoToLine;

	// TCommands interface
	virtual void RegisterCommands() override;
};
