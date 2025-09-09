// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"

class UMainEditorContainer;
class SQCE_MultiLineEditableTextBox;

/**
 * Widget container for the "Go to Line" functionality.
 * Provides a text input box and button to navigate to a specific line number in the text editor.
 */
class QUICKCODEEDITOR_API QCE_GoToLineContainer : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(QCE_GoToLineContainer)
        : _Visibility(EVisibility::Visible)
    {}
        SLATE_ATTRIBUTE(EVisibility, Visibility)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Set the parent editor container reference */
    void SetEditorContainer(UMainEditorContainer* InParentEditor) { ParentEditor = InParentEditor; }

    /** Creates and returns a new go to line container widget */
    static TSharedRef<QCE_GoToLineContainer> Create();

    /** Sets focus to the line number input box */
    void FocusLineNumberInput() const;

private:
    /** Reference to the parent editor container */
    UMainEditorContainer* ParentEditor = nullptr;
    
    /** The line number input text box widget */
    TSharedPtr<SEditableTextBox> LineNumberTextBox;

    /** The go to line button widget */
    TSharedPtr<SButton> GoToLineButton;

    /** Called when the go to line button is clicked */
    FReply OnGoToLineClicked() const;

    /** Called when the close button is clicked */
    FReply OnCloseClicked();

    /** Called when Enter key is pressed in the line number text box */
    void OnLineNumberCommitted(const FText& Text, ETextCommit::Type CommitType) const;

    /** Validates that the entered text is a valid line number */
    bool IsValidLineNumber(const FString& LineNumberText) const;

    /** Executes the go to line functionality */
    void ExecuteGoToLine() const;
};
