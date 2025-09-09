// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QCE_GenericAIClient.h"
#include "Settings/UQCE_EditorSettings.h"

/**
 * Factory class for creating and managing AI client instances.
 * 
 * This factory provides centralized access to AI clients for different providers
 * (Claude, ChatGPT, etc.) and manages their lifecycle through a singleton pattern.
 * Each provider type has a single instance that is created on first access.
 */
class QUICKCODEEDITOR_API FQCE_AIClientFactory
{
public:
	/** 
	 * Gets the AI client for the specified provider.
	 * Creates a new instance if one doesn't exist for this provider.
	 * 
	 * @param Provider The AI provider type to get a client for
	 * @return Reference to the AI client instance for the specified provider
	 */
	static FQCE_GenericAIClient& GetClient(EQCEDefaultAIProvider Provider);

	/** 
	 * Gets the Claude AI client (backward compatibility).
	 * Equivalent to calling GetClient(EQCEDefaultAIProvider::Claude).
	 * 
	 * @return Reference to the Claude AI client instance
	 */
	static FQCE_GenericAIClient& GetClaudeClient();

	/** 
	 * Gets the ChatGPT AI client (backward compatibility).
	 * Equivalent to calling GetClient(EQCEDefaultAIProvider::ChatGPT).
	 * 
	 * @return Reference to the ChatGPT AI client instance
	 */
	static FQCE_GenericAIClient& GetChatGPTClient();

private:
	/** Map of AI provider types to their respective client instances */
	static TMap<EQCEDefaultAIProvider, TUniquePtr<FQCE_GenericAIClient>> ClientInstances;
};