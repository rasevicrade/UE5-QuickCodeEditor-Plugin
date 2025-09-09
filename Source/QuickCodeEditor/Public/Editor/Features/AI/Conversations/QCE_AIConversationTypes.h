// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

/**
 * Defines the type of message in an AI conversation.
 */
enum class EQCEMessageType : uint8
{
	FunctionContext,  // Function content sent first, not displayed to user
	User,
	Assistant
};

/**
 * Represents a single message in an AI conversation.
 * Contains metadata about the message type, role, content and timestamp.
 */
struct QUICKCODEEDITOR_API FQCEConversationMessage
{
	/**
	 * Creates a new conversation message.
	 * @param InType - The type of message (FunctionContext, User, or Assistant)
	 * @param InContent - The content of the message
	 * @param bInDisplayInUI - Whether this message should be displayed in the UI
	 */
	FQCEConversationMessage(EQCEMessageType InType, const FString& InContent, bool bInDisplayInUI = true)
		: MessageType(InType), Content(InContent), Timestamp(FDateTime::Now()), bDisplayInUI(bInDisplayInUI)
	{
		// Set role based on message type
		switch (InType)
		{
		case EQCEMessageType::FunctionContext:
			Role = TEXT("user"); // Function context goes as user message in Claude API
			break;
		case EQCEMessageType::User:
			Role = TEXT("user");
			break;
		case EQCEMessageType::Assistant:
			Role = TEXT("assistant");
			break;
		}
	}
	
	/** Type of the message (FunctionContext, User, or Assistant) */
	EQCEMessageType MessageType = EQCEMessageType::User;
	
	/** Role in Claude API format ("user", "assistant", or "system") */
	FString Role;
	
	/** Actual content of the message */
	FString Content;
	
	/** When the message was created */
	FDateTime Timestamp;
	
	/** Whether this message should be shown in the UI */
	bool bDisplayInUI = true;
};

/**
 * Represents a complete AI conversation about a specific function.
 * Tracks all messages, function context, and metadata about the conversation.
 */
struct QUICKCODEEDITOR_API FQCEAIConversation
{
	/**
	 * Creates a new AI conversation.
	 * @param InKey - Unique identifier for this conversation
	 * @param InFunctionName - Name of the function this conversation is about
	 * @param InClassName - Name of the class containing the function
	 * @param InFilePath - Path to the file containing the function
	 */
	FQCEAIConversation(const FString& InKey, const FString& InFunctionName, const FString& InClassName, const FString& InFilePath)
		: ConversationKey(InKey), FunctionName(InFunctionName), ClassName(InClassName), FilePath(InFilePath), LastAccessed(FDateTime::Now()), bHasFunctionContext(false)
	{
	}

	/** Get messages formatted for the Claude API */
	TArray<TSharedPtr<FJsonObject>> GetClaudeAPIMessages() const;

	/** Get messages formatted for the OpenAI API */
	TArray<TSharedPtr<FJsonObject>> GetOpenAIAPIMessages() const;
	
	/** Unique identifier for this conversation */
	FString ConversationKey;
	
	/** Name of the function this conversation is about */
	FString FunctionName;
	
	/** Name of the class containing the function */
	FString ClassName;
	
	/** Path to the file containing the function */
	FString FilePath;
	
	/** All messages in this conversation */
	TArray<FQCEConversationMessage> Messages;
	
	/** When this conversation was last accessed */
	FDateTime LastAccessed;
	
	/** Whether the function's code context has been added to the conversation */
	bool bHasFunctionContext = false;
};

