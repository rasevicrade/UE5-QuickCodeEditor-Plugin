// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/CodeCompletion/DropdownCodeCompletionEngine.h"
#include "Editor/CustomTextBox/CodeCompletion/Utils/CodeCompletionContext.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateColor.h"

DECLARE_DELEGATE_OneParam(FOnCodeCompletionSelected, TSharedPtr<FCompletionItem>);
DECLARE_DELEGATE(FOnCodeCompletionCancelled);

/**
 * QCE_CodeCompletionSuggestionBox provides a popup suggestion list for code completion.
 * 
 * This widget displays a list of code completion suggestions and handles user interaction
 * including keyboard navigation and selection. It follows the pattern used by SOutputLog
 * for suggestion functionality.
 */
class QUICKCODEEDITOR_API SQCE_CodeCompletionSuggestionBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQCE_CodeCompletionSuggestionBox)
		: _MaxVisibleItems(10)
		, _ItemHeight(20.0f)
	{}
		/** Maximum number of visible items in the suggestion list */
		SLATE_ARGUMENT(int32, MaxVisibleItems)
		
		/** Height of each suggestion item */
		SLATE_ARGUMENT(float, ItemHeight)
		
		/** Called when a completion item is selected */
		SLATE_EVENT(FOnCodeCompletionSelected, OnCompletionSelected)
		
		/** Called when completion is cancelled (Escape pressed) */
		SLATE_EVENT(FOnCodeCompletionCancelled, OnCompletionCancelled)
		
	SLATE_END_ARGS()

	/** Constructs and initializes the suggestion box widget */
	void Construct(const FArguments& InArgs);

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	/**
	 * Sets the suggestions to display in the list
	 * @param InSuggestions Array of completion suggestions to display
	 */
	void SetSuggestions(const TArray<TSharedPtr<FCompletionItem>>& InSuggestions);

	/** Selects the next suggestion in the list (Down arrow) */
	void SelectNextSuggestion();

	/** Selects the previous suggestion in the list (Up arrow) */
	void SelectPreviousSuggestion();

	/** Accepts the currently selected suggestion (Enter key) */
	void AcceptSelectedSuggestion();

	/** Cancels the completion and hides the suggestion box (Escape key) */
	void CancelCompletion();

	/** Gets the currently selected suggestion item */
	TSharedPtr<FCompletionItem> GetSelectedSuggestion() const;
	
	void InitSuggestions(const FString& Code, const int32 CursorPosition, const class SQCE_MultiLineEditableTextBox* CallingTextBox = nullptr);
	
	void SetCompletionEngine(FDropdownCodeCompletionEngine* InCompletionEngine);
protected:
	/** Generate a widget for each suggestion item in the list */
	TSharedRef<ITableRow> OnGenerateRowForSuggestion(TSharedPtr<FCompletionItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handle selection changed in the suggestion list */
	void OnSuggestionSelectionChanged(TSharedPtr<FCompletionItem> InItem, ESelectInfo::Type SelectInfo);

	/** Handle double-click on a suggestion item */
	void OnSuggestionDoubleClicked(TSharedPtr<FCompletionItem> InItem);

private:
	/** Helper method to find the first selectable item in the filtered suggestions */
	TSharedPtr<FCompletionItem> FindFirstSelectableItem() const;
	
	FDropdownCodeCompletionEngine* CompletionEngine = nullptr;
		
	/** List view widget that displays the suggestions */
	TSharedPtr<SListView<TSharedPtr<FCompletionItem>>> SuggestionListView;

	/** All available suggestions */
	TArray<TSharedPtr<FCompletionItem>> AllSuggestions;

	/** Currently filtered and displayed suggestions */
	TArray<TSharedPtr<FCompletionItem>> FilteredSuggestions;

	/** Currently selected suggestion item */
	TSharedPtr<FCompletionItem> SelectedSuggestion;

	/** Maximum number of visible items */
	int32 MaxVisibleItems = 100;

	/** Height of each item */
	float ItemHeight = 12.f;

	/** Delegate called when completion is selected */
	FOnCodeCompletionSelected OnCompletionSelected;

	/** Delegate called when completion is cancelled */
	FOnCodeCompletionCancelled OnCompletionCancelled;
};