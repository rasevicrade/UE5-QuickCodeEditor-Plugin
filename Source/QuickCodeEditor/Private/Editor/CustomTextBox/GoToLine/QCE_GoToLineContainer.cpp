// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/GoToLine/QCE_GoToLineContainer.h"

#include "Editor/MainEditorContainer.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GoToLineContainer"

void QCE_GoToLineContainer::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SHorizontalBox)
        // Close button
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0, 0, 8, 0)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(2))
            .OnClicked(this, &QCE_GoToLineContainer::OnCloseClicked)
            .ToolTipText(LOCTEXT("CloseTooltip", "Close go to line"))
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.X"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
        // Line number label
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0, 0, 8, 0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("LineNumberLabel", "Line:"))
        ]
        // Line number textbox
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SAssignNew(LineNumberTextBox, SEditableTextBox)
            .MinDesiredWidth(80.0f)
            .HintText(LOCTEXT("LineNumberHint", "Line #"))
            .ToolTipText(LOCTEXT("LineNumberTooltip", "Enter line number to go to"))
            .OnTextCommitted(this, &QCE_GoToLineContainer::OnLineNumberCommitted)
        ]
        // Go to line button
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(8, 0, 0, 0)
        .VAlign(VAlign_Center)
        [
            SAssignNew(GoToLineButton, SButton)
            .Text(LOCTEXT("GoToLineButton", "Go To Line"))
            .ToolTipText(LOCTEXT("GoToLineButtonTooltip", "Navigate to the specified line number"))
            .OnClicked(this, &QCE_GoToLineContainer::OnGoToLineClicked)
        ]
    ];

    SetVisibility(InArgs._Visibility.Get());
}

TSharedRef<QCE_GoToLineContainer> QCE_GoToLineContainer::Create()
{
    return SNew(QCE_GoToLineContainer);
}

void QCE_GoToLineContainer::FocusLineNumberInput() const
{
    if (LineNumberTextBox.IsValid())
    {
        FSlateApplication::Get().SetKeyboardFocus(LineNumberTextBox);
        LineNumberTextBox->SelectAllText();
    }
}

FReply QCE_GoToLineContainer::OnGoToLineClicked() const
{
    ExecuteGoToLine();
    return FReply::Handled();
}

FReply QCE_GoToLineContainer::OnCloseClicked()
{
    SetVisibility(EVisibility::Collapsed);
    return FReply::Handled();
}

void QCE_GoToLineContainer::OnLineNumberCommitted(const FText& Text, ETextCommit::Type CommitType) const
{
    if (CommitType == ETextCommit::OnEnter)
    {
        ExecuteGoToLine();
    }
}

bool QCE_GoToLineContainer::IsValidLineNumber(const FString& LineNumberText) const
{
    if (LineNumberText.IsEmpty())
    {
        return false;
    }

    // Check if the string contains only digits
    for (const TCHAR& Char : LineNumberText)
    {
        if (!FChar::IsDigit(Char))
        {
            return false;
        }
    }

    // Convert to integer and check if it's a positive number
    const int32 LineNumber = FCString::Atoi(*LineNumberText);
    return LineNumber > 0;
}

void QCE_GoToLineContainer::ExecuteGoToLine() const
{
    if (!ParentEditor)
    {
        return;
    }

    if (!LineNumberTextBox.IsValid())
    {
        return;
    }

    const FString LineNumberText = LineNumberTextBox->GetText().ToString();
    
    if (!IsValidLineNumber(LineNumberText))
    {
        // Show notification for invalid line number
        FNotificationInfo Info(LOCTEXT("InvalidLineNumber", "Please enter a valid line number (positive integer)"));
        Info.ExpireDuration = 3.0f;
        Info.bFireAndForget = true;
        Info.bUseLargeFont = false;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    const int32 LineNumber = FCString::Atoi(*LineNumberText);
    
    // Try to scroll to the line in both text boxes
    bool bFoundInImplementation = false;
    bool bFoundInDeclaration = false;

    if (ParentEditor->GetImplementationTextBoxWrapper())
    {
        bFoundInImplementation = ParentEditor->GetImplementationTextBoxWrapper()->ScrollToLine(LineNumber);
    }

    if (ParentEditor->GetDeclarationTextBoxWrapper())
    {
        bFoundInDeclaration = ParentEditor->GetDeclarationTextBoxWrapper()->ScrollToLine(LineNumber);
    }

    // Show notification if line number was not found in either text box
    if (!bFoundInImplementation && !bFoundInDeclaration)
    {
        FNotificationInfo Info(FText::Format(LOCTEXT("LineNotFound", "Line {0} not found in the current document"), FText::AsNumber(LineNumber)));
        Info.ExpireDuration = 3.0f;
        Info.bFireAndForget = true;
        Info.bUseLargeFont = false;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

#undef LOCTEXT_NAMESPACE
