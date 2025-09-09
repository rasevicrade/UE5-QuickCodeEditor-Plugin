// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/ICompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

/**
 * Completion provider that analyzes UE reflection data to offer completions for class members,
 * properties, and functions based on the context (e.g., FClass::, MyPointer->).
 */
class QUICKCODEEDITOR_API FReflectionCompletionProvider : public ICompletionProvider
{
public:
	virtual ~FReflectionCompletionProvider() = default;
	
	virtual TArray<FCompletionItem> GetCompletions(const FCompletionContext& Context) override;
	virtual int32 GetPriority() const override { return 150; }
	virtual bool CanHandleContext(const FCompletionContext& Context) const override;

private:
	// Three-stage completion methods
	TArray<FCompletionItem> GetMembersForResolvedType(UStruct* ResolvedType, const FDeclarationContext& DeclarationCtx) const;
	
	TArray<FCompletionItem> GetStaticCompletions(UStruct* Struct, const FString& Filter) const;
	TArray<FCompletionItem> GetInstanceCompletions(UStruct* Struct, const FString& Filter) const;
	
	// Member collection helper methods
	void CollectDirectMembers(UStruct* Struct, EAccessType AccessType, const FString& Filter, TArray<FCompletionItem>& OutCompletions) const;
	void CollectInheritedMembers(UStruct* Struct, EAccessType AccessType, const FString& Filter, TArray<FCompletionItem>& OutCompletions) const;
	bool ShouldIncludeFunction(const UFunction* Function, EAccessType AccessType) const;
	bool ShouldIncludeProperty(const FProperty* Property, EAccessType AccessType) const;
	
	FCompletionItem CreatePropertyCompletion(const FProperty* Property) const;
	FCompletionItem CreateFunctionCompletion(const UFunction* Function) const;
	FString BuildFunctionSignature(const UFunction* Function) const;
};
