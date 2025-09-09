// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/InlineAISuggestion/FInlineAISuggestionEngine.h"
#include "Editor/Features/AI/Agents/IQCE_AIAgent.h"
#include "Editor/Features/AI/Agents/QCE_AIClientFactory.h"
#include "Settings/UQCE_EditorSettings.h"

FInlineAISuggestionEngine& FInlineAISuggestionEngine::Get()
{
	static FInlineAISuggestionEngine Instance;
	return Instance;
}

void FInlineAISuggestionEngine::RequestCompletion(const FUserInputContext& InUserInput, const FOnCompletionReceived& OnComplete) const
{
	if (!IsAvailable())
	{
		FCompletionResponse Response;
		Response.CompletionText = TEXT("AI suggestion engine not available");
		OnComplete.ExecuteIfBound(Response, false);
		return;
	}

	// Get the appropriate AI agent based on settings
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	IQCE_AIAgent* Agent = nullptr;

	if (Settings)
	{
		Agent = &FQCE_AIClientFactory::GetClient(Settings->InlineSuggestionsAIProvider);
	}

	if (!Agent || !Agent->IsAvailable())
	{
		FCompletionResponse Response;
		Response.CompletionText = TEXT("Selected AI provider not available");
		OnComplete.ExecuteIfBound(Response, false);
		return;
	}

	const FAICompletionContext CompletionContext = BuildAICompletionContext(InUserInput);
	Agent->GetCompletion(CompletionContext, [this, OnComplete](const FString& Response, bool bSuccess)
	{
		HandleAgentResponse(Response, bSuccess, OnComplete);
	});
}

bool FInlineAISuggestionEngine::IsAvailable() const
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (!Settings)
	{
		return false;
	}

	// Check if at least one AI provider is available
	bool bClaudeAvailable = FQCE_AIClientFactory::GetClaudeClient().IsAvailable();
	bool bChatGPTAvailable = FQCE_AIClientFactory::GetChatGPTClient().IsAvailable();

	return bClaudeAvailable || bChatGPTAvailable;
}

FAICompletionContext FInlineAISuggestionEngine::BuildAICompletionContext(const FUserInputContext& UserInputContext)
{
	FAICompletionContext LLMContext;
	// Set up the context
	LLMContext.Context.CodeContexWithFillPosition = UserInputContext.Code;
	LLMContext.UserInput = UserInputContext.UserInput;
	LLMContext.SuggestionScope = UserInputContext.CompletionType == EQCEDefaultCompletionType::Block ? ESuggestionScope::Block : ESuggestionScope::Line;
	LLMContext.TextBoxType = UserInputContext.TextBoxType;
	
	// Set default AI parameters (since no specific inline suggestion settings exist)
	LLMContext.Context.Language = TEXT("Unreal Engine 5/C++");
	LLMContext.Temperature = 0.2f; // Lower temperature for more focused code completions
	
	return LLMContext;
}

void FInlineAISuggestionEngine::HandleAgentResponse(const FString& Response, bool bSuccess, const FOnCompletionReceived& OnComplete) const
{
	FCompletionResponse CompletionResponse;
	
	// Remove cpp code fences if present
	FString CleanedResponse = Response;
	CleanedResponse = CleanedResponse.TrimStartAndEnd();
	
	// Remove opening ```cpp fence
	if (CleanedResponse.StartsWith(TEXT("```cpp")))
	{
		CleanedResponse = CleanedResponse.Mid(6); // Remove "```cpp"
		CleanedResponse = CleanedResponse.TrimStart(); // Remove any leading whitespace after the fence
	}
	
	// Remove closing ``` fence
	if (CleanedResponse.EndsWith(TEXT("```")))
	{
		CleanedResponse = CleanedResponse.Left(CleanedResponse.Len() - 3); // Remove trailing "```"
		CleanedResponse = CleanedResponse.TrimEnd(); // Remove any trailing whitespace before the fence
	}
	
	CompletionResponse.CompletionText = CleanedResponse;
	
	OnComplete.ExecuteIfBound(CompletionResponse, bSuccess);
}