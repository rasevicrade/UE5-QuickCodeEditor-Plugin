// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

struct FTrieNode
{
	TMap<TCHAR, TSharedPtr<FTrieNode>> Children;
	TArray<FString> Completions; // Store complete words at this node
	bool bIsEndOfWord = false;
};

/**
 *  tree-like data structure used to store and efficiently search strings,
 *  particularly useful for prefix-based operations.
 *  It's also called a "prefix tree" because each node represents a common prefix shared by multiple strings.
 */
class FTrieCompletion
{
public:
	FTrieCompletion()
	{
		Root = MakeShareable(new FTrieNode());
	}
	
	void InsertWord(const FString& Word);

	TArray<FString> FindCompletions(const FString& Prefix);

private:
	static void CollectAllCompletions(TSharedPtr<FTrieNode> Node, TArray<FString>& Results);
	
	TSharedPtr<FTrieNode> Root;
};
