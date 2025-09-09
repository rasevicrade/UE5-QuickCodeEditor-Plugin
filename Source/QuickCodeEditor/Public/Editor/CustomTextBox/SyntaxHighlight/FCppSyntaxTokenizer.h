// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Framework/Text/SyntaxTokenizer.h"

/**
 * Tokenize text between syntax and literals for C++ syntax.
 */
class FCppSyntaxTokenizer : public ISyntaxTokenizer
{
public:
	static TSharedRef<FCppSyntaxTokenizer> Create();

	virtual void Process(TArray<FTokenizedLine>& OutTokenizedLines, const FString& Input) override;

	FORCEINLINE bool IsUnrealTypeDef(const FString& InToken) const
	{
		return UnrealTypedefs.Contains(InToken);
	}

	FORCEINLINE bool IsKeyword(const FString& InToken) const
	{
		return Keywords.Contains(InToken);
	}

	FORCEINLINE bool IsOperator(const FString& InToken) const
	{
		return Operators.Contains(InToken);
	}
private:
	FCppSyntaxTokenizer();

	void TokenizeLineRanges(const FString& Input, const TArray<FTextRange>& LineRanges, TArray<FTokenizedLine>& OutTokenizedLines);

	static FORCEINLINE bool IsAlpha(TCHAR Char)
	{
		return (Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z');
	}

	static FORCEINLINE bool IsDigit(TCHAR Char)
	{
		return Char >= '0' && Char <= '9';
	}

	static FORCEINLINE bool IsAlphaOrDigit(TCHAR Char)
	{
		return IsAlpha(Char) || IsDigit(Char);
	}

	static FORCEINLINE bool IsIdentifierChar(TCHAR Char)
	{
		return IsAlphaOrDigit(Char) || Char == TEXT('_');
	}

	/** Collection of C++ operators and symbols */
	TSet<FString> Operators;

	/** Collection of C++ keywords and reserved words */
	TSet<FString> Keywords;

	/** Collection of Unreal Engine type definitions */
	TSet<FString> UnrealTypedefs;

	/** State tracking for multiline comments across tokenization */
	bool bInMultilineComment = false;
};