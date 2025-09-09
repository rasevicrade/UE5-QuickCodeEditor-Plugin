// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/FQCESummoner.h"
#include "BlueprintEditor.h"
#include "QuickCodeEditor.h"
#include "Editor/MainEditorContainer.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FQCESummoner"

FQCESummoner::FQCESummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(QuickCodeEditorID, InHostingApp)
{
	TabLabel = LOCTEXT("QCE_TabTitle", "Code Editor");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("QCE_MenuTitle", "Code Editor");
	ViewMenuTooltip = LOCTEXT("QCE_ToolTip", "Shows the Code Editor interface");
}

TSharedRef<SWidget> FQCESummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(STextBlock)
		.Text(LOCTEXT("QCE_Placeholder", "Code Editor - Use the toolbar button to access the full editor"))
		.AutoWrapText(true)
		.Margin(FMargin(10.0f));
}

TSharedRef<SDockTab> FQCESummoner::SpawnTab(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());
	if (!BlueprintEditorPtr.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Error, TEXT("Failed to get valid Blueprint Editor from HostingApp"));
		return FWorkflowTabFactory::SpawnTab(Info);
	}
	
	FBlueprintEditor* BPEditor = BlueprintEditorPtr.Get();
	
	if (!FModuleManager::Get().IsModuleLoaded("QuickCodeEditor"))
	{
		UE_LOG(LogQuickCodeEditor, Error, TEXT("QuickCodeEditor module not loaded"));
		return FWorkflowTabFactory::SpawnTab(Info);
	}
	
	FQuickCodeEditorModule& QCEModule = FModuleManager::GetModuleChecked<FQuickCodeEditorModule>("QuickCodeEditor");
	
	FEditorInstanceData* ExistingData = QCEModule.EditorInstanceMap.Find(BPEditor);
	UMainEditorContainer* QceInstance = nullptr;
	
	if (ExistingData && ExistingData->Container)
	{
		QceInstance = ExistingData->Container;
		QceInstance->SetOwnerBlueprintEditor(BlueprintEditorPtr);
		UE_LOG(LogQuickCodeEditor, Log, TEXT("Using existing MainEditorContainer for Blueprint Editor"));
	}
	else
	{
		QceInstance = NewObject<UMainEditorContainer>();
		QceInstance->AddToRoot();
		QceInstance->SetOwnerBlueprintEditor(BlueprintEditorPtr);
		
		
		if (ExistingData)
		{
			ExistingData->Container = QceInstance;
		}
		else
		{
			FEditorInstanceData NewInstanceData;
			NewInstanceData.Container = QceInstance;
			QCEModule.EditorInstanceMap.Add(BPEditor, NewInstanceData);
		}
		
		UE_LOG(LogQuickCodeEditor, Log, TEXT("Created new MainEditorContainer for Blueprint Editor"));
	}
	
	TSharedRef<SDockTab> NewTab = QceInstance->InitQuickCodeEditor();

	SetupCodeCompletionForTextBox(QceInstance);
		
	
	NewTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([BPEditor, QceInstance](TSharedRef<SDockTab> ClosedTab)
	{
		QceInstance->CleanupOnTabClosed();
		
		QceInstance->RemoveFromRoot();
		
		if (FModuleManager::Get().IsModuleLoaded("QuickCodeEditor"))
		{
			FQuickCodeEditorModule& QCEModule = FModuleManager::GetModuleChecked<FQuickCodeEditorModule>("QuickCodeEditor");
			FEditorInstanceData* InstanceData = QCEModule.EditorInstanceMap.Find(BPEditor);
			if (InstanceData)
			{
				InstanceData->Container = nullptr;
			}
		}
		
		UE_LOG(LogQuickCodeEditor, Log, TEXT("Released QCE instance for manually closed tab"));
	}));
	
	return NewTab;
}

void FQCESummoner::SetupCodeCompletionForTextBox(UMainEditorContainer* QceInstance) const
{
	if (!QceInstance)
	{
		UE_LOG(LogQuickCodeEditor, Error, TEXT("Cannot setup code completion: QceInstance is null"));
		return;
	}

	FQuickCodeEditorModule& QCEModule = FModuleManager::GetModuleChecked<FQuickCodeEditorModule>("QuickCodeEditor");
	FDropdownCodeCompletionEngine* CompletionEngine = QCEModule.GetCodeCompletionEngine();
	if (!CompletionEngine)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Cannot setup code completion: CompletionEngine is not available"));
		return;
	}
	

	if (TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> ImplementationTextBoxWrapper = QceInstance->GetImplementationTextBoxWrapper())
	{
		TSharedPtr<SQCE_MultiLineEditableTextBox> ImplementationTextBox = ImplementationTextBoxWrapper->GetTextBox();
		if (ImplementationTextBox.IsValid())
			ImplementationTextBox->SetCodeCompletionEngine(CompletionEngine);
		
	}


	if (TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> DeclarationTextBoxWrapper = QceInstance->GetDeclarationTextBoxWrapper())
	{
		TSharedPtr<SQCE_MultiLineEditableTextBox> DeclarationTextBox = DeclarationTextBoxWrapper->GetTextBox();
		if (DeclarationTextBox.IsValid())
			DeclarationTextBox->SetCodeCompletionEngine(CompletionEngine);
	}
}

#undef LOCTEXT_NAMESPACE