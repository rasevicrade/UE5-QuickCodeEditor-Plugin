// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FDropdownCodeCompletionEngine;

#define LOCTEXT_NAMESPACE "FQCESummoner"

class UMainEditorContainer;

struct QUICKCODEEDITOR_API FQCESummoner : public FWorkflowTabFactory
{
public:
	FQCESummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedRef<SDockTab> SpawnTab(const FWorkflowTabSpawnInfo& Info) const override;

private:
	/** Sets up code completion engine for the text box if available */
	void SetupCodeCompletionForTextBox(class UMainEditorContainer* QceInstance) const;

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override
	{
		return LOCTEXT("QCETooltip", "The Quick Code Editor provides an advanced code editing interface for Blueprint functions and events.");
	}
};

#undef LOCTEXT_NAMESPACE
