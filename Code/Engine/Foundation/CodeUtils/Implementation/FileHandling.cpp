#include <PCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace ezTokenParseUtils;

ezMap<ezString, ezTokenizedFileCache::FileData>::ConstIterator ezTokenizedFileCache::Lookup(const ezString& sFileName) const
{
  EZ_LOCK(m_Mutex);
  auto it = m_Cache.Find(sFileName);
  return it;
}

void ezTokenizedFileCache::Remove(const ezString& sFileName)
{
  EZ_LOCK(m_Mutex);
  m_Cache.Remove(sFileName);
}

void ezTokenizedFileCache::Clear()
{
  EZ_LOCK(m_Mutex);
  m_Cache.Clear();
}

void ezTokenizedFileCache::SkipWhitespace(ezDeque<ezToken>& Tokens, ezUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() &&
         (Tokens[uiCurToken].m_iType == ezTokenType::BlockComment ||
          Tokens[uiCurToken].m_iType == ezTokenType::LineComment ||
          Tokens[uiCurToken].m_iType == ezTokenType::Newline ||
          Tokens[uiCurToken].m_iType == ezTokenType::Whitespace))
    ++uiCurToken;
}

const ezTokenizer* ezTokenizedFileCache::Tokenize(const ezString& sFileName, ezArrayPtr<const ezUInt8> FileContent, const ezTimestamp& FileTimeStamp, ezLogInterface* pLog)
{
  EZ_LOCK(m_Mutex);

  auto& data = m_Cache[sFileName];

  data.m_Timestamp = FileTimeStamp;
  ezTokenizer* pTokenizer = &data.m_Tokens;
  pTokenizer->Tokenize(FileContent, pLog);

  ezDeque<ezToken>& Tokens = pTokenizer->GetTokens();

  ezHashedString sFile;
  sFile.Assign(sFileName.GetData());

  ezInt32 iLineOffset = 0;

  for (ezUInt32 i = 0; i + 1 < Tokens.GetCount(); ++i)
  {
    const ezUInt32 uiCurLine = Tokens[i].m_uiLine;

    Tokens[i].m_File = sFile;
    Tokens[i].m_uiLine += iLineOffset;

    if (Tokens[i].m_iType == ezTokenType::NonIdentifier &&
        ezString(Tokens[i].m_DataView) == "#")
    {
      ezUInt32 uiNext = i + 1;

      SkipWhitespace(Tokens, uiNext);

      if (uiNext < Tokens.GetCount() &&
          Tokens[uiNext].m_iType == ezTokenType::Identifier &&
          ezString(Tokens[uiNext].m_DataView) == "line")
      {
        ++uiNext;
        SkipWhitespace(Tokens, uiNext);

        if (uiNext < Tokens.GetCount() &&
            Tokens[uiNext].m_iType == ezTokenType::Identifier)
        {
          ezInt32 iNextLine = 0;

          const ezString sNumber = Tokens[uiNext].m_DataView;
          if (ezConversionUtils::StringToInt(sNumber, iNextLine).Succeeded())
          {
            iLineOffset = (iNextLine - uiCurLine) - 1;

            ++uiNext;
            SkipWhitespace(Tokens, uiNext);

            if (uiNext < Tokens.GetCount())
            {
              if (Tokens[uiNext].m_iType == ezTokenType::String1)
              {
                ezStringBuilder sFileName2 = Tokens[uiNext].m_DataView;
                sFileName2.Shrink(1, 1); // remove surrounding "

                sFile.Assign(sFileName2.GetData());
              }
            }
          }
        }
      }
    }
  }

  return pTokenizer;
}


void ezPreprocessor::SetLogInterface(ezLogInterface* pLog)
{
  m_pLog = pLog;
}

void ezPreprocessor::SetFileOpenFunction(FileOpenCB OpenAbsFileCB)
{
  m_FileOpenCallback = OpenAbsFileCB;
}

void ezPreprocessor::SetFileLocatorFunction(FileLocatorCB LocateAbsFileCB)
{
  m_FileLocatorCallback = LocateAbsFileCB;
}

ezResult ezPreprocessor::DefaultFileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType IncType, ezStringBuilder& out_sAbsoluteFilePath)
{
  ezStringBuilder& s = out_sAbsoluteFilePath;

  if (IncType == ezPreprocessor::RelativeInclude)
  {
    s = szCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(szIncludeFile);
    s.MakeCleanPath();
  }
  else
  {
    s = szIncludeFile;
    s.MakeCleanPath();
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::DefaultFileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)
{
  ezFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return EZ_FAILURE;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  ezFileStats stats;
  if (ezOSFile::GetFileStats(r.GetFilePathAbsolute(), stats).Succeeded())
    out_FileModification = stats.m_LastModificationTime;
#endif

  ezUInt8 Temp[4096];

  while (ezUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));
  }

  return EZ_SUCCESS;
}

ezResult ezPreprocessor::OpenFile(const char* szFile, const ezTokenizer** pTokenizer)
{
  EZ_ASSERT_DEV(m_FileOpenCallback.IsValid(), "OpenFile callback has not been set");
  EZ_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_pUsedFileCache->Lookup(szFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value().m_Tokens;
    return EZ_SUCCESS;
  }

  ezTimestamp stamp;

  ezDynamicArray<ezUInt8> Content;
  if (m_FileOpenCallback(szFile, Content, stamp).Failed())
  {
    ezLog::Error(m_pLog, "Could not open file '{0}'", szFile);
    return EZ_FAILURE;
  }

  ezArrayPtr<const ezUInt8> ContentView = Content;

  // the file open callback gives us raw data for the opened file
  // the tokenizer doesn't like the Utf8 BOM, so skip it here, if we detect it
  if (ContentView.GetCount() >= 3) // length of a BOM
  {
    const char* dataStart = reinterpret_cast<const char*>(ContentView.GetPtr());

    if (ezUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      ContentView = ezArrayPtr<const ezUInt8>((const ezUInt8*)dataStart, Content.GetCount() - 3);
    }
  }

  *pTokenizer = m_pUsedFileCache->Tokenize(szFile, ContentView, stamp, m_pLog);

  return EZ_SUCCESS;
}


ezResult ezPreprocessor::HandleInclude(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, TokenStream& TokenOutput)
{
  EZ_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  SkipWhitespace(Tokens, uiCurToken);

  ezStringBuilder sPath;

  IncludeType IncType = IncludeType::GlobalInclude;

  ezUInt32 uiAccepted;
  if (Accept(Tokens, uiCurToken, ezTokenType::String1, &uiAccepted))
  {
    IncType = IncludeType::RelativeInclude;
    sPath = Tokens[uiAccepted]->m_DataView;
    sPath.Shrink(1, 1); // remove " at start and end
  }
  else
  {
    // in global include paths (ie. <bla/blub.h>) we need to handle line comments special
    // because a path with two slashes will be a comment token, although it could be a valid path
    // so we concatenate just everything and then make sure it ends with a >

    if (Expect(Tokens, uiCurToken, "<", &uiAccepted).Failed())
      return EZ_FAILURE;

    TokenStream PathTokens;

    while (uiCurToken < Tokens.GetCount())
    {
      if (Tokens[uiCurToken]->m_iType == ezTokenType::Newline)
      {
        ++uiCurToken;
        continue;
      }

      PathTokens.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
    }

    CombineTokensToString(PathTokens, 0, sPath, false);

    // remove all whitespace at the end (this could be part of a comment, so not tokenized as whitespace)
    while (sPath.EndsWith(" ") || sPath.EndsWith("\t"))
      sPath.Shrink(0, 1);

    // there must always be a > at the end, although it could be a separate token or part of a comment
    // so we check the string, instead of the tokens
    if (sPath.EndsWith(">"))
      sPath.Shrink(0, 1);
    else
    {
      PP_LOG(Error, "Invalid include path '{0}'", Tokens[uiAccepted], sPath);
      return EZ_FAILURE;
    }
  }

  if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
  {
    PP_LOG0(Error, "Expected end-of-line", Tokens[uiCurToken]);
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(!m_sCurrentFileStack.IsEmpty(), "Implementation error.");

  ezStringBuilder sOtherFile;

  if (m_FileLocatorCallback(m_sCurrentFileStack.PeekBack().m_sFileName.GetData(), sPath, IncType, sOtherFile).Failed())
  {
    PP_LOG(Error, "#include file '{0}' could not be located", Tokens[uiAccepted], sPath);
    return EZ_FAILURE;
  }

  const ezTempHashedString sOtherFileHashed(sOtherFile.GetData());

  // if this has been included before, and contains a #pragma once, do not include it again
  if (m_PragmaOnce.Find(sOtherFileHashed).IsValid())
    return EZ_SUCCESS;

  if (ProcessFile(sOtherFile, TokenOutput).Failed())
    return EZ_FAILURE;

  if (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken]->m_iType == ezTokenType::Newline || Tokens[uiCurToken]->m_iType == ezTokenType::EndOfFile))
    TokenOutput.PushBack(Tokens[uiCurToken]);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_FileHandling);
