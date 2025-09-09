// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CodeCompletionContext.h"

/**
 * Interface that should be implemented by different completion providers,
 * for example static symbol table provider and unreal reflection provider
 */
class QUICKCODEEDITOR_API ICompletionProvider
{
public:
	virtual ~ICompletionProvider() = default;
	
	virtual TArray<FCompletionItem> GetCompletions(const FCompletionContext& Context) = 0;
	
	virtual int32 GetPriority() const = 0;
	
	virtual bool CanHandleContext(const FCompletionContext& Context) const = 0;
};
