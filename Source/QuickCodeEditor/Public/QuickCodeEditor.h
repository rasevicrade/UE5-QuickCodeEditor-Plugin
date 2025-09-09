// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandList.h"
#include "Engine/Blueprint.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

class FDropdownCodeCompletionEngine;
class UMainEditorContainer;
class FToolBarBuilder;
class FBlueprintEditor;
class IBlueprintEditor;

/** Struct to hold editor instance data */
struct FEditorInstanceData
{
	/** The main editor container instance */
	UMainEditorContainer* Container;
	
	/** The tab factory for this editor instance */
	FWorkflowAllowedTabSet TabFactory;

	FEditorInstanceData() : Container(nullptr) {}
	FEditorInstanceData(UMainEditorContainer* InContainer, const FWorkflowAllowedTabSet& InTabFactory)
		: Container(InContainer)
		, TabFactory(InTabFactory)
	{}
};

QUICKCODEEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogQuickCodeEditor, Log, All);
static const FName QuickCodeEditorID(TEXT("QuickCodeEditor"));

/**
 * The main functionality is implemented in the UMainEditorContainer class, which this module
 * instantiates and manages through the tab system.
 *
 * a) Each blueprint editor has its own QCE tab
 * b) A spawner for QCE tab is registered when tabs are registered for blueprint editor (on OnRegisterTabsForEditor() event)
 * c) Layout extender makes sure that spawner is ready to invoke QCE tab docked to the bottom of BP editor
 * d) QCE tab is toggled when user clicks the top menu button
 * 
 * @see UMainEditorContainer for the main editor implementation
 * @see UQCE_EditorSettings for plugin configuration options
 */
class FQuickCodeEditorModule final : public IModuleInterface
{
public:
	/** Called when the module is loaded. Initializes commands, settings, and UI integration. */
	virtual void StartupModule() override;
	
	/** Called when the module is unloaded. Cleans up registered settings and commands. */
	virtual void ShutdownModule() override;

private:
	/** Registers QuickCodeEditor commands and integrates them with the Blueprint editor toolbar. */
	void RegisterQceToggleButton();

	/** Registers QCE tab spawner and layout extensions with Blueprint editor module. */
	void RegisterQceTabSpawner();
	
	/** Registers QCE tab factory for a specific Blueprint editor instance. */
	void AddQceTabFactoryForBPEditor(FWorkflowAllowedTabSet& WorkflowAllowedTabs, FName Name, TSharedPtr<FBlueprintEditor> BlueprintEditor);
	
	/** Opens or focuses the QCE tab, creating it if necessary for the current Blueprint editor. */
	void TryInvokeQceTab();
	
	/** Cleans up closed Blueprint editor references from the tracking map. */
	void CleanupClosedEditors();
	
	/** 
	 * Adds the QuickCodeEditor toggle button to the Blueprint editor toolbar.
	 * @param ToolBarBuilder The toolbar builder used to add UI elements to the Blueprint editor
	 */
	void RegisterQceButton(FToolBarBuilder& ToolBarBuilder);

	/** 
	 * Finds and returns the currently focused Blueprint editor instance.
	 * Iterates through all open assets to find Blueprint editors and determines
	 * which one was most recently activated.
	 * @return Pointer to the most recently focused FBlueprintEditor, or nullptr if none found
	 */
	static FBlueprintEditor* GetCurrentlyFocusedBlueprintEditor();

	/** Extends Blueprint editor layout to dock QCE tab after the Bookmarks tab. */
	void DockQceTabToBottom(FLayoutExtender& LayoutExtender);

	FWorkflowAllowedTabSet QuickCodeEditorTabFactory;
	
	/** Command list for QCE toolbar button actions and bindings. */
	TSharedPtr<FUICommandList> CommandList;
	
	TUniquePtr<FDropdownCodeCompletionEngine> CompletionEngine;
	TSharedPtr<FTabManager> TabManager;

public:
	/** Map tracking Blueprint editors to their QCE instance data and tab factories. */
	TMap<FBlueprintEditor*, FEditorInstanceData> EditorInstanceMap;

	FDropdownCodeCompletionEngine* GetCodeCompletionEngine() const { return CompletionEngine.Get(); };

#pragma region Settings Management Methods
	/** 
	 * Registers the plugin's settings with Unreal's settings system.
	 * Makes UQCE_EditorSettings available in the Project Settings under Plugins > Quick Code Editor.
	 */
	static void RegisterQceSettings();

	/** 
	 * Unregisters the plugin's settings from Unreal's settings system.
	 * Called during module shutdown to clean up settings registration.
	 */
	static void UnregisterSettings();
#pragma endregion

};
