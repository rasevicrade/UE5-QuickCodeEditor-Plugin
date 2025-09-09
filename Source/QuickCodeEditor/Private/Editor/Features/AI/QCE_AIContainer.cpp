// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/Features/AI/QCE_AIContainer.h"

#include "Editor/Features/AI/Agents/QCE_AIClientFactory.h"
#include "Editor/Features/AI/Messages/QCE_AIMessageList.h"
#include "Editor/Features/AI/Conversations/QCE_AIConversationTracker.h"
#include "Settings/UQCE_EditorSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/MainEditorContainer.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"

#define LOCTEXT_NAMESPACE "AIContainer"

void QCE_AIContainer::Construct(const FArguments& InArgs)
{
    AIProviders.Add(MakeShareable(new EQCEAIProvider(EQCEAIProvider::Claude)));
    AIProviders.Add(MakeShareable(new EQCEAIProvider(EQCEAIProvider::ChatGPT)));
    
    AIContextOptions.Add(MakeShareable(new EQCEAIContext(EQCEAIContext::VisibleCode)));
    AIContextOptions.Add(MakeShareable(new EQCEAIContext(EQCEAIContext::UserSelection)));
    
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    
    // Set default AI context from settings
    EQCEAIContext DefaultContext = EQCEAIContext::VisibleCode; 
    if (Settings)
    {
        DefaultContext = Settings->DefaultAIContext;
    }
    
    SelectedAIContext = AIContextOptions[0]; // Fallback
    for (const auto& Context : AIContextOptions)
    {
        if (Context.IsValid() && *Context == DefaultContext)
        {
            SelectedAIContext = Context;
            break;
        }
    }
    EQCEAIProvider DefaultProvider = EQCEAIProvider::Claude; 

    if (Settings)
    {
        switch (Settings->DefaultAIProvider)
        {
            case EQCEDefaultAIProvider::Claude:
                DefaultProvider = EQCEAIProvider::Claude;
                break;
            case EQCEDefaultAIProvider::ChatGPT:
            default:
                DefaultProvider = EQCEAIProvider::ChatGPT;
                break;
        }
    }
    
    SelectedAIProvider = AIProviders[0]; 
    for (const auto& Provider : AIProviders)
    {
        if (Provider.IsValid() && *Provider == DefaultProvider)
        {
            SelectedAIProvider = Provider;
            break;
        }
    }

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 8)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(CloseButton, SButton)
                .ButtonStyle(FAppStyle::Get(), "NoBorder")
                .ContentPadding(FMargin(2))
                .OnClicked(this, &QCE_AIContainer::OnCloseClicked)
                .ToolTipText(LOCTEXT("CloseTooltip", "Reset conversation"))
                [
                    SNew(SImage)
                    .Image(FAppStyle::GetBrush("Icons.Delete"))
                    .ColorAndOpacity(FSlateColor::UseForeground())
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(0, 0, 0, 8)
        [
            SAssignNew(MessageList, QCE_AIMessageList)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(InputTextBox, SQCE_MultiLineEditableTextBox)
                .HintText(LOCTEXT("InputHint", "Type your message..."))
                .ToolTipText(LOCTEXT("InputTooltip", "Enter your message to send to the AI"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(AIContextComboBox, SComboBox<TSharedPtr<EQCEAIContext>>)
                .OptionsSource(&AIContextOptions)
                .OnGenerateWidget_Lambda([this](TSharedPtr<EQCEAIContext> InOption)
                {
                    return SNew(STextBlock)
                        .Text(GetAIContextDisplayText(InOption));
                })
                .OnSelectionChanged(this, &QCE_AIContainer::OnAIContextSelectionChanged)
                .InitiallySelectedItem(SelectedAIContext)
                .ToolTipText(LOCTEXT("ContextTooltip", "Select the context to send to AI"))
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        return GetAIContextDisplayText(SelectedAIContext);
                    })
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(8, 0, 0, 0)
            .VAlign(VAlign_Center)
            [
                SAssignNew(AIProviderComboBox, SComboBox<TSharedPtr<EQCEAIProvider>>)
                .OptionsSource(&AIProviders)
                .OnGenerateWidget_Lambda([this](TSharedPtr<EQCEAIProvider> InOption)
                {
                    return SNew(STextBlock)
                        .Text(GetAIProviderDisplayText(InOption));
                })
                .OnSelectionChanged(this, &QCE_AIContainer::OnAIProviderSelectionChanged)
                .InitiallySelectedItem(SelectedAIProvider)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        return GetAIProviderDisplayText(SelectedAIProvider);
                    })
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(8, 0, 0, 0)
            .VAlign(VAlign_Center)
            [
                SAssignNew(SendButton, SButton)
                .Text(LOCTEXT("SendButton", "Send"))
                .ToolTipText(LOCTEXT("SendButtonTooltip", "Send message to AI"))
                .OnClicked(this, &QCE_AIContainer::SendMessage)
            ]
        ]
    ];

    // This is a special case of SQCE_MultiLineEditableTextBox where we want Enter to be used to Send message
    // So we block enter from spawning a new line here
    // and within the implementation of the SQCE_MultiLineEditableTextBox we fire of Enter event
    InputTextBox->SetOnKeyDownHandler(FOnKeyDown::CreateLambda(
        [](const FGeometry& Geometry, const FKeyEvent& KeyEvent) -> FReply
        {
            if (KeyEvent.GetKey() == EKeys::Enter)
            {
                return FReply::Handled();
            }
            return FReply::Unhandled();
        }));
    
    InputTextBox->bIsChatBox = true;
    InputTextBox->OnEnterPressed.BindLambda([this]()
    {
        SendMessage();
    });

    SetVisibility(InArgs._Visibility.Get());
    UpdateCloseButtonState();
    UpdateInputControlsState();
}

void QCE_AIContainer::UpdateCloseButtonState()
{
    if (CloseButton.IsValid())
    {
        CloseButton->SetEnabled(bHasMessages);
    }
}

void QCE_AIContainer::UpdateInputControlsState()
{
    if (InputTextBox.IsValid())
    {
        InputTextBox->SetIsReadOnly(!bIsNodeSelected);
        if (bIsNodeSelected)
        {
            InputTextBox->SetHintText(LOCTEXT("InputHint", "Type your message..."));
        }
        else
        {
            InputTextBox->SetHintText(LOCTEXT("InputHintNoNode", "Select a Blueprint node to start chatting with AI"));
        }
    }
    
    if (SendButton.IsValid())
    {
        SendButton->SetEnabled(bIsNodeSelected);
    }
}

void QCE_AIContainer::LoadConversationForFunction(const FString& FunctionName, const FString& ClassName,
    const FString& FilePath, const FString& FunctionContent)
{
    FQCEAIConversation& Conversation = QCE_AIConversationTracker::Get().GetOrCreateConversation(FunctionName, ClassName, FilePath);
    
    if (!FunctionContent.IsEmpty())
    {
        QCE_AIConversationTracker::Get().SetFunctionContextToConversation(
            Conversation.ConversationKey, 
            FunctionContent
        );
    }
    
    CurrentConversationKey = Conversation.ConversationKey;
    if (MessageList.IsValid())
    {
        MessageList->LoadConversation(Conversation);
    }
    
    bHasMessages = false;
    for (const auto& Message : Conversation.Messages)
    {
        if (Message.MessageType == EQCEMessageType::User || Message.MessageType == EQCEMessageType::Assistant)
        {
            bHasMessages = true;
            break;
        }
    }
    
    bIsNodeSelected = true;
    UpdateCloseButtonState();
    UpdateInputControlsState();
}

FReply QCE_AIContainer::SendMessage()
{
    if (!InputTextBox)
        return FReply::Handled();
    
    FString TrimmedMessage = InputTextBox->GetText().ToString().TrimEnd().TrimStart();
    if (TrimmedMessage.IsEmpty())
        return FReply::Handled();
    
    // Check if User selection context is selected and update conversation context
    if (SelectedAIContext.IsValid() && *SelectedAIContext == EQCEAIContext::UserSelection && !CurrentConversationKey.IsEmpty())
    {
        FString SelectedText = GetSelectedTextFromActiveEditor();
        if (!SelectedText.IsEmpty())
        {
            QCE_AIConversationTracker::Get().SetFunctionContextToConversation(
                CurrentConversationKey, 
                SelectedText
            );
        }
    }

    MessageList->AddUserMessage(FText::FromString(TrimmedMessage));
    if (!CurrentConversationKey.IsEmpty())
    {
        QCE_AIConversationTracker::Get().AddMessageToConversation(
            CurrentConversationKey, 
            EQCEMessageType::User, 
            TrimmedMessage
        );
    }
    
    bHasMessages = true;
    UpdateCloseButtonState();
    
    MessageList->AddLoadingMessage();
    InputTextBox->SetText(FText::GetEmpty());

    if (SelectedAIProvider.IsValid())
    {
        EQCEDefaultAIProvider Provider;
        if (*SelectedAIProvider == EQCEAIProvider::Claude)
        {
            Provider = EQCEDefaultAIProvider::Claude;
        }
        else if (*SelectedAIProvider == EQCEAIProvider::ChatGPT)
        {
            Provider = EQCEDefaultAIProvider::ChatGPT;
        }
        else
        {
            return FReply::Handled();
        }

        FQCE_AIClientFactory::GetClient(Provider).SendMessage(
            CurrentConversationKey,
            TrimmedMessage,
            [this](const FString& Response, bool bSuccess)
            {
                HandleMessageResponse(Response, bSuccess);
            }
        );
    }

    return FReply::Handled();
}

void QCE_AIContainer::HandleMessageResponse(const FString& Response, bool bSuccess) const
{
    AsyncTask(ENamedThreads::GameThread, [this, Response, bSuccess]()
    {
        MessageList->RemoveLoadingMessage();
        
        if (bSuccess)
        {
            MessageList->AddAIResponse(FText::FromString(Response));
            if (!CurrentConversationKey.IsEmpty())
            {
                QCE_AIConversationTracker::Get().AddMessageToConversation(
                    CurrentConversationKey, 
                    EQCEMessageType::Assistant, 
                    Response
                );
            }
        }
        else
        {
            if (Response == TEXT("CLAUDE_API_KEY_MISSING") || Response == TEXT("OPENAI_API_KEY_MISSING"))
            {
                MessageList->AddAPIKeyConfigMessage();
                if (!CurrentConversationKey.IsEmpty())
                {
                    QCE_AIConversationTracker::Get().AddMessageToConversation(
                        CurrentConversationKey, 
                        EQCEMessageType::Assistant, 
                        TEXT("API key is not configured. Please set your API key in the project settings to use AI features.")
                    );
                }
            }
            else
            {
                FString ErrorMessage = FString::Printf(TEXT("Error: %s"), *Response);
                MessageList->AddAIResponse(FText::FromString(ErrorMessage));
                
                if (!CurrentConversationKey.IsEmpty())
                {
                    QCE_AIConversationTracker::Get().AddMessageToConversation(
                        CurrentConversationKey, 
                        EQCEMessageType::Assistant, 
                        ErrorMessage
                    );
                }
            }
        }
    });
}

void QCE_AIContainer::ClearConversation()
{
    CurrentConversationKey.Empty();
    
    if (MessageList.IsValid())
    {
        MessageList->ClearMessages();
    }
    
    bHasMessages = false;
    bIsNodeSelected = false;
    UpdateCloseButtonState();
    UpdateInputControlsState();
}

void QCE_AIContainer::ResetConversation()
{
    if (!CurrentConversationKey.IsEmpty())
    {
        FQCEAIConversation* Conversation = QCE_AIConversationTracker::Get().FindConversation(CurrentConversationKey);
        if (Conversation)
        {
            Conversation->Messages.RemoveAll([](const FQCEConversationMessage& Message)
            {
                return Message.MessageType != EQCEMessageType::FunctionContext;
            });
            
            if (MessageList.IsValid())
            {
                MessageList->LoadConversation(*Conversation);
            }
            
            bHasMessages = false;
            UpdateCloseButtonState();
        }
    }
}

FReply QCE_AIContainer::OnCloseClicked()
{
    ResetConversation();
    return FReply::Handled();
}

FText QCE_AIContainer::GetAIProviderDisplayText(TSharedPtr<EQCEAIProvider> InProvider) const
{
    if (!InProvider.IsValid())
        return FText::FromString(TEXT("Unknown"));
        
    switch (*InProvider)
    {
        case EQCEAIProvider::Claude:
            return LOCTEXT("ClaudeProvider", "Claude");
        case EQCEAIProvider::ChatGPT:
            return LOCTEXT("ChatGPTProvider", "ChatGPT");
        default:
            return FText::FromString(TEXT("Unknown"));
    }
}

FText QCE_AIContainer::GetAIContextDisplayText(TSharedPtr<EQCEAIContext> InContext) const
{
    if (!InContext.IsValid())
        return FText::FromString(TEXT("Unknown"));
        
    switch (*InContext)
    {
        case EQCEAIContext::VisibleCode:
            return LOCTEXT("VisibleCodeContext", "Visible code");
        case EQCEAIContext::UserSelection:
            return LOCTEXT("UserSelectionContext", "User selection");
        default:
            return FText::FromString(TEXT("Unknown"));
    }
}

void QCE_AIContainer::OnAIProviderSelectionChanged(TSharedPtr<EQCEAIProvider> SelectedItem, ESelectInfo::Type SelectInfo)
{
    SelectedAIProvider = SelectedItem;
    if (SelectInfo != ESelectInfo::Direct && SelectedItem.IsValid())
    {
        UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>();
        if (Settings)
        {
            // Convert container enum back to settings enum
            switch (*SelectedItem)
            {
                case EQCEAIProvider::Claude:
                    Settings->DefaultAIProvider = EQCEDefaultAIProvider::Claude;
                    break;
                case EQCEAIProvider::ChatGPT:
                    Settings->DefaultAIProvider = EQCEDefaultAIProvider::ChatGPT;
                    break;
            }
            
            Settings->SaveConfig();
        }
    }
}

void QCE_AIContainer::OnAIContextSelectionChanged(TSharedPtr<EQCEAIContext> SelectedItem, ESelectInfo::Type SelectInfo)
{
    SelectedAIContext = SelectedItem;
    if (SelectInfo != ESelectInfo::Direct && SelectedItem.IsValid())
    {
        UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>();
        if (Settings)
        {
            // Save the selected context directly since we're using the same enum now
            Settings->DefaultAIContext = *SelectedItem;
            
            Settings->SaveConfig();
        }
    }
}

void QCE_AIContainer::SetMainEditorContainer(UMainEditorContainer* InMainEditor)
{
    MainEditorContainer = InMainEditor;
}

FString QCE_AIContainer::GetSelectedTextFromActiveEditor() const
{
    if (!MainEditorContainer)
    {
        return FString();
    }

    TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& ActiveWrapper = MainEditorContainer->GetActiveTextBoxWrapper();
    if (!ActiveWrapper.IsValid())
    {
        return FString();
    }

    TSharedPtr<SQCE_MultiLineEditableTextBox> TextBox = ActiveWrapper->GetTextBox();
    if (!TextBox.IsValid())
    {
        return FString();
    }

    // Get the editable text widget
    TSharedPtr<SMultiLineEditableText> EditableText = TextBox->GetEditableText();
    if (!EditableText.IsValid())
    {
        return FString();
    }

    // Get the current selection
    FTextSelection Selection = EditableText->GetSelection();
   
    FString FullText = EditableText->GetText().ToString();
    FTextLocation StartLoc = Selection.GetBeginning();
    FTextLocation EndLoc = Selection.GetEnd();
    
    // Use the existing utility function to convert text locations to string indices efficiently
    int32 StartIndex = QCE_CommonIOHelpers::ConvertTextLocationToPosition(FullText, StartLoc);
    int32 EndIndex = QCE_CommonIOHelpers::ConvertTextLocationToPosition(FullText, EndLoc);
    
    if (StartIndex != INDEX_NONE && EndIndex != INDEX_NONE && StartIndex < EndIndex && EndIndex <= FullText.Len())
    {
        return FullText.Mid(StartIndex, EndIndex - StartIndex);
    }

    return FString();
}

#undef LOCTEXT_NAMESPACE
