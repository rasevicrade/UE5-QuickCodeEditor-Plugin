// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/Utility/CppIO/FunctionCppReader.h"

#include "QuickCodeEditor.h"

#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SourceCodeNavigation.h"
#include "Editor/CustomTextBox/GenerateDefinition/QCE_GenerateDefinitionHelpers.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"
#include "Misc/CRC.h"

FFunctionCppReader::FFunctionCppReader()
{
    // Initialize instance data as needed
}

bool FFunctionCppReader::GetFunctionDeclaration(const UFunction* Function, FFunctionDeclarationInfo& OutDeclarationInfo, const bool bShouldRefresh)
{
    if (!bShouldRefresh && !LoadedDeclarationInfo.FunctionName.IsEmpty() && (LoadedDeclarationInfo.FunctionName == Function->GetName()))
    {
        OutDeclarationInfo = LoadedDeclarationInfo;
        return true;
    }
        
    FString FileContent;
    FString HeaderPath;
    if (!ReadCppFileContent(Function, FileContent, HeaderPath, QCE_CppFileType::Header))
        return false;

    OutDeclarationInfo.HeaderPath = HeaderPath;
    OutDeclarationInfo.ContentChecksum = FCrc::StrCrc32(*FileContent);
    OutDeclarationInfo.InitialFileContent = FileContent;
    OutDeclarationInfo.ClassName = QCE_CommonIOHelpers::ExtractClassNameFromDeclarationFile(FileContent);
    
    bool bIsDeclarationParsed =  ParseDeclaration(Function, FileContent, OutDeclarationInfo);
    if (!bIsDeclarationParsed)
    {
        OutDeclarationInfo = FFunctionDeclarationInfo();
    }
    LoadedDeclarationInfo = OutDeclarationInfo;
    return bIsDeclarationParsed;
}

bool FFunctionCppReader::GetFunctionImplementation(const UFunction* Function, FFunctionImplementationInfo& OutImplementationInfo, const bool bShouldRefresh)
{
    if (!bShouldRefresh && !LoadedImplementationInfo.FunctionName.IsEmpty() && (LoadedImplementationInfo.FunctionName == Function->GetName()))
    {
        OutImplementationInfo = LoadedImplementationInfo;
        return true;
    }
    
    FString CppPath;
    FString FileContent;
    if (!ReadCppFileContent(Function, FileContent, CppPath, QCE_CppFileType::Implementation))
        return false;

    OutImplementationInfo.CppPath = CppPath;
    OutImplementationInfo.ContentChecksum = FCrc::StrCrc32(*FileContent);
    OutImplementationInfo.InitialFileContent = FileContent;
    
    
    bool bIsImplementationParsed = ParseImplementation(Function, OutImplementationInfo, CppPath);
    if (!bIsImplementationParsed)
    {
        OutImplementationInfo.FunctionImplementation = OutImplementationInfo.InitialFileContent;
        OutImplementationInfo.ImplementationStartPosition = 0;
        OutImplementationInfo.ImplementationEndPosition = OutImplementationInfo.InitialFileContent.Len();
    }
    LoadedImplementationInfo = OutImplementationInfo;
    return bIsImplementationParsed;
}


bool FFunctionCppReader::ParseDeclaration(const UFunction* Function, const FString& FileContent, FFunctionDeclarationInfo& OutDeclarationInfo)
{
    const FString& FunctionName = Function->GetName();
    
    int32 FunctionPosition;
    if (!FindDeclarationPositionInFile(FileContent, Function, FunctionPosition))
        return false;
    
    // Use the refactored helper method to parse the declaration
    if (!QCE_CommonIOHelpers::ParseFunctionDeclarationAtPosition(FileContent, FunctionPosition, FunctionName, OutDeclarationInfo, true))
        return false;
    
    // Apply the more sophisticated const detection that was specific to this method
    OutDeclarationInfo.bIsConst = QCE_CommonIOHelpers::HasConstModifier(OutDeclarationInfo.FunctionDeclaration.TrimStartAndEnd());
    
    return true;
}

bool FFunctionCppReader::ParseImplementation(const UFunction* Function, FFunctionImplementationInfo& OutImplementationInfo, FString& OutCppPath)
{
    FFunctionDeclarationInfo DeclarationInfo;
    if (!GetFunctionDeclaration(Function, DeclarationInfo))
        return false;
    
    FString FileContent;
    if (!ReadCppFileContent(Function, FileContent, OutCppPath, QCE_CppFileType::Implementation))
        return false;

    // Try to find the function implementation start position
    int32 FuncPos;
    if (!QCE_CommonIOHelpers::FindImplementationPositionInContent(FileContent, DeclarationInfo, Function->GetOwnerClass()->GetName(), FuncPos)) 
        return false;

    // Find the start of the function declaration by searching backwards for a complete declaration start
    int32 HeaderStartPos;
    if (!QCE_CommonIOHelpers::FindFunctionImplementationHeaderStart(FileContent, FuncPos, HeaderStartPos))
        return false;

    // Extract implementation parameters
    FString ParameterString;
    if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, FuncPos, ParameterString))
        return false;

    // Find the line containing this function to check modifiers
    int32 LineStart = FuncPos;
    while (LineStart > 0 && FileContent[LineStart - 1] != '\n')
    {
        LineStart--;
    }
    
    int32 LineEnd = FuncPos;
    while (LineEnd < FileContent.Len() && FileContent[LineEnd] != '{' && FileContent[LineEnd] != ';')
    {
        LineEnd++;
    }
    
    FString ImplementationLine = FileContent.Mid(LineStart, LineEnd - LineStart);

    // Find the opening brace while respecting string/comment contexts
    int32 BracePos = INDEX_NONE;
    if (!QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT("{"), FuncPos, ESearchDir::FromStart, BracePos))
        return false;

    // Find the matching closing brace using robust bracket matching
    int32 CloseBracePos = INDEX_NONE;
    if (!QCE_ParameterMatcher::FindMatchingBracket(FileContent, BracePos, TEXT('{'), TEXT('}'), CloseBracePos, true))
        return false;

    // Fill out implementation info
    OutImplementationInfo.FunctionName = DeclarationInfo.FunctionName;
    OutImplementationInfo.Parameters = QCE_ParameterMatcher::ToParameterArray(ParameterString);
    OutImplementationInfo.CppPath = OutCppPath;
    OutImplementationInfo.bIsConst = QCE_CommonIOHelpers::HasConstModifier(ImplementationLine);
    OutImplementationInfo.FunctionImplementation = FileContent.Mid(HeaderStartPos, CloseBracePos - HeaderStartPos + 1);
    
    // Store the implementation positions for writing
    OutImplementationInfo.ImplementationStartPosition = HeaderStartPos;
    OutImplementationInfo.ImplementationEndPosition = CloseBracePos + 1; // Include the closing brace
    
    return true;
}

bool FFunctionCppReader::FindDeclarationPositionInFile(const FString& FileContent, const UFunction* Function, int32& OutFunctionPosition)
{
    const FString FunctionName = Function->GetName();
    
    TArray<int32> PossibleMatchPositions;
    if (!QCE_CommonIOHelpers::FilterPositionsByName(FileContent, FunctionName, PossibleMatchPositions))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Function '%s' not found in file content"), *FunctionName);
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found %d name matches for function '%s'"), PossibleMatchPositions.Num(), *FunctionName);
    
    TArray<int32> NonCommentMatches;
    if (!QCE_CommonIOHelpers::FilterCommentedPositions(FileContent, PossibleMatchPositions, NonCommentMatches))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("All %d name matches for function '%s' were in comments"), PossibleMatchPositions.Num(), *FunctionName);
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After comment filtering: %d matches for function '%s'"), NonCommentMatches.Num(), *FunctionName);

    TArray<int32> NativeMatches;
    if (!QCE_CommonIOHelpers::FilterNativeFunctionPositions(FileContent, NonCommentMatches, NativeMatches))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("None of the %d non-comment matches for function '%s' have UFUNCTION macros"), NonCommentMatches.Num(), *FunctionName);
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After UFUNCTION filtering: %d matches for function '%s'"), NativeMatches.Num(), *FunctionName);

    TArray<int32> TypeMatches;
    if (!FilterPositionsByMatchingNodeParams(FileContent, NativeMatches, Function, TypeMatches))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("None of the %d UFUNCTION matches for function '%s' have matching parameter types"), NativeMatches.Num(), *FunctionName);
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After parameter type filtering: %d matches for function '%s'"), TypeMatches.Num(), *FunctionName);

    // If we found less or more than 1 match, our return wasn't successful
    if (TypeMatches.Num() != 1)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Expected exactly 1 match for function '%s', but found %d matches"), *FunctionName, TypeMatches.Num());
        return false;
    }
    
    OutFunctionPosition = TypeMatches[0];
    return true;
}


bool FFunctionCppReader::FilterPositionsByParamNum(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutParameterMatches)
{
    const FString FunctionName = Function->GetName();
    
    // Calculate expected parameter count from the function
    int32 NodeParamNum = 0;
    for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
        if (const FProperty* Property = *PropIt; Property->HasAnyPropertyFlags(CPF_Parm) && !Property->HasAnyPropertyFlags(CPF_ReturnParm))
            NodeParamNum++;
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Function '%s' expects %d parameters"), *FunctionName, NodeParamNum);
    
    // Collect parameter strings for logging when multiple matches are found
    TArray<FString> FoundParameterStrings;
    
    // Filter only positions which have same number of parameters as node
    for (int32 MatchPos : PossibleMatchPositions)
    {
        FString ParameterString;
        if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, MatchPos, ParameterString))
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Could not extract parameter string at position %d for function '%s'"), MatchPos, *FunctionName);
            continue;
        }
        
        int32 FileParamNum = QCE_ParameterMatcher::ToParameterArray(ParameterString).Num();
        
        if (FileParamNum == NodeParamNum)
        {
            OutParameterMatches.Add(MatchPos);
            FoundParameterStrings.Add(ParameterString);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter count match at position %d: %d parameters for function '%s'"), MatchPos, FileParamNum, *FunctionName);
        }
        else
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter count mismatch at position %d: expected %d, found %d for function '%s'"), MatchPos, NodeParamNum, FileParamNum, *FunctionName);
        }
    }
    
    if (OutParameterMatches.Num() == 0)
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("No parameter count matches found for function '%s'"), *FunctionName);
    }
    else if (OutParameterMatches.Num() > 1)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Found %d parameter count matches for function '%s'. All parameter strings:"), OutParameterMatches.Num(), *FunctionName);
        for (int32 i = 0; i < FoundParameterStrings.Num(); ++i)
        {
            UE_LOG(LogQuickCodeEditor, Warning, TEXT("  Match %d at position %d: '%s'"), i + 1, OutParameterMatches[i], *FoundParameterStrings[i]);
        }
    }
    
    return OutParameterMatches.Num() != 0;
}

bool FFunctionCppReader::FilterPositionsByMatchingNodeParams(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutTypeMatches)
{
    TArray<TPair<FString, bool>> ExpectedNodeParamsWithRef = QCE_CommonIOHelpers::GetExpectedParameterSignature(Function);
    TArray<FString> ExpectedNodeParams;
    for (const TPair<FString, bool>& Param : ExpectedNodeParamsWithRef)
    {
        ExpectedNodeParams.Add(Param.Key);
    }
    
    const FString FunctionName = Function->GetName();
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Function '%s' expects %d parameter types"), *FunctionName, ExpectedNodeParams.Num());
    
    TArray<FString> FoundParameterStrings;
    for (int32 MatchPos : PossibleMatchPositions)
    {
        FString FoundParameterString;
        if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, MatchPos, FoundParameterString))
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Could not extract parameter string at position %d for function '%s'"), MatchPos, *FunctionName);
            continue;
        }
        
        TArray<FString> FoundCodeParams = QCE_ParameterMatcher::ToParameterArray(FoundParameterString);
        
        if (FoundCodeParams.Num() != ExpectedNodeParams.Num())
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter count mismatch at position %d: expected %d, found %d for function '%s'"), 
                   MatchPos, ExpectedNodeParams.Num(), FoundCodeParams.Num(), *FunctionName);
            continue;
        }
        
        bool bTypesMatch = true;
        for (int32 i = 0; i < ExpectedNodeParams.Num(); ++i)
        {
            if (!QCE_ParameterMatcher::DoesFunctionParameterMatchDeclarationParameter(ExpectedNodeParams[i], FoundCodeParams[i], ExpectedNodeParamsWithRef[i].Value, true))
            {
                UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter type mismatch at position %d, param %d: expected '%s', found '%s' for function '%s'"), 
                       MatchPos, i, *ExpectedNodeParams[i], *FoundCodeParams[i], *FunctionName);
                bTypesMatch = false;
                break;
            }
        }

        if (bTypesMatch)
        {
            OutTypeMatches.Add(MatchPos);
            FoundParameterStrings.Add(FoundParameterString);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter type match at position %d for function '%s'"), MatchPos, *FunctionName);
        }
        else
        {
            // If we couldn't find a match for parameters, we fallback to trying to find a match without checking constness of params
            bTypesMatch = true;
            for (int32 i = 0; i < ExpectedNodeParams.Num(); ++i)
            {
                if (!QCE_ParameterMatcher::DoesFunctionParameterMatchDeclarationParameter(ExpectedNodeParams[i], FoundCodeParams[i], ExpectedNodeParamsWithRef[i].Value, false))
                {
                    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("[NoConst] Parameter type mismatch at position %d, param %d: expected '%s', found '%s' for function '%s'"), 
                           MatchPos, i, *ExpectedNodeParams[i], *FoundCodeParams[i], *FunctionName);
                    bTypesMatch = false;
                    break;
                }
            }
            if (bTypesMatch)
            {
                OutTypeMatches.Add(MatchPos);
                FoundParameterStrings.Add(FoundParameterString);
                UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter type match at position %d for function '%s'"), MatchPos, *FunctionName);
            }
        }
        
        
        
       
    }

    // logging
    if (OutTypeMatches.Num() == 0)
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("No parameter type matches found for function '%s'"), *FunctionName);
    }
    else if (OutTypeMatches.Num() > 1)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Found %d parameter type matches for function '%s'. All parameter strings:"), OutTypeMatches.Num(), *FunctionName);
        for (int32 i = 0; i < FoundParameterStrings.Num(); ++i)
        {
            UE_LOG(LogQuickCodeEditor, Warning, TEXT("  Match %d at position %d for function '%s': '%s'"), i + 1, OutTypeMatches[i], *FunctionName, *FoundParameterStrings[i]);
        }
    }
    
    return OutTypeMatches.Num() == 1;
}


bool FFunctionCppReader::ReadCppFileContent(const UFunction* Function, FString& FileContent, FString& OutFilePath, QCE_CppFileType FileType)
{
    if (!Function || !Function->GetOwnerClass())
        return false;
    
    // Validate function flags before proceeding with search
    if (!QCE_CommonIOHelpers::ValidateFunctionFlags(Function))
        return false;
    
    if (FileType == QCE_CppFileType::Header)
    {
        if (!FSourceCodeNavigation::FindClassHeaderPath(Function->GetOwnerClass(), OutFilePath))
        {
            UE_LOG(LogQuickCodeEditor, Error, TEXT("Could not load header file for %s"),
                   *Function->GetOwnerClass()->GetName());
            return false;
        }
    }
    else if (FileType == QCE_CppFileType::Implementation)
    {
        if (!FSourceCodeNavigation::FindClassSourcePath(Function->GetOwnerClass(), OutFilePath))
        {
            UE_LOG(LogQuickCodeEditor, Error, TEXT("Could not load source file for %s"), *Function->GetOwnerClass()->GetName());
            return false;
        } 
    }
    else
    {
        return false;
    }
   

    // Read the header file content
    if (!FFileHelper::LoadFileToString(FileContent, *OutFilePath))
        return false;

    return true;
}



bool FFunctionCppReader::HasFunctionDeclarationChangedOnDisk(const UFunction* Function,
    const FFunctionDeclarationInfo& CurrentDeclarationInfo)
{
    const FString HeaderPath = CurrentDeclarationInfo.HeaderPath;
    if (!Function || HeaderPath.IsEmpty())
    {
        return false;
    }

    FFunctionDeclarationInfo DeclarationInfo;
    if (!GetFunctionDeclaration(Function, DeclarationInfo, false))
    {
        return false;
    }

    // First check if the provided header path matches our last loaded file
    if (HeaderPath != DeclarationInfo.HeaderPath)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Header path mismatch: Expected '%s', got '%s'"), *DeclarationInfo.HeaderPath, *HeaderPath);
        return true; // Consider it changed if paths don't match
    }

    // Read current file content from disk
    FString DiskContent;
    if (!FFileHelper::LoadFileToString(DiskContent, *HeaderPath))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to read header file '%s' from disk"), *HeaderPath);
        return false; // If we can't read the file, assume no change to avoid false positives
    }

    // Calculate and compare checksums
    const uint32 DiskChecksum = FCrc::StrCrc32(*DiskContent);
    const bool bHasChanged = DiskChecksum != DeclarationInfo.ContentChecksum;

    if (bHasChanged)
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Declaration file '%s' has changed on disk. Old checksum: %u, New checksum: %u"), 
               *HeaderPath, DeclarationInfo.ContentChecksum, DiskChecksum);
    }

    return bHasChanged;
}

bool FFunctionCppReader::HasFunctionImplementationChangedOnDisk(const UFunction* Function, const FFunctionImplementationInfo& CurrentImplementationInfo)
{
    const FString CppPath = CurrentImplementationInfo.CppPath;
    if (!Function || CppPath.IsEmpty())
    {
        return false;
    }

    // First check if the provided cpp path matches our last loaded file
    if (CppPath != CurrentImplementationInfo.CppPath)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Implementation path mismatch: Expected '%s', got '%s'"), *CurrentImplementationInfo.CppPath, *CppPath);
        return true; // Consider it changed if paths don't match
    }

    // Read current file content from disk
    FString DiskContent;
    if (!FFileHelper::LoadFileToString(DiskContent, *CppPath))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to read implementation file '%s' from disk"), *CppPath);
        return false; // If we can't read the file, assume no change to avoid false positives
    }

    // Calculate and compare checksums
    const uint32 DiskChecksum = FCrc::StrCrc32(*DiskContent);
    const bool bHasChanged = DiskChecksum != CurrentImplementationInfo.ContentChecksum;

    if (bHasChanged)
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Implementation file '%s' has changed on disk. Old checksum: %u, New checksum: %u"), 
               *CppPath, CurrentImplementationInfo.ContentChecksum, DiskChecksum);
    }

    return bHasChanged;
}
