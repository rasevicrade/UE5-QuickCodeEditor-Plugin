// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "QuickCodeEditor.h"

#include <Editor/QCECommands.h>

#include "CoreMinimal.h"
#include "Editor.h"
#include "EditorSubsystem.h"
#include "BlueprintEditorModule.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Editor/MainEditorContainer.h"
#include "Editor/CodeEditorCommands.h"
#include "Settings/UQCE_EditorSettings.h"
#include "BlueprintEditor.h"
#include "Editor/FQCESummoner.h"
#include "Editor/CustomTextBox/CodeCompletion/DropdownCodeCompletionEngine.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "FQuickCodeEditorModule"

DEFINE_LOG_CATEGORY(LogQuickCodeEditor);

void FQuickCodeEditorModule::StartupModule()
{
	RegisterQceToggleButton();
	RegisterQceSettings();
	RegisterQceTabSpawner();

	FQCECommands::Register();
	CompletionEngine = MakeUnique<FDropdownCodeCompletionEngine>();
	CompletionEngine->Initialize();
}

void FQuickCodeEditorModule::ShutdownModule()
{
	for (auto& Pair : EditorInstanceMap)
	{
		if (Pair.Value.Container)
		{
			Pair.Value.Container->RemoveFromRoot();
		}
	}
	EditorInstanceMap.Empty();
	CompletionEngine.Reset();
	
	UnregisterSettings();
	FQCECommands::Unregister();
}

void FQuickCodeEditorModule::RegisterQceToggleButton()
{
	FCodeEditorCommands::Register();
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	CommandList = MakeShareable(new FUICommandList);
    
	CommandList->MapAction(
		FCodeEditorCommands::Get().CodeEditor_Open,
		FExecuteAction::CreateRaw(this, &FQuickCodeEditorModule::TryInvokeQceTab),
		FCanExecuteAction()); 
	
	ToolbarExtender->AddToolBarExtension(
		"Settings",
		EExtensionHook::After, 
		CommandList,
		FToolBarExtensionDelegate::CreateRaw(this, &FQuickCodeEditorModule::RegisterQceButton)
	);
	
	FBlueprintEditorModule& BlueprintEditorModule = 
		FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.GetMenuExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FQuickCodeEditorModule::RegisterQceTabSpawner()
{
	FBlueprintEditorModule& BlueprintEditorModule = 
		FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

	BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FQuickCodeEditorModule::AddQceTabFactoryForBPEditor);
	BlueprintEditorModule.OnRegisterLayoutExtensions().AddRaw(this, &FQuickCodeEditorModule::DockQceTabToBottom);
}

void FQuickCodeEditorModule::AddQceTabFactoryForBPEditor(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	if (!BlueprintEditor.IsValid())
		return;
    
	TSharedPtr<FAssetEditorToolkit> BPEditorShared = StaticCastSharedPtr<FAssetEditorToolkit>(BlueprintEditor);
	if (!BPEditorShared.IsValid())
		return;
    
	FEditorInstanceData& InstanceData = EditorInstanceMap.FindOrAdd(BlueprintEditor.Get());
	InstanceData.TabFactory.RegisterFactory(MakeShareable(new FQCESummoner(BPEditorShared)));
	TabFactories.RegisterFactory(MakeShareable(new FQCESummoner(BPEditorShared)));
}

void FQuickCodeEditorModule::RegisterQceButton(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.AddToolBarButton(
	FCodeEditorCommands::Get().CodeEditor_Open,
	NAME_None,
	LOCTEXT("ToggleCodeEditor", "Toggle Code Editor"),
	LOCTEXT("CodeEditorTooltip", "Toggle quick code editor."),
	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.TextEditor"));
}

void FQuickCodeEditorModule::TryInvokeQceTab()
{
	FBlueprintEditor* BPEditor = GetCurrentlyFocusedBlueprintEditor();
	if (!BPEditor)
	{
		UE_LOG(LogQuickCodeEditor, Error, TEXT("Failed to get focused blueprint editor."));
		return;
	}
	
	TabManager = BPEditor->GetTabManager();
	if (!TabManager.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Error, TEXT("Failed to get valid tab manager"));
		return;
	}
	
	CleanupClosedEditors();
	
	const FEditorInstanceData* ExistingData = EditorInstanceMap.Find(BPEditor);
	if (TabManager->HasTabSpawner(QuickCodeEditorID))
	{
		TSharedPtr<SDockTab> ExistingTab = TabManager->FindExistingLiveTab(QuickCodeEditorID);
		if (ExistingTab.IsValid())
		{
			ExistingTab->RequestCloseTab();
			return;
		}
		else
		{
			TabManager->TryInvokeTab(QuickCodeEditorID);
			return;
		}
	}
	if (!TabManager->HasTabSpawner(QuickCodeEditorID))
	{
		TSharedPtr<FAssetEditorToolkit> BPEditorShared = StaticCastSharedRef<FAssetEditorToolkit>(BPEditor->AsShared());
		FEditorInstanceData& InstanceData = EditorInstanceMap.Add(BPEditor);
		InstanceData.TabFactory.RegisterFactory(MakeShareable(new FQCESummoner(BPEditorShared)));
		
		BPEditor->PushTabFactories(InstanceData.TabFactory);
	}
	
	if (TabManager->HasTabSpawner(QuickCodeEditorID))
		TabManager->TryInvokeTab(QuickCodeEditorID);
}

void FQuickCodeEditorModule::RegisterQceSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		TSharedPtr<ISettingsSection> SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "Code Editor",
			LOCTEXT("RuntimeSettingsName", "Code Editor"),
			LOCTEXT("RuntimeSettingsDescription", "Configure Code Editor plugin"),
			GetMutableDefault<UQCE_EditorSettings>()
		);

		// Bind the reset defaults delegate to our custom reset method
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnResetDefaults().BindUObject(GetMutableDefault<UQCE_EditorSettings>(), &UQCE_EditorSettings::ResetToDefaults);
		}
	}
}

void FQuickCodeEditorModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Code Editor");
	}
}

FBlueprintEditor* FQuickCodeEditorModule::GetCurrentlyFocusedBlueprintEditor()
{
	if (!GEditor)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("GEditor is null, cannot find Blueprint editor"));
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("AssetEditorSubsystem is null, cannot find Blueprint editor"));
		return nullptr;
	}

	TArray<UObject*> OpenAssets = AssetEditorSubsystem->GetAllEditedAssets();
    
	FBlueprintEditor* MostRecentBlueprintEditor = nullptr;
	double MostRecentActivationTime = 0.0;

	for (UObject* Asset : OpenAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		if (!Blueprint)
		{
			continue;
		}

		TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(Blueprint);
            
		for (IAssetEditorInstance* EditorInstance : Editors)
		{
			if (!EditorInstance)
			{
				continue;
			}

			if (EditorInstance->GetEditorName() != FName("BlueprintEditor"))
			{
				continue;
			}

			FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(EditorInstance);
			if (!BlueprintEditor)
			{
				continue;
			}

			double ActivationTime = BlueprintEditor->GetLastActivationTime();
			if (ActivationTime <= MostRecentActivationTime)
			{
				continue;
			}

			MostRecentActivationTime = ActivationTime;
			MostRecentBlueprintEditor = BlueprintEditor;
		}
	}

	if (MostRecentBlueprintEditor)
	{
		UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found most recent Blueprint editor (activation time: %f)"), MostRecentActivationTime);
	}
	else
	{
		UE_LOG(LogQuickCodeEditor, Log, TEXT("No Blueprint editor found in current session"));
	}

	return MostRecentBlueprintEditor;
}

void FQuickCodeEditorModule::DockQceTabToBottom(FLayoutExtender& LayoutExtender)
{
	LayoutExtender.ExtendLayout(
		FBlueprintEditorTabs::BookmarksID, 
		ELayoutExtensionPosition::After, 
		FTabManager::FTab(QuickCodeEditorID, ETabState::ClosedTab)
	);
}

void FQuickCodeEditorModule::CleanupClosedEditors()
{
	if (!GEditor)
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		return;
	}

	TArray<UObject*> OpenAssets = AssetEditorSubsystem->GetAllEditedAssets();
	TSet<FBlueprintEditor*> ValidEditors;
	for (UObject* Asset : OpenAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		if (!Blueprint)
		{
			continue;
		}

		TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(Blueprint);
		for (IAssetEditorInstance* EditorInstance : Editors)
		{
			if (EditorInstance && EditorInstance->GetEditorName() == FName("BlueprintEditor"))
			{
				FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(EditorInstance);
				if (BlueprintEditor)
				{
					ValidEditors.Add(BlueprintEditor);
				}
			}
		}
	}

	TArray<FBlueprintEditor*> EditorsToRemove;
	for (auto& Pair : EditorInstanceMap)
	{
		if (!ValidEditors.Contains(Pair.Key))
		{
			EditorsToRemove.Add(Pair.Key);
			
			if (Pair.Value.Container)
			{
				Pair.Value.Container->RemoveFromRoot();
				UE_LOG(LogQuickCodeEditor, Log, TEXT("Cleaned up QCE instance for closed Blueprint Editor"));
			}
		}
	}

	for (FBlueprintEditor* EditorToRemove : EditorsToRemove)
	{
		EditorInstanceMap.Remove(EditorToRemove);
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FQuickCodeEditorModule, QuickCodeEditor)