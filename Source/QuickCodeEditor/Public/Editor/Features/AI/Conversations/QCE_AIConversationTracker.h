// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QCE_AIConversationTypes.h"

/**
 * Manages AI conversations about C++ functions in the code editor.
 * This class is responsible for:
 * - Creating and tracking conversations for specific functions
 * - Managing message history and context
 * - Formatting messages for the Claude API
 */
class QUICKCODEEDITOR_API QCE_AIConversationTracker
{
public:
	/** Get singleton instance */
	static QCE_AIConversationTracker& Get();

	/** 
	 * Gets or creates a conversation for the specified function.
	 * @param FunctionName - Name of the function
	 * @param ClassName - Name of the class containing the function
	 * @param FilePath - Path to the file containing the function
	 * @return Reference to the conversation
	 */
	FQCEAIConversation& GetOrCreateConversation(const FString& FunctionName, const FString& ClassName, const FString& FilePath);

	/** 
	 * Adds the function's code as context to a conversation.
	 * @param ConversationKey - Key of the conversation to add context to
	 * @param FunctionContent - The function's code content
	 * @param bRefreshContext
	 */
	void SetFunctionContextToConversation(const FString& ConversationKey, const FString& FunctionContent);

	/** 
	 * Adds a new message to an existing conversation.
	 * @param ConversationKey - Key of the conversation to add the message to
	 * @param MessageType - Type of message (User or Assistant)
	 * @param MessageText - Content of the message
	 */
	void AddMessageToConversation(const FString& ConversationKey, EQCEMessageType MessageType, const FString& MessageText);

	/** 
	 * Finds a conversation by its key.
	 * @param ConversationKey - Key of the conversation to find
	 * @return Pointer to the conversation if found, nullptr otherwise
	 */
	FQCEAIConversation* FindConversation(const FString& ConversationKey);

	/** Removes all conversations from memory */
	void ClearAllConversations();

private:
	/** All active conversations */
	TArray<FQCEAIConversation> Conversations;

	/** 
	 * Creates a unique key for a conversation based on function details.
	 * @param FunctionName - Name of the function
	 * @param ClassName - Name of the class containing the function
	 * @param FilePath - Path to the file containing the function
	 * @return Unique conversation key
	 */
	static FString GenerateConversationKey(const FString& FunctionName, const FString& ClassName, const FString& FilePath);

	/** Private constructor for singleton pattern */
	QCE_AIConversationTracker() = default;
};
