// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/ReflectionCompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Engine/Engine.h"

bool FReflectionCompletionProvider::CanHandleContext(const FCompletionContext& Context) const
{
	FDeclarationContext DeclarationCtx = FCompletionContextUtils::ParseDeclarationContext(Context);
	return DeclarationCtx.AccessType != EAccessType::None;
}

TArray<FCompletionItem> FReflectionCompletionProvider::GetCompletions(const FCompletionContext& Context)
{
	TArray<FCompletionItem> Completions;
	
	// Stage 1: Prepare FDeclarationContext
	FDeclarationContext DeclarationCtx = FCompletionContextUtils::ParseDeclarationContext(Context);
	if (DeclarationCtx.AccessType == EAccessType::None)
	{
		return Completions;
	}
	
	// Stage 2: Resolve Type
	UStruct* ResolvedType = nullptr;
	
	if (DeclarationCtx.AccessType == EAccessType::StaticAccess)
	{
		ResolvedType = FCompletionContextUtils::GetTypeByClassName(DeclarationCtx.ClassName);
	}
	else if (DeclarationCtx.AccessType == EAccessType::PointerAccess || 
			 DeclarationCtx.AccessType == EAccessType::ReferenceAccess)
	{
		if (!DeclarationCtx.ClassName.IsEmpty())
		{
			ResolvedType = FCompletionContextUtils::GetTypeByClassName(DeclarationCtx.ClassName);
		}
	}
	
	if (!ResolvedType)
	{
		return Completions;
	}
	
	// Stage 3: Get Available Members with Hierarchy
	Completions = GetMembersForResolvedType(ResolvedType, DeclarationCtx);
	
	return Completions;
}


TArray<FCompletionItem> FReflectionCompletionProvider::GetMembersForResolvedType(UStruct* ResolvedType, const FDeclarationContext& DeclarationCtx) const
{
	TArray<FCompletionItem> Completions;
	
	if (!ResolvedType)
	{
		return Completions;
	}
	
	CollectDirectMembers(ResolvedType, DeclarationCtx.AccessType, DeclarationCtx.CurrentToken, Completions);
	CollectInheritedMembers(ResolvedType, DeclarationCtx.AccessType, DeclarationCtx.CurrentToken, Completions);
	
	return Completions;
}

void FReflectionCompletionProvider::CollectDirectMembers(UStruct* Struct, EAccessType AccessType, const FString& Filter, TArray<FCompletionItem>& OutCompletions) const
{
	if (!Struct)
	{
		return;
	}
	
	// Collect functions
	for (TFieldIterator<UFunction> FuncIt(Struct, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
	{
		const UFunction* Function = *FuncIt;
		
		if (Function && ShouldIncludeFunction(Function, AccessType))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Function->GetName(), Filter))
			{
				FCompletionItem Item = CreateFunctionCompletion(Function);
				OutCompletions.Add(Item);
			}
		}
	}
	
	// Collect properties
	for (TFieldIterator<FProperty> PropIt(Struct, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		
		if (Property && ShouldIncludeProperty(Property, AccessType))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Property->GetName(), Filter))
			{
				FCompletionItem Item = CreatePropertyCompletion(Property);
				OutCompletions.Add(Item);
			}
		}
	}
}

void FReflectionCompletionProvider::CollectInheritedMembers(UStruct* Struct, EAccessType AccessType, const FString& Filter, TArray<FCompletionItem>& OutCompletions) const
{
	if (!Struct)
	{
		return;
	}
	
	// Track names of direct members to avoid duplicates
	TSet<FString> DirectMemberNames;
	
	// First pass: collect names of direct members
	for (TFieldIterator<UFunction> FuncIt(Struct, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
	{
		const UFunction* Function = *FuncIt;
		if (Function)
		{
			DirectMemberNames.Add(Function->GetName());
		}
	}
	
	for (TFieldIterator<FProperty> PropIt(Struct, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		if (Property)
		{
			DirectMemberNames.Add(Property->GetName());
		}
	}
	
	// Second pass: collect inherited functions, skipping direct member names
	for (TFieldIterator<UFunction> FuncIt(Struct, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
	{
		const UFunction* Function = *FuncIt;
		
		// Skip direct members - we already processed them
		if (Function && Function->GetOwnerStruct() == Struct)
		{
			continue;
		}
		
		// Skip if we already have a direct member with this name (overrides)
		if (Function && DirectMemberNames.Contains(Function->GetName()))
		{
			continue;
		}
		
		if (Function && ShouldIncludeFunction(Function, AccessType))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Function->GetName(), Filter))
			{
				FCompletionItem Item = CreateFunctionCompletion(Function);
				// Lower priority for inherited members
				Item.Score -= 10;
				OutCompletions.Add(Item);
			}
		}
	}
	
	// Collect inherited properties, skipping direct member names
	for (TFieldIterator<FProperty> PropIt(Struct, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		
		// Skip direct members - we already processed them
		if (Property && Property->GetOwnerStruct() == Struct)
		{
			continue;
		}
		
		// Skip if we already have a direct member with this name (overrides)
		if (Property && DirectMemberNames.Contains(Property->GetName()))
		{
			continue;
		}
		
		if (Property && ShouldIncludeProperty(Property, AccessType))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Property->GetName(), Filter))
			{
				FCompletionItem Item = CreatePropertyCompletion(Property);
				// Lower priority for inherited members
				Item.Score -= 10;
				OutCompletions.Add(Item);
			}
		}
	}
}

bool FReflectionCompletionProvider::ShouldIncludeFunction(const UFunction* Function, EAccessType AccessType) const
{
	if (!Function)
	{
		return false;
	}
	
	if (AccessType == EAccessType::StaticAccess)
	{
		// For static access (UClass::), only include static functions
		return Function->HasAnyFunctionFlags(FUNC_Static);
	}
	else if (AccessType == EAccessType::PointerAccess || AccessType == EAccessType::ReferenceAccess)
	{
		// For instance access (MyPointer-> or MyRef.), exclude static functions
		return !Function->HasAnyFunctionFlags(FUNC_Static);
	}
	
	return false;
}

bool FReflectionCompletionProvider::ShouldIncludeProperty(const FProperty* Property, EAccessType AccessType) const
{
	if (!Property)
	{
		return false;
	}
	
	// Only include public properties
	if (!Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
	{
		return false;
	}
	
	if (AccessType == EAccessType::StaticAccess)
	{
		// For static access, typically we don't show instance properties
		// unless they are static/const class members
		// In UE, most static members are functions rather than properties
		return false;
	}
	else if (AccessType == EAccessType::PointerAccess || AccessType == EAccessType::ReferenceAccess)
	{
		// For instance access, include instance properties
		return true;
	}
	
	return false;
}



TArray<FCompletionItem> FReflectionCompletionProvider::GetStaticCompletions(UStruct* Struct, const FString& Filter) const
{
	TArray<FCompletionItem> Completions;
	
	if (!Struct)
	{
		return Completions;
	}
	
	for (TFieldIterator<UFunction> FuncIt(Struct); FuncIt; ++FuncIt)
	{
		const UFunction* Function = *FuncIt;
		
		if (Function && Function->HasAnyFunctionFlags(FUNC_Static))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Function->GetName(), Filter))
			{
				FCompletionItem Item = CreateFunctionCompletion(Function);
				Completions.Add(Item);
			}
		}
	}
	
	for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		
		if (Property && Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Property->GetName(), Filter))
			{
				FCompletionItem Item = CreatePropertyCompletion(Property);
				Completions.Add(Item);
			}
		}
	}
	
	return Completions;
}

TArray<FCompletionItem> FReflectionCompletionProvider::GetInstanceCompletions(UStruct* Struct, const FString& Filter) const
{
	TArray<FCompletionItem> Completions;
	
	if (!Struct)
	{
		return Completions;
	}
	
	for (TFieldIterator<UFunction> FuncIt(Struct); FuncIt; ++FuncIt)
	{
		const UFunction* Function = *FuncIt;
		
		if (Function && !Function->HasAnyFunctionFlags(FUNC_Static))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Function->GetName(), Filter))
			{
				FCompletionItem Item = CreateFunctionCompletion(Function);
				Completions.Add(Item);
			}
		}
	}
	
	for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
	{
		const FProperty* Property = *PropIt;
		
		if (Property && Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic))
		{
			if (FCompletionContextUtils::MatchesCompletionFilter(Property->GetName(), Filter))
			{
				FCompletionItem Item = CreatePropertyCompletion(Property);
				Completions.Add(Item);
			}
		}
	}
	
	return Completions;
}

FCompletionItem FReflectionCompletionProvider::CreatePropertyCompletion(const FProperty* Property) const
{
	FCompletionItem Item;
	
	if (!Property)
	{
		return Item;
	}
	
	Item.DisplayText = Property->GetName();
	Item.InsertText = Property->GetName() + ";";
	Item.Score = 100; 
	
	return Item;
}

FCompletionItem FReflectionCompletionProvider::CreateFunctionCompletion(const UFunction* Function) const
{
	FCompletionItem Item;
	
	if (!Function)
	{
		return Item;
	}
	
	FString FunctionName = Function->GetName();
	FString FunctionSignature = BuildFunctionSignature(Function);
	
	Item.DisplayText = FunctionName;
	Item.InsertText = FunctionSignature + ";";
	Item.Score = 120;
	
	return Item;
}

FString FReflectionCompletionProvider::BuildFunctionSignature(const UFunction* Function) const
{
	if (!Function)
	{
		return FString();
	}
	
	FString FunctionName = Function->GetName();
	FString Signature = FunctionName + TEXT("(");
	
	TArray<FString> ParameterStrings;
	
	// Iterate through function parameters
	for (TFieldIterator<FProperty> ParamIt(Function); ParamIt && (ParamIt->PropertyFlags & CPF_Parm); ++ParamIt)
	{
		const FProperty* Param = *ParamIt;
		
		// Skip return value parameters
		if (Param->PropertyFlags & CPF_ReturnParm)
		{
			continue;
		}
		
		// Get parameter type and name
		FString ParamType = Param->GetCPPType();
		FString ParamName = Param->GetName();
		
		// Build parameter string
		FString ParamString = ParamType;
		if (!ParamName.IsEmpty())
		{
			ParamString += TEXT(" ") + ParamName;
		}
		
		ParameterStrings.Add(ParamString);
	}
	
	// Join parameters with commas
	Signature += FString::Join(ParameterStrings, TEXT(", "));
	Signature += TEXT(")");
	
	return Signature;
}

