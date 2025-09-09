// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/KeywordCompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Interfaces/IPluginManager.h"

void FKeywordCompletionProvider::Initialize()
{
	if (!InitializePluginPaths())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize plugin paths for KeywordCompletionProvider"));
		return;
	}

	LoadCommonKeywordsFromConfig();
	BuildCommonKeywordTrie();
	LoadClassMethodsFromFile();
	bIsInitialized = true;
}

TArray<FCompletionItem> FKeywordCompletionProvider::GetCompletions(const FCompletionContext& Context)
{
	if (!bIsInitialized)
		Initialize();

	// First try to get keywords from class context
	TArray<FCompletionItem> Result;
	if (TryGetClassMethodCompletions(Result, Context))
		return Result;

	TryGetCommonKeywordCompletions(Result, Context);
	return Result;
}

#pragma region Common keywords

void FKeywordCompletionProvider::TryGetCommonKeywordCompletions(TArray<FCompletionItem>& OutResult,
	const FCompletionContext& Context)
{
	// If we couldn't get keywords from class context, get generic keywords
	FString CurrentToken = ExtractCurrentToken(Context);
	if (CurrentToken.Len() < 1)
	{
		OutResult = TArray<FCompletionItem>();
	}

	TArray<FString> Results = CommonKeywordTrieCompletion.FindCompletions(CurrentToken);
	OutResult.Reserve(Results.Num());
	
	for (const FString& Info : Results)
	{
		FCompletionItem NewItem;
		NewItem.DisplayText = Info;
		NewItem.InsertText = Info;
		OutResult.Add(NewItem);
	}
}

void FKeywordCompletionProvider::LoadCommonKeywordsFromConfig()
{
	// Find all JSON files in the Keywords directory
	TArray<FString> JsonFiles;
	IFileManager& FileManager = IFileManager::Get();
	FString SearchPattern = FPaths::Combine(KeywordsDir, TEXT("*.json"));
	FileManager.FindFiles(JsonFiles, *SearchPattern, true, false);
	JsonFiles.Sort();

	int32 LoadedFiles = 0;

	for (const FString& FileName : JsonFiles)
	{
		FString FullPath = FPaths::Combine(KeywordsDir, FileName);
		UE_LOG(LogTemp, Log, TEXT("Processing keyword file: %s"), *FileName);
		
		// Load and validate the file
		if (LoadKeywordsFromValidatedFile(FullPath))
		{
			LoadedFiles++;
		}
	}
}

void FKeywordCompletionProvider::LoadKeywordsFromFile(const FString& FilePath)
{
	LoadKeywordsFromValidatedFile(FilePath);
}

bool FKeywordCompletionProvider::LoadKeywordsFromValidatedFile(const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Keyword config file not found: %s"), *FilePath);
		return false;
	}

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load keyword config file: %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON in keyword config file: %s"), *FilePath);
		return false;
	}

	// Validate the JSON structure
	if (!IsValidKeywordFile(JsonObject, FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping invalid keyword file: %s"), *FilePath);
		return false;
	}

	// Get the keywords object (we know it's valid from validation)
	const TSharedPtr<FJsonObject>* KeywordsObject;
	JsonObject->TryGetObjectField(TEXT("keywords"), KeywordsObject);

	int32 KeywordsLoadedFromFile = 0;

	// Iterate through all keyword categories
	for (const auto& CategoryPair : (*KeywordsObject)->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* KeywordArray;
		if (CategoryPair.Value->TryGetArray(KeywordArray))
		{
			for (const TSharedPtr<FJsonValue>& KeywordValue : *KeywordArray)
			{
				FString Keyword;
				if (KeywordValue->TryGetString(Keyword))
				{
					CommonKeywordDatabase.Add(Keyword);
					KeywordsLoadedFromFile++;
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Successfully loaded %d keywords from file: %s"), KeywordsLoadedFromFile, *FilePath);
	return true;
}

bool FKeywordCompletionProvider::IsValidKeywordFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath) const
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid JSON object in file: %s"), *FilePath);
		return false;
	}

	// Check for required "keywords" field
	const TSharedPtr<FJsonObject>* KeywordsObject;
	if (!JsonObject->TryGetObjectField(TEXT("keywords"), KeywordsObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing 'keywords' field in file: %s"), *FilePath);
		return false;
	}

	if (!KeywordsObject->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid 'keywords' object in file: %s"), *FilePath);
		return false;
	}

	// Validate that all categories contain arrays of strings
	for (const auto& CategoryPair : (*KeywordsObject)->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* KeywordArray;
		if (!CategoryPair.Value->TryGetArray(KeywordArray))
		{
			UE_LOG(LogTemp, Warning, TEXT("Category '%s' is not an array in file: %s"), *CategoryPair.Key, *FilePath);
			return false;
		}

		// Check that array contains valid strings
		for (const TSharedPtr<FJsonValue>& KeywordValue : *KeywordArray)
		{
			FString Keyword;
			if (!KeywordValue->TryGetString(Keyword))
			{
				UE_LOG(LogTemp, Warning, TEXT("Non-string keyword found in category '%s' in file: %s"), *CategoryPair.Key, *FilePath);
				return false;
			}

			if (Keyword.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("Empty keyword found in category '%s' in file: %s"), *CategoryPair.Key, *FilePath);
				return false;
			}
		}
	}

	// Log optional metadata if present
	FString Description;
	if (JsonObject->TryGetStringField(TEXT("description"), Description))
	{
		UE_LOG(LogTemp, Log, TEXT("Keyword file description: %s (from %s)"), *Description, *FilePath);
	}

	FString Version;
	if (JsonObject->TryGetStringField(TEXT("version"), Version))
	{
		UE_LOG(LogTemp, Log, TEXT("Keyword file version: %s (from %s)"), *Version, *FilePath);
	}

	return true;
}

void FKeywordCompletionProvider::BuildCommonKeywordTrie()
{
	for (const FString& KeyWord : CommonKeywordDatabase)
	{
		CommonKeywordTrieCompletion.InsertWord(KeyWord);
	}
}

FString FKeywordCompletionProvider::ExtractCurrentToken(const FCompletionContext& Context) const
{
	const FString& PrecedingText = Context.PrecedingText;
    
	if (PrecedingText.IsEmpty())
	{
		return TEXT("");
	}
    
	// Find the start of the current token
	int32 TokenStart = PrecedingText.Len() - 1;
    
	while (TokenStart >= 0)
	{
		TCHAR Ch = PrecedingText[TokenStart];
        
		if (!FChar::IsAlnum(Ch) && Ch != TEXT('_') && Ch != TEXT('#'))
		{
			break;
		}
		TokenStart--;
	}
    
	TokenStart++; // Move to first character of token
    
	return PrecedingText.Mid(TokenStart);
}

#pragma endregion

#pragma region Class methods

TArray<FCompletionItem> FKeywordCompletionProvider::GetClassMethodCompletions(const FDeclarationContext& DeclarationCtx) const
{
	TArray<FCompletionItem> Items;
	
	if (DeclarationCtx.AccessType != EAccessType::StaticAccess || DeclarationCtx.VariableName.IsEmpty())
	{
		return Items;
	}

	// Find methods for this class using the variable name (which contains the class name for static access)
	const TArray<FClassMethod>* ClassMethods = ClassMethodsData.ClassMethods.Find(DeclarationCtx.VariableName);
	if (!ClassMethods)
	{
		return Items;
	}

	const FString& CurrentToken = DeclarationCtx.CurrentToken;

	// Filter methods based on current token
	for (const FClassMethod& Method : *ClassMethods)
	{
		if (CurrentToken.IsEmpty() || Method.MethodName.StartsWith(CurrentToken))
		{
			FCompletionItem NewItem;
			NewItem.DisplayText = Method.MethodName; // Show full signature
			NewItem.InsertText = Method.MethodSignature + ";";        // Insert just the method name
			NewItem.Score = Method.MethodName.StartsWith(CurrentToken) ? 100 : 50;
			Items.Add(NewItem);
		}
	}

	// Sort by score (higher score first)
	Items.Sort([](const FCompletionItem& A, const FCompletionItem& B)
	{
		return A.Score > B.Score;
	});

	return Items;
}

bool FKeywordCompletionProvider::TryGetClassMethodCompletions(TArray<FCompletionItem>& OutResult, const FCompletionContext& Context)
{
	FDeclarationContext DeclarationCtx = FCompletionContextUtils::ParseDeclarationContext(Context);
	if (DeclarationCtx.AccessType == EAccessType::StaticAccess)
	{
		OutResult = GetClassMethodCompletions(DeclarationCtx);
		if (OutResult.Num() > 0)
		{
			return true;
		}
	}
	return false;
}
bool FKeywordCompletionProvider::LoadClassMethodsFromFile()
{
	FString FilePath = FPaths::Combine(KeywordsDir, TEXT("UnrealClassKeywords.json"));
	if (!FPaths::FileExists(FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Class methods file not found: %s"), *FilePath);
		return false;
	}

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load class methods file: %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON in class methods file: %s"), *FilePath);
		return false;
	}

	// Validate the JSON structure
	if (!IsValidClassMethodsFile(JsonObject, FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid class methods file structure: %s"), *FilePath);
		return false;
	}

	// Reset previous data
	ClassMethodsData.Reset();

	// Load metadata
	JsonObject->TryGetStringField(TEXT("description"), ClassMethodsData.Description);
	JsonObject->TryGetStringField(TEXT("version"), ClassMethodsData.Version);

	// Load class methods
	const TSharedPtr<FJsonObject>* ClassMethodsObject;
	if (JsonObject->TryGetObjectField(TEXT("class_methods"), ClassMethodsObject))
	{
		int32 TotalMethodsLoaded = 0;

		for (const auto& ClassPair : (*ClassMethodsObject)->Values)
		{
			const FString& ClassName = ClassPair.Key;
			const TArray<TSharedPtr<FJsonValue>>* MethodsArray;
			
			if (ClassPair.Value->TryGetArray(MethodsArray))
			{
				TArray<FClassMethod> ClassMethods;
				ClassMethods.Reserve(MethodsArray->Num());

				for (const TSharedPtr<FJsonValue>& MethodValue : *MethodsArray)
				{
					const TSharedPtr<FJsonObject>* MethodObject;
					if (MethodValue->TryGetObject(MethodObject))
					{
						FString MethodName, MethodSignature;
						if ((*MethodObject)->TryGetStringField(TEXT("MethodName"), MethodName) &&
							(*MethodObject)->TryGetStringField(TEXT("MethodSignature"), MethodSignature))
						{
							FClassMethod Method(MethodName, MethodSignature);
							if (Method.IsValid())
							{
								ClassMethods.Add(Method);
								TotalMethodsLoaded++;
							}
						}
					}
				}

				if (ClassMethods.Num() > 0)
				{
					ClassMethodsData.ClassMethods.Add(ClassName, ClassMethods);
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Loaded %d methods for %d classes from: %s"), 
			TotalMethodsLoaded, ClassMethodsData.ClassMethods.Num(), *FilePath);
	}

	return ClassMethodsData.IsValid();
}

bool FKeywordCompletionProvider::IsValidClassMethodsFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath) const
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid JSON object in class methods file: %s"), *FilePath);
		return false;
	}

	// Check for required "class_methods" field
	const TSharedPtr<FJsonObject>* ClassMethodsObject;
	if (!JsonObject->TryGetObjectField(TEXT("class_methods"), ClassMethodsObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing 'class_methods' field in file: %s"), *FilePath);
		return false;
	}

	if (!ClassMethodsObject->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid 'class_methods' object in file: %s"), *FilePath);
		return false;
	}

	// Validate structure - each class should have an array of method objects
	for (const auto& ClassPair : (*ClassMethodsObject)->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* MethodsArray;
		if (!ClassPair.Value->TryGetArray(MethodsArray))
		{
			UE_LOG(LogTemp, Warning, TEXT("Class '%s' methods is not an array in file: %s"), *ClassPair.Key, *FilePath);
			return false;
		}

		// Validate method objects
		for (const TSharedPtr<FJsonValue>& MethodValue : *MethodsArray)
		{
			const TSharedPtr<FJsonObject>* MethodObject;
			if (!MethodValue->TryGetObject(MethodObject))
			{
				UE_LOG(LogTemp, Warning, TEXT("Method in class '%s' is not an object in file: %s"), *ClassPair.Key, *FilePath);
				return false;
			}

			FString MethodName, MethodSignature;
			if (!(*MethodObject)->TryGetStringField(TEXT("MethodName"), MethodName) ||
				!(*MethodObject)->TryGetStringField(TEXT("MethodSignature"), MethodSignature))
			{
				UE_LOG(LogTemp, Warning, TEXT("Method object missing required fields in class '%s' in file: %s"), *ClassPair.Key, *FilePath);
				return false;
			}

			if (MethodName.IsEmpty() || MethodSignature.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("Empty method name or signature in class '%s' in file: %s"), *ClassPair.Key, *FilePath);
				return false;
			}
		}
	}

	return true;
}


#pragma endregion

#pragma region Helpers

bool FKeywordCompletionProvider::CanHandleContext(const FCompletionContext& Context) const
{
	return true;
}

bool FKeywordCompletionProvider::InitializePluginPaths()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("QuickCodeEditor"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("QuickCodeEditor plugin not found for keyword loading"));
		return false;
	}

	PluginDir = Plugin->GetBaseDir();
	KeywordsDir = FPaths::Combine(PluginDir, TEXT("Config"), TEXT("Keywords"));
	if (!FPaths::DirectoryExists(KeywordsDir))
	{
		UE_LOG(LogTemp, Error, TEXT("Keywords directory not found: %s"), *KeywordsDir);
		return false;
	}

	return true;
}

#pragma endregion