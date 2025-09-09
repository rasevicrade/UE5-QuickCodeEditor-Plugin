// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/MainEditorContainer.h"
#include "QuickCodeEditor.h"
#include "BlueprintEditor.h"
#include "ILiveCodingModule.h"
#include "K2Node_CallFunction.h"
#include "SGraphPanel.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/FindAndReplace/QCE_FindAndReplaceContainer.h"
#include "Editor/CustomTextBox/GoToLine/QCE_GoToLineContainer.h"
#include "Editor/CustomTextBox/SyntaxHighlight/CPPSyntaxHighlighterMarshaller.h"
#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextLayout.h"
#include "Editor/CustomTextBox/Utility/QCE_ContextMenuBuilder.h"
#include "Editor/CustomTextBox/Utility/CppIO/FunctionCppReader.h"
#include "Editor/CustomTextBox/Utility/CppIO/FunctionCppWriter.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/SToolTip.h"
#include "Editor/Features/AI/QCE_AIContainer.h"
#include "Misc/MessageDialog.h"
#include "Editor/Features/AI/Conversations/QCE_AIConversationTracker.h"
#include "Framework/Text/ITextLayoutMarshaller.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextLayoutFactory.h"
#include "Misc/Paths.h"
#include "Settings/UQCE_EditorSettings.h"

#define LOCTEXT_NAMESPACE "UCodeEditorMenu"

#pragma region Prepare editor

UMainEditorContainer::UMainEditorContainer()
{
}

UMainEditorContainer::~UMainEditorContainer()
{
	CleanupOnTabClosed();
}

void UMainEditorContainer::CleanupOnTabClosed()
{
	CodeEditorTab.Reset();
	ImplementationEditorTextBoxWrapper.Reset();
	DeclarationEditorTextBoxWrapper.Reset();
	
	ImplementationContextMenuBuilder.Reset();
	DeclarationContextMenuBuilder.Reset();
	
	if (FSlateApplication::IsInitialized() && TabChangedHandle.IsValid())
	{
		FGlobalTabmanager::Get()->OnActiveTabChanged_Unsubscribe(TabChangedHandle);
		TabChangedHandle.Reset();
	}
	
	if (TSharedPtr<FBlueprintEditor> BlueprintEditor = OwnerBlueprintEditor.Pin())
	{
		const UEdGraph* FocusedGraph = BlueprintEditor->GetFocusedGraph();
		if (!FocusedGraph) return;

		const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(FocusedGraph);
		if (!GraphEditor.IsValid()) return;

		SGraphPanel* GraphPanel = GraphEditor->GetGraphPanel();
		if (!GraphPanel) return;
	
		if (GraphPanel->SelectionManager.OnSelectionChanged.IsBound())
		{
			GraphPanel->SelectionManager.OnSelectionChanged.Unbind();
		}
	}
}

TSharedRef<SDockTab> UMainEditorContainer::InitQuickCodeEditor()
{
	DeclarationMarshaller = GetCppMarshaller();
	ImplementationMarshaller = GetCppMarshaller();

	
	#pragma region UI Boilerplate
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				[
					SNew(SSplitter)
					.Orientation(Orient_Horizontal)
					+ SSplitter::Slot()
					.Value(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 2)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SBorder)
								.BorderImage_Lambda([this]()
								{
									return CurrentTabIndex == 0 ? 
										FAppStyle::GetBrush("DetailsView.CategoryTop") : 
										FAppStyle::GetBrush("NoBorder");
								})
								.BorderBackgroundColor_Lambda([this]()
								{
									const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
									return CurrentTabIndex == 0 ? 
										Settings->ActiveTabBorderColor : 
										FLinearColor::Transparent;
								})
								.Padding(FMargin(1))
								[
									SAssignNew(DeclarationTabButton, SButton)
									.ButtonStyle(FAppStyle::Get(), "FlatButton")
									.ContentPadding(FMargin(12, 6))
									.ButtonColorAndOpacity_Lambda([this]()
									{
											const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
										return CurrentTabIndex == 0 ? 
											Settings->ActiveTabBackgroundColor : 
											Settings->InactiveTabBackgroundColor;
									})
									.OnClicked_Lambda([this]()
									{
										SwitchToTab(0);
										return FReply::Handled();
									})
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text_Lambda([this]()
											{
												FString FileName = FPaths::GetCleanFilename(DeclarationInfo.HeaderPath);
												return FText::FromString(FileName.IsEmpty() ? TEXT("Declaration") : FileName);
											})
											.ColorAndOpacity_Lambda([this]()
											{
												const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
												return CurrentTabIndex == 0 ? Settings->ActiveTabTextColor : Settings->InactiveTabTextColor;
											})
											.Font_Lambda([this]()
											{
															return CurrentTabIndex == 0 ? 
													FCoreStyle::GetDefaultFontStyle("Bold", 9) : 
													FCoreStyle::GetDefaultFontStyle("Regular", 9);
											})
										]
										+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										.Padding(4, 0, 0, 0)
										[
											SAssignNew(DeclarationModifiedIndicator, SImage)
											.Image(FAppStyle::Get().GetBrush("Icons.FilledCircle"))
											.ColorAndOpacity_Lambda([this]()
											{
												const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
												return Settings->ModifiedFileIndicatorColor;
											})
											.DesiredSizeOverride(FVector2D(8, 8))
											.Visibility(EVisibility::Collapsed)
										]
									]
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SBorder)
								.BorderImage_Lambda([this]()
								{
									return CurrentTabIndex == 1 ? 
										FAppStyle::GetBrush("DetailsView.CategoryTop") : 
										FAppStyle::GetBrush("NoBorder");
								})
								.BorderBackgroundColor_Lambda([this]()
								{
									const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
									return CurrentTabIndex == 1 ? 
										Settings->ActiveTabBorderColor : 
										FLinearColor::Transparent;
								})
								.Padding(FMargin(1))
								[
									SAssignNew(ImplementationTabButton, SButton)
									.ButtonStyle(FAppStyle::Get(), "FlatButton")
									.ContentPadding(FMargin(12, 6))
									.ButtonColorAndOpacity_Lambda([this]()
									{
											const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
										return CurrentTabIndex == 1 ? 
											Settings->ActiveTabBackgroundColor : 
											Settings->InactiveTabBackgroundColor;
									})
									.OnClicked_Lambda([this]()
									{
										SwitchToTab(1);
										return FReply::Handled();
									})
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text_Lambda([this]()
											{
												FString FileName = FPaths::GetCleanFilename(ImplementationInfo.CppPath);
												return FText::FromString(FileName.IsEmpty() ? TEXT("Implementation") : FileName);
											})
											.ColorAndOpacity_Lambda([this]()
											{
												const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
												return CurrentTabIndex == 1 ? Settings->ActiveTabTextColor : Settings->InactiveTabTextColor;
											})
											.Font_Lambda([this]()
											{
															return CurrentTabIndex == 1 ? 
													FCoreStyle::GetDefaultFontStyle("Bold", 9) : 
													FCoreStyle::GetDefaultFontStyle("Regular", 9);
											})
										]
										+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										.Padding(4, 0, 0, 0)
										[
											SAssignNew(ImplementationModifiedIndicator, SImage)
											.Image(FAppStyle::Get().GetBrush("Icons.FilledCircle"))
											.ColorAndOpacity_Lambda([this]()
											{
												const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
												return Settings->ModifiedFileIndicatorColor;
											})
											.DesiredSizeOverride(FVector2D(8, 8))
											.Visibility(EVisibility::Collapsed)
										]
									]
								]
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SNullWidget::NullWidget
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0, 0, 4, 0)
							[
								SNew(SCheckBox)
								.IsChecked(ECheckBoxState::Checked)
								.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
								{
									bLoadIsolated = NewState == ECheckBoxState::Checked;
									RefreshEditorCode(SelectedNode);
								})
								[
									SNew(STextBlock)
									.Text(LOCTEXT("LoadIsolatedText", "Load isolated"))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								]
							]
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SAssignNew(CodeEditorSwitcher, SWidgetSwitcher)
							.WidgetIndex(CurrentTabIndex)
							+ SWidgetSwitcher::Slot()
							[
								SAssignNew(DeclarationEditorTextBoxWrapper, QCE_MultiLineEditableTextBoxWrapper)
								.AllowMultiLine(true)
								.AutoWrapText(false)
								.IsReadOnly(true)
								.MainEditorContainer(this)
								.ContextMenuExtender_Lambda([this](const FMenuBuilder& InMenuBuilder)
								{
									ExtendCodeEditorMenu(const_cast<FMenuBuilder&>(InMenuBuilder), DeclarationContextMenuBuilder);
								})
								.Marshaller(StaticCastSharedPtr<ITextLayoutMarshaller>(DeclarationMarshaller))
								.ModifierKeyForNewLine(EModifierKey::None)
								.OnSearchRequested_Lambda([this]()
								{
									ToggleSearchContainer();
								})
								.OnSaveRequested_Lambda([this]()
								{
									TrySaveDeclarationAndImplementation();
								})
								.OnSaveAndBuildRequested_Lambda([this]()
								{
									OnSaveAndBuildClicked();
								})
								.OnGoToLineRequested_Lambda([this]()
								{
									ToggleGoToLineContainer();
								})
								.CreateSlateTextLayout(FCreateSlateTextLayout::CreateLambda(
									[this](SWidget* InOwningWidget,
										   const FTextBlockStyle& InDefaultTextStyle) -> TSharedRef<FSlateTextLayout>
									{
										DeclarationTextLayout = FQCE_TextLayout::Create(InOwningWidget, InDefaultTextStyle);
										return DeclarationTextLayout.ToSharedRef();
									}
								))
								.OnTextChanged_Lambda([this](const FText& NewText)
								{
									// Check if returning to original state
									const FText OriginalContent = bLoadIsolated ? 
										FText::FromString(DeclarationInfo.FunctionDeclaration) : 
										FText::FromString(DeclarationInfo.InitialFileContent);
									
									const bool bIsNowOriginal = OriginalContent.EqualTo(NewText);
									
									DeclarationEditorTextBoxWrapper->SetIsModified(!bIsNowOriginal);
									if (DeclarationModifiedIndicator.IsValid())
									{
										DeclarationModifiedIndicator->SetVisibility(
											bIsNowOriginal ? EVisibility::Collapsed : EVisibility::Visible
										);
									}

									UpdateSaveButtonsState();
								})
								.OnGetFocus_Lambda([this]()
								{
									CheckIfCodeWasChangedOutsideOfEditor();
								})
							]
							+ SWidgetSwitcher::Slot()
							[
								SAssignNew(ImplementationEditorTextBoxWrapper, QCE_MultiLineEditableTextBoxWrapper)
								.AllowMultiLine(true)
								.AutoWrapText(false)
								.IsReadOnly(true)
								.MainEditorContainer(this)
								.ContextMenuExtender_Lambda([this](const FMenuBuilder& InMenuBuilder)
								{
									ExtendCodeEditorMenu(const_cast<FMenuBuilder&>(InMenuBuilder), ImplementationContextMenuBuilder);
								})
								.CreateSlateTextLayout(FCreateSlateTextLayout::CreateLambda(
									[this](SWidget* InOwningWidget,
										   const FTextBlockStyle& InDefaultTextStyle) -> TSharedRef<FSlateTextLayout>
									{
										ImplementationTextLayout = FQCE_TextLayout::Create(InOwningWidget, InDefaultTextStyle);
										return ImplementationTextLayout.ToSharedRef();
									}
								))
								.Marshaller(StaticCastSharedPtr<ITextLayoutMarshaller>(ImplementationMarshaller))
								.ModifierKeyForNewLine(EModifierKey::None)
								.OnSearchRequested_Lambda([this]()
								{
									ToggleSearchContainer();
								})
								.OnSaveRequested_Lambda([this]()
								{
									TrySaveDeclarationAndImplementation();
								})
								.OnSaveAndBuildRequested_Lambda([this]()
								{
									OnSaveAndBuildClicked();
								})
								.OnGoToLineRequested_Lambda([this]()
								{
									ToggleGoToLineContainer();
								})
								.OnTextChanged_Lambda([this](const FText& NewText)
								{
									// Check if returning to original state
									const FText OriginalContent = bLoadIsolated ? 
										FText::FromString(ImplementationInfo.FunctionImplementation) : 
										FText::FromString(ImplementationInfo.InitialFileContent);
									
									const bool bIsNowOriginal = OriginalContent.EqualTo(NewText);
									
									ImplementationEditorTextBoxWrapper->SetIsModified(!bIsNowOriginal);
									if (ImplementationModifiedIndicator.IsValid())
									{
										ImplementationModifiedIndicator->SetVisibility(
											bIsNowOriginal ? EVisibility::Collapsed : EVisibility::Visible
										);
									}

									if (CurrentEditedFunction && AIContainer.IsValid())
									{
										const FString FunctionName = CurrentEditedFunction->GetName();
										const FString ClassName = CurrentEditedFunction->GetOwnerClass()
											                          ? CurrentEditedFunction->GetOwnerClass()->
											                          GetName()
											                          : TEXT("");
										const FString FilePath = ImplementationInfo.CppPath;
										const FString NewFunctionCode = NewText.ToString();

										FQCEAIConversation& Conversation = QCE_AIConversationTracker::Get().
											GetOrCreateConversation(FunctionName, ClassName, FilePath);
										QCE_AIConversationTracker::Get().SetFunctionContextToConversation(
											Conversation.ConversationKey, NewFunctionCode);
									}

									UpdateSaveButtonsState();
								})
								.OnGetFocus_Lambda([this]()
								{
									CheckIfCodeWasChangedOutsideOfEditor();
								})
							]
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 0)
				[
					SAssignNew(CollapseButton, SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(2, 0))
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.OnClicked_Lambda([this]()
					{
						bIsAIContainerCollapsed = !bIsAIContainerCollapsed;
						return FReply::Handled();
					})
					[
						SNew(SBox)
						.MinDesiredHeight(100)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return FText::FromString(bIsAIContainerCollapsed ? "<" : ">");
							})
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]()
					{
						return bIsAIContainerCollapsed ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						SAssignNew(AIContainer, QCE_AIContainer)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				SAssignNew(SearchContainer, QCE_FindAndReplaceContainer)
				.Visibility_Lambda([this]() { return bIsSearchVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				SAssignNew(GoToLineContainer, QCE_GoToLineContainer)
				.Visibility_Lambda([this]() { return bIsGoToLineVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNullWidget::NullWidget
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(4, 0, 4, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.ContentPadding(FMargin(8, 4))
					.ToolTipText(LOCTEXT("SaveButtonTooltip", "Save (Ctrl + S)"))
					.IsEnabled_Lambda([this]() { return HasUnsavedChanges(); })
					.OnClicked_Lambda([this]() -> FReply
					{
						if (HasUnsavedChanges())
						{
							TrySaveDeclarationAndImplementation();
						}
						return FReply::Handled();
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0, 0, 4, 0)
						[
							SAssignNew(SaveIcon, SImage)
							.Image(FAppStyle::GetBrush("Icons.Save"))
							.DesiredSizeOverride(FVector2D(16, 16))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SaveButtonText", "Save"))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(4, 0, 4, 0)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "Button")
					.ContentPadding(FMargin(8, 4))
					.ToolTipText(LOCTEXT("SaveAndBuildButtonTooltip", "Save and Build (Live Coding) (Ctrl + Shift + B)"))
					.IsEnabled_Lambda([this]() { return HasUnsavedChanges(); })
					.OnClicked_Lambda([this]() -> FReply
					{
						if (HasUnsavedChanges())
						{
							OnSaveAndBuildClicked();
						}
						return FReply::Handled();
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0, 0, 4, 0)
						[
							SAssignNew(SaveAndBuildIcon, SImage)
							.Image(FAppStyle::GetBrush("LevelEditor.Recompile"))
							.DesiredSizeOverride(FVector2D(16, 16))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SaveAndBuildButtonText", "Save and Build"))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNullWidget::NullWidget
				]
			]
		];
	#pragma endregion

	ImplementationEditorTextBoxWrapper->GetTextBox()->FQCE_TextLayout = ImplementationTextLayout;
	DeclarationEditorTextBoxWrapper->GetTextBox()->FQCE_TextLayout = DeclarationTextLayout;
	CodeEditorTab = NewTab;
	DeclarationMarshaller->SetHighlighterEnabled(DeclarationEditorTextBoxWrapper->GetTextBox().IsValid() && DeclarationEditorTextBoxWrapper->GetTextBox()->IsNodeSelected());
	ImplementationMarshaller->SetHighlighterEnabled(ImplementationEditorTextBoxWrapper->GetTextBox().IsValid() && ImplementationEditorTextBoxWrapper->GetTextBox()->IsNodeSelected());
	CheckSelectedNode();
	SetNodeSelectionListener();

	ImplementationContextMenuBuilder = MakeShared<QCE_ContextMenuBuilder>(SearchContainer, GetImplementationTextBoxWrapper()->GetTextBox(), this);
	DeclarationContextMenuBuilder = MakeShared<QCE_ContextMenuBuilder>(SearchContainer, GetDeclarationTextBoxWrapper()->GetTextBox(), this);
	
	// Set the reference to this MainEditorContainer in the AIContainer
	if (AIContainer.IsValid())
	{
		AIContainer->SetMainEditorContainer(this);
	}
	
	bIsNodeSelected = false;
	UpdateSaveButtonsState();
	return NewTab;
}

void UMainEditorContainer::SwitchToTab(int32 TabIndex)
{
	if (CodeEditorSwitcher.IsValid())
	{
		if (CurrentTabIndex != TabIndex )
		{
			CurrentTabIndex = TabIndex;
			CodeEditorSwitcher->SetActiveWidgetIndex(TabIndex);
			
			// Update AI conversation context with the newly active tab's content
			FString CurrentTabContent;
			if (TabIndex == 0 && DeclarationEditorTextBoxWrapper.IsValid())
			{
				CurrentTabContent = DeclarationEditorTextBoxWrapper->GetText().ToString();
			}
			else if (TabIndex == 1 && ImplementationEditorTextBoxWrapper.IsValid())
			{
				CurrentTabContent = ImplementationEditorTextBoxWrapper->GetText().ToString();
			}
			
			if (!CurrentTabContent.IsEmpty())
			{
				LoadAIConversationForFunction(CurrentTabContent);
			}
		}
		CheckScrollTarget(TabIndex);
	}
}

void UMainEditorContainer::CheckScrollTarget(const int32 TabIndex) const
{
	if (TabIndex == 0)
	{
		if (DeclarationEditorTextBoxWrapper->GetScrollTarget() > 0)
		{
			DeclarationEditorTextBoxWrapper->ScrollToLine(DeclarationEditorTextBoxWrapper->GetScrollTarget());
			DeclarationEditorTextBoxWrapper->SetScrollTarget(-1);
		}
		
	}
	else if (TabIndex == 1)
	{
		if (ImplementationEditorTextBoxWrapper->GetScrollTarget() > 0)
		{
			ImplementationEditorTextBoxWrapper->ScrollToLine(ImplementationEditorTextBoxWrapper->GetScrollTarget());
			ImplementationEditorTextBoxWrapper->SetScrollTarget(-1);
		}
	}
}

void UMainEditorContainer::SetNodeSelectionListener()
{
	if (!OwnerBlueprintEditor.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("SetNodeSelectionListener called but no owner Blueprint editor is set"));
		return;
	}
	
	BindToCurrentBlueprintGraph();
	BindToGraphTabChanges();
}

void UMainEditorContainer::CheckSelectedNode()
{
	TSharedPtr<FBlueprintEditor> BlueprintEditor = OwnerBlueprintEditor.Pin();
	if (!BlueprintEditor.IsValid())
	{
		return;
	}

	const UEdGraph* FocusedGraph = BlueprintEditor->GetFocusedGraph();
	if (!FocusedGraph)
	{
		return;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(FocusedGraph);
	if (!GraphEditor.IsValid())
	{
		return;
	}

	SGraphPanel* GraphPanel = GraphEditor->GetGraphPanel();
	if (!GraphPanel)
	{
		return;
	}

	const FGraphPanelSelectionSet& SelectedNodes = GraphPanel->SelectionManager.GetSelectedNodes();
	if (SelectedNodes.Num() == 1)
	{
		const UObject* NewSelectedNode = *SelectedNodes.CreateConstIterator();
		RefreshEditorCode(NewSelectedNode);
	}
	else
	{
		bIsNodeSelected = false;
		UpdateSaveButtonsState();
	}
}

void UMainEditorContainer::BindToCurrentBlueprintGraph()
{
	TSharedPtr<FBlueprintEditor> BlueprintEditor = OwnerBlueprintEditor.Pin();
	if (!BlueprintEditor.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("BindToCurrentBlueprintGraph called but no owner Blueprint editor is set"));
		return;
	}
	
	const UEdGraph* FocusedGraph = BlueprintEditor->GetFocusedGraph();
	if (!FocusedGraph) return;

	const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(FocusedGraph);
	if (!GraphEditor.IsValid()) return;

	SGraphPanel* GraphPanel = GraphEditor->GetGraphPanel();
	if (!GraphPanel) return;
	
	if (GraphPanel->SelectionManager.OnSelectionChanged.IsBound())
	{
		GraphPanel->SelectionManager.OnSelectionChanged.Unbind();
	}
	
	GraphPanel->SelectionManager.OnSelectionChanged.BindUObject(this, &UMainEditorContainer::OnBlueprintNodesSelected);
}

void UMainEditorContainer::BindToGraphTabChanges()
{
	TabChangedHandle = FGlobalTabmanager::Get()->OnActiveTabChanged_Subscribe(
		FOnActiveTabChanged::FDelegate::CreateLambda([this](TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated)
		{
			if (!NewlyActivated.IsValid()) return;
			
			BindToCurrentBlueprintGraph();
			
		})
	);
}

void UMainEditorContainer::UpdateSaveButtonsState() const
{
	const bool bHasChanges = HasUnsavedChanges();
	const float Alpha = bHasChanges ? 1.0f : 0.5f;
	
	if (SaveIcon.IsValid())
	{
		SaveIcon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
	}
	
	if (SaveAndBuildIcon.IsValid())
	{
		SaveAndBuildIcon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
	}
}

bool UMainEditorContainer::HasUnsavedChanges() const
{
	if (!ImplementationEditorTextBoxWrapper.IsValid() || !DeclarationEditorTextBoxWrapper.IsValid())
	{
		return false;
	}
	
	return ImplementationEditorTextBoxWrapper->IsModified() || DeclarationEditorTextBoxWrapper->IsModified();
}

#pragma endregion

void UMainEditorContainer::OnBlueprintNodesSelected(const FGraphPanelSelectionSet& SelectedNodes)
{
	if (SelectedNodes.Num() != 1)
	{
		bIsNodeSelected = false;
		UpdateSaveButtonsState();
		return;
	}
	
	if (!ImplementationEditorTextBoxWrapper.IsValid() || !DeclarationEditorTextBoxWrapper.IsValid()) return;

	
	const auto NewSelectedNode = *SelectedNodes.CreateConstIterator();
	RefreshEditorCode(NewSelectedNode);
}

void UMainEditorContainer::RefreshEditorCode(const UObject* InNewSelectedNode)
{
	const bool bHasUnsavedChanges = (ImplementationEditorTextBoxWrapper.IsValid() && ImplementationEditorTextBoxWrapper->IsModified())
								|| (DeclarationEditorTextBoxWrapper.IsValid() && DeclarationEditorTextBoxWrapper->IsModified());
	if (bHasUnsavedChanges)
	{
		const FText DialogTitle = LOCTEXT("QuickCodeEditorTitle", "Code Editor");
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			LOCTEXT("UnsavedChangesPrompt", "You have unsaved changes in the Code Editor for this function. Would you like to save them before proceeding?"),
			&DialogTitle
		);

		switch (Response)
		{
		case EAppReturnType::Yes:
			TrySaveDeclarationAndImplementation();
			break;
		case EAppReturnType::Cancel:
			return;
		default:
			break;
		}
	}

	if (InNewSelectedNode)
		SelectedNode = InNewSelectedNode;
	
	bIsNodeChangeImplementationUpdate = InNewSelectedNode != nullptr;
	bIsNodeChangeDeclarationUpdate = InNewSelectedNode != nullptr;
	FString ImplementationCode;
	FString FunctionDeclaration;
	const bool bHasImplementation = GetImplementationCodeForNode(InNewSelectedNode, ImplementationCode);
	const bool bHasDeclaration = GetDeclarationCodeForNode(InNewSelectedNode, FunctionDeclaration);

	if (bHasImplementation || (!ImplementationCode.IsEmpty() && !bLoadIsolated))
	{
		if (ImplementationEditorTextBoxWrapper.IsValid())
		{
			ImplementationMarshaller->SetHighlighterEnabled(true);
			ImplementationEditorTextBoxWrapper->SetNodeSelected(true);
			const FText InitialText = bLoadIsolated ? FText::FromString(ImplementationCode) : FText::FromString(ImplementationInfo.InitialFileContent);
			ImplementationEditorTextBoxWrapper->SetText(InitialText);
			ImplementationEditorTextBoxWrapper->SetIsModified(false);
			ImplementationEditorTextBoxWrapper->SetIsReadOnly(ShouldFileBeReadOnly(ImplementationInfo.CppPath) || ImplementationCode.IsEmpty());
			ImplementationEditorTextBoxWrapper->GetTextBox()->SetTextBoxType(ETextBoxType::Implementation);
			ImplementationModifiedIndicator->SetVisibility(EVisibility::Hidden);

			// Track how this content was loaded for proper saving
			bImplementationLoadedIsolated = bLoadIsolated;

			if (bIsNodeChangeImplementationUpdate && !bLoadIsolated && ImplementationInfo.ImplementationStartPosition > 0)
			{
				ImplementationEditorTextBoxWrapper->ScrollToPosition(ImplementationInfo.ImplementationStartPosition);
			}
			
		}
	}
	else if (ImplementationEditorTextBoxWrapper.IsValid())
	{
		ImplementationMarshaller->SetHighlighterEnabled(false);
		ImplementationEditorTextBoxWrapper->SetText(FText::FromString(""));
		ImplementationEditorTextBoxWrapper->SetToolTip(SNew(SToolTip).Text(FText::FromString("No function selected. Select a node in Blueprint editor to begin editing.")));
		ImplementationEditorTextBoxWrapper->SetIsReadOnly(true);
		ImplementationEditorTextBoxWrapper->SetNodeSelected(false);
		ImplementationModifiedIndicator->SetVisibility(EVisibility::Hidden);

		if (bHasDeclaration)
		{
			SwitchToTab(0); // 0 is the declaration tab index
		}
	}

	if (bHasDeclaration || !bLoadIsolated)
	{
		if (DeclarationEditorTextBoxWrapper.IsValid())
		{
			
			DeclarationMarshaller->SetHighlighterEnabled(true);
			DeclarationEditorTextBoxWrapper->SetNodeSelected(true);
			const FText InitialText = bLoadIsolated ? FText::FromString(FunctionDeclaration) : FText::FromString(DeclarationInfo.InitialFileContent);
			DeclarationEditorTextBoxWrapper->SetText(InitialText);
			DeclarationEditorTextBoxWrapper->SetIsModified(false);
			DeclarationEditorTextBoxWrapper->SetIsReadOnly(ShouldFileBeReadOnly(DeclarationInfo.HeaderPath) || FunctionDeclaration.IsEmpty());
			DeclarationEditorTextBoxWrapper->GetTextBox()->SetTextBoxType(ETextBoxType::Declaration);
			DeclarationModifiedIndicator->SetVisibility(EVisibility::Hidden);

			// Track how this content was loaded for proper saving
			bDeclarationLoadedIsolated = bLoadIsolated;

			if (bIsNodeChangeDeclarationUpdate && !bLoadIsolated && DeclarationInfo.DeclarationStartPosition > 0)
			{
				DeclarationEditorTextBoxWrapper->ScrollToPosition(DeclarationInfo.DeclarationStartPosition);
			}
		}
	}
	else if (DeclarationEditorTextBoxWrapper.IsValid())
	{
		DeclarationMarshaller->SetHighlighterEnabled(false);
		DeclarationEditorTextBoxWrapper->SetText(FText::FromString(""));
		DeclarationEditorTextBoxWrapper->SetToolTip(SNew(SToolTip).Text(FText::FromString("No function selected. Select a node in Blueprint editor to begin editing.")));
		DeclarationEditorTextBoxWrapper->SetIsReadOnly(true);
		DeclarationEditorTextBoxWrapper->SetNodeSelected(false);
		DeclarationModifiedIndicator->SetVisibility(EVisibility::Hidden);
	}

	bIsNodeSelected = bHasImplementation && bHasDeclaration;
	UpdateSaveButtonsState();

	FString CurrentTabContent;
	if (CurrentTabIndex == 0 && DeclarationEditorTextBoxWrapper.IsValid())
	{
		CurrentTabContent = DeclarationEditorTextBoxWrapper->GetText().ToString();
	}
	else if (CurrentTabIndex == 1 && ImplementationEditorTextBoxWrapper.IsValid())
	{
		CurrentTabContent = ImplementationEditorTextBoxWrapper->GetText().ToString();
	}
	
	LoadAIConversationForFunction(CurrentTabContent);
}

void UMainEditorContainer::LoadAIConversationForFunction(const FString& TextBoxContent) const
{
	if (CurrentEditedFunction && AIContainer.IsValid())
	{
		const FString FunctionName = CurrentEditedFunction->GetName();
		const FString ClassName = CurrentEditedFunction->GetOwnerClass() ? CurrentEditedFunction->GetOwnerClass()->GetName() : TEXT("");
		const FString FilePath = ImplementationInfo.CppPath;
		
		AIContainer->LoadConversationForFunction(FunctionName, ClassName, FilePath, TextBoxContent);
	}
	else if (AIContainer.IsValid())
	{
		AIContainer->ClearConversation();
	}
}

bool UMainEditorContainer::GetImplementationCodeForNode(const UObject* InSelectedNode, FString& OutFunctionCode)
{
	const UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(InSelectedNode);
	if (!FuncNode) return false;

	CurrentEditedFunction = FuncNode->GetTargetFunction();
	if (!CurrentEditedFunction) return false;
	
	FunctionReader.GetFunctionImplementation(CurrentEditedFunction, ImplementationInfo);
		

	OutFunctionCode = ImplementationInfo.FunctionImplementation;
	
	if (ImplementationEditorTextBoxWrapper.IsValid())
	{
		ImplementationEditorTextBoxWrapper->SetFilePath(ImplementationInfo.CppPath);
	}
	
	return true;
}


bool UMainEditorContainer::GetDeclarationCodeForNode(const UObject* InSelectedNode, FString& OutFunctionDeclaration)
{
	const UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(InSelectedNode);
	if (!FuncNode) return false;

	CurrentEditedFunction = FuncNode->GetTargetFunction();
	if (!CurrentEditedFunction) return false;
	
	if (!FunctionReader.GetFunctionDeclaration(CurrentEditedFunction, DeclarationInfo))
	{
		OutFunctionDeclaration = "";
		return false;
	}
		

	OutFunctionDeclaration = DeclarationInfo.FunctionDeclaration;
	
	if (DeclarationEditorTextBoxWrapper.IsValid())
	{
		DeclarationEditorTextBoxWrapper->SetFilePath(DeclarationInfo.HeaderPath);
	}
	
	return true;
}

FReply UMainEditorContainer::TrySaveDeclarationAndImplementation(const bool bForceOverwrite)
{
	if (!DeclarationEditorTextBoxWrapper.IsValid()  || !ImplementationEditorTextBoxWrapper.IsValid() || !CurrentEditedFunction) return FReply::Handled();
	
	const FString UpdatedImpementationCode = ImplementationEditorTextBoxWrapper->GetText().ToString();
	const FString UpdatedDeclarationCode = DeclarationEditorTextBoxWrapper->GetText().ToString();

	ImplementationEditorTextBoxWrapper->SetIsModified(false);
	DeclarationEditorTextBoxWrapper->SetIsModified(false);
	WriteUpdatedFunctionCode(UpdatedDeclarationCode, UpdatedImpementationCode, bForceOverwrite);
	

	if (ImplementationModifiedIndicator.IsValid())
	{
		ImplementationModifiedIndicator->SetVisibility(EVisibility::Collapsed);
	}
	if (DeclarationModifiedIndicator.IsValid())
	{
		DeclarationModifiedIndicator->SetVisibility(EVisibility::Collapsed);
	}

	UpdateSaveButtonsState();

	return FReply::Handled();
}

void UMainEditorContainer::WriteUpdatedFunctionCode(const FString& UpdatedFunctionHeaderCode, const FString& UpdatedFunctionImplementationCode, const bool bForceOverwrite) 
{
	FFunctionCppWriter Writer;
	if (!UpdatedFunctionHeaderCode.IsEmpty())
		Writer.WriteFunctionDeclaration(DeclarationInfo, UpdatedFunctionHeaderCode, bDeclarationLoadedIsolated, bForceOverwrite);

	if (!UpdatedFunctionImplementationCode.IsEmpty())
		Writer.WriteFunctionImplementation(ImplementationInfo, UpdatedFunctionImplementationCode, bImplementationLoadedIsolated, bForceOverwrite);
	RefreshEditorCode(SelectedNode);
}

bool UMainEditorContainer::ShouldFileBeReadOnly(const FString& FilePath) const
{
	if (IFileManager::Get().IsReadOnly(*FilePath))
	{
		return true;
	}

	const FString EngineDir = FPaths::EngineDir();
	const FString NormalizedFilePath = FPaths::ConvertRelativePathToFull(FilePath);
	const FString NormalizedEngineDir = FPaths::ConvertRelativePathToFull(EngineDir);
	
	if (NormalizedFilePath.StartsWith(NormalizedEngineDir, ESearchCase::IgnoreCase))
	{
		return true;
	}

	return false;
}

TSharedPtr<QCE_MultiLineEditableTextBoxWrapper>& UMainEditorContainer::GetActiveTextBoxWrapper()
{
	if (CurrentTabIndex == 0)
	{
		return DeclarationEditorTextBoxWrapper;
	}
	else
	{
		return ImplementationEditorTextBoxWrapper;
	}
}

void UMainEditorContainer::MarkImplementationAsModified()
{
	if (ImplementationEditorTextBoxWrapper.IsValid())
	{
		ImplementationEditorTextBoxWrapper->SetIsModified(true);
	}
	
	if (ImplementationModifiedIndicator.IsValid())
	{
		ImplementationModifiedIndicator->SetVisibility(EVisibility::Visible);
	}
}

void UMainEditorContainer::ExtendCodeEditorMenu(FMenuBuilder& MenuBuilder, TSharedPtr<QCE_ContextMenuBuilder> ContextMenuBuilder)
{
	ContextMenuBuilder->AddEditorMenuEntries(MenuBuilder);
}

void UMainEditorContainer::ToggleSearchContainer()
{
    bIsSearchVisible = !bIsSearchVisible;
    if (bIsSearchVisible && SearchContainer.IsValid())
    {
        SearchContainer->SetEditorContainer(this);
        SearchContainer->SetVisibility(EVisibility::Visible);
        if (ImplementationEditorTextBoxWrapper.IsValid())
        {
            TSharedPtr<SQCE_MultiLineEditableTextBox> TextBox = ImplementationEditorTextBoxWrapper->GetTextBox();
            if (TextBox.IsValid())
            {
                const FString CurrentWord = TextBox->GetWordAtCursor();
                if (!CurrentWord.IsEmpty())
                {
                    SearchContainer->SetFindText(CurrentWord);
                }
            }
        }
        SearchContainer->FocusFindTextBox();
    } else
    {
        SearchContainer->SetVisibility(EVisibility::Collapsed);
    }
}

void UMainEditorContainer::ToggleGoToLineContainer()
{
	bIsGoToLineVisible = !bIsGoToLineVisible;
	if (bIsGoToLineVisible && GoToLineContainer.IsValid())
	{
		GoToLineContainer->SetEditorContainer(this);
		GoToLineContainer->SetVisibility(EVisibility::Visible);
		GoToLineContainer->FocusLineNumberInput();
	} 
	else
	{
		GoToLineContainer->SetVisibility(EVisibility::Collapsed);
	}
}

FReply UMainEditorContainer::OnSaveAndBuildClicked()
{
	TrySaveDeclarationAndImplementation();
	
#if WITH_LIVE_CODING
	ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
	if (LiveCoding != nullptr && LiveCoding->IsEnabledByDefault())
	{
		LiveCoding->EnableForSession(true);
		if (LiveCoding->IsEnabledForSession())
		{
			LiveCoding->Compile();
		}
		else
		{
			FText EnableErrorText = LiveCoding->GetEnableErrorText();
			if (EnableErrorText.IsEmpty())
			{
				EnableErrorText = LOCTEXT("NoLiveCodingCompileAfterHotReload", "Live Coding cannot be enabled while hot-reloaded modules are active. Please close the editor and build from your IDE before restarting.");
			}
			FMessageDialog::Open(EAppMsgType::Ok, EnableErrorText);
		}
	}
#endif

	return FReply::Handled();
}


TSharedPtr<FCPPSyntaxHighlighterMarshaller> UMainEditorContainer::GetCppMarshaller()
{
	return FCPPSyntaxHighlighterMarshaller::Create();
}

void UMainEditorContainer::CheckIfCodeWasChangedOutsideOfEditor()
{
	if (FunctionReader.HasFunctionDeclarationChangedOnDisk(CurrentEditedFunction, DeclarationInfo))
	{
		const FText DialogTitle = LOCTEXT("QuickCodeEditorTitle", "Quick Code Editor");
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			LOCTEXT("DeclarationChangedOnDisk", "The function declaration has been modified externally. Reload it? \n Yes: Load external changes (lose current changes)\nNo: Keep current changes (overwrite external changes)"),
			&DialogTitle
		);
								
		if (Response == EAppReturnType::Yes)
		{
			RefreshEditorCode(SelectedNode);
		}
		else if (Response == EAppReturnType::No)
		{
			TrySaveDeclarationAndImplementation(true);
		}
	}

	if (FunctionReader.HasFunctionImplementationChangedOnDisk(CurrentEditedFunction, ImplementationInfo))
	{
		const FText DialogTitle = LOCTEXT("QuickCodeEditorTitle", "Quick Code Editor");
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			LOCTEXT("ImplementationChangedOnDisk", "The function declaration has been modified externally. Reload it? \nYes: Load external changes (lose current changes)\nNo: Keep current changes (overwrite external changes)"),
			&DialogTitle
		);

		if (Response == EAppReturnType::Yes)
		{
			if (SelectedNode)
				RefreshEditorCode(SelectedNode);
		}
		else if (Response == EAppReturnType::No)
		{
			TrySaveDeclarationAndImplementation(true);
		}
	}
}


#undef LOCTEXT_NAMESPACE
