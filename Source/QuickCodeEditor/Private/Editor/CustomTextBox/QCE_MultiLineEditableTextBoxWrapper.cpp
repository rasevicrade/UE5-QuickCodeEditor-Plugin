// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Settings/UQCE_EditorSettings.h"

void QCE_MultiLineEditableTextBoxWrapper::Construct(const FArguments& InArgs)
{
    bEnableLineNumberColumn = InArgs._EnableLineNumbers;
    MainEditorContainer = InArgs._MainEditorContainer;
    
    TSharedRef<SQCE_MultiLineEditableTextBox> TextBoxRef = SNew(SQCE_MultiLineEditableTextBox)
        .AllowMultiLine(InArgs._AllowMultiLine)
        .ClearTextSelectionOnFocusLoss(false)
        .AutoWrapText(InArgs._AutoWrapText)
        .IsReadOnly(InArgs._IsReadOnly)
        .Marshaller(InArgs._Marshaller)
        .ModiferKeyForNewLine(InArgs._ModifierKeyForNewLine)
        .OnTextChanged_Lambda([this, InArgs](const FText& NewText)
        {
            if (bEnableLineNumberColumn)
            {
                UpdateLineNumbers();
            }
            if (InArgs._OnTextChanged.IsBound())
            {
                InArgs._OnTextChanged.Execute(NewText);
            }
        })
        .OnTextCommitted(InArgs._OnTextCommitted)
        .ContextMenuExtender(InArgs._ContextMenuExtender)
        .CreateSlateTextLayout(InArgs._CreateSlateTextLayout)
        .Text(InArgs._Text)
        .OnQCEFocused_Lambda([this, InArgs]()
        {
            if (InArgs._OnGetFocus.IsBound())
            {
                InArgs._OnGetFocus.Execute();
            }
        })
        .OnSearchRequested(InArgs._OnSearchRequested)
        .OnSaveRequested(InArgs._OnSaveRequested)
        .OnSaveAndBuildRequested(InArgs._OnSaveAndBuildRequested)
        .OnGoToLineRequested(InArgs._OnGoToLineRequested);
    TextBox = TextBoxRef;
    TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
    if (bEnableLineNumberColumn)
    {
        LineNumbers = SNew(STextBlock)
            .Justification(ETextJustify::Right)
            .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
        HorizontalBox->AddSlot()
            .AutoWidth()
            .Padding(4, 0, 4, 0)
            [
                SNew(SBox)
                .MinDesiredWidth(24.0f)
                .MaxDesiredWidth(24.0f)
                [
                    LineNumbers.ToSharedRef()
                ]
            ];
    }

    HorizontalBox->AddSlot()
        .FillWidth(1.0f)
        [
            TextBoxRef
        ];

    ChildSlot
    [
        SAssignNew(WrapperScrollBox, SScrollBox)
        + SScrollBox::Slot()
        [
            HorizontalBox
        ]
    ];

    if (bEnableLineNumberColumn)
    {
        UpdateLineNumbers();
    }

    TextBox->SetParentTextBoxWrapper(SharedThis(this));
    if (MainEditorContainer)
    {
        TextBox->SetMainEditorContainer(MainEditorContainer);
    }
}

void QCE_MultiLineEditableTextBoxWrapper::SetIsModified(bool bNewIsModified)
{
    TextBox->SetIsModified(bNewIsModified);
}

const FString& QCE_MultiLineEditableTextBoxWrapper::GetFilePath() const
{
    return FilePath; 
}

void QCE_MultiLineEditableTextBoxWrapper::SetFilePath(const FString& InFilePath)
{
    FilePath = InFilePath;
}


bool QCE_MultiLineEditableTextBoxWrapper::ScrollToLine(const int32 TargetLine) const
{
    if (!WrapperScrollBox.IsValid() || !TextBox.IsValid())
        return false;
    
    if (TargetLine < 1 || TargetLine > LineCount)
        return false;
    
    const FGeometry& ScrollBoxGeometry = WrapperScrollBox->GetCachedGeometry();
    const FVector2D ViewportSize = ScrollBoxGeometry.GetLocalSize();
    
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    check(Settings);
    const float FontSize = static_cast<float>(Settings->FontSize);
    const float LineHeight = FontSize * 1.5f; 
    const int32 VisibleLines = FMath::Max(1, static_cast<int32>(ViewportSize.Y / LineHeight));
    const int32 HalfVisibleLines = VisibleLines / 2;
    const int32 ClampedTargetLine = FMath::Clamp(TargetLine, 1, LineCount);
    const int32 DesiredScrollLine = FMath::Max(0, ClampedTargetLine - HalfVisibleLines - 1);
    const float ScrollOffset = DesiredScrollLine * LineHeight;
    WrapperScrollBox->SetScrollOffset(ScrollOffset);
    
    return true;
}

void QCE_MultiLineEditableTextBoxWrapper::ScrollToPosition(const int32 TargetPosition) const
{
    if (!TextBox.IsValid() || !WrapperScrollBox.IsValid())
        return;
    
    const FString& Text = TextBox->GetText().ToString();
    if (TargetPosition < 0 || TargetPosition >= Text.Len())
        return;

    int32 LineNumber = 1;
    for (int32 i = 0; i < TargetPosition && i < Text.Len(); i++)
    {
        if (Text[i] == TEXT('\n'))
        {
            LineNumber++;
        }
    }

    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    check(Settings);
    const float FontSize = static_cast<float>(Settings->FontSize);
    const float LineHeight = FontSize * 1.5f;
    const float ScrollOffset = (LineNumber - 1) * LineHeight;
    WrapperScrollBox->SetScrollOffset(ScrollOffset);
}

void QCE_MultiLineEditableTextBoxWrapper::UpdateLineNumbers()
{
    if (!bEnableLineNumberColumn || !TextBox.IsValid() || !LineNumbers.IsValid())
        return;

    const FString Text = TextBox->GetText().ToString();
    LineCount = Text.Len() - Text.Replace(TEXT("\n"), TEXT("")).Len() + 1;
    
    FString LineNumbersText;
    for (int32 i = 1; i <= LineCount; ++i)
    {
        LineNumbersText.Append(FString::FromInt(i));
        if (i < LineCount)
            LineNumbersText.Append(TEXT("\n"));
    }

    LineNumbers->SetText(FText::FromString(LineNumbersText));
}
