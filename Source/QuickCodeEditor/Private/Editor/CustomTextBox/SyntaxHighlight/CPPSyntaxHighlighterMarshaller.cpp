// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/SyntaxHighlight/CPPSyntaxHighlighterMarshaller.h"

#include "Editor/CustomTextBox/SyntaxHighlight/FCppSyntaxTokenizer.h"
#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextRun.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "Settings/UQCE_EditorSettings.h"

TSharedRef<FCPPSyntaxHighlighterMarshaller> FCPPSyntaxHighlighterMarshaller::Create()
{
    TSharedPtr<ISyntaxTokenizer> Tokenizer = FCppSyntaxTokenizer::Create();
    return MakeShareable(new FCPPSyntaxHighlighterMarshaller(Tokenizer));
}

FCPPSyntaxHighlighterMarshaller::FCPPSyntaxHighlighterMarshaller(TSharedPtr<ISyntaxTokenizer> InTokenizer)
    : FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer)), CurrentSyntaxStyle(false)
{
    if (UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>())
    {
        Settings->OnSyntaxSettingsUpdated.BindLambda([this]()
        {
            bRefreshStyle = true;
        });
    }
}

void FCPPSyntaxHighlighterMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
    // If no node is selected or text box is not set, just add the text without syntax highlighting
    if (!bShouldApplyHighlights)
    {
        TArray<FTextLayout::FNewLineData> LinesToAdd;
        LinesToAdd.Reserve(TokenizedLines.Num());

        for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); ++LineIndex)
        {
            const ISyntaxTokenizer::FTokenizedLine& TokenizedLine = TokenizedLines[LineIndex];
            TSharedRef<FString> ModelString = MakeShareable(new FString());
            TArray<TSharedRef<IRun>> Runs;

            for (const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
            {
                const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
                const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
                ModelString->Append(TokenText);

                FRunInfo RunInfo(TEXT("SyntaxHighlight.Normal"));
                TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, GetSyntaxTextStyle().NormalTextStyle, ModelRange);
                Runs.Add(Run);
            }

            LinesToAdd.Add(FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs)));
        }

        TargetTextLayout.AddLines(LinesToAdd);
        return;
    }

    TArray<FTextLayout::FNewLineData> LinesToAdd;
    LinesToAdd.Reserve(TokenizedLines.Num());

    for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); ++LineIndex)
    {
        const ISyntaxTokenizer::FTokenizedLine& TokenizedLine = TokenizedLines[LineIndex];

        TSharedRef<FString> ModelString = MakeShareable(new FString());
        TArray<TSharedRef<IRun>> Runs;

        for (const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
        {
            const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
            const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
            ModelString->Append(TokenText);

            FRunInfo RunInfo(TEXT("SyntaxHighlight.Normal"));
             FTextBlockStyle TextBlockStyle = GetSyntaxTextStyle().NormalTextStyle;

            // Determine the style based on the token
            if (Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
            {
                // Check for comments - trust the tokenizer's classification
                // The tokenizer already handles proper context detection
                if (IsCommentToken(SourceString, Token))
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.Comment");
                    TextBlockStyle = GetSyntaxTextStyle().CommentTextStyle;
                }
                // Check for preprocessor directives
                else if (TokenText.StartsWith(TEXT("#")))
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.PreProcessor");
                    TextBlockStyle = GetSyntaxTextStyle().PreProcessorKeywordTextStyle;
                }
                // Check for string literals
                else if (TokenText.StartsWith(TEXT("\"")) || TokenText.StartsWith(TEXT("'")))
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.String");
                    TextBlockStyle = GetSyntaxTextStyle().StringTextStyle;
                }
                // Check for numbers
                else if (TokenText.Len() > 0 && TokenText[0] >= TEXT('0') && TokenText[0] <= TEXT('9'))
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.Number");
                    TextBlockStyle = GetSyntaxTextStyle().NumberTextStyle;
                }
                // Check if it's an Unreal typedef
                else if (TSharedPtr<FCppSyntaxTokenizer> CppTokenizer = StaticCastSharedPtr<FCppSyntaxTokenizer>(Tokenizer))
                {
                    if (CppTokenizer->IsUnrealTypeDef(TokenText))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.UnrealTypeDef");
                        TextBlockStyle = GetSyntaxTextStyle().UnrealTypedefTextStyle;
                    }
                    // Check if it's a known keyword
                    else if (CppTokenizer->IsKeyword(TokenText))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.Keyword");
                        TextBlockStyle = GetSyntaxTextStyle().KeywordTextStyle;
                    }
                    // Check if it's an operator
                    else if (CppTokenizer->IsOperator(TokenText))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.Operator");
                        TextBlockStyle = GetSyntaxTextStyle().OperatorTextStyle;
                    }
                    // Default to keyword style for unrecognized syntax tokens
                    else
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.Keyword");
                        TextBlockStyle = GetSyntaxTextStyle().KeywordTextStyle;
                    }
                }
                // Fallback if tokenizer cast fails - default to keyword style
                else
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.Keyword");
                    TextBlockStyle = GetSyntaxTextStyle().KeywordTextStyle;
                }
            } else // Literal
            {
                // Check for Unreal typedefs first (with safe casting)
                if (TSharedPtr<FCppSyntaxTokenizer> CppTokenizer = StaticCastSharedPtr<FCppSyntaxTokenizer>(Tokenizer))
                {
                    if (CppTokenizer->IsUnrealTypeDef(TokenText))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.UnrealTypeDef");
                        TextBlockStyle = GetSyntaxTextStyle().UnrealTypedefTextStyle;
                    }
                    // Check for string literals
                    else if (TokenText.StartsWith(TEXT("\"")) || TokenText.StartsWith(TEXT("'")))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.StringLiteral");
                        TextBlockStyle = GetSyntaxTextStyle().StringTextStyle;
                    }
                    // Check for numeric literals
                    else if (TokenText.Len() > 0 && TokenText[0] >= TEXT('0') && TokenText[0] <= TEXT('9'))
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.NumericLiteral");
                        TextBlockStyle = GetSyntaxTextStyle().NumberTextStyle;
                    }
                    // Check for identifiers that might be function or class names
                    else if (TokenText.Len() > 0 && FChar::IsAlpha(TokenText[0]))
                    {
                        // Look ahead to detect function names and class names
                        FString NextTokens;
                        int32 CurrentTokenIndex = &Token - &TokenizedLine.Tokens[0];
                        
                        // Collect next few tokens to analyze pattern
                        for (int32 LookAheadIndex = CurrentTokenIndex + 1; 
                             LookAheadIndex < TokenizedLine.Tokens.Num() && LookAheadIndex < CurrentTokenIndex + 3; 
                             ++LookAheadIndex)
                        {
                            const FString NextTokenText = SourceString.Mid(
                                TokenizedLine.Tokens[LookAheadIndex].Range.BeginIndex, 
                                TokenizedLine.Tokens[LookAheadIndex].Range.Len()
                            );
                            NextTokens += NextTokenText;
                        }
                        
                        // Check if followed by :: (class name)
                        if (NextTokens.StartsWith(TEXT("::")))
                        {
                            RunInfo.Name = TEXT("SyntaxHighlight.ClassName");
                            TextBlockStyle = GetSyntaxTextStyle().ClassTextStyle;
                        }
                        // Check if followed by ( (function name)
                        else if (NextTokens.Contains(TEXT("(")) && !NextTokens.StartsWith(TEXT(" class")) && !NextTokens.StartsWith(TEXT(" struct")))
                        {
                            RunInfo.Name = TEXT("SyntaxHighlight.FunctionName");
                            TextBlockStyle = GetSyntaxTextStyle().FunctionTextStyle;
                        }
                        // Default literal style
                        else
                        {
                            RunInfo.Name = TEXT("SyntaxHighlight.Normal");
                            TextBlockStyle = GetSyntaxTextStyle().NormalTextStyle;
                        }
                    }
                    // Default literal style
                    else
                    {
                        RunInfo.Name = TEXT("SyntaxHighlight.Normal");
                        TextBlockStyle = GetSyntaxTextStyle().NormalTextStyle;
                    }
                }
                // Fallback for when tokenizer cast fails
                else
                {
                    RunInfo.Name = TEXT("SyntaxHighlight.Normal");
                    TextBlockStyle = GetSyntaxTextStyle().NormalTextStyle;
                }
            }

            TSharedRef<ISlateRun> Run = FQCE_TextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange);
            Runs.Add(Run);
        }

        LinesToAdd.Add(FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs)));
    }

    TargetTextLayout.AddLines(LinesToAdd);
}

bool FCPPSyntaxHighlighterMarshaller::IsCommentToken(const FString& SourceString, const ISyntaxTokenizer::FToken& Token) const
{
    const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
    
    // Don't treat string literals as comments, even if they contain comment-like text
    if (TokenText.StartsWith(TEXT("\"")) || TokenText.StartsWith(TEXT("'")))
    {
        return false;
    }
    
    // Single-line comments (must start with //)
    if (TokenText.StartsWith(TEXT("//")))
    {
        return true;
    }
    
    // Multiline comment start (must start with /*)
    if (TokenText.StartsWith(TEXT("/*")))
    {
        return true;
    }
    
    // Multiline comment end (must end with */ and not be a string)
    if (TokenText.EndsWith(TEXT("*/")) && !TokenText.StartsWith(TEXT("\"")))
    {
        return true;
    }
    
    // Check for middle lines of multiline comments
    // These typically start with whitespace followed by * (but not inside strings)
    if (!TokenText.StartsWith(TEXT("\"")) && !TokenText.StartsWith(TEXT("'")))
    {
        FString TrimmedToken = TokenText.TrimStart();
        if (TrimmedToken.StartsWith(TEXT("*")) && TrimmedToken.Len() > 1)
        {
            // Additional check: make sure this isn't a multiplication or pointer operation
            TCHAR SecondChar = TrimmedToken.Len() > 1 ? TrimmedToken[1] : 0;
            if (SecondChar == TEXT(' ') || SecondChar == TEXT('\t') || FChar::IsAlpha(SecondChar))
            {
                return true;
            }
        }
    }
    
    return false;
}

FCPPSyntaxHighlighterMarshaller::FSyntaxTextStyle FCPPSyntaxHighlighterMarshaller::GetSyntaxTextStyle()
{
    if (!bRefreshStyle && CurrentSyntaxStyle.bIsSet)
    {
        return CurrentSyntaxStyle;
    }
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    check(Settings);

    // Create the base font style with user settings
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(
        TEXT("Mono"), 
        Settings->FontSize
    );

    // Apply bold if enabled
    if (Settings->bUseBoldFont)
    {
        FontInfo.TypefaceFontName = TEXT("Bold");
    }

    CurrentSyntaxStyle = FSyntaxTextStyle(
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->TextColor), // Normal text
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->TextColor), // Operator text
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->KeywordColor), // Keywords
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->StringColor), // String text
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->NumberColor), // Number text
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->CommentColor), // Comment text
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->KeywordColor), // Preprocessor text - same as keywords
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->TypeColor), // Unreal typedefs
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->FunctionColor), // Function names
            FTextBlockStyle()
            .SetFont(FontInfo)
            .SetColorAndOpacity(Settings->ClassColor) // Class names
        );

    CurrentSyntaxStyle.bIsSet = true;
    bRefreshStyle = false;
    return CurrentSyntaxStyle;
        
}
