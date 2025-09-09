// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/TrieCompletion/FTrieCompletion.h"
#include "Misc/Char.h"

void FTrieCompletion::InsertWord(const FString& Word)
{
	TSharedPtr<FTrieNode> Current = Root;
	for (TCHAR Ch : Word)
	{
		TCHAR LowerCh = FChar::ToLower(Ch);
		if (!Current->Children.Contains(LowerCh))
		{
			Current->Children.Add(LowerCh, MakeShareable(new FTrieNode()));
		}
		Current = Current->Children[LowerCh];
	}
	Current->bIsEndOfWord = true;
	Current->Completions.Add(Word);
}


TArray<FString> FTrieCompletion::FindCompletions(const FString& Prefix)
{
	TArray<FString> Results;
    
	// Handle empty prefix case
	if (Prefix.IsEmpty())
	{
		// Collect all words in the trie
		CollectAllCompletions(Root, Results);
		return Results;
	}
    
	// Navigate to the prefix node using lowercase characters
	TSharedPtr<FTrieNode> Current = Root;
	for (TCHAR Ch : Prefix)
	{
		TCHAR LowerCh = FChar::ToLower(Ch);
		if (!Current->Children.Contains(LowerCh))
		{
			// Prefix doesn't exist in trie
			return Results; // Empty array
		}
		Current = Current->Children[LowerCh];
	}
    
	// Collect all completions from this prefix node
	CollectAllCompletions(Current, Results);
    
	return Results;
}

void FTrieCompletion::CollectAllCompletions(TSharedPtr<FTrieNode> Node, TArray<FString>& Results)
{
	if (!Node.IsValid())
	{
		return;
	}
    
	// Add all completions stored at this node
	Results.Append(Node->Completions);
    
	// Recursively collect from all children
	for (auto& ChildPair : Node->Children)
	{
		CollectAllCompletions(ChildPair.Value, Results);
	}
}
