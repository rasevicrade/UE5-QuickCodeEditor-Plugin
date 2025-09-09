// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/InlineAISuggestion/Utils/InlineAISuggestionTypes.h"

/**
 * Abstract interface for AI agents that provide code assistance and conversation capabilities.
 * 
 * This interface defines the contract for AI providers (ChatGPT, Claude, etc.) that can:
 * - Engage in conversation with context/history tracking
 * - Provide code completion suggestions
 * - Report their availability status
 * 
 * Implementations should handle network communication, API authentication, and response parsing.
 */
class QUICKCODEEDITOR_API IQCE_AIAgent
{
public:
	virtual ~IQCE_AIAgent() = default;

	/**
	 * Sends a message using conversation history and calls the callback with the response.
	 * 
	 * @param ConversationKey - Unique identifier for the conversation context (e.g., function name)
	 * @param Message - The message to send to the AI agent
	 * @param OnComplete - Callback function called when response is received
	 *                   - First parameter: The AI response text
	 *                   - Second parameter: Success flag (true if successful, false on error)
	 */
	virtual void SendMessage(const FString& ConversationKey, const FString& Message, const TFunction<void(const FString&, bool)>& OnComplete) = 0;

	/**
	 * Gets code completion suggestion based on the request context.
	 * 
	 * @param Request - Context information for the completion request including:
	 *                  - Current code context
	 *                  - Cursor position
	 *                  - Additional metadata
	 * @param OnComplete - Callback function called when completion is received
	 *                   - First parameter: The suggested code completion
	 *                   - Second parameter: Success flag (true if successful, false on error)
	 */
	virtual void GetCompletion(const FAICompletionContext& Request, const TFunction<void(const FString&, bool)>& OnComplete) = 0;

	/**
	 * Returns the name/type of this AI agent.
	 * 
	 * @return Human-readable name identifying the AI provider (e.g., "ChatGPT", "Claude")
	 */
	virtual FString GetAgentName() const = 0;

	/**
	 * Returns whether this agent is currently available/configured.
	 * 
	 * @return true if the agent has valid configuration and can make requests, false otherwise
	 */
	virtual bool IsAvailable() const = 0;
};
