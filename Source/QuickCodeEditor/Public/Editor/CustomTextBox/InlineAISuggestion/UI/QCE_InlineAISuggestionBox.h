// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Settings/UQCE_EditorSettings.h"

// Forward declarations
enum class ETextBoxType;
class STextBlock;
template<typename OptionType> class SComboBox;
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateColor.h"

/**
 * Context information for inline AI suggestion confirmation
 */
struct QUICKCODEEDITOR_API FUserInputContext
{
	/** Selected completion type */
	EQCEDefaultCompletionType CompletionType = EQCEDefaultCompletionType::CurrentLine;

	/** Selected context type */
	EQCEDefaultContext ContextType = EQCEDefaultContext::CurrentFunction;

	/** User-provided context text for the AI suggestion */
	FString UserInput;

	FString Code;

	ETextBoxType TextBoxType;

	/** Number of lines to use when ContextType is NLinesAboveCursor */
	int32 NumberOfLines = 5;

	FUserInputContext() = default;

	FUserInputContext(EQCEDefaultCompletionType InCompletionType, EQCEDefaultContext InContextType, const FString& InUserInput)
		: CompletionType(InCompletionType)
		, ContextType(InContextType)
		, UserInput(InUserInput)
	{
	}
};

enum ESuggestionBoxState
{
	ReadyForInput,
	Processing,
	ShowingWarning
};

DECLARE_DELEGATE_OneParam(FOnCompletionTypeChanged, EQCEDefaultCompletionType);
DECLARE_DELEGATE_OneParam(FOnContextTypeChanged, EQCEDefaultContext);
DECLARE_DELEGATE_OneParam(FOnInlineAISuggestionConfirmed, FUserInputContext);
DECLARE_DELEGATE(FOnInlineAISuggestionCancelled);

/**
 * QCE_InlineAISuggestionBox provides a compact popup interface for inline AI suggestion configuration.
 * 
 * This widget displays a four-row interface with minimal height:
 * - Top row: Completion type dropdown (Dropdown/Current Line/Block)
 * - Second row: Context type dropdown (Current Function/Current Function Before Cursor/N Lines Above Cursor)
 * - Third row: Single-line text box for user context input (limited to ~24-30px height)
 * - Bottom row: Status indicator on the left and confirmation shortcut text on the right
 */
class QUICKCODEEDITOR_API SQCE_InlineAISuggestionBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQCE_InlineAISuggestionBox)
		: _MinDesiredWidth(300.0f)
		, _MinDesiredHeight(180.0f)
		, _CompletionType(EQCEDefaultCompletionType::CurrentLine)
		, _ContextType(EQCEDefaultContext::CurrentFunction)
		, _ContextText(FText::GetEmpty())
		, _TextBoxType(ETextBoxType::Implementation)
	{}
		/** Minimum desired width of the suggestion box */
		SLATE_ARGUMENT(float, MinDesiredWidth)
		
		/** Minimum desired height of the suggestion box */
		SLATE_ARGUMENT(float, MinDesiredHeight)
		
		/** Initial completion type selection */
		SLATE_ARGUMENT(EQCEDefaultCompletionType, CompletionType)
		
		/** Initial context type selection */
		SLATE_ARGUMENT(EQCEDefaultContext, ContextType)
		
		/** Initial context text */
		SLATE_ARGUMENT(FText, ContextText)
		
		/** Text box type for context */
		SLATE_ARGUMENT(ETextBoxType, TextBoxType)
		
		/** Called when the completion type dropdown selection changes */
		SLATE_EVENT(FOnCompletionTypeChanged, OnCompletionTypeChanged)
		
		/** Called when the context type dropdown selection changes */
		SLATE_EVENT(FOnContextTypeChanged, OnContextTypeChanged)
		
		/** Called when the user confirms the suggestion (Enter key) */
		SLATE_EVENT(FOnInlineAISuggestionConfirmed, OnConfirmed)
		
		/** Called when the user cancels the suggestion (Escape key) */
		SLATE_EVENT(FOnInlineAISuggestionCancelled, OnCancelled)
		
	SLATE_END_ARGS()

	/** Constructs and initializes the suggestion box widget */
	void Construct(const FArguments& InArgs);

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	/** Handle key input for the suggestion box */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	FReply HandleKeyChar(const FGeometry& Geometry, const FCharacterEvent& CharacterEvent);

	/**
	 * Gets the current context text from the text box
	 * @return The current context text
	 */
	FText GetContextText() const;

	/**
	 * Sets the completion type dropdown selection
	 * @param CompletionType The completion type to select
	 */
	void SetCompletionType(EQCEDefaultCompletionType CompletionType);

	/**
	 * Gets the current completion type selection
	 * @return The currently selected completion type
	 */
	EQCEDefaultCompletionType GetCompletionType() const;

	/**
	 * Sets the context type dropdown selection
	 * @param ContextType The context type to select
	 */
	void SetContextType(EQCEDefaultContext ContextType);

	/**
	 * Gets the current context type selection
	 * @return The currently selected context type
	 */
	EQCEDefaultContext GetContextType() const;

	/**
	 * Sets the number of lines for NLinesAboveCursor context
	 * @param NumberOfLines The number of lines to set
	 */
	void SetNumberOfLines(int32 NumberOfLines);

	/**
	 * Gets the current number of lines selection
	 * @return The currently selected number of lines
	 */
	int32 GetNumberOfLines() const;

	/** Sets keyboard focus to the context text box */
	void FocusContextTextBox() const;

	/** Confirms the current suggestion settings */
	void ConfirmSuggestion();

	/** Cancels the suggestion and hides the box */
	void CancelSuggestion() const;

	/**
	 * Gets the current suggestion box state
	 * @return The current state of the suggestion box
	 */
	ESuggestionBoxState GetSuggestionBoxState() const;

	/**
	 * Sets the suggestion box state and updates UI visibility accordingly
	 * @param NewState The new state to set
	 */
	void SetSuggestionBoxState(ESuggestionBoxState NewState);

	/**
	 * Shows the inline AI suggestion box as a popup menu at the specified screen position
	 * @param ScreenPosition The screen coordinates where the popup should appear
	 * @param OwnerWidget The widget that owns this popup
	 * @return True if the popup was successfully shown, false otherwise
	 */
	static bool ShowInlineAISuggestionBox(const FVector2D& ScreenPosition, TSharedPtr<SWidget> OwnerWidget);

	/** The warning text widget showing validation messages */
	TSharedPtr<STextBlock> WarningTextBlock;
protected:
	/** Handle completion type dropdown selection changes */
	void OnCompletionTypeSelectionChanged(TSharedPtr<EQCEDefaultCompletionType> SelectedItem, ESelectInfo::Type SelectInfo);

	/** Handle context type dropdown selection changes */
	void OnContextTypeSelectionChanged(TSharedPtr<EQCEDefaultContext> SelectedItem, ESelectInfo::Type SelectInfo);

	/** Handle context text box text commits (Enter key) */
	void OnContextTextBoxTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

	/** Handle number of lines spin box value changes */
	void OnNumberOfLinesChanged(int32 NewValue);

	/** Generate text for completion type dropdown options */
	FText GetCompletionTypeText(TSharedPtr<EQCEDefaultCompletionType> CompletionType) const;

	/** Generate text for context type dropdown options */
	FText GetContextTypeText(TSharedPtr<EQCEDefaultContext> ContextType) const;

	/** Updates the visibility of status and shortcut text blocks based on current state */
	void UpdateTextBlockVisibility() const;

	/** Cycles through available completion types */
	void ToggleCompletionType();

	/** Cycles through available context types */
	void ToggleContextType();

private:
	/** Minimum desired width */
	float MinDesiredWidth = 300.0f;

	/** Minimum desired height */
	float MinDesiredHeight = 180.0f;

	/** The completion type dropdown widget */
	TSharedPtr<SComboBox<TSharedPtr<EQCEDefaultCompletionType>>> CompletionTypeComboBox;

	/** The context type dropdown widget for implementation mode */
	TSharedPtr<SComboBox<TSharedPtr<EQCEDefaultContext>>> ImplementationContextTypeComboBox;

	/** The context type dropdown widget for declaration mode */
	TSharedPtr<SComboBox<TSharedPtr<EQCEDefaultContext>>> DeclarationContextTypeComboBox;

	/** The context text box widget */
	TSharedPtr<SMultiLineEditableTextBox> ContextTextBox;

	/** The number of lines spin box widget */
	TSharedPtr<SSpinBox<int32>> NumberOfLinesSpinBox;

	/** Available completion type options */
	TArray<TSharedPtr<EQCEDefaultCompletionType>> CompletionTypeOptions;

	/** Available context type options for implementation mode */
	TArray<TSharedPtr<EQCEDefaultContext>> ImplementationContextTypeOptions;

	/** Available context type options for declaration mode */
	TArray<TSharedPtr<EQCEDefaultContext>> DeclarationContextTypeOptions;

	/** The status text widget showing processing indicator */
	TSharedPtr<STextBlock> StatusTextBlock;

	/** The shortcut text widget showing confirmation shortcut */
	TSharedPtr<STextBlock> ShortcutTextBlock;
	
	/** Current completion type selection */
	EQCEDefaultCompletionType CurrentCompletionType = EQCEDefaultCompletionType::CurrentLine;

	/** Current context type selection */
	EQCEDefaultContext CurrentContextType = EQCEDefaultContext::CurrentFunction;

	/** Current context text */
	FText CurrentContextText;
	
	/** Current number of lines selection */
	int32 CurrentNumberOfLines = 5;
	
	/** Text box type for context */
	ETextBoxType TextBoxType;
	
	/** Delegate called when completion type dropdown selection changes */
	FOnCompletionTypeChanged OnCompletionTypeChanged;

	/** Delegate called when context type dropdown selection changes */
	FOnContextTypeChanged OnContextTypeChanged;

	/** Delegate called when suggestion is confirmed */
	FOnInlineAISuggestionConfirmed OnConfirmed;

	/** Delegate called when suggestion is cancelled */
	FOnInlineAISuggestionCancelled OnCancelled;

	ESuggestionBoxState SuggestionBoxState = ReadyForInput;
};