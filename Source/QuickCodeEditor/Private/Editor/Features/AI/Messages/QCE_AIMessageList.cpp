#include "Editor/Features/AI/Messages/QCE_AIMessageList.h"
#include "Editor/Features/AI/Conversations/QCE_AIConversationTracker.h"
#include "Editor/Features/AI/Messages/QCE_AIMessage.h"

#include "Brushes/SlateBoxBrush.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SHyperlink.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "AIMessageList"


void QCE_AIMessageList::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SAssignNew(MessageScrollBox, SScrollBox)
        .AllowOverscroll(EAllowOverscroll::No)
        .ScrollBarAlwaysVisible(true)
    ];
}

void QCE_AIMessageList::AddUserMessage(const FText& Message) const
{
    if (!MessageScrollBox) return;

    MessageScrollBox->AddSlot()
    .Padding(AIMessageStyle::ContainerPadding)
    .HAlign(HAlign_Right)
    [
        SNew(QCE_AIMessage)
        .Message(Message)
        .IsUserMessage(true)
        .IsReadOnly(true)
    ];

    MessageScrollBox->ScrollToEnd();
}

void QCE_AIMessageList::AddAIResponse(const FText& Message) const
{
    if (!MessageScrollBox) return;

    MessageScrollBox->AddSlot()
    .Padding(AIMessageStyle::ContainerPadding)
    .HAlign(HAlign_Left)
    [
        SNew(QCE_AIMessage)
        .Message(Message)
        .IsUserMessage(false)
        .IsReadOnly(true)
    ];

    MessageScrollBox->ScrollToEnd();
}

void QCE_AIMessageList::AddAPIKeyConfigMessage() const
{
    if (!MessageScrollBox) return;

    MessageScrollBox->AddSlot()
    .Padding(AIMessageStyle::ContainerPadding)
    .HAlign(HAlign_Left)
    [
        SNew(SBorder)
        .BorderBackgroundColor(AIMessageStyle::AIMessageColor)
        .Padding(AIMessageStyle::MessagePadding)
        [
            SNew(SBox)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .MinDesiredWidth(200.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("APIKeyMissingMessage", "AI API key is not configured. Please set your API key in the project settings to use AI features."))
                    .AutoWrapText(true)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 8, 0, 0)
                [
                    SNew(SHyperlink)
                    .Text(LOCTEXT("OpenSettingsLink", "Open Settings"))
                    .ToolTipText(LOCTEXT("OpenSettingsTooltip", "Click to open Project Settings > Plugins > Quick Code Editor"))
                    .OnNavigate_Lambda([]()
                    {
                        if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
                        {
                            SettingsModule->ShowViewer("Project", "Plugins", "Quick Code Editor");
                        }
                    })
                ]
            ]
        ]
    ];

    MessageScrollBox->ScrollToEnd();
}

void QCE_AIMessageList::AddLoadingMessage()
{
    if (!MessageScrollBox || LoadingMessageWidget.IsValid()) return;
    
    TSharedRef<SWidget> LoadingWidget = SNew(SBorder)
        .BorderBackgroundColor(AIMessageStyle::AIMessageColor)
        .Padding(AIMessageStyle::MessagePadding)
        [
            SNew(SBox)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .MinDesiredWidth(200.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0, 0, 8, 0)
                [
                    SNew(SThrobber)
                    .Animate(SThrobber::VerticalAndOpacity)
                ]
                + SHorizontalBox::Slot()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ThinkingMessage", ""))
                    .AutoWrapText(true)
                ]
            ]
        ];

    LoadingMessageWidget = LoadingWidget;

    MessageScrollBox->AddSlot()
    .Padding(AIMessageStyle::ContainerPadding)
    .HAlign(HAlign_Left)
    [
        LoadingWidget
    ];

    MessageScrollBox->ScrollToEnd();
}

void QCE_AIMessageList::RemoveLoadingMessage()
{
    if (!MessageScrollBox || !LoadingMessageWidget.IsValid()) return;
    
    MessageScrollBox->RemoveSlot(LoadingMessageWidget.ToSharedRef());
    LoadingMessageWidget.Reset();
}

void QCE_AIMessageList::LoadConversation(const FQCEAIConversation& Conversation)
{
    if (!MessageScrollBox) return;
    
    ClearMessages();
    for (const FQCEConversationMessage& Message : Conversation.Messages)
    {
        if (!Message.bDisplayInUI)
            continue;
        
        if (Message.MessageType == EQCEMessageType::User)
        {
            AddUserMessage(FText::FromString(Message.Content));
        }
        else if (Message.MessageType == EQCEMessageType::Assistant)
        {
            AddAIResponse(FText::FromString(Message.Content));
        }
    }
}

void QCE_AIMessageList::ClearMessages()
{
    if (!MessageScrollBox) return;
    
    MessageScrollBox->ClearChildren();
    LoadingMessageWidget.Reset();
}

#undef LOCTEXT_NAMESPACE
