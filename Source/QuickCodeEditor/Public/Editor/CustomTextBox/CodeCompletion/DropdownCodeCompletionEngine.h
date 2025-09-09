// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Utils/CodeCompletionContext.h"
#include "CompletionProviders/ICompletionProvider.h"
#include "UObject/Object.h"

class UMainEditorContainer;

/**
 * Main entry point for dropdown code completion (eg. methods/properties available on a type)
 * This class should be called to get completion suggestions.
 */
class QUICKCODEEDITOR_API FDropdownCodeCompletionEngine 
{
public:
	/** Register available ICompletionProviders. */
	void Initialize();
    
	/** Returns completion suggestions for the given code and cursor position. */
	TArray<FCompletionItem> GetCompletions(const FString& Code, int32 CursorPosition, const FString& HeaderText = FString(), const FString& ImplementationText = FString(), UMainEditorContainer* MainEditorContainer = nullptr);
	
public:
	FDropdownCodeCompletionEngine() = default;
	~FDropdownCodeCompletionEngine();
	
	FDropdownCodeCompletionEngine(const FDropdownCodeCompletionEngine&) = delete;
	FDropdownCodeCompletionEngine& operator=(const FDropdownCodeCompletionEngine&) = delete;
	
private:
	/** Registers a completion provider with the engine. */
	void RegisterProvider(ICompletionProvider* Provider);
    
	/** Merges completion results from all providers and sorts by relevance. */
	TArray<FCompletionItem> MergeAndSort(const TArray<TArray<FCompletionItem>>& AllCompletions);

	/** Container for different completion providers. */
	TArray<TUniquePtr<ICompletionProvider>> CompletionProviders;

	/** True if completion providers have been registered for the engine. */
	bool bIsInitialized = false;
};
