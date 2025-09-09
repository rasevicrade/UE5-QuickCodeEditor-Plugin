// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/QCE_InlineAISuggestionBox.h"
#include "Utils/InlineAISuggestionTypes.h"

/** Delegate called when AI completion request finishes */
DECLARE_DELEGATE_TwoParams(FOnCompletionReceived, const FCompletionResponse&, bool /* bSuccess */);

/**
 * Singleton engine for handling inline AI code completion suggestions.
 * Provides context-aware code completions by analyzing user input and cursor position.
 */
class QUICKCODEEDITOR_API FInlineAISuggestionEngine
{
public:
	/** Get singleton instance */
	static FInlineAISuggestionEngine& Get();

	/** Request code completion based on the current context */
	void RequestCompletion(const FUserInputContext& InUserInput, const FOnCompletionReceived& OnComplete) const;

	/** Check if the engine is available and properly configured */
	bool IsAvailable() const;

private:
	FInlineAISuggestionEngine() = default;

	/** Returns CurrentCodeContext with <completion_location></completion_location> at cursor position */
	static FAICompletionContext BuildAICompletionContext(const FUserInputContext& UserInputContext);
	
	/** Handle response from AI agent and convert back to our format */
	void HandleAgentResponse(const FString& Response, bool bSuccess, const FOnCompletionReceived& OnComplete) const;
};
