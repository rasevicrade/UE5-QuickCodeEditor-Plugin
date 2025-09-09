// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QCE_MultiLineEditableTextBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"

class SQCE_MultiLineEditableTextBox;
class UMainEditorContainer;

/**
 * QCE_MultiLineEditableTextBoxWrapper
 * 
 * A comprehensive wrapper widget that combines a multi-line text editor with optional line numbers
 * and synchronized scrolling functionality. This widget provides a complete code editing experience
 * similar to modern IDEs.
 * 
 * Features:
 * - Multi-line text editing with syntax highlighting support
 * - Optional line numbers display with automatic updating
 * - Synchronized scrolling between line numbers and text content
 * - File path association for tracking which file is being edited
 * - Text search and replace functionality
 * - Scroll-to-line and scroll-to-position capabilities
 * - Read-only mode support
 * - Modification tracking with original code comparison
 * - Keyboard shortcuts for save, build, search, and go-to-line operations
 * - Customizable context menu extensions
 * 
 * This wrapper is designed to work seamlessly with the QuickCodeEditor plugin's
 * architecture, providing a unified interface for code editing tasks.
 */
class QUICKCODEEDITOR_API QCE_MultiLineEditableTextBoxWrapper : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(QCE_MultiLineEditableTextBoxWrapper)
        : _AllowMultiLine(true)
        , _AutoWrapText(false)
        , _IsReadOnly(false)
        , _EnableLineNumbers(true)
        , _ModifierKeyForNewLine(EModifierKey::None)
    {}
        /** Whether to allow multi-line text entry */
        SLATE_ARGUMENT(bool, AllowMultiLine)
        
        /** Whether to wrap text automatically when it exceeds the widget width */
        SLATE_ARGUMENT(bool, AutoWrapText)
        
        /** Whether the text box should be read-only (prevents user modifications) */
        SLATE_ARGUMENT(bool, IsReadOnly)
        
        /** Whether to show the line numbers column on the left side */
        SLATE_ARGUMENT(bool, EnableLineNumbers)
        
        /** The text layout marshaller to use for syntax highlighting and formatting */
        SLATE_ARGUMENT(TSharedPtr<ITextLayoutMarshaller>, Marshaller)
        
        /** The modifier key that must be held to create a newline when entering text */
        SLATE_ARGUMENT(EModifierKey::Type, ModifierKeyForNewLine)
        
        /** Called whenever the text is changed interactively by the user */
        SLATE_EVENT(FOnTextChanged, OnTextChanged)
        
        /** Called whenever the text is committed (e.g., user presses Enter or focuses away) */
        SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
        
        /** Menu extender for customizing the right-click context menu */
        SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtender)
        
        /** Called when a custom text layout is needed for advanced text rendering */
        SLATE_EVENT(FCreateSlateTextLayout, CreateSlateTextLayout)
        
        /** Called when the text box gains focus */
        SLATE_EVENT(FSimpleDelegate, OnGetFocus)
        
        /** Called when search functionality is requested (typically Ctrl+F) */
        SLATE_EVENT(FOnSearchRequested, OnSearchRequested)
        
        /** Called when save functionality is requested (typically Ctrl+S) */
        SLATE_EVENT(FOnSaveRequested, OnSaveRequested)
        
        /** Called when save and build functionality is requested (typically Ctrl+Shift+B) */
        SLATE_EVENT(FOnSaveAndBuildRequested, OnSaveAndBuildRequested)
        
        /** Called when go to line functionality is requested (typically Ctrl+G) */
        SLATE_EVENT(FOnGoToLineRequested, OnGoToLineRequested)
        
        /** The initial text that will appear in the text box */
        SLATE_ATTRIBUTE(FText, Text)
        
        /** The main editor container that owns this text box wrapper */
        SLATE_ARGUMENT(UMainEditorContainer*, MainEditorContainer)
    SLATE_END_ARGS()

    /**
     * Constructs the widget with the specified arguments
     * @param InArgs The construction arguments containing all configuration options
     */
    void Construct(const FArguments& InArgs);

#pragma region Widget Access
public:
    /**
     * Gets the underlying text box widget for direct access to text editing functionality
     * @return Shared pointer to the wrapped SQCE_MultiLineEditableTextBox widget
     */
    TSharedPtr<SQCE_MultiLineEditableTextBox> GetTextBox() const { return TextBox; }

    /** Sets the main editor container that owns this text box wrapper */
    void SetMainEditorContainer(UMainEditorContainer* InMainEditorContainer) { MainEditorContainer = InMainEditorContainer; }

    /** Gets the main editor container that owns this text box wrapper */
    UMainEditorContainer* GetMainEditorContainer() const { return MainEditorContainer; }
#pragma endregion

#pragma region Text Content Management
public:
    /**
     * Gets the current text content of the text box
     * @return The current text as FText
     */
    FText GetText() const { return TextBox->GetText(); }

    /**
     * Sets the text content of the text box
     * @param InText The new text content to set
     */
    void SetText(const FText& InText) { TextBox->SetText(InText); }

    /**
     * Checks if the current text content has been modified from the original
     * @return true if the text has been modified, false otherwise
     */
    bool IsModified() const { return TextBox->IsModified(); }

    /**
     * Sets the modification state of the text box
     * @param bNewIsModified true to mark as modified, false to mark as unmodified
     */
    void SetIsModified(bool bNewIsModified);

    /**
     * Sets whether the text box should be read-only
     * @param bInIsReadOnly true to make read-only, false to allow editing
     */
    void SetIsReadOnly(bool bInIsReadOnly) { TextBox->SetIsReadOnly(bInIsReadOnly); }
#pragma endregion

#pragma region File Path Management
public:
    /**
     * Gets the file path associated with this text box's content
     * @return The file path as a string, or empty string if no path is set
     */
    const FString& GetFilePath() const;
    
    /**
     * Sets the file path associated with this text box's content
     * This is used for tracking which file is being edited and for display purposes
     * @param InFilePath The file path to associate with this text box
     */
    void SetFilePath(const FString& InFilePath);
#pragma endregion

#pragma region Text Navigation and Cursor
public:
    /**
     * Gets the word at the current cursor position
     * @return The word under the cursor, or empty string if no valid word
     */
    FString GetWordAtCursor() const { return TextBox->GetWordAtCursor(); }

    /**
     * Gets the word at the specified text location
     * @param TargetLocation The text location to get the word from
     * @return The word at the specified location, or empty string if no valid word
     */
    FString GetWordAtLocation(const FTextLocation TargetLocation) const { return TextBox->GetWordAtLocation(TargetLocation); }

    /**
     * Gets the last known cursor location in the text
     * @return The cursor location as FTextLocation (line and column)
     */
    FTextLocation GetLastCursorLocation() const { return TextBox->GetLastCursorLocation(); }
#pragma endregion

#pragma region Blueprint Node Integration
public:
    /**
     * Checks if a Blueprint node is currently selected
     * @return true if a node is selected, false otherwise
     */
    bool IsNodeSelected() const { return TextBox->IsNodeSelected(); }

    /**
     * Sets whether a Blueprint node is currently selected
     * This affects the behavior of certain editor operations
     * @param bNewIsNodeSelected true if a node is selected, false otherwise
     */
    void SetNodeSelected(bool bNewIsNodeSelected) { TextBox->SetNodeSelected(bNewIsNodeSelected); }
#pragma endregion

#pragma region Scrolling and Navigation
public:
    /**
     * Sets a target line to scroll to when the widget becomes visible
     * This is useful for cases where you want to scroll while the widget is not yet visible,
     * such as when creating a definition from header and wanting to jump to it in the implementation
     * @param NewScrollTarget The line number to scroll to (1-based)
     */
    void SetScrollTarget(const int32 NewScrollTarget) { ScrollTarget = NewScrollTarget; }

    /**
     * Gets the current scroll target line number
     * @return The target line number, or -1 if no target is set
     */
    int32 GetScrollTarget() const { return ScrollTarget; }

    /**
     * Scrolls the text box to display the specified line number
     * The line will be centered in the viewport if possible
     * @param TargetLine The line number to scroll to (1-based)
     * @return true if the scroll was successful, false if the line number is invalid
     */
    bool ScrollToLine(int32 TargetLine) const;

    /**
     * Scrolls the text box to display the specified character position
     * @param TargetPosition The character position to scroll to (0-based)
     */
    void ScrollToPosition(int32 TargetPosition) const;
#pragma endregion

private:
#pragma region Widget Components
    /** The main text editing widget that provides the core text editing functionality */
    TSharedPtr<SQCE_MultiLineEditableTextBox> TextBox;

    /** The line numbers display widget, shown on the left side when enabled */
    TSharedPtr<STextBlock> LineNumbers;

    /** The scroll box that wraps the entire content and provides scrolling functionality */
    TSharedPtr<SScrollBox> WrapperScrollBox;
#pragma endregion

#pragma region State Management
    /** The file path associated with this text box's content */
    FString FilePath;

    /** The current number of lines in the text content */
    int32 LineCount = 0;

    /** Whether the line number column is enabled and should be displayed */
    bool bEnableLineNumberColumn = true;

    /** The target line number to scroll to when the widget becomes visible (-1 = no target) */
    int32 ScrollTarget = -1;

    /** Reference to the main editor container that owns this text box wrapper */
    UMainEditorContainer* MainEditorContainer = nullptr;
#pragma endregion

#pragma region Internal Methods
    /**
     * Updates the line numbers display based on the current text content
     * This method is called automatically when the text changes and line numbers are enabled
     */
    void UpdateLineNumbers();
#pragma endregion
};