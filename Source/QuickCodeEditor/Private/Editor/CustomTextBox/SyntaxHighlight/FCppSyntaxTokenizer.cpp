// Copyright TechnicallyArtist 2025 All Rights Reserved.



#pragma region Definitions
#include "Editor/CustomTextBox/SyntaxHighlight/FCppSyntaxTokenizer.h"

// C++ Keywords
const TCHAR* CPPKeywords[] =
{
  // Standard C++ keywords
    TEXT("alignas"),
    TEXT("alignof"),
    TEXT("and"),
    TEXT("and_eq"),
    TEXT("asm"),
    TEXT("auto"),
    TEXT("bitand"),
    TEXT("bitor"),
    TEXT("bool"),
    TEXT("break"),
    TEXT("case"),
    TEXT("catch"),
    TEXT("char"),
    TEXT("char16_t"),
    TEXT("char32_t"),
    TEXT("class"),
    TEXT("compl"),
    TEXT("const"),
    TEXT("constexpr"),
    TEXT("const_cast"),
    TEXT("continue"),
    TEXT("decltype"),
    TEXT("default"),
    TEXT("delete"),
    TEXT("do"),
    TEXT("double"),
    TEXT("dynamic_cast"),
    TEXT("else"),
    TEXT("enum"),
    TEXT("explicit"),
    TEXT("export"),
    TEXT("extern"),
    TEXT("false"),
    TEXT("float"),
    TEXT("for"),
    TEXT("friend"),
    TEXT("goto"),
    TEXT("if"),
    TEXT("inline"),
    TEXT("int"),
    TEXT("long"),
    TEXT("mutable"),
    TEXT("namespace"),
    TEXT("new"),
    TEXT("noexcept"),
    TEXT("not"),
    TEXT("not_eq"),
    TEXT("nullptr"),
    TEXT("operator"),
    TEXT("or"),
    TEXT("or_eq"),
    TEXT("private"),
    TEXT("protected"),
    TEXT("public"),
    TEXT("register"),
    TEXT("reinterpret_cast"),
    TEXT("return"),
    TEXT("short"),
    TEXT("signed"),
    TEXT("sizeof"),
    TEXT("static"),
    TEXT("static_assert"),
    TEXT("static_cast"),
    TEXT("struct"),
    TEXT("switch"),
    TEXT("template"),
    TEXT("this"),
    TEXT("thread_local"),
    TEXT("throw"),
    TEXT("true"),
    TEXT("try"),
    TEXT("typedef"),
    TEXT("typeid"),
    TEXT("typename"),
    TEXT("union"),
    TEXT("unsigned"),
    TEXT("using"),
    TEXT("virtual"),
    TEXT("void"),
    TEXT("volatile"),
    TEXT("wchar_t"),
    TEXT("while"),
    TEXT("xor"),
    TEXT("xor_eq"),
    
    // C++11/14/17 additions commonly used in UE
    TEXT("override"),
    TEXT("final"),
    TEXT("constexpr"),
    TEXT("decltype"),
    TEXT("nullptr"),
    TEXT("static_assert"),
    TEXT("thread_local"),
    TEXT("alignas"),
    TEXT("alignof"),
    TEXT("noexcept"),
    
    // Unreal Engine type aliases (not keywords but commonly used)
    TEXT("int8"),
    TEXT("int16"),
    TEXT("int32"),
    TEXT("int64"),
    TEXT("uint8"),
    TEXT("uint16"),
    TEXT("uint32"),
    TEXT("uint64"),
    TEXT("TCHAR"),
    TEXT("FString"),
    TEXT("FName"),
    TEXT("FText"),
    
    // Unreal-specific macros that behave like keywords
    TEXT("UCLASS"),
    TEXT("USTRUCT"),
    TEXT("UENUM"),
    TEXT("UFUNCTION"),
    TEXT("UPROPERTY"),
    TEXT("UINTERFACE"),
    TEXT("UDELEGATE"),
    TEXT("DECLARE_DELEGATE"),
    TEXT("DECLARE_MULTICAST_DELEGATE"),
    TEXT("DECLARE_DYNAMIC_DELEGATE"),
    TEXT("DECLARE_DYNAMIC_MULTICAST_DELEGATE"),
    TEXT("GENERATED_BODY"),
    TEXT("GENERATED_UCLASS_BODY"),
    TEXT("GENERATED_USTRUCT_BODY"),
    TEXT("GENERATED_UENUM_BODY"),
    TEXT("GENERATED_UINTERFACE_BODY"),
    TEXT("UPARAM"),
    TEXT("FORCEINLINE"),
    TEXT("FORCENOINLINE"),
    TEXT("RESTRICT"),
    TEXT("DEPRECATED"),
    TEXT("PRAGMA_DISABLE_DEPRECATION_WARNINGS"),
    TEXT("PRAGMA_ENABLE_DEPRECATION_WARNINGS"),
    TEXT("checkf"),
    TEXT("check"),
    TEXT("verify"),
    TEXT("ensure"),
    TEXT("ensureAlways"),
    TEXT("ensureMsgf"),
    TEXT("PURE_VIRTUAL"),
    TEXT("ABSTRACT"),
    TEXT("TEXT"),
    TEXT("TEXTVIEW"),
    TEXT("region"),
    TEXT("endregion"),
    
    // Additional C++ constructs that should be highlighted
    TEXT("requires"),
    TEXT("concept"),
    TEXT("co_await"),
    TEXT("co_yield"),
    TEXT("co_return"),
    
    // Common Unreal macros and functions
    TEXT("IMPLEMENT_CLASS"),
    TEXT("IMPLEMENT_MODULE"),
    TEXT("DEFINE_LOG_CATEGORY"),
    TEXT("UE_LOG"),
    TEXT("DECLARE_LOG_CATEGORY_EXTERN"),
    TEXT("SLATE_BEGIN_ARGS"),
    TEXT("SLATE_END_ARGS"),
    TEXT("SLATE_ARGUMENT"),
    TEXT("SLATE_ATTRIBUTE"),
    TEXT("SLATE_EVENT")
};

// C++ Operators - ORDERED BY LENGTH (longest first for greedy matching)
const TCHAR* CPPOperators[] =
{
    // 3 character operators
    TEXT("<<="),
    TEXT(">>="),
    TEXT("..."),
    
    // 2 character operators  
    TEXT("/*"),
    TEXT("*/"),
    TEXT("//"),
    TEXT("::"),
    TEXT("+="),
    TEXT("++"),
    TEXT("--"),
    TEXT("-="),
    TEXT("->"),
    TEXT("!="),
    TEXT("&="),
    TEXT("*="),
    TEXT("/="),
    TEXT("%="),
    TEXT("<<"),
    TEXT("<="),
    TEXT(">>"),
    TEXT(">="),
    TEXT("=="),
    TEXT("&&"),
    TEXT("^="),
    TEXT("|="),
    TEXT("||"),
    
    // 1 character operators
    TEXT(":"),
    TEXT("+"),
    TEXT("-"),
    TEXT("("),
    TEXT(")"),
    TEXT("["),
    TEXT("]"),
    TEXT("<"),
    TEXT(">"),
    TEXT("."),
    TEXT("!"),
    TEXT("~"),
    TEXT("&"),
    TEXT("*"),
    TEXT("/"),
    TEXT("%"),
    TEXT("^"),
    TEXT("|"),
    TEXT("?"),
    TEXT("="),
    TEXT(","),
    TEXT("{"),
    TEXT("}"),
    TEXT(";"),
    TEXT("\""),
    TEXT("\'"),
};

// C++ Preprocessor Keywords
const TCHAR* CPPPreProcessorKeywords[] =
{
    TEXT("#include"),
    TEXT("#define"),
    TEXT("#ifndef"),
    TEXT("#ifdef"),
    TEXT("#if"),
    TEXT("#else"),
    TEXT("#endif"),
    TEXT("#pragma"),
    TEXT("#undef"),
};

const TCHAR* CPPUnrealTypedefs[] = {
    TEXT("int8"),
    TEXT("int16"),
    TEXT("int32"),
    TEXT("int64"),
    TEXT("uint8"),
    TEXT("uint16"),
    TEXT("uint32"),
    TEXT("uint64"),
    TEXT("TCHAR"),
    TEXT("FString"),
    TEXT("FName"),
    TEXT("FText"),
    TEXT("TArray"),
    TEXT("TSharedPtr"),
    TEXT("TSharedRef"),
    TEXT("TWeakPtr"),
    TEXT("TUniquePtr"),
    TEXT("TInlineComponentArray"),
    TEXT("TInlineAllocator"),
    TEXT("TArrayView"),
    TEXT("TMap"),
    TEXT("TMultiMap"),
    TEXT("TSortedMap"),
    TEXT("TStaticArray"),
    TEXT("TCircularQueue"),
    TEXT("TQueue"),
    TEXT("TDoubleLinkedList"),
    TEXT("TSparseArray"),
    TEXT("TPair"),
    TEXT("TFixedAllocator"),
    TEXT("TSizedHeapAllocator"),
    TEXT("TScriptArray"),
    TEXT("bool"),
    TEXT("UPROPERTY"),
    TEXT("UCLASS"),
    TEXT("USTRUCT"),
    TEXT("UENUM"),
    TEXT("UFUNCTION"),
    TEXT("UINTERFACE"),
    TEXT("GENERATED_BODY"),
    TEXT("GENERATED_UCLASS_BODY"),
    TEXT("GENERATED_USTRUCT_BODY"),
    TEXT("GENERATED_UENUM_BODY"),
    TEXT("GENERATED_UINTERFACE_BODY"),
    TEXT("UPARAM"),
    TEXT("FORCEINLINE"),
    TEXT("FORCENOINLINE"),
    TEXT("RESTRICT"),
    TEXT("DEPRECATED"),
    // UFUNCTION specifiers
    TEXT("BlueprintCallable"),
    TEXT("BlueprintPure"),
    TEXT("BlueprintImplementableEvent"),
    TEXT("BlueprintNativeEvent"),
    TEXT("CallInEditor"),
    TEXT("Category"),
    TEXT("meta"),
    TEXT("DisplayName"),
    TEXT("ToolTip"),
    TEXT("Keywords"),
    TEXT("HidePin"),
    TEXT("ExpandEnumAsExecs"),
    TEXT("CommutativeAssociativeBinaryOperator"),
    TEXT("CompactNodeTitle"),
    TEXT("CustomThunk"),
    TEXT("LatentInfo"),
    TEXT("WorldContext"),
    // UPROPERTY specifiers
    TEXT("EditAnywhere"),
    TEXT("EditDefaultsOnly"),
    TEXT("EditInstanceOnly"),
    TEXT("VisibleAnywhere"),
    TEXT("VisibleDefaultsOnly"),
    TEXT("VisibleInstanceOnly"),
    TEXT("BlueprintReadOnly"),
    TEXT("BlueprintReadWrite"),
    TEXT("BlueprintAssignable"),
    TEXT("BlueprintCallable"),
    TEXT("SaveGame"),
    TEXT("Transient"),
    TEXT("DuplicateTransient"),
    TEXT("TextExportTransient"),
    TEXT("NonPIEDuplicateTransient"),
    TEXT("Export"),
    TEXT("NoClear"),
    TEXT("EditFixedSize"),
    TEXT("Replicated"),
    TEXT("ReplicatedUsing"),
    TEXT("NotReplicated"),
    TEXT("RepSkip"),
    TEXT("Interp"),
    TEXT("NonTransactional"),
    TEXT("Instanced"),
    TEXT("BlueprintGetter"),
    TEXT("BlueprintSetter"),
    TEXT("SelfContext"),
    TEXT("GlobalConfig"),
    TEXT("Config"),
    TEXT("Localized"),
    TEXT("AdvancedDisplay"),
    TEXT("SimpleDisplay"),
    // UCLASS specifiers
    TEXT("Blueprintable"),
    TEXT("BlueprintType"),
    TEXT("NotBlueprintable"),
    TEXT("NotBlueprintType"),
    TEXT("BlueprintSpawnableComponent"),
    TEXT("ChildCanTick"),
    TEXT("ChildCannotTick"),
    TEXT("ClassGroup"),
    TEXT("ComponentWrapperClass"),
    TEXT("HideCategories"),
    TEXT("ShowCategories"),
    TEXT("AutoExpandCategories"),
    TEXT("AutoCollapseCategories"),
    TEXT("DontAutoCollapseCategories"),
    TEXT("CollapseCategories"),
    TEXT("DontCollapseCategories"),
    TEXT("AdvancedClassDisplay"),
    TEXT("ConversionRoot"),
    TEXT("CustomConstructor"),
    TEXT("Deprecated"),
    TEXT("DependsOn"),
    TEXT("EditInlineNew"),
    TEXT("HideDropdown"),
    TEXT("IgnoreCategoryKeywordsInSubclasses"),
    TEXT("IsBlueprintBase"),
    TEXT("MinimalAPI"),
    TEXT("NonTransient"),
    TEXT("Placeable"),
    TEXT("NotPlaceable")
};
#pragma endregion

TSharedRef<FCppSyntaxTokenizer> FCppSyntaxTokenizer::Create()
{
    return MakeShareable(new FCppSyntaxTokenizer());
}

FCppSyntaxTokenizer::FCppSyntaxTokenizer()
{
    // Initialize operators
    for (const auto& Operator : CPPOperators)
    {
        Operators.Emplace(Operator);
    }

    // Initialize keywords
    for (const auto& Keyword : CPPKeywords)
    {
        Keywords.Emplace(Keyword);
    }

    // Initialize preprocessor keywords
    for (const auto& PreProcessorKeyword : CPPPreProcessorKeywords)
    {
        Keywords.Emplace(PreProcessorKeyword);
    }

    for (const auto& UnrealTypeDef : CPPUnrealTypedefs)
    {
        UnrealTypedefs.Emplace(UnrealTypeDef);
    }
}

void FCppSyntaxTokenizer::Process(TArray<FTokenizedLine>& OutTokenizedLines, const FString& Input)
{
    // Reset multiline comment state for new tokenization
    bInMultilineComment = false;
    
#if UE_ENABLE_ICU
    TArray<FTextRange> LineRanges;
    FTextRange::CalculateLineRangesFromString(Input, LineRanges);
    TokenizeLineRanges(Input, LineRanges, OutTokenizedLines);
#else
    FTokenizedLine FakeTokenizedLine;
    FakeTokenizedLine.Range = FTextRange(0, Input.Len());
    FakeTokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, FakeTokenizedLine.Range));
    OutTokenizedLines.Add(FakeTokenizedLine);
#endif
}

void FCppSyntaxTokenizer::TokenizeLineRanges(const FString& Input, const TArray<FTextRange>& LineRanges, TArray<FTokenizedLine>& OutTokenizedLines)
{
    // Process each line with proper multiline comment state tracking
    for (const FTextRange& LineRange : LineRanges)
    {
        FTokenizedLine TokenizedLine;
        TokenizedLine.Range = LineRange;

        if (TokenizedLine.Range.IsEmpty())
        {
            TokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, TokenizedLine.Range));
            OutTokenizedLines.Add(TokenizedLine);
            continue;
        }

        int32 CurrentOffset = LineRange.BeginIndex;

        // If we're already in a multiline comment, handle it first
        if (bInMultilineComment)
        {
            // Look for the end of the multiline comment on this line
            int32 CommentEnd = CurrentOffset;
            bool bFoundEnd = false;
            
            // Search for */ in the current line
            while (CommentEnd < LineRange.EndIndex - 1)
            {
                if (Input[CommentEnd] == TEXT('*') && Input[CommentEnd + 1] == TEXT('/'))
                {
                    CommentEnd += 2; // Include the */
                    bFoundEnd = true;
                    break;
                }
                CommentEnd++;
            }
            
            if (bFoundEnd)
            {
                // End of multiline comment found on this line
                TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, CommentEnd)));
                CurrentOffset = CommentEnd;
                bInMultilineComment = false;
            }
            else
            {
                // Entire line is part of the multiline comment
                TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, LineRange.EndIndex)));
                CurrentOffset = LineRange.EndIndex;
            }
        }

        while (CurrentOffset < LineRange.EndIndex)
        {
            const TCHAR* CurrentString = &Input[CurrentOffset];
            const TCHAR CurrentChar = Input[CurrentOffset];

            bool bHasMatchedSyntax = false;

            // Handle string literals first (highest priority)
            if (CurrentChar == TEXT('\"'))
            {
                int32 StringEnd = CurrentOffset + 1;
                bool bEscaped = false;

                // Find the closing quote, handling escaped quotes
                while (StringEnd < LineRange.EndIndex)
                {
                    const TCHAR StringChar = Input[StringEnd];
                    
                    if (StringChar == TEXT('\\'))
                    {
                        bEscaped = !bEscaped;
                    }
                    else if (StringChar == TEXT('\"') && !bEscaped)
                    {
                        // Found unescaped closing quote
                        StringEnd++;
                        break;
                    }
                    else
                    {
                        bEscaped = false;
                    }
                    
                    StringEnd++;
                }

                TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, StringEnd)));
                CurrentOffset = StringEnd;
                bHasMatchedSyntax = true;
                continue;
            }

            // Handle character literals
            if (CurrentChar == TEXT('\''))
            {
                int32 CharEnd = CurrentOffset + 1;
                bool bEscaped = false;

                // Find the closing quote, handling escaped quotes
                while (CharEnd < LineRange.EndIndex)
                {
                    const TCHAR StringChar = Input[CharEnd];
                    
                    if (StringChar == TEXT('\\'))
                    {
                        bEscaped = !bEscaped;
                    }
                    else if (StringChar == TEXT('\'') && !bEscaped)
                    {
                        // Found unescaped closing quote
                        CharEnd++;
                        break;
                    }
                    else
                    {
                        bEscaped = false;
                    }
                    
                    CharEnd++;
                }

                TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, CharEnd)));
                CurrentOffset = CharEnd;
                bHasMatchedSyntax = true;
                continue;
            }

            // Check for multiline comment start
            if (!bInMultilineComment && CurrentOffset < LineRange.EndIndex - 1 && 
                CurrentChar == TEXT('/') && Input[CurrentOffset + 1] == TEXT('*'))
            {
                // Start of multiline comment
                int32 CommentStart = CurrentOffset;
                CurrentOffset += 2; // Skip the initial /*
                
                // Look for the end of the multiline comment on the same line
                bool bFoundEnd = false;
                while (CurrentOffset < LineRange.EndIndex - 1)
                {
                    if (Input[CurrentOffset] == TEXT('*') && Input[CurrentOffset + 1] == TEXT('/'))
                    {
                        CurrentOffset += 2; // Include the */
                        bFoundEnd = true;
                        break;
                    }
                    CurrentOffset++;
                }
                
                if (bFoundEnd)
                {
                    // Complete multiline comment found on this line (e.g., /* comment */)
                    TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CommentStart, CurrentOffset)));
                }
                else
                {
                    // Multiline comment continues beyond this line
                    TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CommentStart, LineRange.EndIndex)));
                    bInMultilineComment = true;
                    CurrentOffset = LineRange.EndIndex;
                }
                bHasMatchedSyntax = true;
                continue;
            }

            // Greedy matching for operators (ordered by length - longest first)
            for (const FString& Operator : Operators)
            {
                if (FCString::Strncmp(CurrentString, *Operator, Operator.Len()) == 0)
                {
                    // Single-line comments - consume rest of line
                    if (Operator == TEXT("//"))
                    {
                        const int32 SyntaxTokenEnd = LineRange.EndIndex;
                        TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
                        CurrentOffset = SyntaxTokenEnd;
                    }
                    // Skip multiline comment operators as they're handled above
                    else if (Operator != TEXT("/*") && Operator != TEXT("*/"))
                    {
                        const int32 SyntaxTokenEnd = CurrentOffset + Operator.Len();
                        TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));
                        CurrentOffset = SyntaxTokenEnd;
                    }
                    else
                    {
                        // Skip /* and */ operators when not in multiline comment context
                        continue;
                    }

                    check(CurrentOffset <= LineRange.EndIndex);
                    bHasMatchedSyntax = true;
                    break;
                }
            }

            if (bHasMatchedSyntax)
            {
                continue;
            }

            // Handle identifiers and keywords
            int32 PeekOffset = CurrentOffset + 1;
            if (CurrentChar == TEXT('#'))
            {
                // Match PreProcessorKeywords - they contain letters
                while (PeekOffset < LineRange.EndIndex)
                {
                    const TCHAR PeekChar = Input[PeekOffset];
                    if (!IsAlpha(PeekChar))
                    {
                        break;
                    }
                    PeekOffset++;
                }
            }
            else if (IsAlpha(CurrentChar) || CurrentChar == TEXT('_'))
            {
                // Match identifiers - start with letter or underscore, contain letters, numbers, or underscores
                while (PeekOffset < LineRange.EndIndex)
                {
                    const TCHAR PeekChar = Input[PeekOffset];
                    if (!IsIdentifierChar(PeekChar))
                    {
                        break;
                    }
                    PeekOffset++;
                }
            }
            else if (IsDigit(CurrentChar))
            {
                // Handle numeric literals
                while (PeekOffset < LineRange.EndIndex)
                {
                    const TCHAR PeekChar = Input[PeekOffset];
                    if (!IsDigit(PeekChar) && PeekChar != TEXT('.') && PeekChar != TEXT('f') && 
                        PeekChar != TEXT('F') && PeekChar != TEXT('u') && PeekChar != TEXT('U') &&
                        PeekChar != TEXT('l') && PeekChar != TEXT('L') && PeekChar != TEXT('x') &&
                        PeekChar != TEXT('X') && PeekChar != TEXT('e') && PeekChar != TEXT('E') &&
                        PeekChar != TEXT('+') && PeekChar != TEXT('-'))
                    {
                        break;
                    }
                    PeekOffset++;
                }
            }

            const int32 CurrentStringLength = PeekOffset - CurrentOffset;

            if (CurrentStringLength > 0)
            {
                // Check if it is a reserved keyword - FIXED COMPARISON LOGIC
                for (const FString& Keyword : Keywords)
                {
                    // Proper keyword matching: 
                    // 1. The lengths must match exactly
                    // 2. The content must match exactly
                    // 3. Ensure word boundary (next char is not alphanumeric/underscore)
                    if (Keyword.Len() == CurrentStringLength && 
                        FCString::Strncmp(CurrentString, *Keyword, Keyword.Len()) == 0)
                    {
                        // Verify word boundary - ensure we're not matching partial words
                        bool bIsWordBoundary = true;
                        if (CurrentOffset + Keyword.Len() < LineRange.EndIndex)
                        {
                            const TCHAR NextChar = Input[CurrentOffset + Keyword.Len()];
                            if (IsIdentifierChar(NextChar))
                            {
                                bIsWordBoundary = false;
                            }
                        }

                        if (bIsWordBoundary)
                        {
                            const int32 SyntaxTokenEnd = CurrentOffset + Keyword.Len();
                            TokenizedLine.Tokens.Emplace(FToken(ETokenType::Syntax, FTextRange(CurrentOffset, SyntaxTokenEnd)));

                            check(SyntaxTokenEnd <= LineRange.EndIndex);

                            bHasMatchedSyntax = true;
                            CurrentOffset = SyntaxTokenEnd;
                            break;
                        }
                    }
                }
            }

            if (bHasMatchedSyntax)
            {
                continue;
            }

            // If none matched, consume the character(s) as literal text
            // Use at least 1 character to prevent infinite loops
            const int32 TextTokenEnd = CurrentOffset + FMath::Max(1, CurrentStringLength);
            TokenizedLine.Tokens.Emplace(FToken(ETokenType::Literal, FTextRange(CurrentOffset, TextTokenEnd)));
            CurrentOffset = TextTokenEnd;
        }

        OutTokenizedLines.Add(TokenizedLine);
    }
}