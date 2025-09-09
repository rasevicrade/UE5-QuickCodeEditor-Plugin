// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/Features/AI/Agents/QCE_GenericAIClient.h"
#include "Editor/Features/AI/Conversations/QCE_AIConversationTracker.h"
#include "HttpModule.h"
#include "Editor/CustomTextBox/InlineAISuggestion/Utils/InlineAISuggestionTypes.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Interfaces/IHttpResponse.h"
#include "Settings/UQCE_EditorSettings.h"

FQCE_GenericAIClient::FQCE_GenericAIClient(EQCEDefaultAIProvider InProvider)
	: Provider(InProvider)
{
	InitializeProviderConfig();
}

void FQCE_GenericAIClient::InitializeProviderConfig()
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		Config.ApiEndpoint = Settings ? Settings->ClaudeApiEndpoint : TEXT("https://api.anthropic.com/v1/messages");
		Config.AuthHeaderName = TEXT("x-api-key");
		Config.AuthHeaderPrefix = TEXT("");
		Config.ApiVersionHeader = TEXT("anthropic-version");
		Config.ApiVersionValue = TEXT("2023-06-01");
		Config.bSupportsTemperature = false;
		Config.bHasDetailedErrorHandling = false;
		break;
		
	case EQCEDefaultAIProvider::ChatGPT:
		Config.ApiEndpoint = Settings ? Settings->OpenAIApiEndpoint : TEXT("https://api.openai.com/v1/chat/completions");
		Config.AuthHeaderName = TEXT("Authorization");
		Config.AuthHeaderPrefix = TEXT("Bearer ");
		Config.ApiVersionHeader = TEXT("");
		Config.ApiVersionValue = TEXT("");
		Config.bSupportsTemperature = true;
		Config.bHasDetailedErrorHandling = true;
		break;
	}
}

FString FQCE_GenericAIClient::GetApiKey() const
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (!Settings)
	{
		return FString();
	}
	
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		return Settings->ClaudeApiKey;
	case EQCEDefaultAIProvider::ChatGPT:
		return Settings->OpenAIApiKey;
	default:
		return FString();
	}
}

FString FQCE_GenericAIClient::GetModelVersion() const
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (!Settings)
	{
		return FString();
	}
	
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		return Settings->ModelVersion;
	case EQCEDefaultAIProvider::ChatGPT:
		return Settings->OpenAIModelVersion;
	default:
		return FString();
	}
}

FString FQCE_GenericAIClient::GetAgentName() const
{
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		return TEXT("Claude");
	case EQCEDefaultAIProvider::ChatGPT:
		return TEXT("ChatGPT");
	default:
		return TEXT("Unknown");
	}
}

bool FQCE_GenericAIClient::IsAvailable() const
{
	return !GetApiKey().IsEmpty();
}

void FQCE_GenericAIClient::SetupRequestHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest)
{
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(*Config.AuthHeaderName, Config.AuthHeaderPrefix + GetApiKey());
	
	if (!Config.ApiVersionHeader.IsEmpty())
	{
		HttpRequest->SetHeader(*Config.ApiVersionHeader, *Config.ApiVersionValue);
	}
}

void FQCE_GenericAIClient::SendMessage(const FString& ConversationKey, const FString& Message, const TFunction<void(const FString&, bool)>& OnComplete)
{
	FString ApiKey = GetApiKey();
	if (ApiKey.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("%s_API_KEY_MISSING"), *GetAgentName().ToUpper());
		OnComplete(ErrorMessage, false);
		return;
	}
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Config.ApiEndpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	SetupRequestHeaders(HttpRequest);
	TSharedPtr<FJsonObject> JsonPayload = CreateConversationPayload(ConversationKey);
	if (!JsonPayload.IsValid())
	{
		OnComplete(TEXT("Failed to create conversation payload"), false);
		return;
	}
	
	FString RequestString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	
	HttpRequest->SetContentAsString(RequestString);
	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FQCE_GenericAIClient::HandleResponse, OnComplete);
	HttpRequest->ProcessRequest();
}

void FQCE_GenericAIClient::GetCompletion(const FAICompletionContext& Request, const TFunction<void(const FString&, bool)>& OnComplete)
{
	if (Provider == EQCEDefaultAIProvider::Claude && Request.UserInput.IsEmpty() && Request.Context.CodeContexWithFillPosition.TrimStartAndEnd() == "<ins></ins>")
	{
		OnComplete(TEXT("Not enough context to do anything."), false);
		return;
	}
	
	FString ApiKey = GetApiKey();
	if (ApiKey.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("%s_API_KEY_MISSING"), *GetAgentName().ToUpper());
		OnComplete(ErrorMessage, false);
		return;
	}
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Config.ApiEndpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	SetupRequestHeaders(HttpRequest);
	
	TSharedPtr<FJsonObject> JsonPayload = CreateCompletionPayload(Request);
	if (!JsonPayload.IsValid())
	{
		OnComplete(TEXT("Failed to create completion payload"), false);
		return;
	}
	
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (Settings)
	{
		const int32 MaxTokens = Settings->SimpleQueryMaxTokens;
		JsonPayload->SetNumberField(TEXT("max_tokens"), MaxTokens);
	}
	
	FString RequestString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	
	HttpRequest->SetContentAsString(RequestString);
	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FQCE_GenericAIClient::HandleResponse, OnComplete);
	HttpRequest->ProcessRequest();
}

TSharedPtr<FJsonObject> FQCE_GenericAIClient::CreateConversationPayload(const FString& ConversationKey)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (!Settings)
	{
		return nullptr;
	}
	
	FQCEAIConversation* Conversation = QCE_AIConversationTracker::Get().FindConversation(ConversationKey);
	if (!Conversation)
	{
		return nullptr;
	}
	
	TSharedPtr<FJsonObject> JsonPayload = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonObject>> ApiMessages;
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		ApiMessages = Conversation->GetClaudeAPIMessages();
		break;
	case EQCEDefaultAIProvider::ChatGPT:
		ApiMessages = Conversation->GetOpenAIAPIMessages();
		break;
	}
	
	TArray<TSharedPtr<FJsonValue>> MessageValues;
	for (const TSharedPtr<FJsonObject>& MessageObj : ApiMessages)
	{
		MessageValues.Add(MakeShared<FJsonValueObject>(MessageObj));
	}
	
	JsonPayload->SetStringField(TEXT("model"), GetModelVersion());
	JsonPayload->SetArrayField(TEXT("messages"), MessageValues);
	
	const int32 ConversationLength = ApiMessages.Num() > 0 ? ApiMessages.Num() - 1 : 0;
	const int32 MaxTokens = ConversationLength <= 2 ? 
		Settings->SimpleQueryMaxTokens : Settings->RegularMaxTokens;
	
	JsonPayload->SetNumberField(TEXT("max_tokens"), MaxTokens);
	if (Config.bSupportsTemperature)
	{
		JsonPayload->SetNumberField(TEXT("temperature"), 0.7);
	}
	
	return JsonPayload;
}

TSharedPtr<FJsonObject> FQCE_GenericAIClient::CreateCompletionPayload(const FAICompletionContext& Request)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	if (!Settings)
	{
		return nullptr;
	}
	
	TSharedPtr<FJsonObject> JsonPayload = MakeShared<FJsonObject>();
	FString TextBoxTypeInstruction;
	if (Request.TextBoxType == ETextBoxType::Declaration)
	{
		TextBoxTypeInstruction = TEXT("This is a declaration context - return only function declarations without implementation bodies.");
	}
	else if (Request.TextBoxType == ETextBoxType::Implementation && Provider == EQCEDefaultAIProvider::ChatGPT)
	{
		TextBoxTypeInstruction = TEXT("This is an implementation context - return complete function implementations with bodies.");
	}
	else if (Provider == EQCEDefaultAIProvider::ChatGPT)
	{
		TextBoxTypeInstruction = TEXT("This is a standard context - return appropriate code completion.");
	}
	
	FString CompletionPrompt = FString::Printf(
		TEXT("Complete the following %s code. The <ins></ins> marker shows where to insert the completion. Only return the completion text, no explanations or formatting. %s %s TODO: %s \n \n\n```%s```"),
		*Request.Context.Language,
		Request.SuggestionScope == ESuggestionScope::Line ? TEXT("Only finish current line.") : TEXT("Add one or more lines."),
		*TextBoxTypeInstruction,
		*Request.UserInput,
		*Request.Context.CodeContexWithFillPosition
	);
	
	TArray<TSharedPtr<FJsonValue>> Messages;
	TSharedPtr<FJsonObject> MessageObj = MakeShared<FJsonObject>();
	MessageObj->SetStringField(TEXT("role"), TEXT("user"));
	MessageObj->SetStringField(TEXT("content"), CompletionPrompt);
	Messages.Add(MakeShared<FJsonValueObject>(MessageObj));
	
	JsonPayload->SetStringField(TEXT("model"), GetModelVersion());
	JsonPayload->SetArrayField(TEXT("messages"), Messages);
	
	if (Config.bSupportsTemperature)
	{
		JsonPayload->SetNumberField(TEXT("temperature"), Request.Temperature);
	}
	
	return JsonPayload;
}

void FQCE_GenericAIClient::HandleResponse(TSharedPtr<IHttpRequest> HttpRequest, TSharedPtr<IHttpResponse> Response,
	bool bWasSuccessful, TFunction<void(const FString&, bool)> OnComplete)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		FString ErrorMessage = FString::Printf(TEXT("Failed to connect to %s API"), *GetAgentName());
		OnComplete(ErrorMessage, false);
		return;
	}
	
	const FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		FString ErrorMessage = FString::Printf(TEXT("Failed to parse %s response"), *GetAgentName());
		OnComplete(ErrorMessage, false);
		return;
	}
	
	FString Content;
	FString Error;
	if (ParseResponseContent(JsonResponse, Content, Error))
	{
		OnComplete(Content, true);
	}
	else
	{
		OnComplete(Error, false);
	}
}

bool FQCE_GenericAIClient::ParseResponseContent(TSharedPtr<FJsonObject> JsonResponse, FString& OutContent, FString& OutError)
{
	switch (Provider)
	{
	case EQCEDefaultAIProvider::Claude:
		{
			const TArray<TSharedPtr<FJsonValue>>* ContentArray;
			if (JsonResponse->TryGetArrayField(TEXT("content"), ContentArray) && ContentArray->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* ContentObject;
				if ((*ContentArray)[0]->TryGetObject(ContentObject))
				{
					if ((*ContentObject)->TryGetStringField(TEXT("text"), OutContent))
					{
						return true;
					}
				}
			}
			OutError = TEXT("Invalid response format from Claude. Make sure the provided key is valid.");
			return false;
		}
		
	case EQCEDefaultAIProvider::ChatGPT:
		{
			const TSharedPtr<FJsonObject>* ErrorObject;
			if (JsonResponse->TryGetObjectField(TEXT("error"), ErrorObject))
			{
				FString ErrorType;
				FString ErrorMessage;
				
				(*ErrorObject)->TryGetStringField(TEXT("type"), ErrorType);
				(*ErrorObject)->TryGetStringField(TEXT("message"), ErrorMessage);
				if (ErrorType == TEXT("insufficient_quota"))
				{
					OutError = TEXT("OpenAI quota exceeded. Please check your plan and billing details at https://platform.openai.com/account/billing");
					return false;
				}
				
				if (!ErrorMessage.IsEmpty())
				{
					OutError = FString::Printf(TEXT("OpenAI API Error: %s"), *ErrorMessage);
					return false;
				}
				
				OutError = TEXT("Unknown error from OpenAI API");
				return false;
			}
			
			const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
			if (JsonResponse->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* ChoiceObject;
				if ((*ChoicesArray)[0]->TryGetObject(ChoiceObject))
				{
					const TSharedPtr<FJsonObject>* MessageObject;
					if ((*ChoiceObject)->TryGetObjectField(TEXT("message"), MessageObject))
					{
						if ((*MessageObject)->TryGetStringField(TEXT("content"), OutContent))
						{
							return true;
						}
					}
				}
			}
			OutError = TEXT("Invalid response format from OpenAI. Make sure the provided key is valid.");
			return false;
		}
		
	default:
		OutError = TEXT("Unknown AI provider");
		return false;
	}
}