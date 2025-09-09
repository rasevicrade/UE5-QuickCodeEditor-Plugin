// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "Editor/CustomTextBox/Utility/CppIO/QCE_IOTypes.h"

// Constants
#ifndef MAX_SPRINTF
#define MAX_SPRINTF 1024
#endif

// C++ Export Flags for property declarations
enum EQCEPropertyExportCPPFlags
{
	/** Indicates that there are no special C++ export flags */
	QCECPPF_None						=	0x00000000,
	/** Indicates that we are exporting this property's CPP text for an optional parameter value */
	QCECPPF_OptionalValue				=	0x00000001,
	/** Indicates that we are exporting this property's CPP text for an argument or return value */
	QCECPPF_ArgumentOrReturnValue		=	0x00000002,
	/** Indicates thet we are exporting this property's CPP text for C++ definition of a function. */
	QCECPPF_Implementation				=	0x00000004,
	/** Indicates thet we are exporting this property's CPP text with an custom type name */
	QCECPPF_CustomTypeName				=	0x00000008,
	/** No 'const' keyword */
	QCECPPF_NoConst						=	0x00000010,
	/** No reference '&' sign */
	QCECPPF_NoRef						=	0x00000020,
	/** No static array [%d] */
	QCECPPF_NoStaticArray				=	0x00000040,
	/** Blueprint compiler generated C++ code */
	QCECPPF_BlueprintCppBackend			=	0x00000080,
};

// Types of exported declarations
namespace EQCEExportedDeclaration
{
	enum Type
	{
		Local,
		Member,
		Parameter,
		/** Type and name are separated by comma */
		MacroParameter, 
	};
}

class QCE_CommonIOHelpers
{
public:
	/**
	 * Finds all positions in file content where a specific string appears
	 * @param FileContent The content to search in
	 * @param SearchString The string to search for
	 * @param OutPositions Array to store found positions
	 * @return True if any positions were found
	 */
	static bool FilterPositionsByName(const FString& FileContent, const FString& SearchString, TArray<int32>& OutPositions);

	/**
	 * Filters out positions that are within comments
	 * @param FileContent The content to check
	 * @param PossibleMatchPositions Input positions to filter
	 * @param OutNonCommentMatches Output positions that are not in comments
	 * @return True if any non-comment positions were found
	 */
	static bool FilterCommentedPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, TArray<int32>& OutNonCommentMatches);

	/**
	 * Filters positions to only include those that have UFUNCTION macros
	 * @param FileContent The content to check
	 * @param PossibleMatchPositions Input positions to filter
	 * @param OutUFunctionMatches Output positions that have UFUNCTION macros
	 * @return True if any UFUNCTION positions were found
	 */
	static bool FilterNativeFunctionPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, TArray<int32>& OutUFunctionMatches);

	/**
	 * Filters positions to only include those that have class scope (ClassName::FunctionName)
	 * @param FileContent The content to check
	 * @param PossibleMatchPositions Input positions to filter
	 * @param ClassName The class name to look for in scope
	 * @param OutScopedMatches Output positions that have correct scope
	 * @return True if any scoped positions were found
	 */
	static bool FilterScopedFunctionPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const FString& ClassName, TArray<int32>& OutScopedMatches);

	/**
	 * Checks if a position in the file content is within a comment
	 * @param FileContent The content to check
	 * @param Position The position to check
	 * @return True if the position is within a comment
	 */
	static bool IsPositionInComment(const FString& FileContent, int32 Position);

	/**
	 * Checks if a function has a UFUNCTION macro above it with boundary detection
	 * Searches backwards from the function position but stops at statement terminators
	 * (semicolons, closing braces) to prevent matching UFUNCTIONs from other functions.
	 * Limited to searching maximum 20 lines backwards for performance.
	 * @param FileContent The content to check
	 * @param FunctionNamePos Position of the function name
	 * @param OutUFunctionString Output string containing the UFUNCTION macro
	 * @param OutMacroStartPos Output position where the macro starts
	 * @return True if UFUNCTION macro was found within the same function scope, false if terminators encountered first
	 */
	static bool FunctionHasUFunction(const FString& FileContent, int32 FunctionNamePos, FString& OutUFunctionString, int32& OutMacroStartPos);

	/**
	 * Validates that a function has the required flags for C++ operations
	 * @param Function The function to validate
	 * @return True if the function has valid flags
	 */
	static bool ValidateFunctionFlags(const UFunction* Function);

	/**
	 * Reads file content from a file path
	 * @param FilePath Path to the file to read
	 * @param OutContent Output content of the file
	 * @return True if file was read successfully
	 */
	static bool ReadFileContent(const FString& FilePath, FString& OutContent);

	/**
	 * Writes content to a file path
	 * @param FilePath Path to the file to write
	 * @param Content Content to write to the file
	 * @return True if file was written successfully
	 */
	static bool WriteFileContent(const FString& FilePath, const FString& Content);

	/**
	 * Gets expected parameter signature and pass-by-ref info from a UFunction
	 * @param Function The function to get parameters from
	 * @return Array of pairs containing parameter strings and their pass-by-ref status
	 */
	static TArray<TPair<FString, bool>> GetExpectedParameterSignature(const UFunction* Function);

	/**
	 * Checks if parameter signature at a position matches the expected UFunction signature
	 * @param FileContent The content to check
	 * @param Position The position to check
	 * @param Function The function to compare against
	 * @return True if parameter signatures match
	 */
	static bool DoesParameterSignatureMatch(const FString& FileContent, int32 Position, const UFunction* Function);

	/**
	 * Filters positions to only include those that match the function's parameter signature
	 * @param FileContent The content to check
	 * @param PossibleMatchPositions Input positions to filter
	 * @param Function The function to compare against
	 * @param OutSignatureMatches Output positions that match the signature
	 * @return True if any signature matches were found
	 */
	static bool FilterPositionsByParameterSignature(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutSignatureMatches);

	/** Finds the start position of a function header by searching backwards from the function name position */
	static bool FindFunctionImplementationHeaderStart(const FString& FileContent, int32 FunctionNamePos, int32& OutHeaderStartPos);

	/**
	 * Parses a function declaration at a given position into a structured FFunctionDeclarationInfo
	 * @param FileContent The content to parse
	 * @param FunctionPosition Position of the function name in the file content
	 * @param FunctionName The name of the function to parse
	 * @param OutDeclarationInfo Output struct to populate with parsed information
	 * @return True if parsing was successful, false otherwise
	 */
	static bool ParseFunctionDeclarationAtPosition(const FString& FileContent, int32 FunctionPosition, const FString& FunctionName,  FFunctionDeclarationInfo& OutDeclarationInfo, bool bRequiresUfunction = false);

	/**
	 * Extracts the class name from a C++ declaration file content
	 * @param DeclarationFileContent The content of the header file to parse
	 * @return The extracted class name, or empty string if not found
	 */
	static FString ExtractClassNameFromDeclarationFile(const FString& DeclarationFileContent);

	/**
	 * Finds implementation position in file using declaration info
	 * @param FileContent The content to search in
	 * @param DeclarationInfo Declaration info containing function details
	 * @param ClassName The class name to look for in scope
	 * @param OutFunctionPosition Output position where the function was found
	 * @return True if function implementation position was found
	 */
	static bool FindImplementationPositionInContent(const FString& FileContent, const FFunctionDeclarationInfo& DeclarationInfo, const FString& ClassName, int32& OutFunctionPosition);

	/**
	 * Checks if function has const modifier
	 * @param FunctionDeclaration The function declaration to check
	 * @return True if the function has const modifier
	 */
	static bool HasConstModifier(const FString& FunctionDeclaration);

	/**
	 * Validates that implementation signature matches declaration
	 * @param FileContent The content to check
	 * @param FunctionNamePos Position of the function name
	 * @param DeclarationInfo Declaration info to compare against
	 * @return True if implementation signature matches declaration
	 */
	static bool DoesImplementationSignatureMatchDeclaration(const FString& FileContent, int32 FunctionNamePos, const FFunctionDeclarationInfo& DeclarationInfo);

	/**
	 * Filters positions to those matching declaration signature
	 * @param FileContent The content to check
	 * @param PossibleMatchPositions Input positions to filter
	 * @param DeclarationInfo Declaration info to match against
	 * @param OutSignatureMatches Output positions that match the signature
	 * @return True if any signature matches were found
	 */
	static bool FilterPositionsBySignatureMatch(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const FFunctionDeclarationInfo& DeclarationInfo, TArray<int32>& OutSignatureMatches);

	/**
	 * Extracts the return type of a function from header content at the given cursor location
	 * @param HeaderContent The header file content to parse
	 * @param DeclarationCursorLocation Position of the function name in the header
	 * @return The extracted return type, or empty string if not found
	 */
	static FString ExtractReturnType(const FString& HeaderContent, int32 DeclarationCursorLocation);

	/**
	 * Gets the word at a specific position in file content
	 * @param FileContent The content to search in
	 * @param Position The position to get the word from
	 * @return The word at the position, or empty string if not found
	 */
	static FString GetWordAtPosition(const FString& FileContent, int32 Position);

	/**
	 * Checks if a character is a valid word character (alphanumeric or underscore)
	 * @param Char The character to check
	 * @return True if the character is a word character
	 */
	static bool IsWordCharacter(TCHAR Char);

	/**
	 * Converts FTextLocation to linear position in file content
	 * @param FileContent The content to search in
	 * @param TextLocation The text location to convert
	 * @return The linear position in the file content, or INDEX_NONE if invalid
	 */
	static int32 ConvertTextLocationToPosition(const FString& FileContent, const FTextLocation& TextLocation);

	/**
	 * Converts linear position to FTextLocation in file content
	 * @param FileContent The content to search in
	 * @param Position The linear position to convert
	 * @return The FTextLocation representing the position
	 */
	static FTextLocation ConvertPositionToTextLocation(const FString& FileContent, int32 Position);

	/**
	 * Exports C++ declaration for a property (equivalent to FProperty::ExportCppDeclaration)
	 * @param Property The property to export
	 * @param Out Output device to write the declaration to
	 * @param DeclarationType Type of declaration being exported
	 * @param ArrayDimOverride Optional array dimension override
	 * @param AdditionalExportCPPFlags Additional C++ export flags
	 * @param bSkipParameterName Whether to skip the parameter name
	 * @param ActualCppType Optional actual C++ type override
	 * @param ActualExtendedType Optional actual extended type override
	 * @param ActualParameterName Optional actual parameter name override
	 */
	static void ExportPropertyCppDeclaration(const FProperty* Property, FOutputDevice& Out, 
		EQCEExportedDeclaration::Type DeclarationType, const TCHAR* ArrayDimOverride = nullptr, 
		uint32 AdditionalExportCPPFlags = 0, bool bSkipParameterName = false, 
		const FString* ActualCppType = nullptr, const FString* ActualExtendedType = nullptr, 
		const FString* ActualParameterName = nullptr);

	/**
	 * Determines if a property should pass C++ arguments by reference (equivalent to FProperty::PassCPPArgsByRef)
	 * @param Property The property to check
	 * @return True if the property should pass by reference, false if by value
	 */
	static bool GetPropertyPassCPPArgsByRef(const FProperty* Property);
};
