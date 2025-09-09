// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"

class UMainEditorContainer;
class SQCE_MultiLineEditableTextBox;

class QUICKCODEEDITOR_API QCE_FindAndReplaceContainer : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(QCE_FindAndReplaceContainer)
        : _Visibility(EVisibility::Visible)
    {}
        SLATE_ATTRIBUTE(EVisibility, Visibility)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetEditorContainer(UMainEditorContainer* InParentEditor) { ParentEditor = InParentEditor; }

    static TSharedRef<QCE_FindAndReplaceContainer> Create();
    void SetFindText(const FString& TextToFind) const;
    void FocusFindTextBox() const;

private:
    UMainEditorContainer* ParentEditor = nullptr;
    
    TSharedPtr<SEditableTextBox> FindTextBox;
    TSharedPtr<SEditableTextBox> ReplaceTextBox;
    TSharedPtr<SButton> FindButton;
    TSharedPtr<SButton> ReplaceButton;
    TSharedPtr<SButton> ReplaceAllButton;
    TSharedPtr<SCheckBox> MatchCaseCheckBox;
    TSharedPtr<SCheckBox> WholeWordCheckBox;

    FReply OnFindClicked() const;
    FReply OnReplaceClicked() const;
    FReply OnReplaceAllClicked() const;
    void OnMatchCaseChanged(ECheckBoxState NewState);
    void OnWholeWordChanged(ECheckBoxState NewState);
    FReply OnCloseClicked();
    FReply OnFindTextKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

    bool bMatchCase = true;
    bool bWholeWord = true;

    void ReplaceOccurrence(const FText& FindTerm, const FText& ReplaceTerm) const;
    
    void ReplaceOccurrences(const FText& FindTerm, const FText& ReplaceTerm) const;
};
