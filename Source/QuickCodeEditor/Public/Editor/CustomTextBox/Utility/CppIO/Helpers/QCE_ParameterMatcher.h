#pragma once

#include "CoreMinimal.h"

/**
 * Utility class for matching and comparing C++ function parameters.
 */
struct QUICKCODEEDITOR_API FParameterTypeInfo
{
    bool bIsConst;
    bool bIsVolatile;
    bool bIsReference;
    bool bIsPointer;
    FString BaseType;

    FParameterTypeInfo()
        : bIsConst(false)
        , bIsVolatile(false)
        , bIsReference(false)
        , bIsPointer(false)
    {}

    bool operator==(const FParameterTypeInfo& Other) const
    {
        return bIsConst == Other.bIsConst &&
               bIsVolatile == Other.bIsVolatile &&
               bIsReference == Other.bIsReference &&
               bIsPointer == Other.bIsPointer &&
               BaseType == Other.BaseType;
    }

    // Helper method for debugging
    FString ToString() const
    {
        FString Result = FString::Printf(TEXT("BaseType='%s'"), *BaseType);
        if (bIsConst) Result += TEXT(" [const]");
        if (bIsVolatile) Result += TEXT(" [volatile]");
        if (bIsPointer) Result += TEXT(" [pointer]");
        if (bIsReference) Result += TEXT(" [reference]");
        return Result;
    }
};

class QUICKCODEEDITOR_API QCE_ParameterMatcher
{
public:
    /** 
     * Compares two C++ parameter type declarations for equivalence.
     * Handles complex type comparisons including const qualifiers, references, pointers,
     * and template parameters while normalizing whitespace and removing parameter names.
     * 
     * Example:
     * @code
     * DoParameterTypesMatch("const TArray<FString>& OutArray", "const TArray<FString> & Array") // Returns true
     * DoParameterTypesMatch("int32 Count", "const int32& Value") // Returns false
     * @endcode
     */
    static bool DoParameterTypesMatch(const FString& TypeA, const FString& TypeB, bool bMatchConstness = true);

    /** Special version of DoParameterTypesMatch method, where we also check if function parameter is passed by const reference. */
    static bool DoesFunctionParameterMatchDeclarationParameter(const FString& FunctionParam, const FString& DeclarationParam, bool bIsConstRef, bool bMatchConstness);

#pragma region Public utility
public:
    /** 
     * Splits a function parameter list into individual parameter declarations.
     * 
     * Example:
     * @code
     * FString Params = "const TArray<FString>& Arrays, int32 Count = 0, FString Text = TEXT(\"Hello\")";
     * TArray<FString> Result = ToParameterArray(Params);
     * // Result = ["const TArray<FString>& Arrays", "int32 Count = 0", "FString Text = TEXT(\"Hello\")"]
     * @endcode
     */
    static TArray<FString> ToParameterArray(const FString& ParameterString);

    /** 
     * Extracts the parameter string from a function declaration or definition.
     * Finds and returns the content between the outermost parentheses at the given position.
     * 
     * Example:
     * @code
     * FString Content = "void ProcessItems(const TArray<int32>& Items, bool bSort = true) {";
     * FString Result;
     * GetParameterStringAtPosition(Content, 12, Result);
     * // Result = "const TArray<int32>& Items, bool bSort = true"
     * @endcode
     */
    static bool GetParameterStringAtPosition(const FString& FileContent, int32 MatchPos, FString& ParameterString);

    /** 
     * Finds matching brackets while respecting C++ syntax.
     * Handles nested brackets, string literals, character literals, and comments.
     * Used for both parameter extraction and general code parsing.
     * 
     * Example:
     * @code
     * FString Content = "TArray<TSharedPtr<FString>>";
     * int32 ClosePos;
     * FindMatchingBracket(Content, 5, '<', '>', ClosePos); // Returns true, ClosePos = 24
     * @endcode
     */
    static bool FindMatchingBracket(const FString& Content, int32 OpenBracketPos, TCHAR OpenBracket, TCHAR CloseBracket, int32& OutCloseBracketPos, bool bIncludeSingleQuotes = false);

    /** 
     * Normalizes a parameter declaration for comparison.
     * Standardizes whitespace, removes parameter names and optionally default values
     * while preserving type information and qualifiers.
     * 
     * Example:
     * @code
     * FString Param = "const  TArray<int32>&    OutValues = DefaultArray";
     * FString Result = NormalizeParameter(Param, true);
     * // Result = "const TArray<int32>&"
     * @endcode
     */
    static FString NormalizeParameter(const FString& Parameter, bool bRemoveDefaultValue = true, bool bShouldRemoveParamName = true);

    /** 
     * Searches for a character while respecting C++ syntax contexts.
     * Skips occurrences inside string literals, character literals, and comments.
     * 
     * Example:
     * @code
     * FString Content = "int x = 0; // x = 1; \n x = 2;";
     * int32 Pos;
     * FindCharacterRespectingContext(Content, "=", 0, ESearchDir::FromStart, Pos); // Returns true, Pos = 6
     * @endcode
     */
    static bool FindCharacterRespectingContext(const FString& Content, const FString& SearchChar, int32 StartPos, ESearchDir::Type Direction, int32& OutPosition);

    /** 
     * Determines if a position in code is within a string literal or comment.
     * Handles both single-line and multi-line comments, and string/character literals.
     * 
     * Example:
     * @code
     * FString Content = "int x = 0; // Comment here";
     * bool bInComment = IsPositionInStringOrComment(Content, 15); // Returns true
     * bool bInCode = IsPositionInStringOrComment(Content, 5); // Returns false
     * @endcode
     */
    static bool IsPositionInStringOrComment(const FString& Content, int32 Position);
#pragma endregion
private:
    /** 
     * Removes default value assignments from parameter declarations.
     * Handles complex default values including function calls, templates,
     * and string literals while respecting nested structures.
     * 
     * Example:
     * @code
     * FString Param = "int32 Count = FMath::Max(DefaultCount, 5)";
     * FString Result = RemoveDefaultValue(Param);
     * // Result = "int32 Count"
     * @endcode
     */
    static FString RemoveDefaultValue(const FString& Parameter);

    /** 
     * Standardizes whitespace in parameter declarations.
     * Preserves necessary spacing around operators and between tokens
     * while removing redundant whitespace.
     * 
     * Example:
     * @code
     * FString Input = "const    TArray<int32>  *  &   Ptr";
     * FString Result = NormalizeWhitespace(Input);
     * // Result = "const TArray<int32>*& Ptr"
     * @endcode
     */
    static FString NormalizeWhitespace(const FString& Input);

    /** 
     * Removes parameter names from declarations while preserving type information.
     * Handles complex type declarations including function pointers and templates.
     * 
     * Example:
     * @code
     * FString Param = "const TSharedPtr<FString>& OutValue";
     * FString Result = RemoveParameterName(Param);
     * // Result = "const TSharedPtr<FString>&"
     * @endcode
     */
    static FString RemoveParameterName(const FString& Parameter);

    /** 
     * Parses a parameter declaration into its component parts.
     * Extracts information about const/volatile qualifiers, pointers,
     * references, and the base type.
     * 
     * Example:
     * @code
     * FString Param = "const int32* const& Value";
     * FParameterTypeInfo Info = ParseParameterTypeInfo(Param);
     * // Info.BaseType = "int32"
     * // Info.bIsConst = true
     * // Info.bIsPointer = true
     * // Info.bIsReference = true
     * @endcode
     */
    static FParameterTypeInfo ParseParameterTypeInfo(const FString& NormalizedParameter);
};
