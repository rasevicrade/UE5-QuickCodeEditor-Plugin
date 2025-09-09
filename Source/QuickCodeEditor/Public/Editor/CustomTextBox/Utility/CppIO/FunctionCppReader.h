// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QCE_IOTypes.h"
#include "UObject/Class.h"


/**
 * Handles reading and parsing of C++ function declarations and implementations.
 */
class QUICKCODEEDITOR_API FFunctionCppReader
{
public:
	/** Default constructor */
	FFunctionCppReader();

	/** Retrieves C++ function declaration from header file. */
	bool GetFunctionDeclaration(const UFunction* Function, FFunctionDeclarationInfo& OutDeclarationInfo, const bool bShouldRefresh = true);
	
	/** Reads complete C++ function implementation from source file. */
	bool GetFunctionImplementation(const UFunction* Function, FFunctionImplementationInfo& OutImplementationInfo, const bool bShouldRefresh = true);
	
	/** Checks if function declaration in header file has changed compared to provided code. */
	bool HasFunctionDeclarationChangedOnDisk(const UFunction* Function, const FFunctionDeclarationInfo& CurrentDeclarationInfo);

	/** Checks if function implementation in source file has changed compared to provided code. */
	bool HasFunctionImplementationChangedOnDisk(const UFunction* Function, const FFunctionImplementationInfo& CurrentImplementationInfo);
	
private:
	/** Locates function declaration position within file content. */
	bool FindDeclarationPositionInFile(const FString& FileContent, const UFunction* Function, int32& OutFunctionPosition);

	/** Finds function implementation using parsed declaration info. */
	bool ParseImplementation(const UFunction* Function, FFunctionImplementationInfo& OutImplementationInfo, FString& OutCppPath);

	/** Parses declaration into structured info. */
	bool ParseDeclaration(const UFunction* Function, const FString& FileContent, FFunctionDeclarationInfo& OutDeclarationInfo);

	/** Reads C++ file content for given function. */
	bool ReadCppFileContent(const UFunction* Function, FString& FileContent, FString& OutFilePath, QCE_CppFileType FileType);

	/** Filters positions to those matching UFunction parameters. */
	bool FilterPositionsByParamNum(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutParameterMatches);

	/** Filters positions to those matching UFunction parameter types. */
	bool FilterPositionsByMatchingNodeParams(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutTypeMatches);

	/** Instance-specific loaded declaration info for caching */
	FFunctionDeclarationInfo LoadedDeclarationInfo;
	
	/** Instance-specific loaded implementation info for caching */
	FFunctionImplementationInfo LoadedImplementationInfo;
};
