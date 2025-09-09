// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QCE_IOTypes.h"
#include "UObject/Class.h"

/**
 * Handles writing and updating of C++ function declarations and implementations.
 * Uses robust multi-stage filtering to ensure accurate function replacement.
 */
class QUICKCODEEDITOR_API FFunctionCppWriter
{
public:
	/** Writes function declaration to header file preserving UFUNCTION macros. */
	bool WriteFunctionDeclaration(const FFunctionDeclarationInfo& DeclarationInfo, const FString& UpdatedDeclarationCode, const bool bIsLoadedIsolated, const bool bForceOverwrite = false);
	
	/** Writes function implementation to source file using robust position finding. */
	bool WriteFunctionImplementation(const FFunctionImplementationInfo& ImplementationInfo, const FString& UpdatedImplementationCode, const bool bIsLoadedIsolated, const bool bForceOverwrite = false);
};
