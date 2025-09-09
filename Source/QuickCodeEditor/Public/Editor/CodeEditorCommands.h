// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class QUICKCODEEDITOR_API FCodeEditorCommands : public TCommands<FCodeEditorCommands>
{
public:
	FCodeEditorCommands()
		: TCommands<FCodeEditorCommands>(TEXT("CodeEditor"),
			NSLOCTEXT("Contexts", "CodeEditor", "Code Editor"),
			NAME_None, FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override;
	
	TSharedPtr<FUICommandInfo> CodeEditor_Open;
};
