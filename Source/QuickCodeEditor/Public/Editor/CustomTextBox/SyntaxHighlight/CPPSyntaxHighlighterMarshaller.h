// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/SyntaxTokenizer.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/SlateTypes.h"

/**
 * Syntax highlighting for Unreal C++ text
 */
class QUICKCODEEDITOR_API FCPPSyntaxHighlighterMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
    /**
     * Container for all syntax highlighting styles used by the C++ marshaller.
     */
    struct FSyntaxTextStyle
    {
        /**
         * Default constructor for uninitialized style
         */
        FSyntaxTextStyle(bool newIsSet) : bIsSet(newIsSet)
        {
            
        }
        
        FSyntaxTextStyle(
            const FTextBlockStyle& InNormalTextStyle,
            const FTextBlockStyle& InOperatorTextStyle,
            const FTextBlockStyle& InKeywordTextStyle,
            const FTextBlockStyle& InStringTextStyle,
            const FTextBlockStyle& InNumberTextStyle,
            const FTextBlockStyle& InCommentTextStyle,
            const FTextBlockStyle& InPreProcessorKeywordTextStyle,
            const FTextBlockStyle& InUnrealTypedefTextStyle,
            const FTextBlockStyle& InFunctionTextStyle,
            const FTextBlockStyle& InClassTextStyle
        ) :
            NormalTextStyle(InNormalTextStyle),
            OperatorTextStyle(InOperatorTextStyle),
            KeywordTextStyle(InKeywordTextStyle),
            StringTextStyle(InStringTextStyle),
            NumberTextStyle(InNumberTextStyle),
            CommentTextStyle(InCommentTextStyle),
            PreProcessorKeywordTextStyle(InPreProcessorKeywordTextStyle),
            UnrealTypedefTextStyle(InUnrealTypedefTextStyle),
            FunctionTextStyle(InFunctionTextStyle),
            ClassTextStyle(InClassTextStyle), bIsSet(false)
        {
        }

        FTextBlockStyle NormalTextStyle;
        
        FTextBlockStyle OperatorTextStyle;
        
        FTextBlockStyle KeywordTextStyle;
        
        FTextBlockStyle StringTextStyle;
        
        FTextBlockStyle NumberTextStyle;
        
        FTextBlockStyle CommentTextStyle;
        
        FTextBlockStyle PreProcessorKeywordTextStyle;
        
        FTextBlockStyle UnrealTypedefTextStyle;
        
        FTextBlockStyle FunctionTextStyle;
        
        FTextBlockStyle ClassTextStyle;

        bool bIsSet;
    };
    
public:
    /**
     * Creates a new instance of the CPP syntax highlighter marshaller
     */
    static TSharedRef<FCPPSyntaxHighlighterMarshaller> Create();

    void SetHighlighterEnabled(const bool& bShouldEnable) { bShouldApplyHighlights = bShouldEnable; }

protected:
    /**
     * Parses the source string into tokens and applies them to the text layout
     */
    virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

private:
    /**
     * Constructor for the CPP syntax highlighter marshaller
     */
    FCPPSyntaxHighlighterMarshaller(TSharedPtr<ISyntaxTokenizer> InTokenizer);

    /**
     * Retrieves the current syntax text style configuration
     */
    FSyntaxTextStyle GetSyntaxTextStyle();

    /**
     * Determines if a token represents a comment based on proper context detection
     */
    bool IsCommentToken(const FString& SourceString, const ISyntaxTokenizer::FToken& Token) const;

    FSyntaxTextStyle CurrentSyntaxStyle;
    bool bRefreshStyle = false;
    bool bShouldApplyHighlights = false;
};