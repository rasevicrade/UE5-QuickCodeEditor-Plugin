// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/CodeCompletion/DropdownCodeCompletionEngine.h"

#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/ICompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CodeCompletionContext.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CompletionContextUtils.h"
#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/KeywordCompletionProvider.h"
#include "Editor/CustomTextBox/CodeCompletion/CompletionProviders/ReflectionCompletionProvider.h"


void FDropdownCodeCompletionEngine::Initialize()
{
	RegisterProvider(new FKeywordCompletionProvider());
    RegisterProvider(new FReflectionCompletionProvider());
	bIsInitialized = true;
}

void FDropdownCodeCompletionEngine::RegisterProvider(ICompletionProvider* Provider)
{
	CompletionProviders.Emplace(TUniquePtr<ICompletionProvider>(Provider));
}

TArray<FCompletionItem> FDropdownCodeCompletionEngine::GetCompletions(const FString& Code, const int32 CursorPosition, const FString& HeaderText, const FString& ImplementationText, UMainEditorContainer* MainEditorContainer)
{
	if (!bIsInitialized)
		Initialize();

    FCompletionContext Context = FCompletionContextUtils::BuildContext(Code, CursorPosition, HeaderText, ImplementationText, MainEditorContainer);

    TArray<TArray<FCompletionItem>> AllCompletions;

    for (const auto& Provider : CompletionProviders)
    {
        if (Provider && Provider->CanHandleContext(Context))
        {
            TArray<FCompletionItem> ProviderResults = Provider->GetCompletions(Context);
            if (ProviderResults.Num() > 0)
            {
                AllCompletions.Add(ProviderResults);
            }
        }
    }
    
	// Merge and sort all gathered completions
	TArray<FCompletionItem> MergedResults = MergeAndSort(AllCompletions);
	
	// If no completions were found, add an informational entry
	if (MergedResults.Num() == 0)
	{
		FCompletionItem NoCompletionsItem;
		NoCompletionsItem.DisplayText = TEXT("No completions available");
		NoCompletionsItem.InsertText = TEXT(""); // Empty insert text
		NoCompletionsItem.Score = 0;
		NoCompletionsItem.bSelectable = false; // Cannot be selected
		MergedResults.Add(NoCompletionsItem);
	}
	
	return MergedResults;
}

TArray<FCompletionItem> FDropdownCodeCompletionEngine::MergeAndSort(const TArray<TArray<FCompletionItem>>& AllCompletions)
{
    TArray<FCompletionItem> MergedResults;
    
    // Merge all completion arrays
    for (const auto& CompletionArray : AllCompletions)
    {
        MergedResults.Append(CompletionArray);
    }
    
    // Remove duplicates based on DisplayText
    TSet<FString> SeenItems;
    for (int32 i = MergedResults.Num() - 1; i >= 0; --i)
    {
        if (SeenItems.Contains(MergedResults[i].DisplayText))
        {
            MergedResults.RemoveAt(i);
        }
        else
        {
            SeenItems.Add(MergedResults[i].DisplayText);
        }
    }
    
    // Sort by score first, then alphabetically
    MergedResults.Sort([](const FCompletionItem& A, const FCompletionItem& B)
    {
        if (A.Score != B.Score)
        {
            return A.Score > B.Score; // Higher score first
        }
        return A.DisplayText < B.DisplayText; // Alphabetical for same score
    });
    
    return MergedResults;
}

FDropdownCodeCompletionEngine::~FDropdownCodeCompletionEngine()
{
    CompletionProviders.Empty();
}