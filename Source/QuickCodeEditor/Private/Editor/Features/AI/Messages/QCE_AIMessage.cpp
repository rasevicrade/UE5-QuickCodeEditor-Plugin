#include "Editor/Features/AI/Messages/QCE_AIMessage.h"

#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/SyntaxHighlight/CPPSyntaxHighlighterMarshaller.h"
#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextLayout.h"
#include "Editor/Features/AI/Messages/QCE_AIMessageList.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

void QCE_AIMessage::Construct(const FArguments& InArgs)
{
	const bool bIsUserMessage = InArgs._IsUserMessage;
	const bool bIsReadOnly = InArgs._IsReadOnly;
	const FString MessageText = InArgs._Message.Get().ToString();
	TArray<FQCEMessageContentBlock> ContentBlocks = ParseMessageContent(MessageText);
	TSharedRef<SVerticalBox> ContentContainer = SNew(SVerticalBox);

	for (const FQCEMessageContentBlock& Block : ContentBlocks)
	{
		TSharedRef<SWidget> BlockWidget = Block.Type == FQCEMessageContentBlock::EBlockType::CodeBlock
			                                  ? CreateCodeBlockWidget(Block.Content, Block.Language, bIsReadOnly)
			                                  : CreateTextWidget(Block.Content, bIsReadOnly);

		ContentContainer->AddSlot()
		                .AutoHeight()
		                .Padding(0, Block.Type == FQCEMessageContentBlock::EBlockType::CodeBlock ? 4 : 0, 0, 0)
		[
			BlockWidget
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(bIsUserMessage ? AIMessageStyle::UserMessageColor : AIMessageStyle::AIMessageColor)
		.Padding(AIMessageStyle::MessagePadding)
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.MinDesiredWidth(200.0f)
			[
				ContentContainer
			]
		]
	];
}

TArray<FQCEMessageContentBlock> QCE_AIMessage::ParseMessageContent(const FString& MessageText)
{
	TArray<FQCEMessageContentBlock> ContentBlocks;

	int32 CurrentPos = 0;
	const int32 TextLength = MessageText.Len();

	while (CurrentPos < TextLength)
	{
		int32 CodeBlockStart = MessageText.Find(TEXT("```"), ESearchCase::CaseSensitive, ESearchDir::FromStart,
		                                        CurrentPos);

		if (CodeBlockStart == INDEX_NONE)
		{
			FString RemainingText = MessageText.Mid(CurrentPos).TrimStartAndEnd();
			if (!RemainingText.IsEmpty())
			{
				ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::Text, RemainingText));
			}
			break;
		}

		if (CodeBlockStart > CurrentPos)
		{
			FString TextContent = MessageText.Mid(CurrentPos, CodeBlockStart - CurrentPos).TrimStartAndEnd();
			if (!TextContent.IsEmpty())
			{
				ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::Text, TextContent));
			}
		}

		int32 FirstLineEnd = MessageText.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart,
		                                      CodeBlockStart);
		if (FirstLineEnd == INDEX_NONE)
		{
			FString RemainingText = MessageText.Mid(CodeBlockStart).TrimStartAndEnd();
			if (!RemainingText.IsEmpty())
			{
				ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::Text, RemainingText));
			}
			break;
		}

		FString Language = MessageText.Mid(CodeBlockStart + 3, FirstLineEnd - CodeBlockStart - 3).TrimStartAndEnd();
		if (Language.IsEmpty())
		{
			Language = TEXT("cpp"); 
		}

		int32 CodeBlockEnd = MessageText.Find(TEXT("```"), ESearchCase::CaseSensitive, ESearchDir::FromStart,
		                                      FirstLineEnd);
		if (CodeBlockEnd == INDEX_NONE)
		{
			FString CodeContent = MessageText.Mid(FirstLineEnd + 1);
			if (!CodeContent.IsEmpty())
			{
				ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::CodeBlock, CodeContent,
				                                          Language));
			}
			break;
		}

		FString CodeContent = MessageText.Mid(FirstLineEnd + 1, CodeBlockEnd - FirstLineEnd - 1);
		if (!CodeContent.IsEmpty())
		{
			ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::CodeBlock, CodeContent,
			                                          Language));
		}
		CurrentPos = CodeBlockEnd + 3;
	}

	if (ContentBlocks.Num() == 0)
	{
		ContentBlocks.Add(FQCEMessageContentBlock(FQCEMessageContentBlock::EBlockType::Text, MessageText));
	}

	return ContentBlocks;
}

TSharedRef<SWidget> QCE_AIMessage::CreateCodeBlockWidget(const FString& CodeText, const FString& Language,
                                                         bool bIsReadOnly)
{
	bool bUseSyntaxHighlighting = (Language.ToLower().Contains(TEXT("cpp")) ||
		Language.ToLower().Contains(TEXT("c++")) ||
		Language.ToLower().Contains(TEXT("c")) ||
		Language.IsEmpty()); 

	if (bUseSyntaxHighlighting)
	{
		TSharedRef<FCPPSyntaxHighlighterMarshaller> SyntaxHighlighter = FCPPSyntaxHighlighterMarshaller::Create();
		SyntaxHighlighter->SetHighlighterEnabled(true);
		TSharedPtr<FQCE_TextLayout> TextLayout;
		TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> TextBoxWrapper;

		TSharedRef<SWidget> CodeWidget = SAssignNew(TextBoxWrapper, QCE_MultiLineEditableTextBoxWrapper)
			.AllowMultiLine(true)
			.EnableLineNumbers(false)
			.AutoWrapText(false)
			.IsReadOnly(true)
			.Marshaller(SyntaxHighlighter)
			.CreateSlateTextLayout(FCreateSlateTextLayout::CreateLambda(
				[&TextLayout](SWidget* InOwningWidget,
				              const FTextBlockStyle& InDefaultTextStyle) -> TSharedRef<FSlateTextLayout>
				{
					TextLayout = FQCE_TextLayout::Create(InOwningWidget, InDefaultTextStyle);
					return TextLayout.ToSharedRef();
				}
			))
			.Text(FText::FromString(CodeText));

		if (TextBoxWrapper.IsValid() && TextLayout.IsValid())
		{
			TSharedPtr<SQCE_MultiLineEditableTextBox> TextBox = TextBoxWrapper->GetTextBox();
			if (TextBox.IsValid())
			{
				TextBox->FQCE_TextLayout = TextLayout;
				TextBox->SetNodeSelected(true);
			}
		}

		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f))
			.Padding(8.0f)
			[
				CodeWidget
			];
	}
	else
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f))
			.Padding(8.0f)
			[
				SNew(SMultiLineEditableText)
				.Text(FText::FromString(CodeText))
				.AutoWrapText(false)
				.IsReadOnly(true)
				.SelectAllTextWhenFocused(false)
				.ClearTextSelectionOnFocusLoss(false)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
			];
	}
}

TSharedRef<SWidget> QCE_AIMessage::CreateTextWidget(const FString& TextContent, bool bIsReadOnly)
{
	return bIsReadOnly
		       ? StaticCastSharedRef<SWidget>(
			       SNew(STextBlock)
			       .Text(FText::FromString(TextContent))
			       .AutoWrapText(true)
		       )
		       : StaticCastSharedRef<SWidget>(
			       SNew(SMultiLineEditableText)
			       .Text(FText::FromString(TextContent))
			       .AutoWrapText(true)
			       .SelectAllTextWhenFocused(false)
			       .ClearTextSelectionOnFocusLoss(false)
		       );
}
