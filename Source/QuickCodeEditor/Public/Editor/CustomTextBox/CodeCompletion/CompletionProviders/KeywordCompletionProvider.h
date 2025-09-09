// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/ICompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "TrieCompletion/FTrieCompletion.h"

/**
 * Struct to represent a class method from UnrealClassKeywords.json
 */
struct QUICKCODEEDITOR_API FClassMethod
{
	FString MethodName;
	FString MethodSignature;

	FClassMethod() = default;
	FClassMethod(const FString& InMethodName, const FString& InMethodSignature)
		: MethodName(InMethodName), MethodSignature(InMethodSignature) {}

	bool IsValid() const
	{
		return !MethodName.IsEmpty() && !MethodSignature.IsEmpty();
	}
};

/**
 * Struct to represent class methods data loaded from UnrealClassKeywords.json
 */
struct QUICKCODEEDITOR_API FClassMethodsData
{
	FString Description;
	FString Version;
	TMap<FString, TArray<FClassMethod>> ClassMethods;

	bool IsValid() const
	{
		return !ClassMethods.IsEmpty();
	}

	void Reset()
	{
		Description.Empty();
		Version.Empty();
		ClassMethods.Empty();
	}
};

/**
 * Provides code completion for C++ keywords and Unreal class methods
 * Supports both common C++ keywords from config files and class-specific methods
 */
class QUICKCODEEDITOR_API FKeywordCompletionProvider : public ICompletionProvider
{
public:
	virtual TArray<FCompletionItem> GetCompletions(const FCompletionContext& Context) override;
	
	virtual int32 GetPriority() const override { return 100; }
	
	virtual bool CanHandleContext(const FCompletionContext& Context) const override;

private:
	/** Initializes the provider by loading keyword data and building completion structures */
	void Initialize();

#pragma region Common keywords
private:
	/** Adds common C++ keyword completions to the result array */
	void TryGetCommonKeywordCompletions(TArray<FCompletionItem>& OutResult, const FCompletionContext& Context);
	
	/** Loads common keywords from configuration files */
	void LoadCommonKeywordsFromConfig();
	
	void LoadKeywordsFromFile(const FString& FilePath);
	
	bool LoadKeywordsFromValidatedFile(const FString& FilePath);
	
	bool IsValidKeywordFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath) const;
	
	/** Builds the trie structure for efficient keyword completion matching */
	void BuildCommonKeywordTrie();
	
	/** Extracts the current token being typed for completion matching */
	FString ExtractCurrentToken(const FCompletionContext& Context) const;
	
	FTrieCompletion CommonKeywordTrieCompletion;
	
	TArray<FString> CommonKeywordDatabase;
#pragma endregion

#pragma region Class methods
private:
	/** Attempts to provide class method completions based on the current context */
	bool TryGetClassMethodCompletions(TArray<FCompletionItem>& OutResult, const FCompletionContext& Context);
	
	/** Loads Unreal class methods from UnrealClassKeywords.json */
	bool LoadClassMethodsFromFile();
	
	bool IsValidClassMethodsFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath) const;
	
	/** Gets completion items for methods of a specific class */
	TArray<FCompletionItem> GetClassMethodCompletions(const FDeclarationContext& DeclarationCtx) const;
	
	FClassMethodsData ClassMethodsData;
#pragma endregion

private:
	bool InitializePluginPaths();
	bool bIsInitialized = false;
	FString PluginDir, KeywordsDir;
};
