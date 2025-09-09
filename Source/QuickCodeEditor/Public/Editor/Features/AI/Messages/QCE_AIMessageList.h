// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;
struct FQCEAIConversation;

/**
 * Namespace containing style constants for AI message display.
 */
namespace AIMessageStyle
{
	const FMargin MessagePadding(8.0f);
	const FMargin ContainerPadding(16.0f, 4.0f);
	const float MessageMaxWidth = 800.0f;
    
	const FLinearColor UserMessageColor(0.15f, 0.2f, 0.25f);
	const FLinearColor AIMessageColor(0.2f, 0.2f, 0.2f);
}

/**
 * QCE_AIMessageList manages the display of AI conversation messages in the code editor.
 * This widget is responsible for:
 * - Displaying user messages and AI responses in a scrollable list
 * - Managing message alignment (user messages right-aligned, AI responses left-aligned)
 * - Handling loading states during AI responses
 * - Loading and displaying existing conversations
 */
class QUICKCODEEDITOR_API QCE_AIMessageList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(QCE_AIMessageList)
	{}
	SLATE_END_ARGS()

	/** Constructs and initializes the widget */
	void Construct(const FArguments& InArgs);

	/** 
	 * Adds a user message to the conversation.
	 * @param Message - The text content of the user's message
	 */
	void AddUserMessage(const FText& Message) const;

	/** 
	 * Adds an AI response to the conversation.
	 * @param Message - The text content of the AI's response
	 */
	void AddAIResponse(const FText& Message) const;

	/** 
	 * Adds a special AI message for API key configuration with a settings link.
	 */
	void AddAPIKeyConfigMessage() const;

	/** 
	 * Adds a loading indicator message while waiting for AI response.
	 * This indicator will be displayed until RemoveLoadingMessage is called.
	 */
	void AddLoadingMessage();

	/** 
	 * Removes the loading indicator message.
	 * Should be called after receiving an AI response, regardless of success.
	 */
	void RemoveLoadingMessage();

	/** 
	 * Loads an existing conversation into the message list.
	 * @param Conversation - The conversation data to load, containing message history
	 */
	void LoadConversation(const FQCEAIConversation& Conversation);

	/** 
	 * Clears all messages from the conversation.
	 * This resets the message list to its initial empty state.
	 */
	void ClearMessages();
private:
	/** The scroll box containing all messages */
	TSharedPtr<SScrollBox> MessageScrollBox;

	/** Reference to the current loading message slot, if any */
	TSharedPtr<SWidget> LoadingMessageWidget;
};
