// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "Editor/CustomTextBox/Utility/CppIO/QCE_IOTypes.h"

class SQCE_MultiLineEditableTextBox;
class UMainEditorContainer;

/** Helper class for generating C++ function definitions from declarations */
class QUICKCODEEDITOR_API QCE_GenerateDefinitionHelpers
{
public:
	/** Checks if there's a function declaration at the current cursor position */
	static bool HasDeclarationAtCursor(const TSharedPtr<SQCE_MultiLineEditableTextBox>& TextBox, FString& OutDeclarationString);

	/** Generates function definition and inserts it in the implementation editor */
	static bool TryGenerateAndInsertDefinition(UMainEditorContainer* EditorContainer, FTextLocation& OutInsertLocation);

	/** Generates function definition from header content and cursor location */
	static bool GenerateDefinition(const FString& FunctionName, const FString& HeaderContent, const int32 DeclarationCursorLocation, FString& OutFunctionDefinition);


#pragma region Main methods
private:
	/** Gets implementation text from the editor container */
	static bool GetImplementationText(UMainEditorContainer* EditorContainer, FString& OutImplementationText);
	
	/** Finds the appropriate insertion location for the function definition */
	static bool GetInsertLocation(
	bool bIsIsolated,
		const FString& DeclarationContent,
		FTextLocation InDeclarationCursorLocation,
		const FString& ImplementationContent,
		FTextLocation& OutImplementationInsertLocation);

	/** Generates function definition from declaration string and class name */
	bool GenerateDefinition(const FString& InDeclarationString, const FString& ClassName, FString& OutDefinition);

	/** Inserts the generated function definition at the specified location */
	static bool InsertDefinition(const FString& FunctionDefinition, const FTextLocation& InsertLocation, const TSharedPtr<SQCE_MultiLineEditableTextBox>& ImplementationTextBox);

#pragma endregion
	
public:

#pragma region Get declaration string
private:
	/** Gets function declaration string at cursor position */
	static bool GetDeclarationStringAtCursor(const FString& FileContent, int32 CursorPosition, FString& OutFunctionDeclaration);

	/** Validates if word is a valid function name (not a keyword) */
	static bool IsValidFunctionName(const FString& Word);
	
	/** Finds position closest to target from array of positions */
	static int32 FindClosestPosition(const TArray<int32>& Positions, int32 TargetPosition);
	
	/** Checks if position appears to be a function declaration */
	static bool IsPositionLikelyFunction(const FString& FileContent, int32 Position);
	
	/** Extracts complete function declaration including UFUNCTION macro */
	static bool ExtractCompleteFunctionDeclaration(const FString& FileContent, int32 FunctionNamePosition, const FString& FunctionName, FString& OutFunctionDeclaration);

#pragma endregion

#pragma region Find insert position in implementation
	/** Gets the function that should appear before the insertion point */
	static bool GetFunctionBeforePosition(const FString& DeclarationContent, FTextLocation InDeclarationCursorLocation, FFunctionDeclarationInfo& OutFunctionInfo);

	/** Gets the function that should appear after the insertion point */
	static bool GetFunctionAfterPosition(const FString& DeclarationContent, FTextLocation Position, FFunctionDeclarationInfo& OutFunctionInfo);

private:
	/** Finds proper insertion point by skipping over preceding comment lines */
	static int32 FindInsertionPointSkippingComments(const FString& ImplementationContent, int32 FunctionPosition);
#pragma endregion
};
