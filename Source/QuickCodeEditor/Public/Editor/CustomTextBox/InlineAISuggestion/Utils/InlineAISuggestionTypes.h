#pragma once

#include "CoreMinimal.h"

enum class ETextBoxType;

/** Types of AI code completions */
enum class ESuggestionScope : uint8
{
	Line,
	Block
};

struct FCodeContext
{
	FString Language = TEXT("Unreal Engine 5/C++"); // Always Unreal C++
	FString CodeContexWithFillPosition; // E.g. float MyVar<FillPosition> float SecondVar
};

struct FAICompletionContext
{
	FCodeContext Context;
	FString UserInput;
	float Temperature;// - Read from user settings
	ESuggestionScope SuggestionScope = ESuggestionScope::Line;
	ETextBoxType TextBoxType;
	
};

struct FCompletionResponse
{
	FString CompletionText;
};
