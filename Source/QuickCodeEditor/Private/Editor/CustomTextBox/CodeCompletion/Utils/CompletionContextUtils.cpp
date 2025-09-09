// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Engine/Engine.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Internationalization/Regex.h"

bool FCompletionContextUtils::FindLastAccessOperator(const FString& PrecedingText, int32& OutPosition, int32& OutLength, EAccessType& OutAccessType)
{
	if (PrecedingText.IsEmpty())
	{
		return false;
	}
	
	// Find the last occurrence of each access operator
	int32 LastStaticAccess = PrecedingText.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	int32 LastPointerAccess = PrecedingText.Find(TEXT("->"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	int32 LastReferenceAccess = PrecedingText.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	
	// Find the most recent access operator
	int32 LastAccessPos = -1;
	EAccessType AccessType = EAccessType::None;
	int32 OperatorLength = 0;
	
	if (LastStaticAccess != INDEX_NONE && LastStaticAccess > LastAccessPos)
	{
		LastAccessPos = LastStaticAccess;
		AccessType = EAccessType::StaticAccess;
		OperatorLength = 2;
	}
	
	if (LastPointerAccess != INDEX_NONE && LastPointerAccess > LastAccessPos)
	{
		LastAccessPos = LastPointerAccess;
		AccessType = EAccessType::PointerAccess;
		OperatorLength = 2;
	}
	
	if (LastReferenceAccess != INDEX_NONE && LastReferenceAccess > LastAccessPos)
	{
		LastAccessPos = LastReferenceAccess;
		AccessType = EAccessType::ReferenceAccess;
		OperatorLength = 1;
	}
	
	// If no access operator found, return false
	if (LastAccessPos == INDEX_NONE)
	{
		return false;
	}
	
	// Set output parameters
	OutPosition = LastAccessPos;
	OutLength = OperatorLength;
	OutAccessType = AccessType;
	
	return true;
}

EAccessType FCompletionContextUtils::DetectAccessType(const FString& PrecedingText)
{
	int32 OperatorPos, OperatorLength;
	EAccessType AccessType;
	
	if (!FindLastAccessOperator(PrecedingText, OperatorPos, OperatorLength, AccessType))
	{
		return EAccessType::None;
	}
	
	// Validate that there are no breaking characters after the access operator
	int32 TextAfterOperatorStart = OperatorPos + OperatorLength;
	if (TextAfterOperatorStart < PrecedingText.Len())
	{
		FString TextAfterOperator = PrecedingText.Mid(TextAfterOperatorStart);
		
		// Check each character after the operator
		for (int32 i = 0; i < TextAfterOperator.Len(); ++i)
		{
			TCHAR Ch = TextAfterOperator[i];
			
			// Only allow alphanumeric characters and underscores after access operator
			if (!FChar::IsAlnum(Ch) && Ch != TEXT('_'))
			{
				return EAccessType::None;
			}
		}
	}
	
	return AccessType;
}

FString FCompletionContextUtils::ExtractTypeName(const FString& PrecedingText, EAccessType AccessType)
{
	if (PrecedingText.IsEmpty() || AccessType == EAccessType::None)
	{
		return FString();
	}
	
	int32 OperatorPos, OperatorLength;
	EAccessType FoundAccessType;
	
	if (!FindLastAccessOperator(PrecedingText, OperatorPos, OperatorLength, FoundAccessType))
	{
		return FString();
	}
	
	// Verify that the found access type matches the requested one
	if (FoundAccessType != AccessType)
	{
		return FString();
	}
	
	// Extract text before the operator
	FString TextBeforeOperator = PrecedingText.Left(OperatorPos);
	
	// Find the start of the type name by going backward until we hit a non-identifier character
	int32 TypeStart = TextBeforeOperator.Len();
	for (int32 i = TextBeforeOperator.Len() - 1; i >= 0; --i)
	{
		TCHAR Ch = TextBeforeOperator[i];
		
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
		{
			TypeStart = i;
		}
		else
		{
			break;
		}
	}
	
	if (TypeStart < TextBeforeOperator.Len())
	{
		return TextBeforeOperator.Mid(TypeStart);
	}
	
	return FString();
}

FString FCompletionContextUtils::ExtractTokenAfterAccessOperator(const FString& PrecedingText)
{
	int32 OperatorPos, OperatorLength;
	EAccessType AccessType;
	
	if (!FindLastAccessOperator(PrecedingText, OperatorPos, OperatorLength, AccessType))
	{
		return FString();
	}
	
	// Extract text after the access operator as the current token
	int32 TokenStart = OperatorPos + OperatorLength;
	if (TokenStart < PrecedingText.Len())
	{
		return PrecedingText.Mid(TokenStart);
	}
	
	return FString();
}

bool FCompletionContextUtils::IsValidMemberAccessContext(const FCompletionContext& Context)
{
	return DetectAccessType(Context.PrecedingText) != EAccessType::None;
}

FDeclarationContext FCompletionContextUtils::ParseDeclarationContext(const FCompletionContext& Context)
{
	FDeclarationContext DeclarationCtx;
	
	DeclarationCtx.AccessType = DetectAccessType(Context.PrecedingText);
	if (DeclarationCtx.AccessType != EAccessType::None)
	{
		DeclarationCtx.VariableName = ExtractTypeName(Context.PrecedingText, DeclarationCtx.AccessType);
		DeclarationCtx.CurrentToken = ExtractTokenAfterAccessOperator(Context.PrecedingText);
		DeclarationCtx.ClassName = ResolveTypeFromContext(Context, DeclarationCtx.VariableName);
	}
	
	return DeclarationCtx;
}

FString FCompletionContextUtils::ResolveTypeFromContext(const FCompletionContext& Context, const FString& VariableName)
{
	if (VariableName.IsEmpty())
	{
		return "";
	}
	
	// Check if this is a static access - if so, the VariableName is actually the class name
	EAccessType AccessType = DetectAccessType(Context.PrecedingText);
	if (AccessType == EAccessType::StaticAccess)
	{
		// For static access (e.g., UClass::StaticFunction), the VariableName is the class name
		return VariableName;
	}
	
	FString Declaration;
	int32 Position = -1;
	
	if (FindVariableDeclaration(Context.HeaderText, Context.ImplementationText, VariableName, Declaration, Position))
	{
		return ParseVariableType(Declaration);
	}
	
	return FString();
}

bool FCompletionContextUtils::FindVariableDeclaration(const FString& HeaderText, const FString& ImplementationText, const FString& VariableName, FString& OutDeclaration, int32& OutPosition)
{
	// Search patterns for different declaration types
	TArray<FString> SearchPatterns = {
		VariableName + TEXT(" ="),        // Direct assignment: MyVar = 
		VariableName + TEXT(";"),         // Direct declaration: MyVar;
		VariableName + TEXT("("),         // Constructor: MyVar(
		VariableName + TEXT("->"),
		TEXT("* ") + VariableName,        // Pointer: * MyVar
		TEXT("& ") + VariableName,        // Reference: & MyVar
		TEXT(" ") + VariableName  // Spaced: Type MyVar Type
	};
	
	// Try to find in implementation first (more likely to have local variables)
	if (FindVariableInText(ImplementationText, VariableName, SearchPatterns, OutDeclaration, OutPosition))
	{
		return true;
	}
	
	// Fall back to header text (member variables)
	if (FindVariableInText(HeaderText, VariableName, SearchPatterns, OutDeclaration, OutPosition))
	{
		return true;
	}
	
	return false;
}

bool FCompletionContextUtils::FindVariableInText(const FString& Text, const FString& VariableName, const TArray<FString>& SearchPatterns, FString& OutDeclaration, int32& OutPosition)
{
	if (Text.IsEmpty() || VariableName.IsEmpty())
	{
		return false;
	}
	
	// Collect all possible positions for this variable name
	TArray<int32> PossiblePositions;
	
	for (const FString& Pattern : SearchPatterns)
	{
		int32 SearchPos = 0;
		int32 FoundPos = -1;
		
		while ((FoundPos = Text.Find(Pattern, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchPos)) != INDEX_NONE)
		{
			PossiblePositions.AddUnique(FoundPos);
			SearchPos = FoundPos + 1;
		}
	}
	
	// Filter out positions in comments
	TArray<int32> ValidPositions;
	for (int32 Position : PossiblePositions)
	{
		if (!QCE_CommonIOHelpers::IsPositionInComment(Text, Position))
		{
			ValidPositions.Add(Position);
		}
	}
	
	// Find the best match - look for complete variable declarations
	for (int32 Position : ValidPositions)
	{
		// Extract the line containing this position
		int32 LineStart = Position;
		while (LineStart > 0 && Text[LineStart - 1] != TEXT('\n'))
		{
			LineStart--;
		}
		
		int32 LineEnd = Position;
		while (LineEnd < Text.Len() && Text[LineEnd] != TEXT('\n'))
		{
			LineEnd++;
		}
		
		FString LineContent = Text.Mid(LineStart, LineEnd - LineStart).TrimStartAndEnd();
		
		// Check if this looks like a valid variable declaration
		if (IsValidVariableDeclaration(LineContent, VariableName))
		{
			OutDeclaration = LineContent;
			OutPosition = Position;
			return true;
		}
	}
	
	return false;
}

bool FCompletionContextUtils::IsValidVariableDeclaration(const FString& LineContent, const FString& VariableName)
{
	// Skip obvious non-declarations
	if (LineContent.StartsWith(TEXT("//")))
	{
		return false;
	}
	
	if (LineContent.StartsWith(TEXT("*")))
	{
		return false;
	}
	
	// Must contain the variable name
	if (!LineContent.Contains(VariableName))
	{
		return false;
	}
	
	// Should have assignment or semicolon for proper declaration
	bool bHasDeclarationEnding = LineContent.Contains(TEXT("=")) ||
								 LineContent.Contains(TEXT(";")) ||
								 LineContent.Contains(TEXT("("));
	
	if (!bHasDeclarationEnding)
	{
		return false;
	}
	
	// Check for simple type declaration pattern: TypeName VariableName;
	// Pattern matches: [optional const] TypeName [optional*&] VariableName [;=(]
	FString RegexPattern = FString::Printf(TEXT("\\b(?:const\\s+)?([A-Za-z_][A-Za-z0-9_]*)\\s*[*&]*\\s+%s\\s*[;=(]"), *VariableName);
	FRegexPattern Pattern(RegexPattern);
	FRegexMatcher Matcher(Pattern, LineContent);
	
	if (Matcher.FindNext())
	{
		return true;
	}
	
	// Look for typical declaration patterns (existing logic as fallback)
	bool bHasTypeIndicators = LineContent.Contains(TEXT("*")) || 
							  LineContent.Contains(TEXT("&")) ||
							  LineContent.Contains(TEXT("TObjectPtr")) ||
							  LineContent.Contains(TEXT("TSharedPtr")) ||
							  LineContent.Contains(TEXT("TWeakPtr")) ||
							  LineContent.Contains(TEXT("auto"));
	
	return bHasTypeIndicators;
}

FString FCompletionContextUtils::ParseVariableType(const FString& Declaration)
{
	if (Declaration.IsEmpty())
	{
		return FString();
	}
	
	FString CleanDeclaration = Declaration.TrimStartAndEnd();
	
	// Handle auto declarations - try to extract type from assignment
	if (CleanDeclaration.Contains(TEXT("auto")))
	{
		return ParseAutoType(CleanDeclaration);
	}
	
	// Handle template types (TObjectPtr, TSharedPtr, etc.)
	if (CleanDeclaration.Contains(TEXT("TObjectPtr<")) ||
		CleanDeclaration.Contains(TEXT("TSharedPtr<")) ||
		CleanDeclaration.Contains(TEXT("TWeakPtr<")))
	{
		return ParseTemplateType(CleanDeclaration);
	}
	
	// Handle simple pointer/reference types
	return ParseSimpleType(CleanDeclaration);
}

FString FCompletionContextUtils::ParseAutoType(const FString& Declaration)
{
	// Look for patterns like: auto* MyVar = GetComponent<UStaticMeshComponent>()
	int32 AngleBracketStart = Declaration.Find(TEXT("<"));
	int32 AngleBracketEnd = Declaration.Find(TEXT(">"));
	
	if (AngleBracketStart != INDEX_NONE && AngleBracketEnd != INDEX_NONE && AngleBracketEnd > AngleBracketStart)
	{
		FString TemplateType = Declaration.Mid(AngleBracketStart + 1, AngleBracketEnd - AngleBracketStart - 1);
		return TemplateType.TrimStartAndEnd();
	}
	
	return FString();
}

FString FCompletionContextUtils::ParseTemplateType(const FString& Declaration)
{
	// Extract type from TObjectPtr<UStaticMeshComponent>, TSharedPtr<AActor>, etc.
	int32 AngleBracketStart = Declaration.Find(TEXT("<"));
	int32 AngleBracketEnd = Declaration.Find(TEXT(">"));
	
	if (AngleBracketStart != INDEX_NONE && AngleBracketEnd != INDEX_NONE && AngleBracketEnd > AngleBracketStart)
	{
		FString TemplateType = Declaration.Mid(AngleBracketStart + 1, AngleBracketEnd - AngleBracketStart - 1);
		return TemplateType.TrimStartAndEnd();
	}
	
	return FString();
}

FString FCompletionContextUtils::ParseSimpleType(const FString& Declaration)
{
	// Parse patterns like: AActor* MyActor = nullptr;
	TArray<FString> Words;
	Declaration.ParseIntoArray(Words, TEXT(" "), true);
	
	if (Words.Num() < 2)
	{
		return FString();
	}
	
	// Look for the type before the variable name
	for (int32 i = 0; i < Words.Num() - 1; ++i)
	{
		FString Word = Words[i].TrimStartAndEnd();
		
		// Skip common C++ keywords that aren't types
		if (Word == TEXT("const") || Word == TEXT("static") || Word == TEXT("mutable") || 
			Word == TEXT("UPROPERTY()") || Word.StartsWith(TEXT("UPROPERTY")))
		{
			continue;
		}
		
		// If this word ends with * or &, extract the base type
		if (Word.EndsWith(TEXT("*")) || Word.EndsWith(TEXT("&")))
		{
			return Word.LeftChop(1).TrimStartAndEnd();
		}
		
		// Check if next word starts with variable name patterns
		if (i + 1 < Words.Num())
		{
			FString NextWord = Words[i + 1];
			if (NextWord.Contains(TEXT("=")) || NextWord.Contains(TEXT(";")))
			{
				return Word;
			}
		}
	}
	
	return FString();
}

UStruct* FCompletionContextUtils::GetTypeByClassName(const FString& TypeName)
{
	if (TypeName.IsEmpty())
	{
		return nullptr;
	}
	
	// Clean up the type name - remove common prefixes/suffixes that might interfere
	FString CleanTypeName = TypeName.TrimStartAndEnd();
	
	// Remove const qualifier
	CleanTypeName = CleanTypeName.Replace(TEXT("const "), TEXT(""));
	CleanTypeName = CleanTypeName.Replace(TEXT(" const"), TEXT(""));

	// Try systematic resolution with different approaches
	UStruct* Result = nullptr;
	
	// 1. Try exact name first (handles cases like "UStaticMeshComponent" directly)
	Result = FindObject<UClass>(nullptr, *CleanTypeName, true);
	if (Result)
	{
		return Result;
	}

	Result = FindObject<UStruct>(nullptr, *CleanTypeName, true);
	if (Result)
	{
		return Result;
	}
	
	// 2. Try with common package paths for core engine classes
	TArray<FString> CommonPackagePaths = {
		TEXT("/Script/Engine."),
		TEXT("/Script/CoreUObject."),
		TEXT("/Script/UMG."),
		TEXT("/Engine/Transient.")
	};
	
	for (const FString& PackagePath : CommonPackagePaths)
	{
		FString FullPath = PackagePath + CleanTypeName;
		Result = FindObject<UClass>(nullptr, *FullPath, true);
		if (Result)
		{
			return Result;
		}
		
		Result = FindObject<UStruct>(nullptr, *FullPath, true);
		if (Result)
		{
			return Result;
		}
	}
	
	// 3. Try FindFirstObject for broader search if exact matching failed
	Result = FindFirstObject<UClass>(*CleanTypeName, EFindFirstObjectOptions::ExactClass);
	if (Result)
	{
		return Result;
	}
	
	Result = FindFirstObject<UStruct>(*CleanTypeName, EFindFirstObjectOptions::ExactClass);
	if (Result)
	{
		return Result;
	}
	
	// 4. Try systematic prefix addition for names without prefixes
	if (!CleanTypeName.StartsWith(TEXT("U")) && !CleanTypeName.StartsWith(TEXT("A")) && !CleanTypeName.StartsWith(TEXT("F")))
	{
		// Try U prefix (most common for components and objects)
		FString UTypeName = TEXT("U") + CleanTypeName;
		Result = FindObject<UClass>(nullptr, *UTypeName, true);
		if (Result)
		{
			return Result;
		}
		
		// Try with FindFirstObject for U prefix
		Result = FindFirstObject<UClass>(*UTypeName, EFindFirstObjectOptions::ExactClass);
		if (Result)
		{
			return Result;
		}
		
		// Try A prefix for actors
		FString ATypeName = TEXT("A") + CleanTypeName;
		Result = FindObject<UClass>(nullptr, *ATypeName, true);
		if (Result)
		{
			return Result;
		}
		
		Result = FindFirstObject<UClass>(*ATypeName, EFindFirstObjectOptions::ExactClass);
		if (Result)
		{
			return Result;
		}
		
		// Try F prefix for structs
		FString FTypeName = TEXT("F") + CleanTypeName;
		Result = FindObject<UStruct>(nullptr, *FTypeName, true);
		if (Result)
		{
			return Result;
		}
		
		Result = FindFirstObject<UStruct>(*FTypeName, EFindFirstObjectOptions::ExactClass);
		if (Result)
		{
			return Result;
		}
	}
	
	// 5. Try removing prefixes if they exist (handles cases where prefix was added incorrectly)
	if (CleanTypeName.Len() > 1)
	{
		TCHAR FirstChar = CleanTypeName[0];
		if (FirstChar == TEXT('U') || FirstChar == TEXT('A') || FirstChar == TEXT('F'))
		{
			FString WithoutPrefix = CleanTypeName.Mid(1);
			
			// Try all prefixes on the name without prefix
			TArray<FString> PrefixesToTry = { TEXT("U"), TEXT("A"), TEXT("F"), TEXT("") };
			
			for (const FString& Prefix : PrefixesToTry)
			{
				FString TestName = Prefix + WithoutPrefix;
				
				Result = FindFirstObject<UClass>(*TestName, EFindFirstObjectOptions::ExactClass);
				if (Result)
				{
					return Result;
				}
				
				Result = FindFirstObject<UStruct>(*TestName, EFindFirstObjectOptions::ExactClass);
				if (Result)
				{
					return Result;
				}
			}
		}
	}
	
	return nullptr;
}

void FCompletionContextUtils::FilterCompletionItems(TArray<FCompletionItem>& InOutCompletions, const FString& Filter)
{
	if (Filter.IsEmpty())
	{
		// No filtering needed if no filter
		return;
	}

	// Filter to only include items that match the filter
	InOutCompletions.RemoveAll([&Filter](const FCompletionItem& Item)
	{
		return !MatchesCompletionFilter(Item.InsertText, Filter);
	});

	// Update scores based on how well they match the filter
	for (FCompletionItem& Item : InOutCompletions)
	{
		// Exact prefix match gets highest score bonus
		if (Item.InsertText.StartsWith(Filter, ESearchCase::CaseSensitive))
		{
			Item.Score += 20;
		}
		// Case-insensitive prefix match gets medium bonus
		else if (Item.InsertText.StartsWith(Filter, ESearchCase::IgnoreCase))
		{
			Item.Score += 10;
		}
		// Subsequence match gets small bonus
		else if (IsSubsequenceMatch(Item.InsertText, Filter))
		{
			Item.Score += 5;
		}
	}

	// Sort by score (highest first)
	InOutCompletions.Sort([](const FCompletionItem& A, const FCompletionItem& B)
	{
		return A.Score > B.Score;
	});
}

bool FCompletionContextUtils::MatchesCompletionFilter(const FString& Name, const FString& Filter)
{
	if (Filter.IsEmpty())
	{
		return true;
	}
	
	// First try exact prefix matching (fastest and most common case)
	if (Name.StartsWith(Filter, ESearchCase::IgnoreCase))
	{
		return true;
	}
	
	// Try subsequence matching for partial matches like "te1" matching "TestFunction1"
	return IsSubsequenceMatch(Name, Filter);
}

bool FCompletionContextUtils::IsSubsequenceMatch(const FString& Name, const FString& Filter)
{
	if (Filter.IsEmpty())
	{
		return true;
	}
	
	if (Name.IsEmpty())
	{
		return false;
	}
	
	int32 NameIndex = 0;
	int32 FilterIndex = 0;
	
	// Convert to lowercase for case-insensitive matching
	FString LowerName = Name.ToLower();
	FString LowerFilter = Filter.ToLower();
	
	// Check if all characters in Filter appear in Name in the same order
	while (NameIndex < LowerName.Len() && FilterIndex < LowerFilter.Len())
	{
		if (LowerName[NameIndex] == LowerFilter[FilterIndex])
		{
			FilterIndex++;
		}
		NameIndex++;
	}
	
	// All filter characters were found in sequence
	return FilterIndex == LowerFilter.Len();
}

FCompletionContext FCompletionContextUtils::BuildContext(const FString& Code, int32 CursorPosition, const FString& HeaderText, const FString& ImplementationText, UMainEditorContainer* MainEditorContainer)
{
	FCompletionContext Context;
    
    CursorPosition = FMath::Clamp(CursorPosition, 0, Code.Len());
    Context.PrecedingText = Code.Left(CursorPosition);
    Context.CurrentToken = ExtractCurrentToken(Context.PrecedingText);
    Context.HeaderText = HeaderText;
    Context.ImplementationText = ImplementationText;
    Context.MainEditorContainer = MainEditorContainer;
    
    return Context;
}

FString FCompletionContextUtils::ExtractCurrentToken(const FString& PrecedingText)
{
    if (PrecedingText.IsEmpty())
        return FString();
    
    // Find the end of the current token (where the cursor is)
    int32 TokenEnd = PrecedingText.Len();
    
    // Look backwards from cursor to find start of current token
    int32 TokenStart = TokenEnd;
    for (int32 i = TokenEnd - 1; i >= 0; --i)
    {
        TCHAR Ch = PrecedingText[i];
        
        // Token characters: alphanumeric, underscore
        if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
        {
            TokenStart = i;
        }
        else
        {
            break;
        }
    }
    
    // Extract the token
    if (TokenStart < TokenEnd)
    {
        return PrecedingText.Mid(TokenStart, TokenEnd - TokenStart);
    }
    
    return FString();
}