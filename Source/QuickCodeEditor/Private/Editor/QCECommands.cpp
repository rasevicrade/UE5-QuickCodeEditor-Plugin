// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/QCECommands.h"
#include "Settings/UQCE_EditorSettings.h"

#define LOCTEXT_NAMESPACE "QCECommands"

FQCECommands::FQCECommands()
	: TCommands<FQCECommands>(
		TEXT("QuickCodeEditor"), 
		NSLOCTEXT("Contexts", "QuickCodeEditor", "Quick Code Editor"),
		NAME_None, 
		FCoreStyle::Get().GetStyleSetName() 
	)
{
}

void FQCECommands::RegisterCommands()
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	
	FInputChord FindChord = (Settings && Settings->FindKeybinding.IsValidChord()) 
		? Settings->FindKeybinding 
		: FInputChord(EKeys::F, EModifierKey::Control);
	
	UI_COMMAND(FindAndReplace, "Find/Replace", "Opens the find and replace panel", 
		EUserInterfaceActionType::Button, FindChord);
	
	UI_COMMAND(GenerateDefinition, "Generate Definition", "Generate implementation for the function declaration at cursor", 
		EUserInterfaceActionType::Button, FInputChord(EKeys::G, EModifierKey::Control | EModifierKey::Shift));
	
	UI_COMMAND(OpenInExplorer, "Open in Explorer", "Opens the file location in Windows Explorer", 
		EUserInterfaceActionType::Button, FInputChord());
	
	UI_COMMAND(GoToLine, "Go to Line", "Opens the go to line panel", 
		EUserInterfaceActionType::Button, FInputChord(EKeys::G, EModifierKey::Control));
}

#undef LOCTEXT_NAMESPACE
