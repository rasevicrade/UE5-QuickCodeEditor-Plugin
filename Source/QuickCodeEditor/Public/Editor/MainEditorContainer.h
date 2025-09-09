// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "CustomTextBox/Utility/CppIO/QCE_IOTypes.h"
#include "CustomTextBox/Utility/CppIO/FunctionCppReader.h"
#include "BlueprintEditorTabs.h"
#include "Framework/Docking/TabManager.h"
#include "MainEditorContainer.generated.h"


class QCE_MultiLineEditableTextBoxWrapper;
class SQCE_MultiLineEditableTextBox;
class FQCE_TextLayout;
class FCPPSyntaxHighlighterMarshaller;

class QCE_AIContainer;
class QCE_FindAndReplaceContainer;
class QCE_GoToLineContainer;
class QCE_ContextMenuBuilder;

class UK2Node_CallFunction;

/**
 * UMainEditorContainer is the main controller for the QuickCodeEditor plugin.
 * 
 * This class provides a comprehensive code editing interface that integrates with Unreal's Blueprint system,
 * allowing developers to view and edit the underlying C++ implementation of Blueprint function nodes.
 * 
 * Key Features:
 * - Dual-pane editor for function declarations (.h files) and implementations (.cpp files)
 * - Blueprint node integration with automatic code loading
 * - AI-powered code assistance and conversation tracking
 * - Advanced text editing features (find/replace, go-to-line, syntax highlighting)
 * - Live coding integration with save and build functionality
 * - Context-aware menus and keyboard shortcuts
 */
UCLASS()
class QUICKCODEEDITOR_API UMainEditorContainer : public UObject
{
	GENERATED_BODY()

#pragma region Core Interface
public:
	/** Default constructor */
	UMainEditorContainer();
	
	/** Virtual destructor to ensure proper cleanup */
	virtual ~UMainEditorContainer() override;
	
	/** Gets the declaration text box wrapper for external access */
	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& GetDeclarationTextBoxWrapper() { return DeclarationEditorTextBoxWrapper; }

	/** Gets the implementation text box wrapper for external access */
	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& GetImplementationTextBoxWrapper() { return ImplementationEditorTextBoxWrapper; }
	
	/** Gets the current function declaration info, or nullptr if no function is loaded */
	const FFunctionDeclarationInfo* GetCurrentFunctionDeclarationInfo() const { return &DeclarationInfo; }

	const FFunctionImplementationInfo* GetCurrentFunctionImplementationInfo() const { return &ImplementationInfo; }

	/** Gets the current function being edited, or nullptr if no function is loaded */
	const UFunction* GetCurrentFunction() const { return CurrentEditedFunction; }

	/** Gets the function reader instance for this container */
	FFunctionCppReader& GetFunctionReader() { return FunctionReader; }

	/** Sets the Blueprint editor that owns this container */
	void SetOwnerBlueprintEditor(TSharedPtr<class FBlueprintEditor> InOwnerBlueprintEditor) { OwnerBlueprintEditor = InOwnerBlueprintEditor; }

	/** Gets the Blueprint editor that owns this container */
	TSharedPtr<class FBlueprintEditor> GetOwnerBlueprintEditor() const { return OwnerBlueprintEditor.Pin(); }

	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& GetActiveTextBoxWrapper();
	
	/** Marks the implementation text box as modified and shows the indicator */
	void MarkImplementationAsModified();

	bool IsLoadIsolated() const { return bLoadIsolated; }
private:
	/** Writes updated function code to both header and implementation files */
	void WriteUpdatedFunctionCode(const FString& UpdatedFunctionHeaderCode, const FString& UpdatedFunctionImplementationCode, const bool bForceOverwrite = false);

	/** Checks if a file should be read-only based on file system status and UE engine source detection */
	bool ShouldFileBeReadOnly(const FString& FilePath) const;
#pragma endregion
	
#pragma region Editor Initialization & Management
public:
	/** Initializes and returns the editor widget with all necessary UI elements */
	TSharedRef<SDockTab> InitQuickCodeEditor();
	
	/** Clean up handlers and delegates when the tab is being closed */
	void CleanupOnTabClosed();
private:
	/** Sets up the listener for blueprint node selection changes */
	void SetNodeSelectionListener();

	/** Check if a node is already selected in currently focused blueprint editor, and update code editor with it's code. */
	void CheckSelectedNode();
	
	/** Creates and returns a C++ syntax highlighter instance for the editor */
	TSharedPtr<FCPPSyntaxHighlighterMarshaller> GetCppMarshaller();
#pragma endregion

#pragma region Configuration & State
private:
	/** Whether to load functions in isolation mode */
	bool bLoadIsolated = true;

	/** Current active tab index (0 = Declaration, 1 = Implementation) */
	int32 CurrentTabIndex = 1; // Default to implementation tab

	/** Whether a node is currently selected in the Blueprint editor */
	bool bIsNodeSelected = false;

	/** Whether the search interface is currently visible */
	bool bIsSearchVisible = false;

	/** Whether the go to line interface is currently visible */
	bool bIsGoToLineVisible = false;

	/** Whether the AI container is collapsed */
	bool bIsAIContainerCollapsed = true;

	/** Last known size of the AI container */
	float LastAIContainerSize = 0.3f;

	/** Flags to track whether text changes are from node changes (not user modifications) */
	bool bIsNodeChangeImplementationUpdate = false;
	bool bIsNodeChangeDeclarationUpdate = false;

	/** Tracks how the declaration content was originally loaded (true = isolated, false = full file) */
	bool bDeclarationLoadedIsolated = true;

	/** Tracks how the implementation content was originally loaded (true = isolated, false = full file) */
	bool bImplementationLoadedIsolated = true;
#pragma endregion

#pragma region Data Storage
public:
	FFunctionCppReader GetFunctionReader() const { return FunctionReader; }
private:
	/** Currently selected blueprint node */
	UPROPERTY()
	const UObject* SelectedNode;

	/** Reference to the currently edited UFunction */
	UFunction* CurrentEditedFunction;

	/** The Blueprint editor that owns this container instance */
	TWeakPtr<FBlueprintEditor> OwnerBlueprintEditor;

	/** Information about the current function's declaration */
	FFunctionDeclarationInfo DeclarationInfo;

	/** Information about the current function's implementation */
	FFunctionImplementationInfo ImplementationInfo;

	/** Instance of FFunctionCppReader for this container */
	FFunctionCppReader FunctionReader;
#pragma endregion

#pragma region UI Components
private:
	/** Weak pointer to the current editor tab instance */
	TWeakPtr<SDockTab> CodeEditorTab;

	/** Widget switcher for tabbed interface between declaration and implementation */
	TSharedPtr<SWidgetSwitcher> CodeEditorSwitcher;

	/** The main text editing widget for declaration code */
	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> DeclarationEditorTextBoxWrapper;

	/** The main text editing widget for implementation code */
	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> ImplementationEditorTextBoxWrapper;

	/** Text layout managers for declaration and implementation editors */
	TSharedPtr<FQCE_TextLayout> DeclarationTextLayout;
	TSharedPtr<FQCE_TextLayout> ImplementationTextLayout;

	/** C++ syntax highlighter marshallers for both editors */
	TSharedPtr<FCPPSyntaxHighlighterMarshaller> DeclarationMarshaller;
	TSharedPtr<FCPPSyntaxHighlighterMarshaller> ImplementationMarshaller;

	/** Tab buttons for switching between declaration and implementation views */
	TSharedPtr<SButton> DeclarationTabButton;
	TSharedPtr<SButton> ImplementationTabButton;

	/** Modification indicator icons (filled circles) for both editors */
	TSharedPtr<SImage> DeclarationModifiedIndicator;
	TSharedPtr<SImage> ImplementationModifiedIndicator;

	/** Action buttons and their icons */
	TSharedPtr<SImage> SaveIcon;
	TSharedPtr<SImage> SaveAndBuildIcon;

	/** AI container widget for code assistance */
	TSharedPtr<QCE_AIContainer> AIContainer;

	/** Button to collapse/expand the AI container */
	TSharedPtr<SButton> CollapseButton;

	/** Container for find and replace functionality */
	TSharedPtr<QCE_FindAndReplaceContainer> SearchContainer;

	/** Container for go-to-line functionality */
	TSharedPtr<QCE_GoToLineContainer> GoToLineContainer;



	/** Handle for the global tab manager's OnActiveTabChanged delegate */
	FDelegateHandle TabChangedHandle;
#pragma endregion

#pragma region Blueprint Integration
private:
	/** Called when blueprint node selection changes in the editor */
	void OnBlueprintNodesSelected(const FGraphPanelSelectionSet& SelectedNodes);

	/** Refreshes the editor code display when a new node is selected */
	void RefreshEditorCode(const UObject* InNewSelectedNode = nullptr);

	/** Retrieves the C++ function implementation for a given blueprint node */
	bool GetImplementationCodeForNode(const UObject* InSelectedNode, FString& OutFunctionCode);

	/** Retrieves the C++ function declaration for a given blueprint node */
	bool GetDeclarationCodeForNode(const UObject* InSelectedNode, FString& OutFunctionDeclaration);

	/** Binds the node selection listener to the currently focused graph in the owner Blueprint editor */
	void BindToCurrentBlueprintGraph();
	
	void BindToGraphTabChanges();

	/** Loads an AI conversation for the currently selected function */
	void LoadAIConversationForFunction(const FString& TextBoxContent) const;
#pragma endregion

#pragma region User Actions & UI Events
public: 
	/** Switches to the specified tab and updates button states */
	void SwitchToTab(int32 TabIndex);
	
	/** Toggles the visibility of the go-to-line container */
	void ToggleGoToLineContainer();
private:
	/** Handles save button click - saves current code changes to disk */
	FReply TrySaveDeclarationAndImplementation(const bool bForceOverwrite = false);

	/** Handles save and build button click - saves changes and triggers a build */
	FReply OnSaveAndBuildClicked();

	/** Extends the code editor menu with additional functionality */
	void ExtendCodeEditorMenu(FMenuBuilder& MenuBuilder, TSharedPtr<QCE_ContextMenuBuilder> ContextMenuBuilder);

	/** Toggles the visibility of the find and replace container */
	void ToggleSearchContainer();

	/** Updates the enabled state of Save and Save and Build buttons based on unsaved changes */
	void UpdateSaveButtonsState() const;

	/** Checks if there are unsaved changes in either text editor */
	bool HasUnsavedChanges() const;

	/** Checks and handles scroll targets for the specified tab */
	void CheckScrollTarget(int32 TabIndex) const;

	/** Checks if code was changed outside of the editor */
	void CheckIfCodeWasChangedOutsideOfEditor();

private:
	TSharedPtr<QCE_ContextMenuBuilder> ImplementationContextMenuBuilder;
	TSharedPtr<QCE_ContextMenuBuilder> DeclarationContextMenuBuilder;
#pragma endregion
};