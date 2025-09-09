// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Json.h"
#include "IQCE_AIAgent.h"
#include "Settings/UQCE_EditorSettings.h"


/**
 * Configuration settings for different AI provider endpoints
 * Contains provider-specific API configuration like endpoints, headers, and capabilities
 */
struct FAIProviderConfig
{
	/** The base URL endpoint for the AI provider's API */
	FString ApiEndpoint;
	
	/** Name of the authorization header (e.g. "Authorization", "x-api-key") */
	FString AuthHeaderName;
	
	/** Prefix for the authorization header value (e.g. "Bearer ", "sk-") */
	FString AuthHeaderPrefix;
	
	/** Header name for API version specification */
	FString ApiVersionHeader;
	
	/** API version value to send in requests */
	FString ApiVersionValue;
	
	/** Whether this provider supports temperature parameter for response randomness */
	bool bSupportsTemperature = false;
	
	/** Whether this provider returns detailed error information in responses */
	bool bHasDetailedErrorHandling = false;
};

/**
 * Generic AI client that can work with multiple AI providers (OpenAI, Anthropic, etc.)
 * Implements the IQCE_AIAgent interface to provide consistent AI functionality
 * across different provider endpoints and API formats.
 */
class QUICKCODEEDITOR_API FQCE_GenericAIClient : public IQCE_AIAgent
{
public:
	/**
	 * Constructor - initializes the client for a specific AI provider
	 * @param Provider The AI provider type to configure this client for
	 */
	FQCE_GenericAIClient(EQCEDefaultAIProvider Provider);
	
	/**
	 * Sends a message using conversation history and calls the callback with the response
	 * @param ConversationKey Unique identifier for the conversation thread
	 * @param Message The user message to send to the AI
	 * @param OnComplete Callback function called with (response, success) when request completes
	 */
	virtual void SendMessage(const FString& ConversationKey, const FString& Message, const TFunction<void(const FString&, bool)>& OnComplete) override;

	/**
	 * Gets code completion suggestion based on the request context
	 * @param Request Context information for the completion request (cursor position, surrounding code, etc.)
	 * @param OnComplete Callback function called with (completion_text, success) when request completes
	 */
	virtual void GetCompletion(const FAICompletionContext& Request, const TFunction<void(const FString&, bool)>& OnComplete) override;

	/**
	 * Returns the name/type of this AI agent
	 * @return Display name of the configured AI provider
	 */
	virtual FString GetAgentName() const override;

	/**
	 * Returns whether this agent is currently available/configured
	 * @return true if API key is configured and provider settings are valid
	 */
	virtual bool IsAvailable() const override;

private:
	/** The AI provider type this client is configured for */
	EQCEDefaultAIProvider Provider;
	
	/** Provider-specific configuration (endpoints, headers, capabilities) */
	FAIProviderConfig Config;

	/**
	 * Initializes provider-specific configuration based on the Provider type
	 * Sets up API endpoints, authentication headers, and provider capabilities
	 */
	void InitializeProviderConfig();

	/**
	 * Gets the API key for the current provider from editor settings
	 * @return The configured API key, or empty string if not set
	 */
	FString GetApiKey() const;

	/**
	 * Gets the model version/name for the current provider from editor settings
	 * @return The configured model identifier (e.g. "gpt-4", "claude-3-sonnet")
	 */
	FString GetModelVersion() const;

	/**
	 * Handles the HTTP response from the AI provider API
	 * Parses the response and extracts the AI-generated content
	 * @param HttpRequest The original HTTP request object
	 * @param HttpResponse The HTTP response received from the provider
	 * @param bWasSuccessful Whether the HTTP request succeeded
	 * @param OnComplete Callback to invoke with the parsed response
	 */
	void HandleResponse(TSharedPtr<IHttpRequest> HttpRequest, TSharedPtr<IHttpResponse> HttpResponse, bool bWasSuccessful, TFunction<void(const FString&, bool)> OnComplete);

	/**
	 * Creates the JSON payload for conversational API requests
	 * Includes conversation history and the new message in provider-specific format
	 * @param ConversationKey Identifier for the conversation thread
	 * @return JSON object ready to be sent to the AI provider
	 */
	TSharedPtr<FJsonObject> CreateConversationPayload(const FString& ConversationKey);

	/**
	 * Creates the JSON payload for code completion requests
	 * Formats the completion context in provider-specific format
	 * @param Request Context information for the completion (code, cursor position, etc.)
	 * @return JSON object ready to be sent to the AI provider
	 */
	TSharedPtr<FJsonObject> CreateCompletionPayload(const FAICompletionContext& Request);

	/**
	 * Parses response content from the AI provider's JSON response
	 * Extracts the generated text and any error information
	 * @param JsonResponse The parsed JSON response from the provider
	 * @param OutContent [out] The extracted AI-generated content
	 * @param OutError [out] Any error message from the response
	 * @return true if parsing succeeded, false if there was an error
	 */
	bool ParseResponseContent(TSharedPtr<FJsonObject> JsonResponse, FString& OutContent, FString& OutError);

	/**
	 * Sets up HTTP request headers required by the AI provider
	 * Adds authentication, content type, API version, and other required headers
	 * @param HttpRequest The HTTP request to configure
	 */
	void SetupRequestHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest);
};