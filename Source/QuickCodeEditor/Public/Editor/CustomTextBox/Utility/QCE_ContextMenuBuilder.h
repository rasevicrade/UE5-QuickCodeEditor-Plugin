// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandList.h"

class UMainEditorContainer;
class SQCE_MultiLineEditableTextBox;
class QCE_FindAndReplaceContainer;
class QCE_MultiLineEditableTextBoxWrapper;

/**
 * Context menu holds features available in the editor.
 * This class extends the existing text editor menu to add those features.
 */
class QUICKCODEEDITOR_API QCE_ContextMenuBuilder
{
public:
	/** Constructor that takes references to the search container, target text box, code editor, and command list */
	QCE_ContextMenuBuilder(
		const TSharedPtr<QCE_FindAndReplaceContainer>& InSearchContainer,
		const TSharedPtr<SQCE_MultiLineEditableTextBox>& InTargetTextBox,
		UMainEditorContainer* InCodeEditor
	);
	
	/** Extends the code editor menu with additional functionality */
	void AddEditorMenuEntries(FMenuBuilder& MenuBuilder) const;

private:
	/** Binds UI commands to actions */
	void RegisterEditorContextMenuCommands();
	
	void AddGenerateDefinitionFunction(FMenuBuilder& MenuBuilder) const;
	
	/** Called when the search word under cursor menu item is clicked */
	void OnFindAndReplaceClicked() const;

	/** Called when the Generate Definition menu item is clicked */
	void OnGenerateDefinitionClicked() const;

	/** Called when the Open in Explorer menu item is clicked */
	void OnOpenInExplorerClicked(const FString& FilePath) const;

	/** Determines if the Open in Explorer functionality can be used */
	bool CanOpenInExplorer(const FString& FilePath) const { return !FilePath.IsEmpty(); }

	TSharedPtr<QCE_FindAndReplaceContainer> SearchContainer;
	
	/** Command list for UI commands */
	TSharedPtr<FUICommandList> CommandList;
	
	TSharedPtr<SQCE_MultiLineEditableTextBox> TargetTextBox;
	
	TWeakObjectPtr<UMainEditorContainer> CodeEditor;
};
