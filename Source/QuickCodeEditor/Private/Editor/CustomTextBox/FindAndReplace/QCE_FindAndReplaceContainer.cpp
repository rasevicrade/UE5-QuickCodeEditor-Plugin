// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/FindAndReplace/QCE_FindAndReplaceContainer.h"

#include "Editor/MainEditorContainer.h"
#include "Editor/CustomTextBox/FindAndReplace/QCE_FindAndReplaceManager.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "FindAndReplaceContainer"

void QCE_FindAndReplaceContainer::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)
            // Close button
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "NoBorder")
                .ContentPadding(FMargin(2, 2))
                .OnClicked(this, &QCE_FindAndReplaceContainer::OnCloseClicked)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.X"))
                    .ColorAndOpacity(FSlateColor::UseForeground())
                ]
            ]
            // Find textbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(FindTextBox, SEditableTextBox)
                .MinDesiredWidth(120.0f)
                .HintText(LOCTEXT("FindHint", "Find"))
                .ToolTipText(LOCTEXT("FindTooltip", "Enter text to search for"))
                .OnKeyDownHandler(this, &QCE_FindAndReplaceContainer::OnFindTextKeyDown)
            ]
            // Find button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0, 0, 0)
            [
                SAssignNew(FindButton, SButton)
                .Text(LOCTEXT("FindButtonText", "Find"))
                .ToolTipText(LOCTEXT("FindButtonTooltip", "Find next occurrence"))
                .OnClicked(this, &QCE_FindAndReplaceContainer::OnFindClicked)
            ]
            // Replace textbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SAssignNew(ReplaceTextBox, SEditableTextBox)
                .MinDesiredWidth(120.0f)
                .HintText(LOCTEXT("ReplaceHint", "Replace"))
                .ToolTipText(LOCTEXT("ReplaceTooltip", "Text to replace with"))
            ]
            // Replace button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0, 0, 0)
            [
                SAssignNew(ReplaceButton, SButton)
                .Text(LOCTEXT("ReplaceButtonText", "Replace"))
                .ToolTipText(LOCTEXT("ReplaceButtonTooltip", "Replace current occurrence"))
                .OnClicked(this, &QCE_FindAndReplaceContainer::OnReplaceClicked)
            ]
            // Replace All button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0, 0, 0)
            [
                SAssignNew(ReplaceAllButton, SButton)
                .Text(LOCTEXT("ReplaceAllButtonText", "Replace All"))
                .ToolTipText(LOCTEXT("ReplaceAllButtonTooltip", "Replace all occurrences"))
                .OnClicked(this, &QCE_FindAndReplaceContainer::OnReplaceAllClicked)
            ]
            // Match Case checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SAssignNew(MatchCaseCheckBox, SCheckBox)
                .IsChecked(bMatchCase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                .OnCheckStateChanged(this, &QCE_FindAndReplaceContainer::OnMatchCaseChanged)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MatchCaseLabel", "Match Case"))
                ]
            ]
            // Whole Word checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0, 0, 0)
            [
                SAssignNew(WholeWordCheckBox, SCheckBox)
                .IsChecked(bWholeWord ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                .OnCheckStateChanged(this, &QCE_FindAndReplaceContainer::OnWholeWordChanged)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("WholeWordLabel", "Whole Word"))
                ]
            ]
        ]
    ];

    SetVisibility(InArgs._Visibility.Get());
}

TSharedRef<QCE_FindAndReplaceContainer> QCE_FindAndReplaceContainer::Create()
{
    return SNew(QCE_FindAndReplaceContainer);
}

void QCE_FindAndReplaceContainer::SetFindText(const FString& TextToFind) const
{
    if (FindTextBox.IsValid())
    {
        FindTextBox->SetText(FText::FromString(TextToFind));
    }
}

void QCE_FindAndReplaceContainer::OnMatchCaseChanged(ECheckBoxState NewState)
{
    bMatchCase = (NewState == ECheckBoxState::Checked);
}

void QCE_FindAndReplaceContainer::OnWholeWordChanged(ECheckBoxState NewState)
{
    bWholeWord = (NewState == ECheckBoxState::Checked);
}

FReply QCE_FindAndReplaceContainer::OnFindClicked() const
{
    if (!ParentEditor)
        return FReply::Handled();
    
    const FText FindTerm = FindTextBox->GetText();
    if (FindTerm.IsEmpty()) return FReply::Handled();

    int32 NextOccurrenceLine = 0;
    if (QCE_FindAndReplaceManager::FindOccurrence(ParentEditor->GetActiveTextBoxWrapper()->GetTextBox().Get(), FindTerm, bMatchCase, bWholeWord, &NextOccurrenceLine))
    {
        ParentEditor->GetActiveTextBoxWrapper()->ScrollToLine(NextOccurrenceLine);
    }

    return FReply::Handled();
}

FReply QCE_FindAndReplaceContainer::OnReplaceClicked() const
{
    const FText FindTerm = FindTextBox->GetText();
    if (FindTerm.IsEmpty()) return FReply::Handled();

    const FText ReplaceTerm = ReplaceTextBox->GetText();
    if (ReplaceTerm.IsEmpty()) return FReply::Handled();

    ReplaceOccurrence(FindTerm, ReplaceTerm);
    
    return FReply::Handled();
}

FReply QCE_FindAndReplaceContainer::OnReplaceAllClicked() const
{
    const FText FindTerm = FindTextBox->GetText();
    if (FindTerm.IsEmpty()) return FReply::Handled();

    const FText ReplaceTerm = ReplaceTextBox->GetText();
    if (ReplaceTerm.IsEmpty()) return FReply::Handled();

    ReplaceOccurrences(FindTerm, ReplaceTerm);
    
    return FReply::Handled();
}

FReply QCE_FindAndReplaceContainer::OnCloseClicked()
{
    SetVisibility(EVisibility::Collapsed);
    return FReply::Handled();
}

FReply QCE_FindAndReplaceContainer::OnFindTextKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Enter)
    {
        return OnFindClicked();
    }
    return FReply::Unhandled();
}

void QCE_FindAndReplaceContainer::ReplaceOccurrence(const FText& FindTerm, const FText& ReplaceTerm) const
{
    if (ParentEditor->GetImplementationTextBoxWrapper() && ParentEditor->GetImplementationTextBoxWrapper()->GetTextBox().IsValid())
    {
        QCE_FindAndReplaceManager::ReplaceOccurrence(ParentEditor->GetImplementationTextBoxWrapper()->GetTextBox().Get(), FindTerm, ReplaceTerm, bMatchCase, bWholeWord);
    }
    if (ParentEditor->GetDeclarationTextBoxWrapper() && ParentEditor->GetDeclarationTextBoxWrapper()->GetTextBox().IsValid())
    {
        QCE_FindAndReplaceManager::ReplaceOccurrence(ParentEditor->GetDeclarationTextBoxWrapper()->GetTextBox().Get(), FindTerm, ReplaceTerm, bMatchCase, bWholeWord);
    }
}

void QCE_FindAndReplaceContainer::ReplaceOccurrences(const FText& FindTerm, const FText& ReplaceTerm) const
{
    if (ParentEditor->GetImplementationTextBoxWrapper() && ParentEditor->GetImplementationTextBoxWrapper()->GetTextBox().IsValid())
    {
        QCE_FindAndReplaceManager::ReplaceOccurrences(ParentEditor->GetImplementationTextBoxWrapper()->GetTextBox().Get(), FindTerm, ReplaceTerm, bMatchCase, bWholeWord);
    }
    if (ParentEditor->GetDeclarationTextBoxWrapper() && ParentEditor->GetDeclarationTextBoxWrapper()->GetTextBox().IsValid())
    {
        QCE_FindAndReplaceManager::ReplaceOccurrences(ParentEditor->GetDeclarationTextBoxWrapper()->GetTextBox().Get(), FindTerm, ReplaceTerm, bMatchCase, bWholeWord);
    }
}

void QCE_FindAndReplaceContainer::FocusFindTextBox() const
{
    if (FindTextBox.IsValid())
    {
        FSlateApplication::Get().SetKeyboardFocus(FindTextBox.ToSharedRef(), EFocusCause::SetDirectly);
    }
}

#undef LOCTEXT_NAMESPACE
