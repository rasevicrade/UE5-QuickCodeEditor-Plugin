// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/Utility/CppIO/FunctionCppWriter.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/CRC.h"
#include "QuickCodeEditor.h"

bool FFunctionCppWriter::WriteFunctionDeclaration(const FFunctionDeclarationInfo& DeclarationInfo,
    const FString& UpdatedDeclarationCode, const bool bIsLoadedIsolated, const bool bForceOverwrite)
{
    // Validate input parameters
    if (DeclarationInfo.HeaderPath.IsEmpty())
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: HeaderPath is empty"));
        return false;
    }

    if (UpdatedDeclarationCode.IsEmpty())
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: UpdatedDeclarationCode is empty"));
        return false;
    }

    if (DeclarationInfo.DeclarationStartPosition == -1 || DeclarationInfo.DeclarationEndPosition == -1)
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: Invalid declaration positions (Start: %d, End: %d)"), 
               DeclarationInfo.DeclarationStartPosition, DeclarationInfo.DeclarationEndPosition);
        return false;
    }

    if (DeclarationInfo.DeclarationStartPosition >= DeclarationInfo.DeclarationEndPosition)
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: Invalid position range (Start: %d >= End: %d)"), 
               DeclarationInfo.DeclarationStartPosition, DeclarationInfo.DeclarationEndPosition);
        return false;
    }

    // Read current file content from disk
    FString CurrentFileContent;
    if (!FFileHelper::LoadFileToString(CurrentFileContent, *DeclarationInfo.HeaderPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: Failed to read header file '%s'"), 
               *DeclarationInfo.HeaderPath);
        return false;
    }

    // Verify file hasn't changed since we read it
    const uint32 CurrentChecksum = FCrc::StrCrc32(*CurrentFileContent);
    if (!bForceOverwrite && CurrentChecksum != DeclarationInfo.ContentChecksum)
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: File '%s' has changed since last read (Expected checksum: %u, Current: %u)"), 
               *DeclarationInfo.HeaderPath, DeclarationInfo.ContentChecksum, CurrentChecksum);
        return false;
    }
    

    // Extract the current declaration for validation
    const int32 CurrentDeclarationLength = DeclarationInfo.DeclarationEndPosition - DeclarationInfo.DeclarationStartPosition;
    const FString CurrentDeclaration = CurrentFileContent.Mid(DeclarationInfo.DeclarationStartPosition, CurrentDeclarationLength);
    

    // Create backup file
    const FString BackupPath = DeclarationInfo.HeaderPath + TEXT(".backup");
    if (!FFileHelper::SaveStringToFile(CurrentFileContent, *BackupPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: Failed to create backup file '%s'"), *BackupPath);
        return false;
    }

    // Build the new file content
    FString NewFileContent;
    if (bIsLoadedIsolated)
    {
        NewFileContent = CurrentFileContent.Left(DeclarationInfo.DeclarationStartPosition) + 
                          UpdatedDeclarationCode + 
                          CurrentFileContent.Mid(DeclarationInfo.DeclarationEndPosition);
    }
    else
    {
        NewFileContent = UpdatedDeclarationCode;
    }
  

    // Write the updated content to file
    if (!FFileHelper::SaveStringToFile(NewFileContent, *DeclarationInfo.HeaderPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: Failed to write updated content to '%s'"), 
               *DeclarationInfo.HeaderPath);
        
        // Attempt to restore from backup
        if (FFileHelper::SaveStringToFile(CurrentFileContent, *DeclarationInfo.HeaderPath))
        {
            UE_LOG(LogQuickCodeEditor, Warning, TEXT("WriteFunctionDeclaration: Restored original content from backup"));
        }
        else
        {
            UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionDeclaration: CRITICAL - Failed to restore original content! Backup available at '%s'"), *BackupPath);
        }
        return false;
    }

    // Clean up backup file on success
    IFileManager::Get().Delete(*BackupPath);

    UE_LOG(LogQuickCodeEditor, Log, TEXT("WriteFunctionDeclaration: Successfully updated function '%s' in '%s'"), 
           *DeclarationInfo.FunctionName, *DeclarationInfo.HeaderPath);
    return true;
}

bool FFunctionCppWriter::WriteFunctionImplementation(const FFunctionImplementationInfo& ImplementationInfo,
    const FString& UpdatedImplementationCode, const bool bIsLoadedIsolated, const bool bForceOverwrite)
{
    // Validate input parameters
    if (ImplementationInfo.CppPath.IsEmpty())
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: CppPath is empty"));
        return false;
    }

    if (UpdatedImplementationCode.IsEmpty())
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: UpdatedImplementationCode is empty"));
        return false;
    }
    
    if (ImplementationInfo.ImplementationStartPosition > ImplementationInfo.ImplementationEndPosition)
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: Invalid position range (Start: %d >= End: %d)"), 
               ImplementationInfo.ImplementationStartPosition, ImplementationInfo.ImplementationEndPosition);
        return false;
    }

    // Read current file content from disk
    FString CurrentFileContent;
    if (!FFileHelper::LoadFileToString(CurrentFileContent, *ImplementationInfo.CppPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: Failed to read implementation file '%s'"), 
               *ImplementationInfo.CppPath);
        return false;
    }

    // Verify file hasn't changed since we read it
    const uint32 CurrentChecksum = FCrc::StrCrc32(*CurrentFileContent);
    if (!bForceOverwrite && (CurrentChecksum != ImplementationInfo.ContentChecksum))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: File '%s' has changed since last read (Expected checksum: %u, Current: %u)"), 
               *ImplementationInfo.CppPath, ImplementationInfo.ContentChecksum, CurrentChecksum);
        return false;
    }
    

    // Extract the current implementation for validation
    const int32 CurrentImplementationLength = ImplementationInfo.ImplementationEndPosition - ImplementationInfo.ImplementationStartPosition;
    const FString CurrentImplementation = CurrentFileContent.Mid(ImplementationInfo.ImplementationStartPosition, CurrentImplementationLength);

    // Create backup file
    const FString BackupPath = ImplementationInfo.CppPath + TEXT(".backup");
    if (!FFileHelper::SaveStringToFile(CurrentFileContent, *BackupPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: Failed to create backup file '%s'"), *BackupPath);
        return false;
    }

    // Build the new file content
    FString NewFileContent;
    if (bIsLoadedIsolated)
    {
        NewFileContent = CurrentFileContent.Left(ImplementationInfo.ImplementationStartPosition) + 
                           UpdatedImplementationCode + 
                           CurrentFileContent.Mid(ImplementationInfo.ImplementationEndPosition);
    }
    else
    {
        NewFileContent = UpdatedImplementationCode;
    }
   

    // Write the updated content to file
    if (!FFileHelper::SaveStringToFile(NewFileContent, *ImplementationInfo.CppPath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: Failed to write updated content to '%s'"), 
               *ImplementationInfo.CppPath);
        
        // Attempt to restore from backup
        if (FFileHelper::SaveStringToFile(CurrentFileContent, *ImplementationInfo.CppPath))
        {
            UE_LOG(LogQuickCodeEditor, Warning, TEXT("WriteFunctionImplementation: Restored original content from backup"));
        }
        else
        {
            UE_LOG(LogQuickCodeEditor, Error, TEXT("WriteFunctionImplementation: CRITICAL - Failed to restore original content! Backup available at '%s'"), *BackupPath);
        }
        return false;
    }

    // Clean up backup file on success
    IFileManager::Get().Delete(*BackupPath);

    UE_LOG(LogQuickCodeEditor, Log, TEXT("WriteFunctionImplementation: Successfully updated function '%s' in '%s'"), 
           *ImplementationInfo.FunctionName, *ImplementationInfo.CppPath);
    return true;
}
