// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/Utility/Indentation/QCE_IndentationManager.h"

#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Settings/UQCE_EditorSettings.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

void QCE_IndentationManager::IndentLine(SQCE_MultiLineEditableTextBox* TextBox)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	if (!EditableText.IsValid())
		return;

	SMultiLineEditableText::FScopedEditableTextTransaction Transaction(EditableText);

	FTextSelection Selection = EditableText->GetSelection();
	FTextLocation StartLoc = Selection.GetBeginning();
	FTextLocation EndLoc = Selection.GetEnd();

	FString FullText = EditableText->GetText().ToString();
	TArray<FString> Lines;
	FullText.ParseIntoArrayLines(Lines, false);
	if (!Lines.IsValidIndex(StartLoc.GetLineIndex()))
		return;
	if (!Lines.IsValidIndex(EndLoc.GetLineIndex()))
		EndLoc = FTextLocation(Lines.Num() - 1, 0);

	int32 FirstLine = StartLoc.GetLineIndex();
	int32 LastLine = EndLoc.GetLineIndex();
	FString IndentToInsert = GetSingleIndentString();

	for (int32 i = FirstLine; i <= LastLine; ++i)
	{
		Lines[i].InsertAt(0, IndentToInsert);
	}

	FString NewText = FString::Join(Lines, TEXT("\n"));
	TextBox->SetText(FText::FromString(NewText));

	int32 IndentLength = IndentToInsert.Len();
	if (FirstLine == LastLine)
	{
		int32 ColIndex = StartLoc.GetOffset();
		int32 NewCol = ColIndex + IndentLength;
		EditableText->GoTo(FTextLocation(FirstLine, NewCol));
	}
	else
	{
		int32 NewStartCol = StartLoc.GetOffset();
		int32 NewEndCol = EndLoc.GetOffset();
		NewStartCol += IndentLength;
		NewEndCol += IndentLength;
		EditableText->SelectText(
			FTextLocation(FirstLine, NewStartCol),
			FTextLocation(LastLine, NewEndCol)
		);
	}
}

void QCE_IndentationManager::UnindentLine(SQCE_MultiLineEditableTextBox* TextBox)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	if (!EditableText.IsValid())
		return;
	
	SMultiLineEditableText::FScopedEditableTextTransaction Transaction(EditableText);
	FTextSelection Selection = EditableText->GetSelection();
	FTextLocation StartLoc = Selection.GetBeginning();
	FTextLocation EndLoc = Selection.GetEnd();

	FString FullText = EditableText->GetText().ToString();
	TArray<FString> Lines;
	FullText.ParseIntoArrayLines(Lines, false);
	if (!Lines.IsValidIndex(StartLoc.GetLineIndex()))
		return;
	if (!Lines.IsValidIndex(EndLoc.GetLineIndex()))
		EndLoc = FTextLocation(Lines.Num() - 1, 0);

	TArray<int32> RemovedCounts;
	RemovedCounts.SetNumZeroed(Lines.Num());

	int32 FirstLine = StartLoc.GetLineIndex();
	int32 LastLine = EndLoc.GetLineIndex();
	
	for (int32 i = FirstLine; i <= LastLine; ++i)
	{
		FString& Line = Lines[i];
		int32 RemoveCount = 0;
		
		if (Settings->IndentationType == EQCEIndentationType::Tabs)
		{
			if (Line.StartsWith(TEXT("\t")))
			{
				Line.RemoveAt(0);
				RemoveCount = 1;
			}
			else
			{
				int32 MaxRemove = Settings->TabSpaceCount;
				while (RemoveCount < MaxRemove && Line.Len() > 0 && Line[0] == TEXT(' '))
				{
					Line.RemoveAt(0);
					++RemoveCount;
				}
			}
		}
		else
		{
			int32 MaxRemove = Settings->TabSpaceCount;
			while (RemoveCount < MaxRemove && Line.Len() > 0 && Line[0] == TEXT(' '))
			{
				Line.RemoveAt(0);
				++RemoveCount;
			}
			if (RemoveCount == 0 && Line.StartsWith(TEXT("\t")))
			{
				Line.RemoveAt(0);
				RemoveCount = 1;
			}
		}
		
		RemovedCounts[i] = RemoveCount;
	}

	FString NewText = FString::Join(Lines, TEXT("\n"));
	EditableText->SetText(FText::FromString(NewText));

	if (FirstLine == LastLine)
	{
		int32 NewCol = FMath::Max(StartLoc.GetOffset() - RemovedCounts[FirstLine], 0);
		EditableText->GoTo(FTextLocation(FirstLine, NewCol));
	}
	else
	{
		int32 NewStartCol = StartLoc.GetOffset();
		int32 NewEndCol = EndLoc.GetOffset();
		if (RemovedCounts[FirstLine] > 0)
			NewStartCol = FMath::Max(NewStartCol - RemovedCounts[FirstLine], 0);
		if (RemovedCounts[LastLine] > 0)
			NewEndCol = FMath::Max(NewEndCol - RemovedCounts[LastLine], 0);
		EditableText->SelectText(
			FTextLocation(FirstLine, NewStartCol),
			FTextLocation(LastLine, NewEndCol)
		);
	}
}

bool QCE_IndentationManager::GetLineIndentation(const SQCE_MultiLineEditableTextBox* TextBox, FString& OutIndentation)
{
	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	const FTextLocation CursorLocation = EditableText->GetCursorLocation();
	const FString FullText = EditableText->GetText().ToString();

	TArray<FString> Lines;
	FullText.ParseIntoArrayLines(Lines, false);

	if (!Lines.IsValidIndex(CursorLocation.GetLineIndex()))
		return false;

	const FString& CurrentLine = Lines[CursorLocation.GetLineIndex()];
	OutIndentation = "";

	for (int32 i = 0; i < CurrentLine.Len(); ++i)
	{
		const TCHAR& Char = CurrentLine[i];
		if (Char == TEXT(' ') || Char == TEXT('\t'))
		{
			OutIndentation.AppendChar(Char);
		}
		else
		{
			break;
		}
	}

	if (OutIndentation.IsEmpty())
	{
		const int32 NextLineIndex = CursorLocation.GetLineIndex() + 1;
		if (Lines.IsValidIndex(NextLineIndex))
		{
			const FString& NextLine = Lines[NextLineIndex];
			for (int32 i = 0; i < NextLine.Len(); ++i)
			{
				const TCHAR& Char = NextLine[i];
				if (Char == TEXT(' ') || Char == TEXT('\t'))
				{
					OutIndentation.AppendChar(Char);
				}
				else
				{
					break;
				}
			}
		}
	}

	return true;
}

FString QCE_IndentationManager::ProcessCompletionTextIndentation(const SQCE_MultiLineEditableTextBox* TextBox, const FString& CompletionText)
{
	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();

	if (!CompletionText.Contains(TEXT("\n")))
	{
		return CompletionText;
	}

	FString CurrentIndentation;
	if (!GetLineIndentation(TextBox, CurrentIndentation))
	{
		return CompletionText;
	}

	TArray<FString> CompletionLines;
	CompletionText.ParseIntoArrayLines(CompletionLines, false);

	if (CompletionLines.Num() <= 1)
	{
		return CompletionText;
	}

	FString ProcessedText = CompletionLines[0];
	
	for (int32 i = 1; i < CompletionLines.Num(); ++i)
	{
		FString Line = CompletionLines[i];
		int32 LeadingWhitespace = 0;
		for (int32 j = 0; j < Line.Len(); ++j)
		{
			if (Line[j] == TEXT(' ') || Line[j] == TEXT('\t'))
			{
				LeadingWhitespace++;
			}
			else
			{
				break;
			}
		}
		
		FString LineContent = Line.Mid(LeadingWhitespace);
		if (LeadingWhitespace > 0)
		{
			int32 EstimatedLevels = FMath::Max(1, LeadingWhitespace / 4);
			FString AdditionalIndent = GetIndentString(EstimatedLevels);
			ProcessedText += TEXT("\n") + CurrentIndentation + AdditionalIndent + LineContent;
		}
		else
		{
			ProcessedText += TEXT("\n") + CurrentIndentation + LineContent;
		}
	}

	return ProcessedText;
	
}

FString QCE_IndentationManager::GetEnterKeyIndentation(const SQCE_MultiLineEditableTextBox* TextBox)
{
	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	if (!EditableText.IsValid())
		return TEXT("\n");

	const FTextLocation CursorLocation = EditableText->GetCursorLocation();
	const FString FullText = EditableText->GetText().ToString();
	TArray<FString> Lines;
	FullText.ParseIntoArrayLines(Lines, false);
	
	if (!Lines.IsValidIndex(CursorLocation.GetLineIndex()))
		return TEXT("\n");

	const FString& CurrentLine = Lines[CursorLocation.GetLineIndex()];
	
	FString CurrentIndentation;
	for (int32 i = 0; i < CurrentLine.Len(); ++i)
	{
		const TCHAR& Char = CurrentLine[i];
		if (Char == TEXT(' ') || Char == TEXT('\t'))
		{
			CurrentIndentation.AppendChar(Char);
		}
		else
		{
			break;
		}
	}

	FString TrimmedLine = CurrentLine.TrimEnd();
	if (TrimmedLine.EndsWith(TEXT("{")))
	{
		FString AdditionalIndent = GetSingleIndentString();
		return TEXT("\n") + CurrentIndentation + AdditionalIndent;
	}

	const int32 NextLineIndex = CursorLocation.GetLineIndex() + 1;
	if (Lines.IsValidIndex(NextLineIndex))
	{
		const FString& NextLine = Lines[NextLineIndex];
		FString NextIndentation;
		for (int32 i = 0; i < NextLine.Len(); ++i)
		{
			const TCHAR& Char = NextLine[i];
			if (Char == TEXT(' ') || Char == TEXT('\t'))
			{
				NextIndentation.AppendChar(Char);
			}
			else
			{
				break;
			}
		}

		if (NextIndentation.Len() > CurrentIndentation.Len())
		{
			return TEXT("\n") + NextIndentation;
		}
	}

	return TEXT("\n") + CurrentIndentation;
}

void QCE_IndentationManager::MoveCursorToFirstNonWhitespace(SQCE_MultiLineEditableTextBox* TextBox)
{
	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	if (!EditableText.IsValid())
		return;

	const FTextLocation CurrentCursor = EditableText->GetCursorLocation();
	const FString FullText = EditableText->GetText().ToString();
	TArray<FString> Lines;
	FullText.ParseIntoArrayLines(Lines, false);
	
	if (!Lines.IsValidIndex(CurrentCursor.GetLineIndex()))
		return;

	const FString& CurrentLine = Lines[CurrentCursor.GetLineIndex()];
	
	int32 FirstNonWhitespaceIndex = 0;
	for (int32 i = 0; i < CurrentLine.Len(); ++i)
	{
		const TCHAR& Char = CurrentLine[i];
		if (Char != TEXT(' ') && Char != TEXT('\t'))
		{
			FirstNonWhitespaceIndex = i;
			break;
		}
	}
	
	if (CurrentCursor.GetOffset() < FirstNonWhitespaceIndex)
	{
		EditableText->GoTo(FTextLocation(CurrentCursor.GetLineIndex(), FirstNonWhitespaceIndex));
	}
	
}

bool QCE_IndentationManager::HandleSmartBackspace(SQCE_MultiLineEditableTextBox* TextBox)
{
	TSharedPtr< SMultiLineEditableText > EditableText = TextBox->GetEditableText();
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	if (!EditableText.IsValid())
		return false;

	return false;
}

FString QCE_IndentationManager::GetSingleIndentString()
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	if (Settings->IndentationType == EQCEIndentationType::Tabs)
	{
		return TEXT("\t");
	}
	else
	{
		FString Spaces;
		Spaces.Reserve(Settings->TabSpaceCount);
		for (int32 i = 0; i < Settings->TabSpaceCount; ++i)
		{
			Spaces.AppendChar(TEXT(' '));
		}
		return Spaces;
	}
}

FString QCE_IndentationManager::GetIndentString(int32 IndentLevels)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	if (Settings->IndentationType == EQCEIndentationType::Tabs)
	{
		FString Tabs;
		Tabs.Reserve(IndentLevels);
		for (int32 i = 0; i < IndentLevels; ++i)
		{
			Tabs.AppendChar(TEXT('\t'));
		}
		return Tabs;
	}
	else
	{
		const int32 TotalSpaces = Settings->TabSpaceCount * IndentLevels;
		FString Spaces;
		Spaces.Reserve(TotalSpaces);
		for (int32 i = 0; i < TotalSpaces; ++i)
		{
			Spaces.AppendChar(TEXT(' '));
		}
		return Spaces;
	}
}
