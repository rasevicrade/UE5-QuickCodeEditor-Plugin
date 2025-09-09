// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/Features/AI/Agents/QCE_AIClientFactory.h"

TMap<EQCEDefaultAIProvider, TUniquePtr<FQCE_GenericAIClient>> FQCE_AIClientFactory::ClientInstances;

FQCE_GenericAIClient& FQCE_AIClientFactory::GetClient(EQCEDefaultAIProvider Provider)
{
	if (!ClientInstances.Contains(Provider))
	{
		ClientInstances.Add(Provider, MakeUnique<FQCE_GenericAIClient>(Provider));
	}
	
	return *ClientInstances[Provider];
}

FQCE_GenericAIClient& FQCE_AIClientFactory::GetClaudeClient()
{
	return GetClient(EQCEDefaultAIProvider::Claude);
}

FQCE_GenericAIClient& FQCE_AIClientFactory::GetChatGPTClient()
{
	return GetClient(EQCEDefaultAIProvider::ChatGPT);
}