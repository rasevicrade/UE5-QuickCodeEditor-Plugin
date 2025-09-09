// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/InlineAISuggestion/UI/QCE_InlineAISuggestionBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SToolTip.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IMenu.h"
#include "Styling/CoreStyle.h"
#include "Input/Events.h"
#include "Settings/UQCE_EditorSettings.h"

void SQCE_InlineAISuggestionBox::Construct(const FArguments& InArgs)
{
	MinDesiredWidth = InArgs._MinDesiredWidth;
	MinDesiredHeight = InArgs._MinDesiredHeight;
	TextBoxType = InArgs._TextBoxType;
	OnCompletionTypeChanged = InArgs._OnCompletionTypeChanged;
	OnContextTypeChanged = InArgs._OnContextTypeChanged;
	OnConfirmed = InArgs._OnConfirmed;
	OnCancelled = InArgs._OnCancelled;
	SetSuggestionBoxState(ReadyForInput);

	// Load settings from configuration
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	// Initialize values from settings or arguments
	CurrentCompletionType = InArgs._CompletionType != EQCEDefaultCompletionType::CurrentLine ? 
		InArgs._CompletionType : Settings->DefaultCompletionType;
		
	if (TextBoxType == ETextBoxType::Declaration)
	{
		CurrentContextType = InArgs._ContextType != EQCEDefaultContext::CurrentFunction ? 
			InArgs._ContextType : Settings->DefaultDeclarationContextType;
	}
	else
	{
		CurrentContextType = InArgs._ContextType != EQCEDefaultContext::CurrentFunction ? 
			InArgs._ContextType : Settings->DefaultImplementationContextType;
	}
	
	CurrentContextText = InArgs._ContextText;
	CurrentNumberOfLines = Settings->DefaultNumberOfLines;

	// Initialize completion type options
	CompletionTypeOptions.Add(MakeShared<EQCEDefaultCompletionType>(EQCEDefaultCompletionType::CurrentLine));
	CompletionTypeOptions.Add(MakeShared<EQCEDefaultCompletionType>(EQCEDefaultCompletionType::Block));

	// Initialize implementation context type options
	ImplementationContextTypeOptions.Add(MakeShared<EQCEDefaultContext>(EQCEDefaultContext::CurrentFunction));
	ImplementationContextTypeOptions.Add(MakeShared<EQCEDefaultContext>(EQCEDefaultContext::CurrentLineOrFunction));
	ImplementationContextTypeOptions.Add(MakeShared<EQCEDefaultContext>(EQCEDefaultContext::NLinesAboveCursor));

	// Initialize declaration context type options (only CurrentLine and NLinesAboveCursor)
	DeclarationContextTypeOptions.Add(MakeShared<EQCEDefaultContext>(EQCEDefaultContext::CurrentLineOrFunction)); // Using as "Current Line"
	DeclarationContextTypeOptions.Add(MakeShared<EQCEDefaultContext>(EQCEDefaultContext::NLinesAboveCursor));

	// I want to...

	// Dropdown for implementation context
	SAssignNew(ImplementationContextTypeComboBox, SComboBox<TSharedPtr<EQCEDefaultContext>>)
		.OptionsSource(&ImplementationContextTypeOptions)
		.OnGenerateWidget_Lambda([this](TSharedPtr<EQCEDefaultContext> Option) {
			return SNew(STextBlock).Text(GetContextTypeText(Option));
		})
		.OnSelectionChanged(this, &SQCE_InlineAISuggestionBox::OnContextTypeSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([this]() {
				auto SelectedItem = ImplementationContextTypeComboBox->GetSelectedItem();
				return SelectedItem.IsValid() ? GetContextTypeText(SelectedItem) : FText::FromString(TEXT("Select..."));
			})
		];

	// Dropdown for declaration context
	SAssignNew(DeclarationContextTypeComboBox, SComboBox<TSharedPtr<EQCEDefaultContext>>)
		.OptionsSource(&DeclarationContextTypeOptions)
		.OnGenerateWidget_Lambda([this](TSharedPtr<EQCEDefaultContext> Option) {
			return SNew(STextBlock).Text(GetContextTypeText(Option));
		})
		.OnSelectionChanged(this, &SQCE_InlineAISuggestionBox::OnContextTypeSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([this]() {
				auto SelectedItem = DeclarationContextTypeComboBox->GetSelectedItem();
				return SelectedItem.IsValid() ? GetContextTypeText(SelectedItem) : FText::FromString(TEXT("Select..."));
			})
		];
	
	// to

	// Dropdown for completion type
	SAssignNew(CompletionTypeComboBox, SComboBox<TSharedPtr<EQCEDefaultCompletionType>>)
		.OptionsSource(&CompletionTypeOptions)
		.OnGenerateWidget_Lambda([this](TSharedPtr<EQCEDefaultCompletionType> Option) {
			return SNew(STextBlock).Text(GetCompletionTypeText(Option));
		})
		.OnSelectionChanged(this, &SQCE_InlineAISuggestionBox::OnCompletionTypeSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([this]() {
				auto SelectedItem = CompletionTypeComboBox->GetSelectedItem();
				return SelectedItem.IsValid() ? GetCompletionTypeText(SelectedItem) : FText::FromString(TEXT("Select..."));
			})
		];

	

	// Set initial selections
	for (const auto& Option : CompletionTypeOptions)
	{
		if (*Option == CurrentCompletionType)
		{
			CompletionTypeComboBox->SetSelectedItem(Option);
			break;
		}
	}

	// Set initial selection for the appropriate combo box based on TextBoxType
	if (TextBoxType == ETextBoxType::Declaration)
	{
		for (const auto& Option : DeclarationContextTypeOptions)
		{
			if (*Option == CurrentContextType)
			{
				DeclarationContextTypeComboBox->SetSelectedItem(Option);
				break;
			}
		}
	}
	else
	{
		for (const auto& Option : ImplementationContextTypeOptions)
		{
			if (*Option == CurrentContextType)
			{
				ImplementationContextTypeComboBox->SetSelectedItem(Option);
				break;
			}
		}
	}

	FString Tooltip;
	if (TextBoxType == ETextBoxType::Declaration)
	{
		Tooltip = TEXT("Example: Generate a blueprint callable function which returns a string and takes a static mesh as parameter.");
	} else if (TextBoxType == ETextBoxType::Implementation)
	{
		Tooltip = TEXT("Example: Add a for loop that iterates over above defined array.");
	}
	
	// Create the number of lines spin box
	SAssignNew(NumberOfLinesSpinBox, SSpinBox<int32>)
		.Value(CurrentNumberOfLines)
		.MinValue(1)
		.MaxValue(50)
		.MinSliderValue(1)
		.MaxSliderValue(20)
		.Delta(1)
		.OnValueChanged(this, &SQCE_InlineAISuggestionBox::OnNumberOfLinesChanged);

	// Create the context text box
	SAssignNew(ContextTextBox, SMultiLineEditableTextBox)
		.Text(CurrentContextText)
		.HintText(FText::FromString(Tooltip))
		.OnTextCommitted(this, &SQCE_InlineAISuggestionBox::OnContextTextBoxTextCommitted)
		.OnTextChanged_Lambda([this](const FText& InText) {
			// Clear warning state when user starts typing
			if (GetSuggestionBoxState() == ShowingWarning)
			{
				if (!InText.IsEmptyOrWhitespace())
					SetSuggestionBoxState(ReadyForInput);
			}
		})
		.AllowMultiLine(true)
		.IsCaretMovedWhenGainFocus(true)
		.SelectAllTextWhenFocused(false)
		.ClearTextSelectionOnFocusLoss(true)
		.RevertTextOnEscape(false)
		.ClearKeyboardFocusOnCommit(false)
		.AllowContextMenu(true)
		.AutoWrapText(true)
		.WrapTextAt(MinDesiredWidth - 20.0f) // Account for padding
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.Padding(FMargin(4.0f, 2.0f));

	// Create the status text block with loading indicator
	SAssignNew(StatusTextBlock, STextBlock)
		.Text(FText::FromString(TEXT("Processing...")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Visibility(EVisibility::Hidden); // Initially hidden

	// Create the shortcut text block
	SAssignNew(ShortcutTextBlock, STextBlock)
		.Text(FText::FromString(TEXT("Ctrl + Space to confirm")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		.Visibility(EVisibility::Visible); // Initially visible

	// Create the warning text block
	SAssignNew(WarningTextBlock, STextBlock)
		.Text(FText::FromString(TEXT("Please provide context to continue...")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.6f, 0.0f, 1.0f))) // Orange color
		.Visibility(EVisibility::Hidden); // Initially hidden


	// Main widget construction with three-row layout
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("Menu.Background"))
		.BorderBackgroundColor(FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.95f)))
		.Padding(FMargin(8.0f))
		[
			SNew(SBox)
			.MinDesiredWidth(MinDesiredWidth)
			.MinDesiredHeight(MinDesiredHeight)
			.MaxDesiredWidth(500.0f)
			.MaxDesiredHeight(MinDesiredHeight)
			[
				SNew(SVerticalBox)
				
				// Top row: "I want to" + ContextTypeComboBox + CompletionTypeComboBox
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("I want to")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						// Conditionally show the appropriate context combo box based on TextBoxType
						TextBoxType == ETextBoxType::Declaration ? 
							DeclarationContextTypeComboBox.ToSharedRef() : 
							ImplementationContextTypeComboBox.ToSharedRef()
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("to")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						CompletionTypeComboBox.ToSharedRef()
					]
				]

				// Second row: Number of lines (only visible for NLinesAboveCursor)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this]() {
						return (GetContextType() == EQCEDefaultContext::NLinesAboveCursor) ? EVisibility::Visible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Number of lines:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(80.0f)
						[
							NumberOfLinesSpinBox.ToSharedRef()
						]
					]
				]

				// Third row: Context text box
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SBox)
					.MinDesiredHeight(24.0f)
					.MaxDesiredHeight(30.0f)
					[
						ContextTextBox.ToSharedRef()
					]
				]

				// Fourth row: Status, warning, and shortcut text
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					// Left side: Status text with loading indicator
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						StatusTextBlock.ToSharedRef()
					]
					// Left side: Warning text
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						WarningTextBlock.ToSharedRef()
					]
					// Right side: Shortcut text
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						ShortcutTextBlock.ToSharedRef()
					]
				]
			]
		]
	];

	ContextTextBox->SetOnKeyCharHandler(FOnKeyChar::CreateRaw(this, &SQCE_InlineAISuggestionBox::HandleKeyChar));

	// Initialize text block visibility based on initial state
	UpdateTextBlockVisibility();
}

FReply SQCE_InlineAISuggestionBox::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	// Focus the context text box when the suggestion box receives focus
	if (ContextTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(ContextTextBox, InFocusEvent.GetCause());
	}
	return SCompoundWidget::OnFocusReceived(MyGeometry, InFocusEvent);
}

FReply SQCE_InlineAISuggestionBox::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);
	
	const FInputChord KeyEventChord(InKeyEvent.GetKey(),
	                                EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(),
	                                                        InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));

	// Handle Find/Search keybinding
	if (Settings->FindKeybinding.IsValidChord() && Settings->FindKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - find functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle Save keybinding
	if (Settings->SaveKeybinding.IsValidChord() && Settings->SaveKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - save functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle Save and Build keybinding
	if (Settings->SaveAndBuildKeybinding.IsValidChord() && Settings->SaveAndBuildKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - save and build functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle Indent keybinding
	if (Settings->IndentKeybinding.IsValidChord() && Settings->IndentKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - indent functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle Unindent keybinding
	if (Settings->UnindentKeybinding.IsValidChord() && Settings->UnindentKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - unindent functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle Go to Line keybinding
	if (Settings->GoToLineKeybinding.IsValidChord() && Settings->GoToLineKeybinding == KeyEventChord)
	{
		// Forward to parent widget or main editor - go to line functionality not directly handled here
		return FReply::Unhandled();
	}

	// Handle AI Inline Completion keybinding (primary function of this widget)
	if (Settings->AIInlineCompletionKeybinding.IsValidChord() && Settings->AIInlineCompletionKeybinding == KeyEventChord)
	{
		ConfirmSuggestion();
		return FReply::Handled();
	}

	// Handle Cancel Inline AI Suggestion keybinding
	if (Settings->CancelInlineAISuggestionKeybinding.IsValidChord() && Settings->CancelInlineAISuggestionKeybinding == KeyEventChord)
	{
		CancelSuggestion();
		SetSuggestionBoxState(ReadyForInput);
		return FReply::Handled();
	}

	// Handle Toggle Completion Type keybinding
	if (Settings->ToggleCompletionTypeKeybinding.IsValidChord() && Settings->ToggleCompletionTypeKeybinding == KeyEventChord)
	{
		ToggleCompletionType();
		return FReply::Handled();
	}

	// Handle Toggle Context Type keybinding
	if (Settings->ToggleContextTypeKeybinding.IsValidChord() && Settings->ToggleContextTypeKeybinding == KeyEventChord)
	{
		ToggleContextType();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SQCE_InlineAISuggestionBox::HandleKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
	check(Settings);

	const TCHAR Key = InCharacterEvent.GetCharacter();

	// If spacebar is reserved for AI inline completion with a keybinding, we want to block typing space when the combination is pressed
	if (Settings->AIInlineCompletionKeybinding.IsValidChord() && Settings->AIInlineCompletionKeybinding.Key == EKeys::SpaceBar && Key == TEXT(' '))
	{
		if ((Settings->AIInlineCompletionKeybinding.bCtrl && InCharacterEvent.IsControlDown()) ||
			(Settings->AIInlineCompletionKeybinding.bAlt && InCharacterEvent.IsAltDown()) ||
			(Settings->AIInlineCompletionKeybinding.bShift && InCharacterEvent.IsShiftDown()) ||
			(Settings->AIInlineCompletionKeybinding.bCmd && InCharacterEvent.IsCommandDown()))
		{
			return FReply::Handled();
		}
	}

	return SCompoundWidget::OnKeyChar(MyGeometry, InCharacterEvent);
}

FText SQCE_InlineAISuggestionBox::GetContextText() const
{
	if (ContextTextBox.IsValid())
	{
		return ContextTextBox->GetText();
	}
	return CurrentContextText;
}

void SQCE_InlineAISuggestionBox::SetCompletionType(EQCEDefaultCompletionType CompletionType)
{
	CurrentCompletionType = CompletionType;
	if (CompletionTypeComboBox.IsValid())
	{
		for (const auto& Option : CompletionTypeOptions)
		{
			if (*Option == CompletionType)
			{
				CompletionTypeComboBox->SetSelectedItem(Option);
				break;
			}
		}
	}
}

EQCEDefaultCompletionType SQCE_InlineAISuggestionBox::GetCompletionType() const
{
	if (CompletionTypeComboBox.IsValid())
	{
		auto SelectedItem = CompletionTypeComboBox->GetSelectedItem();
		if (SelectedItem.IsValid())
		{
			return *SelectedItem;
		}
	}
	return CurrentCompletionType;
}

void SQCE_InlineAISuggestionBox::SetContextType(EQCEDefaultContext ContextType)
{
	CurrentContextType = ContextType;
	
	if (TextBoxType == ETextBoxType::Declaration)
	{
		if (DeclarationContextTypeComboBox.IsValid())
		{
			for (const auto& Option : DeclarationContextTypeOptions)
			{
				if (*Option == ContextType)
				{
					DeclarationContextTypeComboBox->SetSelectedItem(Option);
					break;
				}
			}
		}
	}
	else
	{
		if (ImplementationContextTypeComboBox.IsValid())
		{
			for (const auto& Option : ImplementationContextTypeOptions)
			{
				if (*Option == ContextType)
				{
					ImplementationContextTypeComboBox->SetSelectedItem(Option);
					break;
				}
			}
		}
	}
}

EQCEDefaultContext SQCE_InlineAISuggestionBox::GetContextType() const
{
	if (TextBoxType == ETextBoxType::Declaration)
	{
		if (DeclarationContextTypeComboBox.IsValid())
		{
			auto SelectedItem = DeclarationContextTypeComboBox->GetSelectedItem();
			if (SelectedItem.IsValid())
			{
				return *SelectedItem;
			}
		}
	}
	else
	{
		if (ImplementationContextTypeComboBox.IsValid())
		{
			auto SelectedItem = ImplementationContextTypeComboBox->GetSelectedItem();
			if (SelectedItem.IsValid())
			{
				return *SelectedItem;
			}
		}
	}
	return CurrentContextType;
}

void SQCE_InlineAISuggestionBox::FocusContextTextBox() const
{
	if (ContextTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(ContextTextBox, EFocusCause::SetDirectly);
	}
}

void SQCE_InlineAISuggestionBox::ConfirmSuggestion()
{
	// Get current input text
	FString UserInput = ContextTextBox.IsValid() ? ContextTextBox->GetText().ToString().TrimStartAndEnd() : FString();
	
	// Check if input is empty or whitespace only
	if (UserInput.IsEmpty())
	{
		// Show warning instead of processing
		WarningTextBlock->SetText(FText::FromString(TEXT("Please provide context to continue...")));
		SetSuggestionBoxState(ShowingWarning);
		return;
	}
	
	if (OnConfirmed.IsBound())
	{
		// Create context struct with current state
		FUserInputContext Context(GetCompletionType(), GetContextType(), UserInput);
		Context.TextBoxType = TextBoxType;
		Context.NumberOfLines = GetNumberOfLines();
		OnConfirmed.Execute(Context);
	}
}

void SQCE_InlineAISuggestionBox::CancelSuggestion() const
{
	if (OnCancelled.IsBound())
	{
		OnCancelled.Execute();
	}
}

ESuggestionBoxState SQCE_InlineAISuggestionBox::GetSuggestionBoxState() const
{
	return SuggestionBoxState;
}

void SQCE_InlineAISuggestionBox::SetSuggestionBoxState(ESuggestionBoxState NewState)
{
	if (SuggestionBoxState != NewState)
	{
		SuggestionBoxState = NewState;
		UpdateTextBlockVisibility();
	}
}

void SQCE_InlineAISuggestionBox::UpdateTextBlockVisibility() const
{
	if (StatusTextBlock.IsValid() && ShortcutTextBlock.IsValid() && WarningTextBlock.IsValid())
	{
		switch (SuggestionBoxState)
		{
			case Processing:
				StatusTextBlock->SetVisibility(EVisibility::Visible);
				ShortcutTextBlock->SetVisibility(EVisibility::Hidden);
				WarningTextBlock->SetVisibility(EVisibility::Hidden);
				break;
			case ShowingWarning:
				StatusTextBlock->SetVisibility(EVisibility::Hidden);
				ShortcutTextBlock->SetVisibility(EVisibility::Hidden);
				WarningTextBlock->SetVisibility(EVisibility::Visible);
				break;
			case ReadyForInput:
			default:
				StatusTextBlock->SetVisibility(EVisibility::Hidden);
				ShortcutTextBlock->SetVisibility(EVisibility::Visible);
				WarningTextBlock->SetVisibility(EVisibility::Hidden);
				break;
		}
	}
}

void SQCE_InlineAISuggestionBox::OnCompletionTypeSelectionChanged(TSharedPtr<EQCEDefaultCompletionType> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		CurrentCompletionType = *SelectedItem;
		
		// Save to settings
		UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>();
		if (Settings)
		{
			Settings->DefaultCompletionType = CurrentCompletionType;
			Settings->SaveConfig();
		}
		
		if (OnCompletionTypeChanged.IsBound())
		{
			OnCompletionTypeChanged.Execute(CurrentCompletionType);
		}
	}
}

void SQCE_InlineAISuggestionBox::OnContextTypeSelectionChanged(TSharedPtr<EQCEDefaultContext> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		CurrentContextType = *SelectedItem;
		
		// Save to settings based on text box type
		UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>();
		if (Settings)
		{
			if (TextBoxType == ETextBoxType::Declaration)
			{
				Settings->DefaultDeclarationContextType = CurrentContextType;
			}
			else
			{
				Settings->DefaultImplementationContextType = CurrentContextType;
			}
			Settings->SaveConfig();
		}
		
		if (OnContextTypeChanged.IsBound())
		{
			OnContextTypeChanged.Execute(CurrentContextType);
		}
	}
}


void SQCE_InlineAISuggestionBox::OnContextTextBoxTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
	CurrentContextText = InText;
}

void SQCE_InlineAISuggestionBox::SetNumberOfLines(int32 NumberOfLines)
{
	CurrentNumberOfLines = FMath::Clamp(NumberOfLines, 1, 50);
	if (NumberOfLinesSpinBox.IsValid())
	{
		NumberOfLinesSpinBox->SetValue(CurrentNumberOfLines);
	}
}

int32 SQCE_InlineAISuggestionBox::GetNumberOfLines() const
{
	if (NumberOfLinesSpinBox.IsValid())
	{
		return NumberOfLinesSpinBox->GetValue();
	}
	return CurrentNumberOfLines;
}

void SQCE_InlineAISuggestionBox::OnNumberOfLinesChanged(int32 NewValue)
{
	CurrentNumberOfLines = FMath::Clamp(NewValue, 1, 50);
	
	// Save to settings
	UQCE_EditorSettings* Settings = GetMutableDefault<UQCE_EditorSettings>();
	if (Settings)
	{
		Settings->DefaultNumberOfLines = CurrentNumberOfLines;
		Settings->SaveConfig();
	}
}

bool SQCE_InlineAISuggestionBox::ShowInlineAISuggestionBox(const FVector2D& ScreenPosition, TSharedPtr<SWidget> OwnerWidget)
{
	if (!OwnerWidget.IsValid())
	{
		return false;
	}

	// Create the suggestion box widget
	TSharedPtr<SQCE_InlineAISuggestionBox> SuggestionBox;
	SAssignNew(SuggestionBox, SQCE_InlineAISuggestionBox)
		.MinDesiredWidth(350.0f)
		.MinDesiredHeight(90.0f)
		.CompletionType(EQCEDefaultCompletionType::CurrentLine)
		.ContextType(EQCEDefaultContext::CurrentFunction)
		.ContextText(FText::GetEmpty())
		.OnConfirmed_Lambda([](const FUserInputContext& Context) {
			// Handle confirmation - close the popup
			if (FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().DismissAllMenus();
			}
		})
		.OnCancelled_Lambda([]() {
			// Handle cancellation - close the popup
			if (FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().DismissAllMenus();
			}
		});

	if (!SuggestionBox.IsValid())
	{
		return false;
	}

	// Show the suggestion box as a popup menu
	TSharedPtr<IMenu> MenuContainer = FSlateApplication::Get().PushMenu(
		OwnerWidget.ToSharedRef(),
		FWidgetPath(),
		SuggestionBox.ToSharedRef(),
		ScreenPosition,
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		false, // bFocusImmediately
		FVector2D(1.0f, 1.0f) // WindowAnchor (top-left corner)
	);

	// Focus the context text box after the popup is shown
	if (MenuContainer.IsValid())
	{
		SuggestionBox->FocusContextTextBox();
		return true;
	}

	return false;
}

void SQCE_InlineAISuggestionBox::ToggleCompletionType()
{
	if (!CompletionTypeComboBox.IsValid())
	{
		return;
	}

	// Find current selection index
	int32 CurrentIndex = 0;
	auto CurrentSelection = CompletionTypeComboBox->GetSelectedItem();
	if (CurrentSelection.IsValid())
	{
		for (int32 i = 0; i < CompletionTypeOptions.Num(); i++)
		{
			if (*CompletionTypeOptions[i] == *CurrentSelection)
			{
				CurrentIndex = i;
				break;
			}
		}
	}

	// Move to next option (wrap around)
	int32 NextIndex = (CurrentIndex + 1) % CompletionTypeOptions.Num();
	
	// Set the new selection
	if (CompletionTypeOptions.IsValidIndex(NextIndex))
	{
		CompletionTypeComboBox->SetSelectedItem(CompletionTypeOptions[NextIndex]);
		OnCompletionTypeSelectionChanged(CompletionTypeOptions[NextIndex], ESelectInfo::Direct);
	}
}

void SQCE_InlineAISuggestionBox::ToggleContextType()
{
	// Use the appropriate combo box based on TextBoxType
	TSharedPtr<SComboBox<TSharedPtr<EQCEDefaultContext>>> ActiveComboBox;
	TArray<TSharedPtr<EQCEDefaultContext>>* ActiveOptions = nullptr;
	
	if (TextBoxType == ETextBoxType::Declaration)
	{
		ActiveComboBox = DeclarationContextTypeComboBox;
		ActiveOptions = &DeclarationContextTypeOptions;
	}
	else
	{
		ActiveComboBox = ImplementationContextTypeComboBox;
		ActiveOptions = &ImplementationContextTypeOptions;
	}

	if (!ActiveComboBox.IsValid() || !ActiveOptions)
	{
		return;
	}

	// Find current selection index
	int32 CurrentIndex = 0;
	auto CurrentSelection = ActiveComboBox->GetSelectedItem();
	if (CurrentSelection.IsValid())
	{
		for (int32 i = 0; i < ActiveOptions->Num(); i++)
		{
			if (*(*ActiveOptions)[i] == *CurrentSelection)
			{
				CurrentIndex = i;
				break;
			}
		}
	}

	// Move to next option (wrap around)
	int32 NextIndex = (CurrentIndex + 1) % ActiveOptions->Num();
	
	// Set the new selection
	if (ActiveOptions->IsValidIndex(NextIndex))
	{
		ActiveComboBox->SetSelectedItem((*ActiveOptions)[NextIndex]);
		OnContextTypeSelectionChanged((*ActiveOptions)[NextIndex], ESelectInfo::Direct);
	}
}

FText SQCE_InlineAISuggestionBox::GetCompletionTypeText(TSharedPtr<EQCEDefaultCompletionType> CompletionType) const
{
	if (!CompletionType.IsValid())
	{
		return FText::FromString(TEXT("Unknown"));
	}

	switch (*CompletionType)
	{
		case EQCEDefaultCompletionType::CurrentLine:
			return FText::FromString(TEXT("complete this line"));
		case EQCEDefaultCompletionType::Block:
			return FText::FromString(TEXT("generate a code block"));
		default:
			return FText::FromString(TEXT("Unknown"));
	}
}

FText SQCE_InlineAISuggestionBox::GetContextTypeText(TSharedPtr<EQCEDefaultContext> ContextType) const
{
	if (!ContextType.IsValid())
	{
		return FText::FromString(TEXT("Unknown"));
	}

	// Different text based on TextBoxType
	if (TextBoxType == ETextBoxType::Declaration)
	{
		switch (*ContextType)
		{
			case EQCEDefaultContext::CurrentLineOrFunction:
				return FText::FromString(TEXT("use current line"));
			case EQCEDefaultContext::NLinesAboveCursor:
				return FText::FromString(TEXT("use nearby lines"));
			default:
				return FText::FromString(TEXT("Unknown"));
		}
	}
	else
	{
		switch (*ContextType)
		{
			case EQCEDefaultContext::CurrentFunction:
				return FText::FromString(TEXT("use the function"));
			case EQCEDefaultContext::CurrentLineOrFunction:
				return FText::FromString(TEXT("use code above"));
			case EQCEDefaultContext::NLinesAboveCursor:
				return FText::FromString(TEXT("use nearby lines"));
			default:
				return FText::FromString(TEXT("Unknown"));
		}
	}
}