// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/Utility/QCE_ContextMenuBuilder.h"

#include "Editor/MainEditorContainer.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/FindAndReplace/QCE_FindAndReplaceContainer.h"
#include "Editor/CustomTextBox/GoToLine/QCE_GoToLineContainer.h"
#include "Editor/CustomTextBox/GenerateDefinition/QCE_GenerateDefinitionHelpers.h"
#include "Editor/QCECommands.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/MultiBox/MultiBox.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "CodeEditor"

QCE_ContextMenuBuilder::QCE_ContextMenuBuilder(
    const TSharedPtr<QCE_FindAndReplaceContainer>& InSearchContainer,
    const TSharedPtr<SQCE_MultiLineEditableTextBox>& InTargetTextBox,
    UMainEditorContainer* InCodeEditor
)
    : SearchContainer(InSearchContainer)
    , TargetTextBox(InTargetTextBox)
    , CodeEditor(InCodeEditor)
{
    CommandList = MakeShareable(new FUICommandList);
    
    RegisterEditorContextMenuCommands();
}

void QCE_ContextMenuBuilder::RegisterEditorContextMenuCommands()
{
    if (!FQCECommands::IsRegistered())
    {
        FQCECommands::Register();
    }
    CommandList->MapAction(
        FQCECommands::Get().FindAndReplace,
        FExecuteAction::CreateRaw(this, &QCE_ContextMenuBuilder::OnFindAndReplaceClicked),
        FCanExecuteAction()
    );

    CommandList->MapAction(
        FQCECommands::Get().GenerateDefinition,
        FExecuteAction::CreateRaw(this, &QCE_ContextMenuBuilder::OnGenerateDefinitionClicked),
        FCanExecuteAction()
    );

    CommandList->MapAction(
        FQCECommands::Get().OpenInExplorer,
        FExecuteAction::CreateLambda([this]()
        {
            if (CodeEditor.IsValid())
            {
                const FString& FilePath = TargetTextBox->GetParentTextBoxWrapper()->GetFilePath();
                OnOpenInExplorerClicked(FilePath);
            }
        }),
        FCanExecuteAction()
    );

    CommandList->MapAction(
        FQCECommands::Get().GoToLine,
        FExecuteAction::CreateLambda([this]()
        {
            if (CodeEditor.IsValid())
            {
                CodeEditor->ToggleGoToLineContainer();
            }
        }),
        FCanExecuteAction()
    );
    
}

void QCE_ContextMenuBuilder::AddEditorMenuEntries(FMenuBuilder& MenuBuilder) const
{
    MenuBuilder.PushCommandList(CommandList.ToSharedRef());
    MenuBuilder.BeginSection("Code Editor", LOCTEXT("Heading", "Code Editor"));
    {
        AddGenerateDefinitionFunction(MenuBuilder);
        MenuBuilder.AddMenuEntry(FQCECommands::Get().FindAndReplace);
        MenuBuilder.AddMenuEntry(FQCECommands::Get().GoToLine);
        MenuBuilder.AddMenuEntry(FQCECommands::Get().OpenInExplorer);
    }
    MenuBuilder.EndSection();
    MenuBuilder.PopCommandList();
}


void QCE_ContextMenuBuilder::AddGenerateDefinitionFunction(FMenuBuilder& MenuBuilder) const
{
    if (!CodeEditor.IsValid() || !TargetTextBox.IsValid())
    {
        return;
    }

    bool bIsDeclarationEditor = false;
    if (CodeEditor->GetDeclarationTextBoxWrapper().IsValid() && !CodeEditor->IsLoadIsolated())
    {
        bIsDeclarationEditor = (TargetTextBox == CodeEditor->GetDeclarationTextBoxWrapper()->GetTextBox());
    }

    if (!bIsDeclarationEditor)
    {
        return; 
    }

    FString FunctionDeclaration;
    if (QCE_GenerateDefinitionHelpers::HasDeclarationAtCursor(TargetTextBox, FunctionDeclaration))
    {
        MenuBuilder.AddMenuEntry(FQCECommands::Get().GenerateDefinition);
    }
}

void QCE_ContextMenuBuilder::OnFindAndReplaceClicked() const
{
    if (!TargetTextBox.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnFindAndReplaceClicked: TargetTextBox is not valid"));
        return;
    }
    
    if (!SearchContainer.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnFindAndReplaceClicked: SearchContainer is not valid"));
        return;
    }
    
    if (!CodeEditor.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnFindAndReplaceClicked: CodeEditor is not valid"));
        return;
    }

    FString WordUnderCursor = TargetTextBox->GetWordAtCursor();
    
    if (!WordUnderCursor.IsEmpty())
    {
        SearchContainer->SetFindText(WordUnderCursor);
    }
    SearchContainer->SetVisibility(EVisibility::Visible);
    SearchContainer->SetEditorContainer(CodeEditor.Get());
    SearchContainer->FocusFindTextBox();
}


void QCE_ContextMenuBuilder::OnGenerateDefinitionClicked() const
{
    if (!TargetTextBox.IsValid() || !CodeEditor.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnGenerateDefinitionClicked: Invalid components"));
        return;
    }

    FString FunctionDeclaration;
    if (QCE_GenerateDefinitionHelpers::HasDeclarationAtCursor(TargetTextBox, FunctionDeclaration))
    {
        FTextLocation InsertLocation;
        if (QCE_GenerateDefinitionHelpers::TryGenerateAndInsertDefinition(CodeEditor.Get(), InsertLocation))
        {
            CodeEditor->SwitchToTab(1); 
            TSharedPtr<SQCE_MultiLineEditableTextBox> ImplementationTextBox = CodeEditor->GetImplementationTextBoxWrapper()->GetTextBox();
            if (ImplementationTextBox.IsValid())
            {
                ImplementationTextBox->GetParentTextBoxWrapper()->ScrollToLine(InsertLocation.GetLineIndex());
            }
        }
    }
    
}

void QCE_ContextMenuBuilder::OnOpenInExplorerClicked(const FString& FilePath) const
{
    if (!FilePath.IsEmpty())
    {
        const FString FileDir = FPaths::GetPath(FilePath);
        if (IFileManager::Get().DirectoryExists(*FileDir))
        {
            FPlatformProcess::ExploreFolder(*FilePath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Directory does not exist: %s"), *FileDir);
        }
    } else
    {
        UE_LOG(LogTemp, Warning, TEXT("Filepath empty"));
    }
}

#undef LOCTEXT_NAMESPACE
