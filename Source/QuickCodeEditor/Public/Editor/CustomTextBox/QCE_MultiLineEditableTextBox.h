// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Editor/CustomTextBox/InlineAISuggestion/Utils/InlineAISuggestionTypes.h"

class QCE_IndentationManager;
struct FUserInputContext;
class FDropdownCodeCompletionEngine;

/** Defines the type of code being edited in this text box */
UENUM()
enum class ETextBoxType
{
	Standard,
	Implementation,
	Declaration
};

enum class EInlineAICompletionState
{
	None,
	MenuVisible,
	CompletionOffered
};

class QCE_MultiLineEditableTextBoxWrapper;
class FQCE_TextLayout;
class SQCE_CodeCompletionSuggestionBox;
struct FCompletionItem;
class UMainEditorContainer;

DECLARE_DELEGATE(FOnEnterPressed)
DECLARE_DELEGATE(FOnQCEFocused)
DECLARE_DELEGATE(FOnSearchRequested)
DECLARE_DELEGATE(FOnSaveRequested)
DECLARE_DELEGATE(FOnSaveAndBuildRequested)
DECLARE_DELEGATE(FOnGoToLineRequested)
DECLARE_DELEGATE(FOnCodeCompletionRequested)

/**
 * Custom multi-line editable text box that extends SMultiLineEditableTextBox with additional functionality.
 * 
 * This class provides enhanced text editing capabilities including:
 * - Word highlighting under cursor
 * - Custom text layout for advanced rendering
 * - Word occurrence selection without moving cursor
 */
class QUICKCODEEDITOR_API SQCE_MultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
#pragma region SlateInit
    SLATE_BEGIN_ARGS(SQCE_MultiLineEditableTextBox)
    	: _Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
		, _Marshaller()
		, _Text()
		, _HintText()
		, _SearchText()
		, _Font()
        , _ForegroundColor()
        , _ReadOnlyForegroundColor()
        , _FocusedForegroundColor()
        , _Justification(ETextJustify::Left)
        , _LineHeightPercentage(1.0f)
        , _IsReadOnly(false)
        , _AllowMultiLine(true)
        , _IsCaretMovedWhenGainFocus(true)
        , _SelectAllTextWhenFocused(false)
        , _ClearTextSelectionOnFocusLoss(true)
        , _RevertTextOnEscape(false)
        , _ClearKeyboardFocusOnCommit(true)
        , _AllowContextMenu(true)
        , _AlwaysShowScrollbars(false)
        , _WrapTextAt(0.0f)
        , _AutoWrapText(false)
        , _WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)
        , _SelectAllTextOnCommit(false)
        , _SelectWordOnMouseDoubleClick(true)
        , _BackgroundColor()
        , _Padding()
        , _Margin()
        , _ModiferKeyForNewLine(EModifierKey::None)
        , _VirtualKeyboardOptions(FVirtualKeyboardOptions())
        , _VirtualKeyboardTrigger(EVirtualKeyboardTrigger::OnFocusByPointer)
        , _VirtualKeyboardDismissAction(EVirtualKeyboardDismissAction::TextChangeOnDismiss)
    {}
        /** The styling of the textbox */
        SLATE_STYLE_ARGUMENT(FEditableTextBoxStyle, Style)

    	SLATE_ARGUMENT(TSharedPtr< ITextLayoutMarshaller >, Marshaller)

        /** Sets the text content for this editable text box widget */
        SLATE_ATTRIBUTE(FText, Text)

        /** Hint text that appears when there is no text in the text box */
        SLATE_ATTRIBUTE(FText, HintText)

        /** Text to search for (a new search is triggered whenever this text changes) */
        SLATE_ATTRIBUTE(FText, SearchText)

    	SLATE_EVENT( FMenuExtensionDelegate, ContextMenuExtender )
	    
        /** Font color and opacity (overrides Style) */
        SLATE_ATTRIBUTE(FSlateFontInfo, Font)

        /** Text color and opacity (overrides Style) */
        SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)

        /** Text color and opacity when read-only (overrides Style) */
        SLATE_ATTRIBUTE(FSlateColor, ReadOnlyForegroundColor)

        /** Text color and opacity when this box has keyboard focus (overrides Style) */
        SLATE_ATTRIBUTE(FSlateColor, FocusedForegroundColor)

        /** How the text should be aligned with the margin */
        SLATE_ATTRIBUTE(ETextJustify::Type, Justification)

        /** The amount to scale each lines height by */
        SLATE_ATTRIBUTE(float, LineHeightPercentage)

        /** Sets whether this text box can actually be modified interactively by the user */
        SLATE_ATTRIBUTE(bool, IsReadOnly)

    	SLATE_EVENT( FCreateSlateTextLayout, CreateSlateTextLayout )

        /** Whether to allow multi-line text */
        SLATE_ATTRIBUTE(bool, AllowMultiLine)

        /** Workaround as we loose focus when the auto completion closes */
        SLATE_ATTRIBUTE(bool, IsCaretMovedWhenGainFocus)

        /** Whether to select all text when the user clicks to give focus on the widget */
        SLATE_ATTRIBUTE(bool, SelectAllTextWhenFocused)

        /** Whether to clear text selection when focus is lost */
        SLATE_ATTRIBUTE(bool, ClearTextSelectionOnFocusLoss)

        /** Whether to allow the user to back out of changes when they press the escape key */
        SLATE_ATTRIBUTE(bool, RevertTextOnEscape)

        /** Whether to clear keyboard focus when pressing enter to commit changes */
        SLATE_ATTRIBUTE(bool, ClearKeyboardFocusOnCommit)

        /** Whether the context menu can be opened */
        SLATE_ATTRIBUTE(bool, AllowContextMenu)

        /** Should we always show the scrollbars */
        SLATE_ARGUMENT(bool, AlwaysShowScrollbars)

        /** The horizontal scroll bar widget */
        SLATE_ARGUMENT(TSharedPtr<SScrollBar>, HScrollBar)

        /** The vertical scroll bar widget */
        SLATE_ARGUMENT(TSharedPtr<SScrollBar>, VScrollBar)

        /** Padding around the horizontal scrollbar */
        SLATE_ATTRIBUTE(FMargin, HScrollBarPadding)

        /** Padding around the vertical scrollbar */
        SLATE_ATTRIBUTE(FMargin, VScrollBarPadding)

        /** Called whenever the text is changed programmatically or interactively by the user */
        SLATE_EVENT(FOnTextChanged, OnTextChanged)

        /** Called whenever the text is committed */
        SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

        /** Called whenever the text is changed programmatically or interactively by the user */
        SLATE_EVENT(FOnVerifyTextChanged, OnVerifyTextChanged)

        /** Called whenever the horizontal scrollbar is moved by the user */
        SLATE_EVENT(FOnUserScrolled, OnHScrollBarUserScrolled)

        /** Called whenever the vertical scrollbar is moved by the user */
        SLATE_EVENT(FOnUserScrolled, OnVScrollBarUserScrolled)

        /** Called when the cursor is moved within the text area */
        SLATE_EVENT(SMultiLineEditableText::FOnCursorMoved, OnCursorMoved)

        /** Called when the text box gains focus */
        SLATE_EVENT(FOnQCEFocused, OnQCEFocused)

        /** Called when search is requested (Ctrl+F) */
        SLATE_EVENT(FOnSearchRequested, OnSearchRequested)

        /** Called when save is requested (Ctrl+S) */
        SLATE_EVENT(FOnSaveRequested, OnSaveRequested)

        /** Called when save and build is requested (Ctrl+Shift+B) */
        SLATE_EVENT(FOnSaveAndBuildRequested, OnSaveAndBuildRequested)

        /** Called when go to line is requested (Ctrl+G) */
        SLATE_EVENT(FOnGoToLineRequested, OnGoToLineRequested)

        /** Called when code completion is requested (Ctrl+Space) */
        SLATE_EVENT(FOnCodeCompletionRequested, OnCodeCompletionRequested)

        /** Whether text wraps onto a new line when it's length exceeds this width */
        SLATE_ATTRIBUTE(float, WrapTextAt)

        /** Whether to wrap text automatically based on the widget's computed horizontal space */
        SLATE_ATTRIBUTE(bool, AutoWrapText)

        /** The wrapping policy to use */
        SLATE_ATTRIBUTE(ETextWrappingPolicy, WrappingPolicy)

        /** Whether to select all text when pressing enter to commit changes */
        SLATE_ATTRIBUTE(bool, SelectAllTextOnCommit)

        /** Whether to select word on mouse double click */
        SLATE_ATTRIBUTE(bool, SelectWordOnMouseDoubleClick)

        /** The color of the background/border around the editable text */
        SLATE_ATTRIBUTE(FSlateColor, BackgroundColor)

        /** Padding between the box/border and the text widget inside */
        SLATE_ATTRIBUTE(FMargin, Padding)

        /** The amount of blank space left around the edges of text area */
        SLATE_ATTRIBUTE(FMargin, Margin)

        /** The optional modifier key necessary to create a newline when typing into the editor */
        SLATE_ARGUMENT(EModifierKey::Type, ModiferKeyForNewLine)

        /** Additional options used by the virtual keyboard summoned by this widget */
        SLATE_ARGUMENT(FVirtualKeyboardOptions, VirtualKeyboardOptions)

        /** The type of event that will trigger the display of the virtual keyboard */
        SLATE_ATTRIBUTE(EVirtualKeyboardTrigger, VirtualKeyboardTrigger)

        /** The message action to take when the virtual keyboard is dismissed by the user */
        SLATE_ATTRIBUTE(EVirtualKeyboardDismissAction, VirtualKeyboardDismissAction)

    SLATE_END_ARGS()
#pragma endregion 

    /** Constructs and initializes the widget with custom text styling and event handling */
    void Construct(const FArguments& InArgs);

	void SetParentTextBoxWrapper(const TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& InParentWrapper) { ParentTextBoxWrapper = InParentWrapper; }

	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> GetParentTextBoxWrapper() const { return ParentTextBoxWrapper.Pin(); }

	/** Sets the main editor container that owns this text box */
	void SetMainEditorContainer(UMainEditorContainer* InMainEditorContainer) { MainEditorContainer = InMainEditorContainer; }

	/** Gets the main editor container that owns this text box */
	UMainEditorContainer* GetMainEditorContainer() const { return MainEditorContainer; }

	bool IsModified() const { return bIsModified; }
	
	/** Set to false when we initially open a node. */
	void SetIsModified(bool bNewIsModified) { bIsModified = bNewIsModified; }
    
    /** 
     * Gets the word at the current cursor position
     * @return The word under the cursor as a string
     */
    FString GetWordAtCursor() const;

    /** 
      * Gets the word at the given location.
      * @return The word at given location within textbox.
      */
    FString GetWordAtLocation(const FTextLocation TargetLocation) const;

    /** 
     * Custom text layout instance for advanced text rendering and highlighting */
    TSharedPtr<FQCE_TextLayout> FQCE_TextLayout;

    /** 
     * Gets the last known cursor location in the text
     * @return The last recorded cursor position as FTextLocation
     */
    FTextLocation GetLastCursorLocation() const { return LastCursorLocation; }

	FOnEnterPressed OnEnterPressed;
	FOnQCEFocused OnQCEFocused;
	FOnSearchRequested OnSearchRequested;
	FOnSaveRequested OnSaveRequested;
	FOnSaveAndBuildRequested OnSaveAndBuildRequested;
	FOnGoToLineRequested OnGoToLineRequested;

	TSharedPtr< SMultiLineEditableText > GetEditableText() const { return EditableText;  };
	
	/**
	 * If true, this editable textbox will act as a chatbox,
	 * meaning Enter button by default will fire an event
	 * instead of wriitng a new line.
	 */
	bool bIsChatBox;

	bool bIsNodeSelected = false;

	virtual void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
	
public:
	bool IsNodeSelected() const { return bIsNodeSelected; }
	void SetNodeSelected(bool bNewIsNodeSelected) { bIsNodeSelected = bNewIsNodeSelected; }

	/** 
	 * Selects all occurrences of the specified word in the text
	 * @param TargetWord The word to find and select all occurrences of
	 */
	void SelectWordOccurrences(const FString& TargetWord);

	/**
	 * Selects a specific occurrence of a word and moves the cursor to it
	 * @param Word The word to highlight and select
	 * @param AbsolutePosition The absolute character position in the text where the occurrence starts
	 * @param Length The length of the word
	 */
	void SelectSpecificOccurrence(const FString& Word, int32 AbsolutePosition, int32 Length);
	
	/** Tracks the last cursor position for maintaining state during focus changes */
	FTextLocation LastCursorLocation;
	
protected:
	/** Used to handle Enter keydown in case this is being used as a chatbox, when it should be sent instead of new line. */
	FReply HandleKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	/** We can't use OnKeyDown to handle Space and character keys, so we also override OnKeyChar using this method. */
	FReply HandleKeyChar(const FGeometry& Geometry, const FCharacterEvent& CharacterEvent);
	
	/** Used to handle Shift + Enter keyup in case this is being used as a chatbox, when it should enter a new line. */
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	/**
	* Handle all custom shortcuts set up by QCE_EditorSettings.
	*/
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	
	virtual FReply OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TWeakPtr<QCE_MultiLineEditableTextBoxWrapper> ParentTextBoxWrapper;
	
	/** Reference to the main editor container that owns this text box */
	UMainEditorContainer* MainEditorContainer = nullptr;
    /** 
     * Selects all occurrences of the word under the cursor
     * @param CursorLocation The text location to check for word selection
     */
    void SelectCursorWordOccurrences(const FTextLocation& CursorLocation);
	
    /** Caches the last word that was highlighted to avoid unnecessary updates */
    FString LastHighlightedWord;

    /** Defines the background color used for highlighting word occurrences */
    FLinearColor HighlightedBackgroundColor = FLinearColor(0.14f, 0.3f, 0.83f);
	
	bool bIsModified = false;

public:

	/** Gets the type of code being edited in this text box */
	ETextBoxType GetTextBoxType() const { return TextBoxType; }

	/** Sets the type of code being edited in this text box */
	void SetTextBoxType(ETextBoxType NewType) { TextBoxType = NewType; }
private:
	/** The type of code being edited in this text box */
	ETextBoxType TextBoxType = ETextBoxType::Implementation;

#pragma region CodeCompletion
public:
	void SetCodeCompletionEngine(FDropdownCodeCompletionEngine* InCompletionEngine);
	
	/** Shows the code completion suggestion box */
	void ShowMemberSuggestions();

	/** Hides the code completion suggestion box */
	void HideMemberSuggestions();

	/** Toggles the code completion dropdown visibility */
	void ToggleCodeCompletionDropdown();

	FOnCodeCompletionRequested OnCodeCompletionRequested;

private:
	FDropdownCodeCompletionEngine* CompletionEngine = nullptr;

	/** Container for QCE_CodeCompletionSuggestionBox*/
	TSharedPtr<IMenu> CodeCompletionMenuContainer;
	
	/** The code completion suggestion box widget */
	TSharedPtr<SQCE_CodeCompletionSuggestionBox> QCE_CodeCompletionSuggestionBox;

	bool bShouldFocusCodeCompletionMenu = false;

	/** Handle completion selected from suggestion box */
	void OnMemberSuggestionSelected(TSharedPtr<FCompletionItem> SelectedItem);

#pragma endregion CodeCompletion

#pragma region AIInlineCompletion
private:
	/** Current state of the inline AI completion system */
	EInlineAICompletionState InlineAICompletionState = EInlineAICompletionState::None;
	
	/** The cursor position where the AI completion was inserted */
	FTextLocation PendingCompletionCursorPosition;
	
	/** The end position after the AI completion text was inserted */
	FTextLocation PendingCompletionEndPosition;
	
	/** The text of the pending AI completion for potential rollback */
	FString PendingCompletionText;


	/** Whether multiline AI completion is enabled */
	bool bIsMultilineAIEnabled = false;
	
	/** Reference to the inline AI suggestion box widget */
	TSharedPtr<class SQCE_InlineAISuggestionBox> InlineAISuggestionBox;
	
	/** Container for the inline AI suggestion popup menu */
	TSharedPtr<IMenu> InlineAISuggestionMenuContainer;
	
	/** Handle AI inline completion request */
	void TriggerInlineSuggestion();
	
	/** Handle accepting AI completion */
	void HandleAICompletionAccept();
	
	/** Handle rejecting/cancelling AI completion */
	void HandleAICompletionReject();
	
	/** Callback for when AI completion is received */
	void OnAICompletionReceived(const FCompletionResponse& Response, bool bSuccess);
	
	/** Show the inline AI suggestion popup menu */
	void ShowInlineAISuggestionMenu();
	
	/** Hide the inline AI suggestion popup menu */
	void HideInlineAISuggestionMenu();
	
	/** Handle inline AI suggestion confirmation */
	void OnInlineAISuggestionConfirmed(FUserInputContext UserInputContext);
	
	/** Handle inline AI suggestion cancellation */
	void OnInlineAISuggestionCancelled();

#pragma endregion AIInlineCompletion

#pragma region ApplicationFocus
private:
	/** Delegate handle for application activation state changes */
	FDelegateHandle ApplicationFocusChangedHandle;

	/** Handles application focus changes - clears text box focus when app loses focus */
	void OnApplicationFocusChanged(bool bIsFocused);

#pragma endregion ApplicationFocus
};

