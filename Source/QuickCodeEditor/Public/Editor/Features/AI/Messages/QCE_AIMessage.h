// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Represents a parsed content block that can be either text or code
 */
struct FQCEMessageContentBlock
{
	enum class EBlockType
	{
		Text,
		CodeBlock
	};
	
	EBlockType Type;
	FString Content;
	FString Language; // For code blocks, language identifier (e.g., "cpp")
	
	FQCEMessageContentBlock(EBlockType InType, const FString& InContent, const FString& InLanguage = TEXT(""))
		: Type(InType), Content(InContent), Language(InLanguage)
	{
	}
};

/**
 * QCE_AIMessage represents a single message bubble in the AI conversation interface.
 * This widget is responsible for:
 * - Displaying individual message content with appropriate styling
 * - Managing message alignment based on sender (user vs AI)
 * - Supporting both editable and read-only modes
 * - Applying consistent visual styling using AIMessageStyle constants
 * - Rendering code blocks with syntax highlighting
 */
class QUICKCODEEDITOR_API QCE_AIMessage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(QCE_AIMessage)
			: _Message()
			  , _IsUserMessage(false)
			  , _IsReadOnly(true)
		{
		}

		/** The message text to display in the bubble */
		SLATE_ATTRIBUTE(FText, Message)
		/** Whether this is a user message (right-aligned) or AI response (left-aligned) */
		SLATE_ARGUMENT(bool, IsUserMessage)
		/** Whether the message should be read-only (uses STextBlock) or editable (uses SMultiLineEditableText) */
		SLATE_ARGUMENT(bool, IsReadOnly)
	SLATE_END_ARGS()

	/** 
	 * Constructs and initializes the message bubble widget
	 * @param InArgs - The Slate argument structure containing message content and alignment settings
	 */
	void Construct(const FArguments& InArgs);

private:
	/**
	 * Parses message text and extracts code blocks and regular text blocks
	 * @param MessageText - The raw message text to parse
	 * @return Array of content blocks (text and code blocks)
	 */
	TArray<FQCEMessageContentBlock> ParseMessageContent(const FString& MessageText);
	
	/**
	 * Creates a widget for displaying syntax-highlighted code
	 * @param CodeText - The code content to highlight
	 * @param Language - The programming language for syntax highlighting
	 * @param bIsReadOnly - Whether the code should be read-only
	 * @return Widget containing syntax-highlighted code
	 */
	TSharedRef<SWidget> CreateCodeBlockWidget(const FString& CodeText, const FString& Language, bool bIsReadOnly);
	
	/**
	 * Creates a widget for displaying regular text content
	 * @param TextContent - The text content to display
	 * @param bIsReadOnly - Whether the text should be read-only
	 * @return Widget containing the text
	 */
	TSharedRef<SWidget> CreateTextWidget(const FString& TextContent, bool bIsReadOnly);
};
