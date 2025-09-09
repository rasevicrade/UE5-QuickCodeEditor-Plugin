// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/Features/AI/Conversations/QCE_AIConversationTracker.h"
#include "Misc/Paths.h"
#include "Json.h"
#include "Settings/UQCE_EditorSettings.h"

QCE_AIConversationTracker& QCE_AIConversationTracker::Get()
{
	static QCE_AIConversationTracker Instance;
	return Instance;
}

FQCEAIConversation& QCE_AIConversationTracker::GetOrCreateConversation(const FString& FunctionName, const FString& ClassName, const FString& FilePath)
{
	const FString ConversationKey = GenerateConversationKey(FunctionName, ClassName, FilePath);
	for (FQCEAIConversation& Conversation : Conversations)
	{
		if (Conversation.ConversationKey == ConversationKey)
		{
			Conversation.LastAccessed = FDateTime::Now();
			return Conversation;
		}
	}
	
	FQCEAIConversation NewConversation(ConversationKey, FunctionName, ClassName, FilePath);
	Conversations.Add(NewConversation);
	
	return Conversations.Last();
}

void QCE_AIConversationTracker::SetFunctionContextToConversation(const FString& ConversationKey, const FString& FunctionContent)
{
	for (FQCEAIConversation& Conversation : Conversations)
	{
		if (Conversation.ConversationKey == ConversationKey)
		{
			const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
			FString SystemInstructions = Settings ? Settings->SystemInstructions : TEXT("");
			
			if (!SystemInstructions.IsEmpty() && !SystemInstructions.EndsWith(TEXT("\n\n")))
			{
				SystemInstructions += TEXT("\n\n");
			}
			
			TArray<FString> Lines;
			FunctionContent.ParseIntoArrayLines(Lines, false);
			FString NumberedContent;
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				NumberedContent += FString::Printf(TEXT("%3d | %s\n"), i + 1, *Lines[i]);
			}
			
			FString FullContext = SystemInstructions + FString::Printf(TEXT("```cpp\n%s```"), *NumberedContent);
			FQCEConversationMessage ContextMessage(EQCEMessageType::FunctionContext, FullContext, false);
			if (Conversation.Messages.Num() > 0)
			{
				Conversation.Messages[0] = ContextMessage;
			}
			else
			{
				Conversation.Messages.Insert(ContextMessage, 0);
			}
			Conversation.bHasFunctionContext = true;
			Conversation.LastAccessed = FDateTime::Now();
			break;
		}
	}
}

void QCE_AIConversationTracker::AddMessageToConversation(const FString& ConversationKey, EQCEMessageType MessageType, const FString& MessageText)
{
	for (FQCEAIConversation& Conversation : Conversations)
	{
		if (Conversation.ConversationKey == ConversationKey)
		{
			FQCEConversationMessage NewMessage(MessageType, MessageText);
			Conversation.Messages.Add(NewMessage);
			Conversation.LastAccessed = FDateTime::Now();
			break;
		}
	}
}

FQCEAIConversation* QCE_AIConversationTracker::FindConversation(const FString& ConversationKey)
{
	for (FQCEAIConversation& Conversation : Conversations)
	{
		if (Conversation.ConversationKey == ConversationKey)
		{
			return &Conversation;
		}
	}
	return nullptr;
}

void QCE_AIConversationTracker::ClearAllConversations()
{
	Conversations.Empty();
}

FString QCE_AIConversationTracker::GenerateConversationKey(const FString& FunctionName, const FString& ClassName, const FString& FilePath)
{
	const FString CleanFilePath = FPaths::GetCleanFilename(FilePath);
	return FString::Printf(TEXT("%s::%s@%s"), *ClassName, *FunctionName, *CleanFilePath);
}

TArray<TSharedPtr<FJsonObject>> FQCEAIConversation::GetClaudeAPIMessages() const
{
	TArray<TSharedPtr<FJsonObject>> ApiMessages;
	if (Messages.Num() > 0 && Messages[0].MessageType == EQCEMessageType::FunctionContext)
	{
		TSharedPtr<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), Messages[0].Role);
		MessageObj->SetStringField(TEXT("content"), Messages[0].Content);
		ApiMessages.Add(MessageObj);
	}
	
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	const int32 MaxHistory = Settings ? Settings->MaxHistoryMessages : 5;
	int32 StartIdx = FMath::Max(1, Messages.Num() - MaxHistory);
	
	for (int32 i = StartIdx; i < Messages.Num(); i++)
	{
		TSharedPtr<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), Messages[i].Role);
		MessageObj->SetStringField(TEXT("content"), Messages[i].Content);
		ApiMessages.Add(MessageObj);
	}
	
	return ApiMessages;
}

TArray<TSharedPtr<FJsonObject>> FQCEAIConversation::GetOpenAIAPIMessages() const
{
	TArray<TSharedPtr<FJsonObject>> ApiMessages;
	
	if (Messages.Num() > 0 && Messages[0].MessageType == EQCEMessageType::FunctionContext)
	{
		TSharedPtr<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), TEXT("system"));
		MessageObj->SetStringField(TEXT("content"), Messages[0].Content);
		ApiMessages.Add(MessageObj);
	}
	
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	const int32 MaxHistory = Settings ? Settings->MaxHistoryMessages : 5;
	int32 StartIdx = FMath::Max(1, Messages.Num() - MaxHistory);
	
	for (int32 i = StartIdx; i < Messages.Num(); i++)
	{
		TSharedPtr<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), Messages[i].Role);
		MessageObj->SetStringField(TEXT("content"), Messages[i].Content);
		ApiMessages.Add(MessageObj);
	}
	
	return ApiMessages;
}
