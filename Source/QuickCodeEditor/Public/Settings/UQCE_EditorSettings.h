// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/Color.h"
#include "Framework/Commands/InputChord.h"
#include "Editor/Features/AI/QCE_AIContainer.h"
#include "UQCE_EditorSettings.generated.h"

UENUM(BlueprintType)
enum class EQCEColorPreset : uint8
{
	MidnightStudio		UMETA(DisplayName = "Midnight Studio"),
	CosmicCode			UMETA(DisplayName = "Cosmic Code"),
	Custom			UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EQCEDefaultAIProvider : uint8
{
	Claude		UMETA(DisplayName = "Claude"),
	ChatGPT		UMETA(DisplayName = "ChatGPT")
};


UENUM(BlueprintType)
enum class EQCEDefaultCompletionType : uint8
{
	CurrentLine		UMETA(DisplayName = "Current Line"),
	Block			UMETA(DisplayName = "Block")
};

UENUM(BlueprintType)
enum class EQCEDefaultContext : uint8
{
	CurrentFunction				UMETA(DisplayName = "Current Function"),
	CurrentLineOrFunction		UMETA(DisplayName = "Current Function Before Cursor"), // This is Line in case of declaration, and function in case of implementation context
	NLinesAboveCursor			UMETA(DisplayName = "N Lines Above Cursor")
};

UENUM(BlueprintType)
enum class EQCEIndentationType : uint8
{
	Tabs		UMETA(DisplayName = "Tabs"),
	Spaces		UMETA(DisplayName = "Spaces")
};

DECLARE_DELEGATE(FOnSyntaxSettingsUpdated)
DECLARE_DELEGATE(FOnKeybindingsUpdated)

UCLASS(config = EditorPerProjectUserSettings)
class QUICKCODEEDITOR_API UQCE_EditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UQCE_EditorSettings();

	FOnSyntaxSettingsUpdated OnSyntaxSettingsUpdated;
	FOnKeybindingsUpdated OnKeybindingsUpdated;
	
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Claude", 
		meta = (DisplayName = "Claude API Key"))
	FString ClaudeApiKey;

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Claude", 
	meta = (DisplayName = "Claude Model"))
	FString ModelVersion = TEXT("claude-3-5-sonnet-20241022");

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Claude", 
		meta = (DisplayName = "Claude API Endpoint"))
	FString ClaudeApiEndpoint = TEXT("https://api.anthropic.com/v1/messages");

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Chat GPT", 
		meta = (DisplayName = "OpenAI API Key"))
	FString OpenAIApiKey;
	
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Chat GPT", 
		meta = (DisplayName = "OpenAI Model"))
	FString OpenAIModelVersion = TEXT("gpt-4o");

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Common|Chat GPT", 
		meta = (DisplayName = "OpenAI API Endpoint"))
	FString OpenAIApiEndpoint = TEXT("https://api.openai.com/v1/chat/completions");

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat", 
		meta = (DisplayName = "Default Chat Agent"))
	EQCEDefaultAIProvider DefaultAIProvider = EQCEDefaultAIProvider::Claude;

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat", 
		meta = (DisplayName = "Default AI Context"))
	EQCEAIContext DefaultAIContext = EQCEAIContext::VisibleCode;

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion",
		meta = (DisplayName = "Max Context Lines", ClampMin = "10", ClampMax = "200",
			ToolTip = "Maximum number of lines of code context to include when requesting AI inline completions. Lower values reduce token usage."))
	int32 MaxInlineContextLines = 5;

	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion", 
		meta = (DisplayName = "Inline Completions Agent"))
	EQCEDefaultAIProvider InlineSuggestionsAIProvider = EQCEDefaultAIProvider::Claude;

	/** Default completion type for inline completion */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion",
		meta = (DisplayName = "Default Completion Type",
			ToolTip = "Default type of completion to use (Dropdown, Current Line, or Block)"))
	EQCEDefaultCompletionType DefaultCompletionType = EQCEDefaultCompletionType::Block;

	/** Default context for implementation files (.cpp) inline completion */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion",
		meta = (DisplayName = "Default Implementation Context Type",
			ToolTip = "Default context to use for AI completions in implementation files"))
	EQCEDefaultContext DefaultImplementationContextType = EQCEDefaultContext::CurrentLineOrFunction;

	/** Default context for declaration files (.h) inline completion */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion",
		meta = (DisplayName = "Default Declaration Context Type",
			ToolTip = "Default context to use for AI completions in declaration files"))
	EQCEDefaultContext DefaultDeclarationContextType = EQCEDefaultContext::NLinesAboveCursor;

	/** Default number of lines to use when context type is 'N Lines Above Cursor' */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Inline Completion",
		meta = (DisplayName = "Default Number of Lines",
			ToolTip = "Default number of lines to include when using 'N Lines Above Cursor' context", ClampMin = "1", ClampMax = "50"))
	int32 DefaultNumberOfLines = 5;

	/** Maximum number of recent messages to include in API calls (excluding function context) */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat",
		meta = (DisplayName = "Max History Messages", ClampMin = "1", ClampMax = "50", 
			ToolTip = "Maximum number of recent messages to include in API calls (excluding function context). Lower values reduce token usage."))
	int32 MaxHistoryMessages = 5;

	/** Maximum tokens for simple queries (1-2 messages) */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat",
		meta = (DisplayName = "Simple Query Max Tokens", ClampMin = "100", ClampMax = "4096",
			ToolTip = "Maximum tokens for simple queries with 1-2 messages. Lower values reduce token usage."))
	int32 SimpleQueryMaxTokens = 1024;

	/** Maximum tokens for regular conversations */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat",
		meta = (DisplayName = "Regular Max Tokens", ClampMin = "100", ClampMax = "4096",
			ToolTip = "Maximum tokens for regular conversations. Used when conversation has more than 2 messages."))
	int32 RegularMaxTokens = 2048;

	/** Enable code completion feature (master switch) */
	// UPROPERTY(Config, EditAnywhere, Category = "Editor Settings",
	// 	meta = (DisplayName = "Is Enabled",
	// 		ToolTip = "Enable or disable the method/function dropdown feature.(BETA)"))
	// bool bIsDropdownCompletionEnabled = false;

	/** System instructions sent to the AI assistant for each conversation */
	UPROPERTY(Config, EditAnywhere, Category = "AI Settings|Chat", 
		meta = (DisplayName = "System Instructions", MultiLine = true))
	FString SystemInstructions =  TEXT("- UE C++ function context\n- Keep answers concise\n- Help understand/optimize/expand function\n- Follow UE5.1+ conventions\n- Verify functions exist");

	/** Font Settings */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Settings|Font",
		meta = (DisplayName = "Font Size", ClampMin = "8", ClampMax = "72"))
	int32 FontSize = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Editor Settings|Font",
		meta = (DisplayName = "Bold Font"))
	bool bUseBoldFont = false;

	/** Editor Behavior Settings */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Settings|Indentation",
		meta = (DisplayName = "Tab Space Count", ClampMin = "1", ClampMax = "8",
			ToolTip = "Number of spaces to insert when pressing the Tab key"))
	int32 TabSpaceCount = 4;

	UPROPERTY(Config, EditAnywhere, Category = "Editor Settings|Indentation",
		meta = (DisplayName = "Indentation Type",
			ToolTip = "Choose whether to use tabs or spaces for indentation"))
	EQCEIndentationType IndentationType = EQCEIndentationType::Tabs;

	/** Keyboard Shortcuts */
	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Find/Search"))
	FInputChord FindKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Save"))
	FInputChord SaveKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Save and Build"))
	FInputChord SaveAndBuildKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Indent"))
	FInputChord IndentKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Unindent"))
	FInputChord UnindentKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Go to Line"))
	FInputChord GoToLineKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts|AI",
		meta = (DisplayName = "AI Inline Completion Keybinding"))
	FInputChord AIInlineCompletionKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts",
		meta = (DisplayName = "Autocompletion Dropdown"))
	FInputChord AutocompletionDropdownKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts|AI",
		meta = (DisplayName = "Cancel AI Inline Suggestion Keybinding"))
	FInputChord CancelInlineAISuggestionKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts|AI",
		meta = (DisplayName = "Toggle Completion Type Keybinding",
			ToolTip = "Keybinding to cycle through completion types (Dropdown -> Current Line -> Block)"))
	FInputChord ToggleCompletionTypeKeybinding;

	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Shortcuts|AI",
		meta = (DisplayName = "Toggle Context Type Keybinding",
			ToolTip = "Keybinding to cycle through context types (Current Function -> Current Function Before Cursor -> N Lines Above Cursor)"))
	FInputChord ToggleContextTypeKeybinding;

	/** Color Preset Settings */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Color Preset"))
	EQCEColorPreset ColorPreset = EQCEColorPreset::MidnightStudio;
	

	/** Apply a color preset to all color settings */
	UFUNCTION(BlueprintCallable, Category = "Editor Colors")
	void ApplyColorPreset(EQCEColorPreset Preset);
	void SetKeybindings();

	/** Reset all keybindings to their default values */
	UFUNCTION(BlueprintCallable, Category = "Keyboard Shortcuts")
	void ResetKeybindingsToDefaults();

	/** Reset all settings to their default values */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	bool ResetToDefaults();

#if WITH_EDITOR
	/** Override to automatically apply color presets when changed in the editor */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
	meta = (DisplayName = "Text"))
	FLinearColor TextColor = FLinearColor(0.863f, 0.863f, 0.863f, 1.0f); // #DCDCDC
	
	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Keywords"))
	FLinearColor KeywordColor = FLinearColor(0.15f, 0.301f, 0.831f, 1.f); // #569CD6 (blue)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Comments"))
	FLinearColor CommentColor = FLinearColor(0.235f, 0.552f, 0.15f, 1.f); // #67A667 (green)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Strings"))
	FLinearColor StringColor = FLinearColor(0.584f, 0.361f, 0.153f, 1.f); // #D3AB80 (orange-tan)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Numbers"))
	FLinearColor NumberColor = FLinearColor(0.847f, 0.296f, 0.527f, 1.f); // #B5DCB5 (light green)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Types"))
	FLinearColor TypeColor = FLinearColor(0.533f, 0.283f, 1.f, 1.f); // #4EC6C8 (cyan)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Function Names"))
	FLinearColor FunctionColor = FLinearColor(0.822786f, 0.760525f, 0.278894f, 1.000000f); // Function names (green)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Syntax",
		meta = (DisplayName = "Class Names"))
	FLinearColor ClassColor = FLinearColor(0.533f, 0.283f, 1.f, 1.f); // Class names (purple)

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Highlight",
		meta = (DisplayName = "Word Highlight"))
	FLinearColor WordHighlightColor =FLinearColor(0.14f, 0.3f, 0.83f, 0.3f);; // Semi-transparent yellow

	/** Tab Colors */
	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Active Tab Background"))
	FLinearColor ActiveTabBackgroundColor = FLinearColor(0.15f, 0.35f, 0.65f, 0.0f); // Blue

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Active Tab Border"))
	FLinearColor ActiveTabBorderColor = FLinearColor::White; // Lighter blue

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Active Tab Text"))
	FLinearColor ActiveTabTextColor = FLinearColor::White;

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Inactive Tab Background"))
	FLinearColor InactiveTabBackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.3f); // Dark gray

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Inactive Tab Text"))
	FLinearColor InactiveTabTextColor = FLinearColor(0.7f, 0.7f, 0.7f); // Light gray

	UPROPERTY(Config, EditAnywhere, Category = "Editor Colors|Tabs",
		meta = (DisplayName = "Modified File Indicator"))
	FLinearColor ModifiedFileIndicatorColor =FLinearColor(1.000000f, 0.743137f, 0.145098f, 1.0f);;
	
};
