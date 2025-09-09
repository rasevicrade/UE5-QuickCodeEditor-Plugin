// Copyright TechnicallyArtist 2025 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

class UMainEditorContainer;

/**
 * Contains information about preceding text and currently typed token.
 */
struct QUICKCODEEDITOR_API FCompletionContext
{
    /** All text before the cursor position */
    FString PrecedingText;
    
    /** The current token/word being typed */
    FString CurrentToken;
    FString HeaderText;
    FString ImplementationText;
    
    /** Reference to the main editor container for accessing function info */
    UMainEditorContainer* MainEditorContainer = nullptr;
};

/**
 * Completion item that will be shown to the user
 */
struct QUICKCODEEDITOR_API FCompletionItem
{
    /** Text shown in the completion list */
    FString DisplayText;
    
    /** Text that will be inserted when selected */
    FString InsertText;
    
    /** Score for sorting (higher = better match) */
    int32 Score = 0;
    
    /** Whether this item can be selected and inserted */
    bool bSelectable = true;
    
    UTexture2D* Icon = nullptr;

    FCompletionItem() = default;
    
    bool operator==(const FCompletionItem& Other) const
    {
        return InsertText == Other.InsertText;
    }
    
    /** Get hash for TSet/TMap usage */
    friend uint32 GetTypeHash(const FCompletionItem& Item)
    {
        return GetTypeHash(Item.InsertText);
    }
};