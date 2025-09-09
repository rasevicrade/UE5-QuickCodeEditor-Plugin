// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SQCE_MultiLineEditableTextBox;
class QCE_AIMessageList;
class UMainEditorContainer;

/**
 * Enum representing available AI providers
 */
UENUM()
enum class EQCEAIProvider : uint8
{
	Claude		UMETA(DisplayName = "Claude"),
	ChatGPT		UMETA(DisplayName = "ChatGPT")
};

/**
 * Enum representing available AI context options
 */
UENUM()
enum class EQCEAIContext : uint8
{
	VisibleCode		UMETA(DisplayName = "Visible code"),
	UserSelection	UMETA(DisplayName = "User selection")
};

/**
 * QCE_AIContainer manages the AI conversation interface in the code editor.
 * This widget is responsible for:
 * - Displaying the AI conversation history
 * - Providing user input interface for messages
 * - Managing conversation state for specific functions
 * - Handling message sending and responses
 */
class QUICKCODEEDITOR_API QCE_AIContainer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(QCE_AIContainer)
		: _Visibility(EVisibility::Visible)
	{}
		SLATE_ATTRIBUTE(EVisibility, Visibility)
	SLATE_END_ARGS()

	/** Constructs and initializes the widget */
	void Construct(const FArguments& InArgs);

	/** 
	 * Loads an existing conversation for the specified function.
	 * @param FunctionName - Name of the function to load conversation for
	 * @param ClassName - Name of the class containing the function
	 * @param FilePath - Path to the file containing the function
	 * @param FunctionContent - Optional content of the function
	 */
	void LoadConversationForFunction(const FString& FunctionName, const FString& ClassName,
		const FString& FilePath, const FString& FunctionContent = TEXT(""));

	/** Clears the current conversation and resets the widget state */
	void ClearConversation();

	/** Resets the conversation by clearing user and assistant messages while keeping function context */
	void ResetConversation();

	/**
	 * Sets the reference to the main editor container.
	 * @param InMainEditor - Pointer to the main editor container
	 */
	void SetMainEditorContainer(UMainEditorContainer* InMainEditor);

private:
	/** Widget displaying the conversation history between user and AI */
	TSharedPtr<QCE_AIMessageList> MessageList;

	/** Text input widget for composing user messages */
	TSharedPtr<SQCE_MultiLineEditableTextBox> InputTextBox;

	/** Dropdown widget for selecting AI provider */
	TSharedPtr<SComboBox<TSharedPtr<EQCEAIProvider>>> AIProviderComboBox;

	/** Dropdown widget for selecting AI context */
	TSharedPtr<SComboBox<TSharedPtr<EQCEAIContext>>> AIContextComboBox;

	/** Button widget that triggers message sending */
	TSharedPtr<SButton> SendButton;

	/** Close button widget that resets the conversation */
	TSharedPtr<SButton> CloseButton;

	/** List of available AI providers for the dropdown */
	TArray<TSharedPtr<EQCEAIProvider>> AIProviders;

	/** Currently selected AI provider */
	TSharedPtr<EQCEAIProvider> SelectedAIProvider;

	/** List of available AI context options for the dropdown */
	TArray<TSharedPtr<EQCEAIContext>> AIContextOptions;

	/** Currently selected AI context */
	TSharedPtr<EQCEAIContext> SelectedAIContext;

	/** Whether there are currently messages in the conversation (excluding context) */
	bool bHasMessages = false;

	/** Whether a node is currently selected in the Blueprint editor */
	bool bIsNodeSelected = false;

	/** 
	 * Handles the send button click event.
	 * @return FReply indicating if the event was handled
	 */
	FReply SendMessage();

	/** 
	 * Handles the close button click event.
	 * @return FReply indicating if the event was handled
	 */
	FReply OnCloseClicked();

	/** 
	 * Processes the AI's response to a message.
	 * @param Response - The response text from the AI
	 * @param bSuccess - Whether the response was successfully received
	 */
	void HandleMessageResponse(const FString& Response, bool bSuccess) const;

	/**
	 * Gets the display text for an AI provider option.
	 * @param InProvider - The AI provider to get display text for
	 * @return FText representing the provider name
	 */
	FText GetAIProviderDisplayText(TSharedPtr<EQCEAIProvider> InProvider) const;

	/**
	 * Handles selection change in the AI provider dropdown.
	 * @param SelectedItem - The newly selected AI provider
	 * @param SelectInfo - Selection type information
	 */
	void OnAIProviderSelectionChanged(TSharedPtr<EQCEAIProvider> SelectedItem, ESelectInfo::Type SelectInfo);

	/**
	 * Gets the display text for an AI context option.
	 * @param InContext - The AI context to get display text for
	 * @return FText representing the context name
	 */
	FText GetAIContextDisplayText(TSharedPtr<EQCEAIContext> InContext) const;

	/**
	 * Handles selection change in the AI context dropdown.
	 * @param SelectedItem - The newly selected AI context
	 * @param SelectInfo - Selection type information
	 */
	void OnAIContextSelectionChanged(TSharedPtr<EQCEAIContext> SelectedItem, ESelectInfo::Type SelectInfo);

	/** Unique key identifying the current conversation context */
	FString CurrentConversationKey;

	/** Reference to the main editor container for accessing selected text */
	UMainEditorContainer* MainEditorContainer = nullptr;

	/** Updates the enabled state of the close button based on whether there are messages */
	void UpdateCloseButtonState();

	/** Updates the enabled state of input controls based on node selection */
	void UpdateInputControlsState();

	/**
	 * Gets the selected text from the active editor.
	 * @return Selected text from the currently active text editor, or empty string if no selection
	 */
	FString GetSelectedTextFromActiveEditor() const;
};
