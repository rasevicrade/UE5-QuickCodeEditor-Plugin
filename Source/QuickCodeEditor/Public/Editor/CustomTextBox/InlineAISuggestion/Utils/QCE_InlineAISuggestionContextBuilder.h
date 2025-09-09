// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/Utility/CppIO/QCE_IOTypes.h"

/**
 * This class uses QCE context to figure out minimal
 * context that needs to be sent to be sent to AI
 * to get a useful inline suggestion.
 */
class QCE_InlineAISuggestionContextBuilder
{
public:
	static bool GetAIContext(
		const FString& VisibleCode,
		const FTextLocation& CursorLocation,
		const ETextBoxType& FileType,
		const FUserInputContext& UserContext,
		FString& OutContext
	);

private:
	
	static bool GetDeclarationContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);

	static bool GetImplementationContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);
	
	static bool GetGeneralContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);

	/** Extracts the complete current function implementation including cursor marker */
	static bool GetCurrentFunction(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);

	/** Extracts the current function implementation but only until the cursor location */
	static bool GetFunctionBeforeCursor(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);

	/** Extracts the current line up to cursor position with cursor marker */
	static bool GetCurrentLine(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);

	/** Fallback method that extracts N lines above cursor position when cursor is not within a function */
	static bool GetLinesAboveCursor(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext);
};