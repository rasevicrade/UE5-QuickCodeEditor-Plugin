// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Settings/UQCE_EditorSettings.h"

#if WITH_EDITOR
#include "Engine/Engine.h"
#endif

UQCE_EditorSettings::UQCE_EditorSettings()
{
	SetKeybindings();
}

void UQCE_EditorSettings::ApplyColorPreset(EQCEColorPreset Preset)
{
	switch (Preset)
	{
		case EQCEColorPreset::MidnightStudio:
			TextColor = FLinearColor(0.863f, 0.863f, 0.863f, 1.0f);
			KeywordColor = FLinearColor(0.15f, 0.301f, 0.831f, 1.f);;
			CommentColor = FLinearColor(0.235f, 0.552f, 0.15f, 1.f);;
			StringColor = FLinearColor(0.584f, 0.361f, 0.153f, 1.f);;
			NumberColor = FLinearColor(0.847f, 0.296f, 0.527f, 1.f);;
			TypeColor = FLinearColor(0.533f, 0.283f, 1.f, 1.f);
			FunctionColor = FLinearColor(0.822786f, 0.760525f, 0.278894f, 1.000000f);
			ClassColor = FLinearColor(0.533f, 0.283f, 1.f, 1.f);
			break;

		case EQCEColorPreset::CosmicCode:
		default: 
			TextColor = FLinearColor(0.863f, 0.863f, 0.863f, 1.0f);
			KeywordColor = FLinearColor(0.15f, 0.501f, 0.921f, 1.0f);
			CommentColor = FLinearColor(0.376f, 0.557f, 0.376f, 1.0f);
			StringColor = FLinearColor(0.847f, 0.533f, 0.376f, 1.0f);
			NumberColor = FLinearColor(0.714f, 0.408f, 0.408f, 1.0f);
			TypeColor = FLinearColor(0.921569f, 0.482353f, 0.266667f, 1.0f);
			FunctionColor = FLinearColor(0.822786f, 0.760525f, 0.278894f, 1.000000f);
			ClassColor = FLinearColor(0.074510f, 0.584314f, 0.435294f, 1.0f);;
			break;
	}

	ColorPreset = Preset;
	SaveConfig();
}

void UQCE_EditorSettings::SetKeybindings()
{
	FindKeybinding = FInputChord(EKeys::F, EModifierKey::Control);
	SaveKeybinding = FInputChord(EKeys::S, EModifierKey::Control);
	SaveAndBuildKeybinding = FInputChord(EKeys::B, EModifierKey::Control | EModifierKey::Shift);
	IndentKeybinding = FInputChord(EKeys::Tab);
	UnindentKeybinding = FInputChord(EKeys::Tab, EModifierKey::Shift);
	GoToLineKeybinding = FInputChord(EKeys::G, EModifierKey::Control);
	AIInlineCompletionKeybinding = FInputChord(EKeys::SpaceBar, EModifierKey::Control);
	AutocompletionDropdownKeybinding = FInputChord(EKeys::SpaceBar, EModifierKey::Control | EModifierKey::Shift);
	CancelInlineAISuggestionKeybinding = FInputChord(EKeys::Escape);
	ToggleCompletionTypeKeybinding = FInputChord(EKeys::T, EModifierKey::Control | EModifierKey::Alt);
	ToggleContextTypeKeybinding = FInputChord(EKeys::R, EModifierKey::Control | EModifierKey::Alt);
}

void UQCE_EditorSettings::ResetKeybindingsToDefaults()
{
	SetKeybindings();

	SaveConfig();
	OnKeybindingsUpdated.ExecuteIfBound();
}

bool UQCE_EditorSettings::ResetToDefaults()
{
	// Reset AI Settings
	ModelVersion = TEXT("claude-3-5-sonnet-20241022");
	ClaudeApiEndpoint = TEXT("https://api.anthropic.com/v1/messages");
	OpenAIModelVersion = TEXT("gpt-4o");
	OpenAIApiEndpoint = TEXT("https://api.openai.com/v1/chat/completions");
	DefaultAIProvider = EQCEDefaultAIProvider::Claude;
	MaxInlineContextLines = 5;
	InlineSuggestionsAIProvider = EQCEDefaultAIProvider::Claude;
	DefaultCompletionType = EQCEDefaultCompletionType::Block;
	DefaultImplementationContextType = EQCEDefaultContext::CurrentLineOrFunction;
	DefaultDeclarationContextType = EQCEDefaultContext::NLinesAboveCursor;
	DefaultNumberOfLines = 5;
	MaxHistoryMessages = 5;
	SimpleQueryMaxTokens = 1024;
	RegularMaxTokens = 2048;

	// Reset Editor Settings
	FontSize = 10;
	bUseBoldFont = false;
	TabSpaceCount = 4;
	IndentationType = EQCEIndentationType::Tabs;

	// Reset Keybindings
	SetKeybindings();

	// Reset Color Settings to default preset
	ColorPreset = EQCEColorPreset::MidnightStudio;
	ApplyColorPreset(ColorPreset);

	// Reset Tab Colors
	ActiveTabBackgroundColor = FLinearColor(0.15f, 0.35f, 0.65f, 0.0f);
	ActiveTabBorderColor = FLinearColor::White;
	ActiveTabTextColor = FLinearColor::White;
	InactiveTabBackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.3f);
	InactiveTabTextColor = FLinearColor(0.7f, 0.7f, 0.7f);
	ModifiedFileIndicatorColor = FLinearColor(1.000000f, 0.743137f, 0.145098f, 1.0f);

	// Save the reset configuration
	SaveConfig();

	// Notify delegates
	OnSyntaxSettingsUpdated.ExecuteIfBound();
	OnKeybindingsUpdated.ExecuteIfBound();

	return true;
}

#if WITH_EDITOR
void UQCE_EditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, ColorPreset))
		{
			if (ColorPreset != EQCEColorPreset::Custom)
			{
				ApplyColorPreset(ColorPreset);
			}
		}
		
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, FindKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, SaveKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, SaveAndBuildKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, IndentKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, UnindentKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, GoToLineKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, AIInlineCompletionKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, AutocompletionDropdownKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, CancelInlineAISuggestionKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, ToggleCompletionTypeKeybinding) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UQCE_EditorSettings, ToggleContextTypeKeybinding))
		{
			OnKeybindingsUpdated.ExecuteIfBound();
		}
		
		
		OnSyntaxSettingsUpdated.ExecuteIfBound();
	}
}
#endif