// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CodeEditorCommands.h"


#define LOCTEXT_NAMESPACE "FCodeEditorCommands"

PRAGMA_DISABLE_OPTIMIZATION
void FCodeEditorCommands::RegisterCommands()
{
	UI_COMMAND(
		CodeEditor_Open,
		"Toggle Code Editor",
		"Show or hide quick code editor.", 
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Alt, EKeys::C));
}
PRAGMA_ENABLE_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
