// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/CodeCompletion/UI/QCE_CodeCompletionSuggestionBox.h"

#include "Editor/CustomTextBox/CodeCompletion/Utils/CodeCompletionContext.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/MainEditorContainer.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

void SQCE_CodeCompletionSuggestionBox::Construct(const FArguments& InArgs)
{
	MaxVisibleItems = InArgs._MaxVisibleItems;
	ItemHeight = InArgs._ItemHeight;
	OnCompletionSelected = InArgs._OnCompletionSelected;
	OnCompletionCancelled = InArgs._OnCompletionCancelled;

	// Create dummy suggestions
	FilteredSuggestions = AllSuggestions;

	// Create the suggestion list view
	SAssignNew(SuggestionListView, SListView<TSharedPtr<FCompletionItem>>)
		.ListItemsSource(&FilteredSuggestions)
		.OnGenerateRow(this, &SQCE_CodeCompletionSuggestionBox::OnGenerateRowForSuggestion)
		.OnSelectionChanged(this, &SQCE_CodeCompletionSuggestionBox::OnSuggestionSelectionChanged)
		.OnMouseButtonDoubleClick(this, &SQCE_CodeCompletionSuggestionBox::OnSuggestionDoubleClicked)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false);

	// Set initial selection to first selectable item
	if (FilteredSuggestions.Num() > 0)
	{
		TSharedPtr<FCompletionItem> FirstSelectableItem = FindFirstSelectableItem();
		if (FirstSelectableItem.IsValid())
		{
			SuggestionListView->SetSelection(FirstSelectableItem);
			SelectedSuggestion = FirstSelectableItem;
		}
	}

	// Main widget construction
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("Menu.Background"))
		.Padding(FMargin(2.0f))
		[
			SNew(SBox)
			.MaxDesiredHeight(ItemHeight * MaxVisibleItems)
			.MinDesiredWidth(200.0f)
			.MaxDesiredWidth(400.0f)
			[
				SuggestionListView.ToSharedRef()
			]
		]
	];

	
}

FReply SQCE_CodeCompletionSuggestionBox::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	if (SuggestionListView.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SuggestionListView, InFocusEvent.GetCause());
	}
	return SCompoundWidget::OnFocusReceived(MyGeometry, InFocusEvent);
}

void SQCE_CodeCompletionSuggestionBox::SetSuggestions(const TArray<TSharedPtr<FCompletionItem>>& InSuggestions)
{
	AllSuggestions = InSuggestions;
	FilteredSuggestions = InSuggestions;
	
	if (SuggestionListView.IsValid())
	{
		SuggestionListView->RequestListRefresh();
		
		// Select first selectable item if available
		if (FilteredSuggestions.Num() > 0)
		{
			TSharedPtr<FCompletionItem> FirstSelectableItem = FindFirstSelectableItem();
			if (FirstSelectableItem.IsValid())
			{
				SuggestionListView->SetSelection(FirstSelectableItem);
				SelectedSuggestion = FirstSelectableItem;
			}
		}
	}
}

void SQCE_CodeCompletionSuggestionBox::SelectNextSuggestion()
{
	if (!SuggestionListView.IsValid() || FilteredSuggestions.Num() == 0)
	{
		return;
	}

	int32 CurrentIndex = FilteredSuggestions.IndexOfByKey(SelectedSuggestion);
	int32 NextIndex = CurrentIndex;
	
	// Find next selectable item
	do
	{
		NextIndex = (NextIndex + 1) % FilteredSuggestions.Num();
		if (FilteredSuggestions[NextIndex]->bSelectable)
		{
			break;
		}
	} while (NextIndex != CurrentIndex);
	
	// Only update if we found a different selectable item
	if (NextIndex != CurrentIndex && FilteredSuggestions[NextIndex]->bSelectable)
	{
		SuggestionListView->SetSelection(FilteredSuggestions[NextIndex]);
		SuggestionListView->RequestScrollIntoView(FilteredSuggestions[NextIndex]);
	}
}

void SQCE_CodeCompletionSuggestionBox::SelectPreviousSuggestion()
{
	if (!SuggestionListView.IsValid() || FilteredSuggestions.Num() == 0)
	{
		return;
	}

	int32 CurrentIndex = FilteredSuggestions.IndexOfByKey(SelectedSuggestion);
	int32 PrevIndex = CurrentIndex;
	
	// Find previous selectable item
	do
	{
		PrevIndex = (PrevIndex - 1 + FilteredSuggestions.Num()) % FilteredSuggestions.Num();
		if (FilteredSuggestions[PrevIndex]->bSelectable)
		{
			break;
		}
	} while (PrevIndex != CurrentIndex);
	
	// Only update if we found a different selectable item
	if (PrevIndex != CurrentIndex && FilteredSuggestions[PrevIndex]->bSelectable)
	{
		SuggestionListView->SetSelection(FilteredSuggestions[PrevIndex]);
		SuggestionListView->RequestScrollIntoView(FilteredSuggestions[PrevIndex]);
	}
}

void SQCE_CodeCompletionSuggestionBox::AcceptSelectedSuggestion()
{
	if (SelectedSuggestion.IsValid() && SelectedSuggestion->bSelectable && OnCompletionSelected.IsBound())
	{
		OnCompletionSelected.Execute(SelectedSuggestion);
	}
}

void SQCE_CodeCompletionSuggestionBox::CancelCompletion()
{
	if (OnCompletionCancelled.IsBound())
	{
		OnCompletionCancelled.Execute();
	}
}

TSharedPtr<FCompletionItem> SQCE_CodeCompletionSuggestionBox::GetSelectedSuggestion() const
{
	return SelectedSuggestion;
}

void SQCE_CodeCompletionSuggestionBox::InitSuggestions(const FString& Code, const int32 CursorPosition, const SQCE_MultiLineEditableTextBox* CallingTextBox)
{
	if (!CompletionEngine)
		return;

	TArray<FCompletionItem> Completions;
	UMainEditorContainer* MainContainer = CallingTextBox->GetMainEditorContainer();
	if (!MainContainer) // Edge case if we couldn't load editor container
	{
		Completions = CompletionEngine->GetCompletions(Code, CursorPosition, FString(), FString(), nullptr);
	}
	else // If we have it, we will use it's declaration/implementation info to for more useful context for code completion
	{
		
		if (MainContainer->IsLoadIsolated()) // If we only loaded code related to current node, use initial file info to get context
		{
			Completions = CompletionEngine->GetCompletions(
				Code,
				CursorPosition,
				MainContainer->GetCurrentFunctionDeclarationInfo()->InitialFileContent,
				MainContainer->GetCurrentFunctionImplementationInfo()->InitialFileContent,
				MainContainer);
		}
		else // use visible content in editors
		{
			Completions = CompletionEngine->GetCompletions(
				Code,
				CursorPosition,
				MainContainer->GetDeclarationTextBoxWrapper()->GetText().ToString(),
				MainContainer->GetImplementationTextBoxWrapper()->GetText().ToString(),
				MainContainer);
		}
	}
	
	// Get completions from engine and convert to shared pointers
	
	FilteredSuggestions.Empty();
	AllSuggestions.Empty();
	
	for (const FCompletionItem& Item : Completions)
	{
		TSharedPtr<FCompletionItem> SharedItem = MakeShareable(new FCompletionItem(Item));
		FilteredSuggestions.Add(SharedItem);
		AllSuggestions.Add(SharedItem);
	}
	
	// Refresh the list view if it exists
	if (SuggestionListView.IsValid())
	{
		SuggestionListView->RequestListRefresh();
		
		// Select first selectable item if available
		if (FilteredSuggestions.Num() > 0)
		{
			TSharedPtr<FCompletionItem> FirstSelectableItem = FindFirstSelectableItem();
			if (FirstSelectableItem.IsValid())
			{
				SuggestionListView->SetSelection(FirstSelectableItem);
				SelectedSuggestion = FirstSelectableItem;
			}
		}
	}
}

void SQCE_CodeCompletionSuggestionBox::SetCompletionEngine(FDropdownCodeCompletionEngine* InCompletionEngine)
{
	CompletionEngine = InCompletionEngine;
}

TSharedRef<ITableRow> SQCE_CodeCompletionSuggestionBox::OnGenerateRowForSuggestion(TSharedPtr<FCompletionItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	// Use different colors for selectable vs non-selectable items
	FSlateColor TextColor = InItem->bSelectable ? 
		FSlateColor::UseForeground() : 
		FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)); // Grayed out for non-selectable items
		
	return SNew(STableRow<TSharedPtr<FCompletionItem>>, OwnerTable)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(InItem->DisplayText))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(TextColor)
		];
}

void SQCE_CodeCompletionSuggestionBox::OnSuggestionSelectionChanged(TSharedPtr<FCompletionItem> InItem, ESelectInfo::Type SelectInfo)
{
	SelectedSuggestion = InItem;
}

void SQCE_CodeCompletionSuggestionBox::OnSuggestionDoubleClicked(TSharedPtr<FCompletionItem> InItem)
{
	if (InItem.IsValid() && InItem->bSelectable && OnCompletionSelected.IsBound())
	{
		OnCompletionSelected.Execute(InItem);
	}
}

TSharedPtr<FCompletionItem> SQCE_CodeCompletionSuggestionBox::FindFirstSelectableItem() const
{
	for (const auto& Item : FilteredSuggestions)
	{
		if (Item.IsValid() && Item->bSelectable)
		{
			return Item;
		}
	}
	return nullptr;
}