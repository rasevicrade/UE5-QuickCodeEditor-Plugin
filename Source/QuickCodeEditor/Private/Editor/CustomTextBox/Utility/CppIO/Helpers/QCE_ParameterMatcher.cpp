// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"

#include "CoreMinimal.h"
#include "Internationalization/Regex.h"

bool QCE_ParameterMatcher::DoParameterTypesMatch(const FString& TypeA, const FString& TypeB, bool bMatchConstness)
{
    // First normalize both types
    FString NormalizedA = NormalizeParameter(TypeA, true);
    FString NormalizedB = NormalizeParameter(TypeB, true);

	if (!NormalizedA.IsEmpty() && NormalizedA == NormalizedB)
		return true;

    // Parse into type info structs
    FParameterTypeInfo InfoA = ParseParameterTypeInfo(NormalizedA);
    FParameterTypeInfo InfoB = ParseParameterTypeInfo(NormalizedB);

    // Log detailed comparison for debugging
    UE_LOG(LogTemp, VeryVerbose, TEXT("DoParameterTypesMatch: Node type '%s' -> %s"), *TypeA, *InfoA.ToString());
    UE_LOG(LogTemp, VeryVerbose, TEXT("DoParameterTypesMatch: Code type '%s' -> %s"), *TypeB, *InfoB.ToString());

    bool bMatch = InfoA.BaseType == InfoB.BaseType &&
                 InfoA.bIsVolatile == InfoB.bIsVolatile &&
                 InfoA.bIsPointer == InfoB.bIsPointer &&
                 InfoA.bIsReference == InfoB.bIsReference &&
                 (bMatchConstness ? InfoA.bIsConst == InfoB.bIsConst : true);
    
    if (!bMatch)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("DoParameterTypesMatch: Node type \"%s\" and Code type \"%s\" do not match"), *NormalizedA, *NormalizedB);
        if (InfoA.BaseType != InfoB.BaseType)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("  BaseType mismatch: Node='%s' vs Code='%s'"), *InfoA.BaseType, *InfoB.BaseType);
        }
        if (InfoA.bIsVolatile != InfoB.bIsVolatile)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("  Volatile mismatch: Node=%s vs Code=%s"), InfoA.bIsVolatile ? TEXT("true") : TEXT("false"), InfoB.bIsVolatile ? TEXT("true") : TEXT("false"));
        }
        if (InfoA.bIsPointer != InfoB.bIsPointer)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("  Pointer mismatch: Node=%s vs Code=%s"), InfoA.bIsPointer ? TEXT("true") : TEXT("false"), InfoB.bIsPointer ? TEXT("true") : TEXT("false"));
        }
        if (InfoA.bIsReference != InfoB.bIsReference)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("  Reference mismatch: Node=%s vs Code=%s"), InfoA.bIsReference ? TEXT("true") : TEXT("false"), InfoB.bIsReference ? TEXT("true") : TEXT("false"));
        }
        if (bMatchConstness && InfoA.bIsConst != InfoB.bIsConst)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("  Const mismatch: Node=%s vs Code=%s"), InfoA.bIsConst ? TEXT("true") : TEXT("false"), InfoB.bIsConst ? TEXT("true") : TEXT("false"));
        }
    }

    return bMatch;
}

bool QCE_ParameterMatcher::DoesFunctionParameterMatchDeclarationParameter(const FString& FunctionParam,
	const FString& DeclarationParam, bool bIsConstRef, bool bMatchConstness)
{
	if (!bIsConstRef)
	{
		return DoParameterTypesMatch(FunctionParam, DeclarationParam, bMatchConstness);
	}
	FString FunctionParamNonConstRef = FunctionParam;
	FunctionParamNonConstRef.RemoveFromStart(TEXT("const "));
	FunctionParamNonConstRef.ReplaceInline(TEXT("*&"), TEXT("*"));

	return DoParameterTypesMatch(FunctionParamNonConstRef, DeclarationParam, bMatchConstness);
}

TArray<FString> QCE_ParameterMatcher::ToParameterArray(const FString& ParameterString)
{
    // --- Example ---
    // Input string:
    // "const TArray<FString>& Arrays, int32 Count = 0, FString DefaultText = TEXT("Hello, World")"
    //
    // Will be split into:
    // [
    //     "const TArray<FString>& Arrays",
    //     "int32 Count = 0", 
    //     "FString DefaultText = TEXT(\"Hello, World\")"
    // ]
    
    // Handle empty input
    if (ParameterString.IsEmpty())
    {
        return TArray<FString>();
    }
    
    TArray<FString> ResultParameterArray;
    
    FString CurrentParam;
    int32 ParenDepth = 0;
    int32 AngleDepth = 0;
    int32 BraceDepth = 0;
    bool bInQuotes = false;
    bool bInSingleQuotes = false;
    bool bInLineComment = false;
    bool bInBlockComment = false;
    bool bEscapeNext = false;

    
    for (int32 i = 0; i < ParameterString.Len(); i++)
    {
       TCHAR Char = ParameterString[i];
       TCHAR NextChar = (i + 1 < ParameterString.Len()) ? ParameterString[i + 1] : 0;
       
       if (bEscapeNext)
       {
          CurrentParam += Char;
          bEscapeNext = false;
          continue;
       }
       
       if (Char == TEXT('\\') && (bInQuotes || bInSingleQuotes))
       {
          bEscapeNext = true;
          CurrentParam += Char;
          continue;
       }
       
       // Handle line comments
       if (bInLineComment)
       {
          CurrentParam += Char;
          if (Char == TEXT('\n') || Char == TEXT('\r'))
          {
             bInLineComment = false;
          }
          continue;
       }
       
       // Handle block comments
       if (bInBlockComment)
       {
          CurrentParam += Char;
          if (Char == TEXT('*') && NextChar == TEXT('/'))
          {
             bInBlockComment = false;
             CurrentParam += NextChar;
             i++; // Skip the '/'
          }
          continue;
       }
       
       // Handle string literals
       if (Char == TEXT('"') && !bInSingleQuotes)
       {
          bInQuotes = !bInQuotes;
          CurrentParam += Char;
          continue;
       }
       
       // Handle character literals
       if (Char == TEXT('\'') && !bInQuotes)
       {
          bInSingleQuotes = !bInSingleQuotes;
          CurrentParam += Char;
          continue;
       }
       
       // If we're in quotes or single quotes, just add the character
       if (bInQuotes || bInSingleQuotes)
       {
          CurrentParam += Char;
          continue;
       }
       
       // Check for comment starts (only when not in quotes)
       if (Char == TEXT('/') && NextChar == TEXT('/'))
       {
          bInLineComment = true;
          CurrentParam += Char;
          continue;
       }
       
       if (Char == TEXT('/') && NextChar == TEXT('*'))
       {
          bInBlockComment = true;
          CurrentParam += Char;
          continue;
       }
       
       // Track nesting depth when NOT in quotes or comments
       if (Char == TEXT('('))
       {
          ParenDepth++;
       }
       else if (Char == TEXT(')'))
       {
          ParenDepth--;
       }
       else if (Char == TEXT('<'))
       {
          // Enhanced template detection - only count as template if preceded by identifier or closing bracket
          bool bLikelyTemplate = false;
          if (i > 0)
          {
             TCHAR PrevChar = ParameterString[i - 1];
             // Check if preceded by identifier character, closing bracket, or namespace operator
             bLikelyTemplate = FChar::IsAlnum(PrevChar) || PrevChar == TEXT('_') || 
                              PrevChar == TEXT('>') || PrevChar == TEXT(')') ||
                              PrevChar == TEXT(':');
          }
          
          if (bLikelyTemplate)
          {
             AngleDepth++;
          }
       }
       else if (Char == TEXT('>'))
       {
          // Only decrement if we have angle depth to decrement
          if (AngleDepth > 0)
          {
             AngleDepth--;
          }
       }
       else if (Char == TEXT('{'))
       {
          BraceDepth++;
       }
       else if (Char == TEXT('}'))
       {
          BraceDepth--;
       }
       // If character is ',' and we aren't inside any brackets, we add CurrentParam to array
       else if (Char == TEXT(',') && ParenDepth == 0 && AngleDepth == 0 && BraceDepth == 0)
       {
          FString TrimmedParam = CurrentParam.TrimStartAndEnd();
          if (!TrimmedParam.IsEmpty())
          {
             ResultParameterArray.Add(TrimmedParam);
          }
          CurrentParam.Empty();
          
          // Skip any whitespace after the comma
          while (i + 1 < ParameterString.Len() && FChar::IsWhitespace(ParameterString[i + 1]))
          {
             i++;
          }
          
          // Check for comments after the comma
          if (i + 1 < ParameterString.Len())
          {
             // Check for line comment
             if (ParameterString[i + 1] == TEXT('/') && i + 2 < ParameterString.Len() && ParameterString[i + 2] == TEXT('/'))
             {
                // Skip until newline
                while (i + 1 < ParameterString.Len() && ParameterString[i + 1] != TEXT('\n') && ParameterString[i + 1] != TEXT('\r'))
                {
                   i++;
                }
             }
             // Check for block comment
             else if (ParameterString[i + 1] == TEXT('/') && i + 2 < ParameterString.Len() && ParameterString[i + 2] == TEXT('*'))
             {
                // Skip until */
                i += 2; // Move past /*
                while (i + 2 < ParameterString.Len() && !(ParameterString[i + 1] == TEXT('*') && ParameterString[i + 2] == TEXT('/')))
                {
                   i++;
                }
                i += 2; // Move past */
             }
          }
          continue;
       }
       
       CurrentParam += Char;
    }
    
    // Add the last parameter if not empty
    FString TrimmedParam = CurrentParam.TrimStartAndEnd();
    if (!TrimmedParam.IsEmpty())
    {
       ResultParameterArray.Add(TrimmedParam);
    }
    
    // Optional: Log warning for unbalanced brackets (helpful for debugging)
    if (ParenDepth != 0 || AngleDepth != 0 || BraceDepth != 0)
    {
       UE_LOG(LogTemp, Warning, TEXT("Unbalanced brackets in parameter string: Parentheses=%d, Angles=%d, Braces=%d"), 
              ParenDepth, AngleDepth, BraceDepth);
    }
    
    return ResultParameterArray;
}

bool QCE_ParameterMatcher::GetParameterStringAtPosition(const FString& FileContent, const int32 MatchPos, FString& ParameterString)
{
	// We want to get the outtermost parantheses of this function and get the string between them
	int32 OpenParenPos = INDEX_NONE;
	if (!FindCharacterRespectingContext(FileContent, TEXT("("), MatchPos, ESearchDir::FromStart, OpenParenPos))
		return false;

	int32 CloseParenPos = INDEX_NONE;
	if (!FindMatchingBracket(FileContent, OpenParenPos, TEXT('('), TEXT(')'), CloseParenPos, false))
		return false;

	ParameterString = FileContent.Mid(OpenParenPos + 1, CloseParenPos - OpenParenPos - 1).TrimStartAndEnd();
	return true;
}

bool QCE_ParameterMatcher::FindMatchingBracket(const FString& Content, int32 OpenBracketPos, TCHAR OpenBracket, TCHAR CloseBracket, int32& OutCloseBracketPos, bool bIncludeSingleQuotes)
{
	if (OpenBracketPos >= Content.Len() || Content[OpenBracketPos] != OpenBracket)
		return false;

	int32 BracketCount = 1;
	int32 SearchPos = OpenBracketPos + 1;
	bool bInDoubleQuotes = false;
	bool bInSingleQuotes = false;
	bool bInLineComment = false;
	bool bInBlockComment = false;
	bool bEscapeNext = false;

	while (BracketCount > 0 && SearchPos < Content.Len())
	{
		TCHAR CurrentChar = Content[SearchPos];
		TCHAR NextChar = (SearchPos + 1 < Content.Len()) ? Content[SearchPos + 1] : 0;
        
		// Handle escape sequences
		if (bEscapeNext)
		{
			bEscapeNext = false;
			SearchPos++;
			continue;
		}
        
		if (CurrentChar == TEXT('\\') && (bInDoubleQuotes || (bIncludeSingleQuotes && bInSingleQuotes)))
		{
			bEscapeNext = true;
			SearchPos++;
			continue;
		}
        
		// Handle line comments
		if (bInLineComment)
		{
			if (CurrentChar == TEXT('\n'))
			{
				bInLineComment = false;
			}
			SearchPos++;
			continue;
		}
		
		// Handle block comments
		if (bInBlockComment)
		{
			if (CurrentChar == TEXT('*') && NextChar == TEXT('/'))
			{
				bInBlockComment = false;
				SearchPos++; // Skip the '/'
			}
			SearchPos++;
			continue;
		}
        
		// Handle quote states
		if (CurrentChar == TEXT('"') && (!bIncludeSingleQuotes || !bInSingleQuotes))
		{
			bInDoubleQuotes = !bInDoubleQuotes;
			SearchPos++;
			continue;
		}
        
		if (bIncludeSingleQuotes && CurrentChar == TEXT('\'') && !bInDoubleQuotes)
		{
			bInSingleQuotes = !bInSingleQuotes;
			SearchPos++;
			continue;
		}
        
		// If we're in quotes, just continue
		if (bInDoubleQuotes || (bIncludeSingleQuotes && bInSingleQuotes))
		{
			SearchPos++;
			continue;
		}
		
		// Check for comment starts (only when not in quotes)
		if (CurrentChar == TEXT('/') && NextChar == TEXT('/'))
		{
			bInLineComment = true;
			SearchPos++;
			continue;
		}
		
		if (CurrentChar == TEXT('/') && NextChar == TEXT('*'))
		{
			bInBlockComment = true;
			SearchPos++;
			continue;
		}
		
		// Only count brackets when NOT inside quotes or comments
		if (CurrentChar == OpenBracket)
			BracketCount++;
		else if (CurrentChar == CloseBracket)
			BracketCount--;

		if (BracketCount == 0)
		{
			OutCloseBracketPos = SearchPos;
			return true;
		}

		SearchPos++;
	}

	return false;
}

FString QCE_ParameterMatcher::NormalizeParameter(const FString& Parameter, bool bRemoveDefaultValue, bool bShouldRemoveParamName)
{
	FString Result = Parameter.TrimStartAndEnd();
	
	// Remove default value if requested
	if (bRemoveDefaultValue)
	{
		Result = RemoveDefaultValue(Result);
	}
	
	
	if (bShouldRemoveParamName)
	{
		Result = NormalizeWhitespace(Result);
		Result = RemoveParameterName(Result);
	}
	
	return Result;
}

FString QCE_ParameterMatcher::RemoveDefaultValue(const FString& Parameter)
{
	//The function removes everything from the = sign onwards in a parameter string, but only if the = is not inside nested structures like parentheses, angle brackets, or braces.
	FString Result = Parameter.TrimStartAndEnd();
	int32 EqualPos = INDEX_NONE;
	int32 ParenDepth = 0;
	int32 AngleDepth = 0;
	int32 BraceDepth = 0;
	bool bInQuotes = false;
	bool bInSingleQuotes = false;
	bool bInLineComment = false;
	bool bInBlockComment = false;
	bool bEscapeNext = false;
	
	// Find the = sign that indicates default value, but not inside nested structures
	for (int32 i = 0; i < Result.Len(); i++)
	{
		TCHAR Char = Result[i];
		TCHAR NextChar = (i + 1 < Result.Len()) ? Result[i + 1] : 0;
		
		if (bEscapeNext)
		{
			bEscapeNext = false;
			continue;
		}
		
		if (Char == TEXT('\\') && (bInQuotes || bInSingleQuotes))
		{
			bEscapeNext = true;
			continue;
		}
		
		// Handle line comments
		if (bInLineComment)
		{
			if (Char == TEXT('\n'))
			{
				bInLineComment = false;
			}
			continue;
		}
		
		// Handle block comments
		if (bInBlockComment)
		{
			if (Char == TEXT('*') && NextChar == TEXT('/'))
			{
				bInBlockComment = false;
				i++; // Skip the '/'
			}
			continue;
		}
		
		if (Char == TEXT('"') && !bInSingleQuotes)
		{
			bInQuotes = !bInQuotes;
			continue;
		}
		
		if (Char == TEXT('\'') && !bInQuotes)
		{
			bInSingleQuotes = !bInSingleQuotes;
			continue;
		}
		
		if (bInQuotes || bInSingleQuotes)
		{
			continue;
		}
		
		// Check for comment starts (only when not in quotes)
		if (Char == TEXT('/') && NextChar == TEXT('/'))
		{
			bInLineComment = true;
			continue;
		}
		
		if (Char == TEXT('/') && NextChar == TEXT('*'))
		{
			bInBlockComment = true;
			continue;
		}
		
		// Track nesting when not in quotes or comments
		if (Char == TEXT('('))
		{
			ParenDepth++;
		}
		else if (Char == TEXT(')'))
		{
			ParenDepth--;
		}
		else if (Char == TEXT('<'))
		{
			AngleDepth++;
		}
		else if (Char == TEXT('>'))
		{
			AngleDepth--;
		}
		else if (Char == TEXT('{'))
		{
			BraceDepth++;
		}
		else if (Char == TEXT('}'))
		{
			BraceDepth--;
		}
		else if (Char == TEXT('=') && ParenDepth == 0 && AngleDepth == 0 && BraceDepth == 0)
		{
			EqualPos = i;
			break;
		}
	}
	
	if (EqualPos != INDEX_NONE)
	{
		Result = Result.Left(EqualPos).TrimStartAndEnd();
	}
	
	return Result;
}

FString QCE_ParameterMatcher::NormalizeWhitespace(const FString& Input)
{
	FString Result;
	bool bLastWasSpace = false;
	bool bInQuotes = false;
	bool bInSingleQuotes = false;
	bool bEscapeNext = false;
	
	for (int32 i = 0; i < Input.Len(); i++)
	{
		TCHAR Char = Input[i];
		
		if (bEscapeNext)
		{
			Result += Char;
			bEscapeNext = false;
			bLastWasSpace = false;
			continue;
		}
		
		if (Char == TEXT('\\'))
		{
			bEscapeNext = true;
			Result += Char;
			bLastWasSpace = false;
			continue;
		}
		
		if (Char == TEXT('"') && !bInSingleQuotes)
		{
			bInQuotes = !bInQuotes;
			Result += Char;
			bLastWasSpace = false;
			continue;
		}
		
		if (Char == TEXT('\'') && !bInQuotes)
		{
			bInSingleQuotes = !bInSingleQuotes;
			Result += Char;
			bLastWasSpace = false;
			continue;
		}
		
		if (bInQuotes || bInSingleQuotes)
		{
			Result += Char;
			bLastWasSpace = false;
			continue;
		}
		
		if (Char == TEXT(' ') || Char == TEXT('\t') || Char == TEXT('\n') || Char == TEXT('\r'))
		{
			if (!bLastWasSpace && !Result.IsEmpty())
			{
				// Check if we need space around operators
				TCHAR LastChar = Result.Len() > 0 ? Result[Result.Len() - 1] : 0;
				TCHAR NextChar = (i + 1 < Input.Len()) ? Input[i + 1] : 0;
				
				// Add space if not around certain operators
				if (LastChar != TEXT('*') && LastChar != TEXT('&') && 
					NextChar != TEXT('*') && NextChar != TEXT('&') &&
					LastChar != TEXT('<') && NextChar != TEXT('>'))
				{
					Result += TEXT(" ");
				}
				bLastWasSpace = true;
			}
		}
		else
		{
			Result += Char;
			bLastWasSpace = false;
		}
	}
	
	return Result.TrimStartAndEnd();
}

FString QCE_ParameterMatcher::RemoveParameterName(const FString& Parameter)
{
	FString Result = Parameter.TrimStartAndEnd();
	
	if (Result.IsEmpty())
	{
		return Result;
	}
	
	// Work backwards to find the last identifier that represents the parameter name
	int32 LastIdentifierStart = INDEX_NONE;
	int32 LastIdentifierEnd = INDEX_NONE;
	int32 ParenDepth = 0;
	int32 AngleDepth = 0;
	int32 BraceDepth = 0;
	bool bInQuotes = false;
	bool bInSingleQuotes = false;
	bool bFoundIdentifier = false;
	
	// First pass: find the position of the last identifier
	for (int32 i = Result.Len() - 1; i >= 0; i--)
	{
		TCHAR Char = Result[i];
		
		// Handle quotes (working backwards)
		if (Char == TEXT('"') && !bInSingleQuotes)
		{
			bInQuotes = !bInQuotes;
			continue;
		}
		
		if (Char == TEXT('\'') && !bInQuotes)
		{
			bInSingleQuotes = !bInSingleQuotes;
			continue;
		}
		
		if (bInQuotes || bInSingleQuotes)
		{
			continue;
		}
		
		// Track bracket depth (working backwards)
		if (Char == TEXT(')'))
		{
			ParenDepth++;
		}
		else if (Char == TEXT('('))
		{
			ParenDepth--;
		}
		else if (Char == TEXT('>'))
		{
			AngleDepth++;
		}
		else if (Char == TEXT('<'))
		{
			AngleDepth--;
		}
		else if (Char == TEXT('}'))
		{
			BraceDepth++;
		}
		else if (Char == TEXT('{'))
		{
			BraceDepth--;
		}
		
		// Only consider characters outside of brackets
		if (ParenDepth == 0 && AngleDepth == 0 && BraceDepth == 0)
		{
			if (FChar::IsAlnum(Char) || Char == TEXT('_'))
			{
				if (!bFoundIdentifier)
				{
					LastIdentifierEnd = i;
					bFoundIdentifier = true;
				}
				LastIdentifierStart = i;
			}
			else if (bFoundIdentifier && !FChar::IsWhitespace(Char))
			{
				// Hit a non-identifier, non-whitespace character - we've found our identifier
				break;
			}
		}
	}
	
	// If we found an identifier, remove it
	if (bFoundIdentifier && LastIdentifierStart != INDEX_NONE && LastIdentifierEnd != INDEX_NONE)
	{
		// Remove the identifier and any trailing whitespace before it
		FString LeftPart = Result.Left(LastIdentifierStart).TrimStartAndEnd();
		
		// Make sure we're not removing the entire type (e.g., just "int" with no parameter name)
		if (!LeftPart.IsEmpty())
		{
			Result = LeftPart;
		}
	}
	
	return Result.TrimStartAndEnd();
}

bool QCE_ParameterMatcher::IsPositionInStringOrComment(const FString& Content, int32 Position)
{
	bool bInLineComment = false;
	bool bInBlockComment = false;
	bool bInString = false;
	bool bInSingleQuotes = false;
	bool bEscapeNext = false;
	
	for (int32 i = 0; i < Position && i < Content.Len(); i++)
	{
		TCHAR Char = Content[i];
		TCHAR NextChar = (i + 1 < Content.Len()) ? Content[i + 1] : 0;
		
		if (bEscapeNext)
		{
			bEscapeNext = false;
			continue;
		}
		
		if (Char == TEXT('\\') && (bInString || bInSingleQuotes))
		{
			bEscapeNext = true;
			continue;
		}
		
		if (bInLineComment)
		{
			if (Char == TEXT('\n'))
			{
				bInLineComment = false;
			}
			continue;
		}
		
		if (bInBlockComment)
		{
			if (Char == TEXT('*') && NextChar == TEXT('/'))
			{
				bInBlockComment = false;
				i++; // Skip the '/'
			}
			continue;
		}
		
		if (bInString)
		{
			if (Char == TEXT('"'))
			{
				bInString = false;
			}
			continue;
		}
		
		if (bInSingleQuotes)
		{
			if (Char == TEXT('\''))
			{
				bInSingleQuotes = false;
			}
			continue;
		}
		
		// Check for comment starts
		if (Char == TEXT('/') && NextChar == TEXT('/'))
		{
			bInLineComment = true;
			i++; // Skip the second '/'
			continue;
		}
		
		if (Char == TEXT('/') && NextChar == TEXT('*'))
		{
			bInBlockComment = true;
			i++; // Skip the '*'
			continue;
		}
		
		// Check for string starts
		if (Char == TEXT('"'))
		{
			bInString = true;
			continue;
		}
		
		if (Char == TEXT('\''))
		{
			bInSingleQuotes = true;
			continue;
		}
	}
	
	return bInLineComment || bInBlockComment || bInString || bInSingleQuotes;
}

bool QCE_ParameterMatcher::FindCharacterRespectingContext(const FString& Content, const FString& SearchChar, int32 StartPos, ESearchDir::Type Direction, int32& OutPosition)
{
	int32 SearchPos = StartPos;
	int32 Step = (Direction == ESearchDir::FromStart) ? 1 : -1;
	int32 EndPos = (Direction == ESearchDir::FromStart) ? Content.Len() : -1;
	
	while (SearchPos != EndPos)
	{
		int32 FoundPos = Content.Find(SearchChar, ESearchCase::CaseSensitive, Direction, SearchPos);
		if (FoundPos == INDEX_NONE)
			return false;
		
		// Check if this position is in a string or comment
		if (!IsPositionInStringOrComment(Content, FoundPos))
		{
			OutPosition = FoundPos;
			return true;
		}
		
		// Move past this occurrence and continue searching
		SearchPos = FoundPos + Step;
	}
	
	return false;
}

FParameterTypeInfo QCE_ParameterMatcher::ParseParameterTypeInfo(const FString& NormalizedParameter)
{
    FParameterTypeInfo Info;
    FString WorkingStr = NormalizedParameter.TrimStartAndEnd();
    
    if (WorkingStr.IsEmpty())
    {
        return Info;
    }
    
    // Use regex patterns to extract type information
    // Note: UE4/5 uses FRegexPattern and FRegexMatcher for regex operations
    
    // Pattern for const (can appear at beginning or after other qualifiers)
    // Handle both "const Type" and "Type const" patterns
    const FString ConstPattern = TEXT("\\bconst\\b");
    FRegexPattern ConstRegex(ConstPattern);
    FRegexMatcher ConstMatcher(ConstRegex, WorkingStr);
    Info.bIsConst = ConstMatcher.FindNext();

    
    // Pattern for volatile (can appear at beginning or after other qualifiers)
    const FString VolatilePattern = TEXT("\\bvolatile\\b");
    FRegexPattern VolatileRegex(VolatilePattern);
    FRegexMatcher VolatileMatcher(VolatileRegex, WorkingStr);
    Info.bIsVolatile = VolatileMatcher.FindNext();
	
	int32 RefCharPos;
	Info.bIsReference = FindCharacterRespectingContext(WorkingStr, TEXT("&"), 0, ESearchDir::FromStart, RefCharPos);
	
    // Check for rvalue reference (&&) - treat as reference for our purposes
    if (!Info.bIsReference)
    {
        const FString RValueRefPattern = TEXT("&&(?![^<]*>)\\s*$");
        FRegexPattern RValueRefRegex(RValueRefPattern);
        FRegexMatcher RValueRefMatcher(RValueRefRegex, WorkingStr);
        Info.bIsReference = RValueRefMatcher.FindNext();
    }
    
    // Pattern for pointer (* at the end, not inside templates or comments)
    // This regex ensures * is not inside < > brackets and handles multiple *
    const FString PointerPattern = TEXT("\\*+(?![^<]*>)\\s*$");
    FRegexPattern PointerRegex(PointerPattern);
    FRegexMatcher PointerMatcher(PointerRegex, WorkingStr);
    Info.bIsPointer = PointerMatcher.FindNext();
    
    // Extract base type by removing all qualifiers and pointer/reference indicators
    FString BaseTypeStr = WorkingStr;
    
    // Remove const keywords using string replacement (since UE regex doesn't have ReplaceAll)
    BaseTypeStr = BaseTypeStr.Replace(TEXT("const"), TEXT(""));
    
    // Remove volatile keywords
    BaseTypeStr = BaseTypeStr.Replace(TEXT("volatile"), TEXT(""));
	BaseTypeStr = BaseTypeStr.Replace(TEXT("*"), TEXT(""));
	BaseTypeStr = BaseTypeStr.Replace(TEXT("&"), TEXT(""));
	BaseTypeStr = BaseTypeStr.Replace(TEXT("  "), TEXT(" "));
	BaseTypeStr = BaseTypeStr.Replace(TEXT(" "), TEXT(""));
	
    // Remove any trailing/leading whitespace
    Info.BaseType = BaseTypeStr.TrimStartAndEnd();
    
    // Handle edge case where we might have removed everything
    if (Info.BaseType.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ParseParameterTypeInfo: Could not extract base type from '%s'"), *NormalizedParameter);
        Info.BaseType = NormalizedParameter; // Fallback to original
    }
    
    return Info;
}

