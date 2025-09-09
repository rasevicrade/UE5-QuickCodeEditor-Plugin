// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

enum class QCE_CppFileType
{
	None,
	Header,
	Implementation
};

/**
 * Structure to hold parsed function declaration information.
 * This structure contains all the essential components of a C++ function declaration,
 * parsed from source files for analysis and matching purposes.
 */
struct FFunctionDeclarationInfo
{
	/** The name of the function without qualifiers or parameters */
	FString FunctionName;
	
	/** The name of the class that owns this function */
	FString ClassName;
	
	/** The return type of the function, including any modifiers like const, static, etc. */
	FString ReturnType;
	
	/** Array of parameter declarations as they appear in the function signature */
	TArray<FString> Parameters;
	
	/** The complete function declaration as it appears in the source file */
	FString FunctionDeclaration;

	/** The path to the header file containing this declaration */
	FString HeaderPath;
	
	/** Whether the function is declared as const */
	bool bIsConst = false;

	/** CRC32 checksum of the header file content */
	uint32 ContentChecksum = 0;
	
	FString InitialFileContent;

	/** Start position of the function declaration in the file */
	int32 DeclarationStartPosition = -1;

	/** End position of the function declaration in the file */
	int32 DeclarationEndPosition = -1;

	/** Default constructor */
	FFunctionDeclarationInfo() = default;
};

/**
 * Structure to hold parsed function implementation information.
 * This structure contains all the essential components of a C++ function implementation,
 * parsed from source files for analysis and matching purposes.
 */
struct FFunctionImplementationInfo
{
	/** The name of the function without qualifiers or parameters */
	FString FunctionName;
	
	/** Array of parameter declarations as they appear in the function signature */
	TArray<FString> Parameters;
	
	/** The complete function implementation as it appears in the source file */
	FString FunctionImplementation;

	/** The path to the implementation file containing this function */
	FString CppPath;
	
	/** Whether the function is implemented as const */
	bool bIsConst = false;

	/** CRC32 checksum of the implementation file content */
	uint32 ContentChecksum = 0;
	
	FString InitialFileContent;

	/** Start position of the function implementation in the file */
	int32 ImplementationStartPosition = -1;

	/** End position of the function implementation in the file */
	int32 ImplementationEndPosition = -1;

	/** Default constructor */
	FFunctionImplementationInfo() = default;
};

