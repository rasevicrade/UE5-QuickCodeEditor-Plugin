// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CodeCompletionContext.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

class UMainEditorContainer;

enum class EAccessType : uint8
{
	None,
	StaticAccess,    // UClass::
	PointerAccess,   // MyPointer->
	ReferenceAccess  // MyRef.
};

/**
 * Generic declaration context structure used by completion providers
 * to analyze variable/member access patterns in code.
 */
struct QUICKCODEEDITOR_API FDeclarationContext
{
	EAccessType AccessType = EAccessType::None;
	FString VariableName;
	FString ClassName;
	FString CurrentToken;
};

/**
 * Shared utility class for completion context detection and parsing.
 * Provides common functionality used by multiple completion providers to detect
 * access operators (::, ->, .) and extract context information.
 */
class QUICKCODEEDITOR_API FCompletionContextUtils
{
public:
	/**
	 * Finds the last access operator in the preceding text and returns its details.
	 * @param PrecedingText The text before the cursor position
	 * @param OutPosition Position of the last access operator
	 * @param OutLength Length of the access operator (1 for '.', 2 for '::' and '->')
	 * @param OutAccessType Type of access operator found
	 * @return true if an access operator was found, false otherwise
	 */
	static bool FindLastAccessOperator(const FString& PrecedingText, int32& OutPosition, int32& OutLength, EAccessType& OutAccessType);

	/**
	 * Detects the type of access in the preceding text and validates the context.
	 * @param PrecedingText The text before the cursor position
	 * @return The type of access detected, or EAccessType::None if invalid context
	 */
	static EAccessType DetectAccessType(const FString& PrecedingText);

	/**
	 * Extracts the type/variable name before the access operator.
	 * @param PrecedingText The text before the cursor position
	 * @param AccessType The type of access operator
	 * @return The extracted type/variable name, or empty string if not found
	 */
	static FString ExtractTypeName(const FString& PrecedingText, EAccessType AccessType);

	/**
	 * Extracts the current token being typed after the access operator.
	 * @param PrecedingText The text before the cursor position
	 * @return The partial token after the access operator, or empty string if not found
	 */
	static FString ExtractTokenAfterAccessOperator(const FString& PrecedingText);

	/**
	 * Checks if the context is suitable for member/method completion (not keyword completion).
	 * @param Context The completion context
	 * @return true if the context contains valid access operators for member completion
	 */
	static bool IsValidMemberAccessContext(const FCompletionContext& Context);

	/**
	 * Parses the completion context to extract declaration information including access type,
	 * variable/class names, and current token. This is a generic version suitable for
	 * multiple completion providers.
	 * @param Context The completion context to parse
	 * @return Parsed declaration context with extracted information
	 */
	static FDeclarationContext ParseDeclarationContext(const FCompletionContext& Context);

	/**
	 * Resolves the type of a variable from the completion context by analyzing variable declarations
	 * in both header and implementation text.
	 * @param Context The completion context containing header and implementation text
	 * @param VariableName The name of the variable to resolve the type for
	 * @return The resolved type name, or empty string if not found
	 */
	static FString ResolveTypeFromContext(const FCompletionContext& Context, const FString& VariableName);

	
	static UStruct* GetTypeByClassName(const FString& TypeName);

	/**
	 * Filters an array of completion items based on a filter string.
	 * This method modifies the array in-place, removing non-matching items and updating scores.
	 * @param InOutCompletions Array of completion items to filter (modified in place)
	 * @param Filter The filter string to match against
	 */
	static void FilterCompletionItems(TArray<FCompletionItem>& InOutCompletions, const FString& Filter);

	/**
	 * Checks if a completion name matches a filter string using both prefix and subsequence matching.
	 * @param Name The completion name to check
	 * @param Filter The filter string to match against
	 * @return true if the name matches the filter, false otherwise
	 */
	static bool MatchesCompletionFilter(const FString& Name, const FString& Filter);

	/**
	 * Checks if a filter is a subsequence of a name (fuzzy matching).
	 * For example, "tef" matches "TestFunction" because t-e-f appear in order.
	 * @param Name The name to check
	 * @param Filter The filter subsequence
	 * @return true if filter is a subsequence of name, false otherwise
	 */
	static bool IsSubsequenceMatch(const FString& Name, const FString& Filter);

	/**
	 * Builds completion context from code and cursor position.
	 * @param Code The source code text
	 * @param CursorPosition The position of the cursor in the code
	 * @param HeaderText Optional header text for context
	 * @param ImplementationText Optional implementation text for context
	 * @param MainEditorContainer Optional editor container for additional context
	 * @return The built completion context
	 */
	static FCompletionContext BuildContext(const FString& Code, int32 CursorPosition, const FString& HeaderText = FString(), const FString& ImplementationText = FString(), UMainEditorContainer* MainEditorContainer = nullptr);

	/**
	 * Extracts the current token being typed at cursor position.
	 * @param PrecedingText The text before the cursor position
	 * @return The current token being typed, or empty string if none found
	 */
	static FString ExtractCurrentToken(const FString& PrecedingText);
private:
	// Helper functions for type resolution
	static bool FindVariableDeclaration(const FString& HeaderText, const FString& ImplementationText, const FString& VariableName, FString& OutDeclaration, int32& OutPosition);
	static bool FindVariableInText(const FString& Text, const FString& VariableName, const TArray<FString>& SearchPatterns, FString& OutDeclaration, int32& OutPosition);
	static bool IsValidVariableDeclaration(const FString& LineContent, const FString& VariableName);
	static FString ParseVariableType(const FString& Declaration);
	static FString ParseAutoType(const FString& Declaration);
	static FString ParseTemplateType(const FString& Declaration);
	static FString ParseSimpleType(const FString& Declaration);
};