// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "Editor/CustomTextBox/Utility/CppIO/QCE_IOTypes.h"

/**
 * Utility class for finding appropriate insertion locations for function implementations in C++ files.
 */
class QUICKCODEEDITOR_API QCE_ImplementationLocationFinder
{
public:
	/**
	 * Finds the optimal insertion location for a function implementation between two existing functions.
	 */
	static int32 FindInsertionLocation(const FString& CppFileContent, const FFunctionDeclarationInfo& PreviousFunction, const FFunctionDeclarationInfo& NextFunction);

private:
	/** Converts an absolute character position to line and character position within that line. */
	static bool ConvertToLineCharPosition(const FString& FileContent, int32 CharPosition, int32& OutLineNumber, int32& OutCharacterPosition);

	/** Finds the end position of a function implementation by locating matching braces. */
	static int32 FindFunctionEnd(const FString& FileContent, int32 FunctionStartPos);
};
