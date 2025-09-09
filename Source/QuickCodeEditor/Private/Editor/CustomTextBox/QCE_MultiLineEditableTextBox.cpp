// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextLayout.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Editor/CustomTextBox/CodeCompletion/UI/QCE_CodeCompletionSuggestionBox.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Internationalization/Text.h"
#include "Settings/UQCE_EditorSettings.h"
#include "Editor/CustomTextBox/InlineAISuggestion/FInlineAISuggestionEngine.h"
#include "Editor/CustomTextBox/InlineAISuggestion/Utils/QCE_InlineAISuggestionContextBuilder.h"
#include "Editor/CustomTextBox/InlineAISuggestion/UI/QCE_InlineAISuggestionBox.h"
#include "Editor/CustomTextBox/Utility/Indentation/QCE_IndentationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#pragma region General
void SQCE_MultiLineEditableTextBox::Construct(const FArguments& InArgs)
{
	OnQCEFocused = InArgs._OnQCEFocused;

	SMultiLineEditableTextBox::Construct(
		SMultiLineEditableTextBox::FArguments()
		.Style(InArgs._Style)
		.Marshaller(InArgs._Marshaller)
		.Text(InArgs._Text)
		.HintText(InArgs._HintText)
		.SearchText(InArgs._SearchText)
		.Font(InArgs._Font)
		.ForegroundColor(InArgs._ForegroundColor)
		.CreateSlateTextLayout(InArgs._CreateSlateTextLayout)
		.ReadOnlyForegroundColor(InArgs._ReadOnlyForegroundColor)
		.FocusedForegroundColor(InArgs._FocusedForegroundColor)
		.Justification(InArgs._Justification)
		.LineHeightPercentage(InArgs._LineHeightPercentage)
		.IsReadOnly(InArgs._IsReadOnly)
		.AllowMultiLine(InArgs._AllowMultiLine)
		.IsCaretMovedWhenGainFocus(InArgs._IsCaretMovedWhenGainFocus)
		.SelectAllTextWhenFocused(InArgs._SelectAllTextWhenFocused)
		.ClearTextSelectionOnFocusLoss(InArgs._ClearTextSelectionOnFocusLoss)
		.RevertTextOnEscape(InArgs._RevertTextOnEscape)
		.ClearKeyboardFocusOnCommit(InArgs._ClearKeyboardFocusOnCommit)
		.AllowContextMenu(InArgs._AllowContextMenu)
		.AlwaysShowScrollbars(InArgs._AlwaysShowScrollbars)
		.HScrollBar(InArgs._HScrollBar)
		.VScrollBar(InArgs._VScrollBar)
		.HScrollBarPadding(InArgs._HScrollBarPadding)
		.VScrollBarPadding(InArgs._VScrollBarPadding)
		.OnTextChanged(InArgs._OnTextChanged)
		.OnTextCommitted(InArgs._OnTextCommitted)
		.OnVerifyTextChanged(InArgs._OnVerifyTextChanged)
		.OnHScrollBarUserScrolled(InArgs._OnHScrollBarUserScrolled)
		.OnVScrollBarUserScrolled(InArgs._OnVScrollBarUserScrolled)
		.OnCursorMoved(
			SMultiLineEditableText::FOnCursorMoved::CreateRaw(
				this, &SQCE_MultiLineEditableTextBox::SelectCursorWordOccurrences))
		.WrapTextAt(InArgs._WrapTextAt)
		.AutoWrapText(InArgs._AutoWrapText)
		.ContextMenuExtender(InArgs._ContextMenuExtender)
		.WrappingPolicy(InArgs._WrappingPolicy)
		.SelectAllTextOnCommit(InArgs._SelectAllTextOnCommit)
		.SelectWordOnMouseDoubleClick(InArgs._SelectWordOnMouseDoubleClick)
		.BackgroundColor(InArgs._BackgroundColor)
		.Padding(InArgs._Padding)
		.Margin(InArgs._Margin)
		.ModiferKeyForNewLine(InArgs._ModiferKeyForNewLine)
		.VirtualKeyboardOptions(InArgs._VirtualKeyboardOptions)
		.VirtualKeyboardTrigger(InArgs._VirtualKeyboardTrigger)
		.VirtualKeyboardDismissAction(InArgs._VirtualKeyboardDismissAction)
	);

	LastHighlightedWord = FString();
	OnQCEFocused = InArgs._OnQCEFocused;
	OnSearchRequested = InArgs._OnSearchRequested;
	OnSaveRequested = InArgs._OnSaveRequested;
	OnSaveAndBuildRequested = InArgs._OnSaveAndBuildRequested;
	OnGoToLineRequested = InArgs._OnGoToLineRequested;
	OnCodeCompletionRequested = InArgs._OnCodeCompletionRequested;
	
	SetOnKeyDownHandler(FOnKeyDown::CreateRaw(this, &SQCE_MultiLineEditableTextBox::HandleKeyDown));
	SetOnKeyCharHandler(FOnKeyChar::CreateRaw(this, &SQCE_MultiLineEditableTextBox::HandleKeyChar));

	ApplicationFocusChangedHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddSP(
		this, &SQCE_MultiLineEditableTextBox::OnApplicationFocusChanged);
}

FReply SQCE_MultiLineEditableTextBox::HandleKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	const auto Key = KeyEvent.GetKey();
	if (Key == EKeys::BackSpace && !bIsChatBox && !bShouldFocusCodeCompletionMenu)
	{
		if (QCE_IndentationManager::HandleSmartBackspace(this))
		{
			return FReply::Handled();
		}
	}
	
	if (Key == EKeys::Enter && !bIsChatBox && !bShouldFocusCodeCompletionMenu)
	{
		QCE_IndentationManager::MoveCursorToFirstNonWhitespace(this);
		FString NewlineWithIndentation = QCE_IndentationManager::GetEnterKeyIndentation(this);
		if (NewlineWithIndentation == TEXT("\n"))
		{
			return OnKeyDown(Geometry, KeyEvent);
		}
		
		EditableText->InsertTextAtCursor(FText::FromString(NewlineWithIndentation));
		return FReply::Handled();
	}

	if (bShouldFocusCodeCompletionMenu)
	{
		if (Key == EKeys::Up)
		{
			QCE_CodeCompletionSuggestionBox->SelectPreviousSuggestion();
			return FReply::Handled();
		}
		if (Key == EKeys::Down)
		{
			QCE_CodeCompletionSuggestionBox->SelectNextSuggestion();
			return FReply::Handled();
		}
		if (Key == EKeys::Enter)
		{
			QCE_CodeCompletionSuggestionBox->AcceptSelectedSuggestion();
			return FReply::Handled();
		}
	}

	
	return SMultiLineEditableTextBox::OnKeyDown(Geometry, KeyEvent);
}

FReply SQCE_MultiLineEditableTextBox::HandleKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InKeyEvent)
{
	const auto Key = InKeyEvent.GetCharacter();
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	if (Key == TEXT('{'))
	{
		if (!EditableText.IsValid())
		{
			return SMultiLineEditableTextBox::OnKeyChar(MyGeometry, InKeyEvent);
		}

		SMultiLineEditableText::FScopedEditableTextTransaction Transaction(EditableText);

		const FTextLocation CursorLocation = EditableText->GetCursorLocation();
		const FString FullText = GetText().ToString();
		
		FString CurrentIndentation;
		QCE_IndentationManager::GetLineIndentation(this, CurrentIndentation);
		FString AdditionalIndent;
		for (int32 i = 0; i < Settings->TabSpaceCount; ++i)
		{
			AdditionalIndent.AppendChar(TEXT(' '));
		}
		
		FString BracketText = TEXT("{\n") + CurrentIndentation + AdditionalIndent + TEXT("\n") + CurrentIndentation + TEXT("}");
		EditableText->InsertTextAtCursor(FText::FromString(BracketText));
		int32 NewLineIndex = CursorLocation.GetLineIndex() + 1;
		int32 NewOffset = CurrentIndentation.Len() + AdditionalIndent.Len();
		EditableText->GoTo(FTextLocation(NewLineIndex, NewOffset));
		
		return FReply::Handled();
	}

	if (Key == TEXT(' '))
	{
		// Prevent space input when Ctrl is held (e.g., Ctrl+Space for dropdown toggle)
		if (FSlateApplication::Get().GetModifierKeys().IsControlDown())
		{
			return FReply::Handled();
		}
		
		if (CodeCompletionMenuContainer.IsValid())
			HideMemberSuggestions();
	}
	return SMultiLineEditableTextBox::OnKeyChar(MyGeometry, InKeyEvent);
}

FReply SQCE_MultiLineEditableTextBox::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const auto Key = InKeyEvent.GetKey();
	
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);
	
	const FInputChord KeyEventChord(InKeyEvent.GetKey(),
	                                EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(),
	                                                        InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));
	
	if (Settings->FindKeybinding.IsValidChord() && Settings->FindKeybinding == KeyEventChord)
	{
		OnSearchRequested.ExecuteIfBound();
		return FReply::Handled();
	}

	if (Settings->SaveKeybinding.IsValidChord() && Settings->SaveKeybinding == KeyEventChord)
	{
		OnSaveRequested.ExecuteIfBound();
		return FReply::Handled();
	}

	if (Settings->SaveAndBuildKeybinding.IsValidChord() && Settings->SaveAndBuildKeybinding == KeyEventChord)
	{
		OnSaveAndBuildRequested.ExecuteIfBound();
		return FReply::Handled();
	}
	
	if (InlineAICompletionState == EInlineAICompletionState::CompletionOffered && Key == EKeys::Escape)
	{
		HandleAICompletionReject();
		return FReply::Handled();
	}
	
	if (Settings->IndentKeybinding.IsValidChord() && Settings->IndentKeybinding == KeyEventChord )
	{
		QCE_IndentationManager::IndentLine(this);
		return FReply::Handled();
	}
	
	if (Settings->UnindentKeybinding.IsValidChord() && Settings->UnindentKeybinding == KeyEventChord)
	{
		QCE_IndentationManager::UnindentLine(this);
		return FReply::Handled();
	}

	if (Settings->GoToLineKeybinding.IsValidChord() && Settings->GoToLineKeybinding == KeyEventChord)
	{
		OnGoToLineRequested.ExecuteIfBound();
		return FReply::Handled();
	}

	if (Settings->AIInlineCompletionKeybinding.IsValidChord() && Settings->AIInlineCompletionKeybinding == KeyEventChord)
	{
		TriggerInlineSuggestion();
		return FReply::Handled();
	}

	if (Settings->AutocompletionDropdownKeybinding.IsValidChord() && Settings->AutocompletionDropdownKeybinding == KeyEventChord)
	{
		ToggleCodeCompletionDropdown();
		return FReply::Handled();
	}

	return SMultiLineEditableTextBox::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SQCE_MultiLineEditableTextBox::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bShouldFocusCodeCompletionMenu)
	{
		HideMemberSuggestions();
	}
	
	return FReply::Unhandled();
}

FReply SQCE_MultiLineEditableTextBox::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bIsChatBox)
		return SMultiLineEditableTextBox::OnKeyUp(MyGeometry, InKeyEvent);

	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		const bool bIsShiftDown = InKeyEvent.IsShiftDown();

		if (!bIsShiftDown)
		{
			OnEnterPressed.Execute();
			return FReply::Handled();
		}
	}

	return SMultiLineEditableTextBox::OnKeyUp(MyGeometry, InKeyEvent);
}

void SQCE_MultiLineEditableTextBox::OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath,
													const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	bool bIsGettingFocused = false;
	if (NewWidgetPath.IsValid())
	{
		for (int32 WidgetIndex = 0; WidgetIndex < NewWidgetPath.Widgets.Num(); ++WidgetIndex)
		{
			if (NewWidgetPath.Widgets[WidgetIndex].Widget == SMultiLineEditableTextBox::AsShared())
			{
				bIsGettingFocused = true;
				break;
			}
		}
	}

	if (bIsGettingFocused)
	{
		OnQCEFocused.ExecuteIfBound();
	}

	SMultiLineEditableTextBox::OnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
}


void SQCE_MultiLineEditableTextBox::SelectCursorWordOccurrences(const FTextLocation& CursorLocation)
{
	LastCursorLocation = CursorLocation;

	const FString CurrentWord = GetWordAtCursor();
	if (CurrentWord == LastHighlightedWord) return;

	SelectWordOccurrences(CurrentWord);
}

void SQCE_MultiLineEditableTextBox::SelectWordOccurrences(const FString& TargetWord)
{
	if (!LastHighlightedWord.IsEmpty() && FQCE_TextLayout)
		FQCE_TextLayout->ClearHighlights();

	if (!TargetWord.IsEmpty() && FQCE_TextLayout)
		FQCE_TextLayout->HighlightWord(TargetWord);

	LastHighlightedWord = TargetWord;
}

void SQCE_MultiLineEditableTextBox::SelectSpecificOccurrence(const FString& Word, int32 AbsolutePosition, int32 Length)
{
	if (!EditableText.IsValid())
		return;

	if (FQCE_TextLayout)
		FQCE_TextLayout->ClearHighlights();

	const FString TextString = GetText().ToString();

	int32 LineIndex = 0;
	int32 LineStartPos = 0;
	int32 CurrentPos = 0;

	while (CurrentPos < AbsolutePosition && CurrentPos < TextString.Len())
	{
		if (TextString[CurrentPos] == TEXT('\n'))
		{
			LineIndex++;
			LineStartPos = CurrentPos + 1;
		}
		CurrentPos++;
	}

	int32 StartOffset = AbsolutePosition - LineStartPos;
	int32 EndOffset = StartOffset + Length;

	if (FQCE_TextLayout)
	{
		FQCE_TextLayout->HighlightSpecificOccurrence(Word, LineIndex, StartOffset, EndOffset);
	}

	FTextLocation StartLocation(LineIndex, StartOffset);
	FTextLocation EndLocation(LineIndex, EndOffset);

	EditableText->SelectText(StartLocation, EndLocation);
	LastCursorLocation = EndLocation;
	LastHighlightedWord = Word;
}

FString SQCE_MultiLineEditableTextBox::GetWordAtCursor() const
{
	return GetWordAtLocation(EditableText->GetCursorLocation());
}

FString SQCE_MultiLineEditableTextBox::GetWordAtLocation(const FTextLocation TargetLocation) const
{
	const FText CurrentText = GetText();
	const FString TextString = CurrentText.ToString();
	FString CurrentLine;
	GetCurrentTextLine(CurrentLine);
	int32 WordStart = TargetLocation.GetOffset();
	int32 WordEnd = WordStart;

	while (WordStart > 0 && FChar::IsAlnum(CurrentLine[WordStart - 1]))
	{
		--WordStart;
	}

	while (WordEnd < CurrentLine.Len() && FChar::IsAlnum(CurrentLine[WordEnd]))
	{
		++WordEnd;
	}

	return CurrentLine.Mid(WordStart, WordEnd - WordStart);
}

#pragma endregion

#pragma region CodeCompletion

void SQCE_MultiLineEditableTextBox::ShowMemberSuggestions()
{
	if (!QCE_CodeCompletionSuggestionBox.IsValid())
	{
		SAssignNew(QCE_CodeCompletionSuggestionBox, SQCE_CodeCompletionSuggestionBox)
		.OnCompletionSelected(this, &SQCE_MultiLineEditableTextBox::OnMemberSuggestionSelected)
		.OnCompletionCancelled(this, &SQCE_MultiLineEditableTextBox::HideMemberSuggestions);

		if (CompletionEngine)
			QCE_CodeCompletionSuggestionBox->SetCompletionEngine(CompletionEngine);
	}

	const FTextLocation CursorLocation = EditableText->GetCursorLocation();
	const FString TextString = GetText().ToString();
	int32 AbsoluteCursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(TextString, CursorLocation);
	
	if (AbsoluteCursorPosition == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid cursor position for code completion: Line %d, Offset %d"), 
			CursorLocation.GetLineIndex(), CursorLocation.GetOffset());
		return;
	}
	
	QCE_CodeCompletionSuggestionBox->InitSuggestions(TextString, AbsoluteCursorPosition, this);
	
	CodeCompletionMenuContainer = FSlateApplication::Get().PushMenu(
		SMultiLineEditableTextBox::AsShared(),
		FWidgetPath(), 
		QCE_CodeCompletionSuggestionBox.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		false, 
		FVector2D(1.0f, 1.0f) 
	);

	bShouldFocusCodeCompletionMenu = true;
}

void SQCE_MultiLineEditableTextBox::HideMemberSuggestions()
{
	bShouldFocusCodeCompletionMenu = false;
	CodeCompletionMenuContainer->Dismiss();
}

void SQCE_MultiLineEditableTextBox::ToggleCodeCompletionDropdown()
{
	if (bShouldFocusCodeCompletionMenu)
	{
		HideMemberSuggestions();
	}
	else
	{
		ShowMemberSuggestions();
	}
}

void SQCE_MultiLineEditableTextBox::OnMemberSuggestionSelected(TSharedPtr<FCompletionItem> SelectedItem)
{
	if (!SelectedItem.IsValid())
	{
		HideMemberSuggestions();
		return;
	}

	FString CurrentWord = GetWordAtCursor();
	FTextLocation CursorLocation = EditableText->GetCursorLocation();

	FString CompletionText = SelectedItem->InsertText;

	if (!CurrentWord.IsEmpty())
	{
		FString CurrentLine;
		GetCurrentTextLine(CurrentLine);
		
		int32 WordStart = CursorLocation.GetOffset();
		while (WordStart > 0 && (FChar::IsAlnum(CurrentLine[WordStart - 1]) || CurrentLine[WordStart - 1] == TEXT('_')))
		{
			WordStart--;
		}
		
		int32 WordEnd = CursorLocation.GetOffset();
		while (WordEnd < CurrentLine.Len() && (FChar::IsAlnum(CurrentLine[WordEnd]) || CurrentLine[WordEnd] == TEXT('_')))
		{
			WordEnd++;
		}

		FTextLocation WordStartLocation(CursorLocation.GetLineIndex(), WordStart);
		FTextLocation WordEndLocation(CursorLocation.GetLineIndex(), WordEnd);
		EditableText->SelectText(WordStartLocation, WordEndLocation);
		EditableText->InsertTextAtCursor(FText::FromString(CompletionText));
	}
	else
	{
		EditableText->InsertTextAtCursor(FText::FromString(CompletionText));
	}

	HideMemberSuggestions();
}

void SQCE_MultiLineEditableTextBox::SetCodeCompletionEngine(FDropdownCodeCompletionEngine* InCompletionEngine)
{
	CompletionEngine = InCompletionEngine;
}

#pragma endregion CodeCompletion

#pragma region InlineSuggestions

void SQCE_MultiLineEditableTextBox::TriggerInlineSuggestion()
{
	if (InlineAICompletionState == EInlineAICompletionState::None)
		ShowInlineAISuggestionMenu();
}

void SQCE_MultiLineEditableTextBox::HandleAICompletionAccept()
{
	if (InlineAICompletionState == EInlineAICompletionState::CompletionOffered)
	{
		if (EditableText.IsValid())
		{
			EditableText->ClearSelection();
		}
		
		GoTo(PendingCompletionEndPosition);
		PendingCompletionText.Empty();
		InlineAICompletionState = EInlineAICompletionState::None;
	}
}

void SQCE_MultiLineEditableTextBox::HandleAICompletionReject()
{
	if (InlineAICompletionState == EInlineAICompletionState::CompletionOffered)
	{
		if (EditableText.IsValid())
		{
			EditableText->SelectText(PendingCompletionCursorPosition, PendingCompletionEndPosition);
			EditableText->DeleteSelectedText();
		}
		
		GoTo(PendingCompletionCursorPosition);
		PendingCompletionText.Empty();
		InlineAICompletionState = EInlineAICompletionState::None;
	}
}

void SQCE_MultiLineEditableTextBox::OnAICompletionReceived(const FCompletionResponse& Response, bool bSuccess)
{
	if (bSuccess && !Response.CompletionText.IsEmpty())
	{
		PendingCompletionCursorPosition = GetCursorLocation();
		FString ProcessedCompletionText = QCE_IndentationManager::ProcessCompletionTextIndentation(this, Response.CompletionText);
		InsertTextAtCursor(FText::FromString(ProcessedCompletionText));
		PendingCompletionText = Response.CompletionText;
		PendingCompletionEndPosition = GetCursorLocation();
		
		if (EditableText.IsValid())
		{
			EditableText->SelectText(PendingCompletionCursorPosition, PendingCompletionEndPosition);
		}
		
		InlineAISuggestionBox->SetSuggestionBoxState(ReadyForInput);
	}
	else
	{
		InlineAICompletionState = EInlineAICompletionState::None;
		PendingCompletionText.Empty();
	}
	HideInlineAISuggestionMenu();
}

void SQCE_MultiLineEditableTextBox::ShowInlineAISuggestionMenu()
{
	if (!InlineAISuggestionBox.IsValid())
	{
		SAssignNew(InlineAISuggestionBox, SQCE_InlineAISuggestionBox)
		.MinDesiredWidth(400.0f)
		.MinDesiredHeight(120.0f)
		.TextBoxType(GetTextBoxType())
		.OnConfirmed(this, &SQCE_MultiLineEditableTextBox::OnInlineAISuggestionConfirmed) // <--- Continue if input correct
		.OnCancelled(this, &SQCE_MultiLineEditableTextBox::OnInlineAISuggestionCancelled);
	}

	InlineAISuggestionMenuContainer = FSlateApplication::Get().PushMenu(
		SMultiLineEditableTextBox::AsShared(), 
		FWidgetPath(), 
		InlineAISuggestionBox.ToSharedRef(),
		FSlateApplication::Get().GetCursorPos(), 
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		false, 
		FVector2D(1.0f, 1.0f) 
	);

	if (InlineAISuggestionBox.IsValid())
	{
		InlineAISuggestionBox->FocusContextTextBox();
	}
}

void SQCE_MultiLineEditableTextBox::HideInlineAISuggestionMenu()
{
	InlineAISuggestionMenuContainer->Dismiss();
}

void SQCE_MultiLineEditableTextBox::OnInlineAISuggestionConfirmed(FUserInputContext UserInputContext)
{
	FInlineAISuggestionEngine& AIEngine = FInlineAISuggestionEngine::Get();
	if (AIEngine.IsAvailable())
	{
		const FTextLocation CursorLocation = GetCursorLocation();
		PendingCompletionCursorPosition = CursorLocation;

		FOnCompletionReceived CompletionDelegate;
		CompletionDelegate.BindSP(this, &SQCE_MultiLineEditableTextBox::OnAICompletionReceived);

		const FString CurrentTypedCode = GetText().ToString();
		
		FString RequiredCodeContext;
		bool bCodeContextExtracted = QCE_InlineAISuggestionContextBuilder::GetAIContext(CurrentTypedCode, CursorLocation, GetTextBoxType(), UserInputContext, RequiredCodeContext);
		if (bCodeContextExtracted)
			UserInputContext.Code = RequiredCodeContext;

		UserInputContext.TextBoxType = GetTextBoxType();
		InlineAISuggestionBox->SetSuggestionBoxState(Processing);
		AIEngine.RequestCompletion(UserInputContext, CompletionDelegate);
	}
	else
	{
		InlineAISuggestionBox->SetSuggestionBoxState(ShowingWarning);
		InlineAISuggestionBox->WarningTextBlock->SetText(FText::FromString(TEXT("Please set the AI API key in Code Editor settings...")));
		InlineAICompletionState = EInlineAICompletionState::None;
	}
}

void SQCE_MultiLineEditableTextBox::OnInlineAISuggestionCancelled()
{
	HideInlineAISuggestionMenu();
	InlineAICompletionState = EInlineAICompletionState::None;
}

#pragma endregion InlineSuggestions

#pragma region ApplicationFocus

void SQCE_MultiLineEditableTextBox::OnApplicationFocusChanged(bool bIsFocused)
{
	if (!bIsFocused)
	{
		if (HasKeyboardFocus())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::OtherWidgetLostFocus);
		}
	}
}

#pragma endregion ApplicationFocus
