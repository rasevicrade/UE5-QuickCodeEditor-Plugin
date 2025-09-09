// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Utils/InlineAISuggestionTypes.h"

// Forward declaration to avoid naming conflicts
struct FAgentCompletionRequest;

DECLARE_DELEGATE_TwoParams(FOnCompletionReceived, const FCompletionResponse&, bool /* bSuccess */);

class QUICKCODEEDITOR_API InlineAISuggestionEngine
{
public:
	static InlineAISuggestionEngine& Get();

	void RequestCompletion(const FCompletionRequest& Request, const FOnCompletionReceived& OnComplete);

	bool IsAvailable() const;

private:
	InlineAISuggestionEngine() = default;

	FAgentCompletionRequest ConvertToAgentRequest(const FCompletionRequest& InRequest) const;

	void HandleAgentResponse(const FString& Response, bool bSuccess, const FOnCompletionReceived& OnComplete) const;
};
