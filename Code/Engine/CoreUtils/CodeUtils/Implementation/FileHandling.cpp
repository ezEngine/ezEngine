#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

void ezPreprocessor::SetLogInterface(ezLogInterface* pLog)
{
  m_pLog = pLog;
}

void ezPreprocessor::SetFileCallbacks(FileOpenCB OpenAbsFileCB, FileLocatorCB LocateAbsFileCB)
{
  m_FileOpenCallback = OpenAbsFileCB;
  m_FileLocatorCallback = LocateAbsFileCB;
}

ezResult ezPreprocessor::OpenFile(const char* szFile, ezTokenizer** pTokenizer)
{
  EZ_ASSERT(m_FileOpenCallback != nullptr, "OpenFile callback has not been set");
  EZ_ASSERT(m_FileLocatorCallback != nullptr, "File locator callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_FileCache.Find(szFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value();
    return EZ_SUCCESS;
  }

  ezDynamicArray<ezUInt8> Content;
  if (m_FileOpenCallback(szFile, Content).Failed())
  {
    ezLog::Error(m_pLog, "Could not open file '%s'", szFile);
    return EZ_FAILURE;
  }

  *pTokenizer = &m_FileCache[szFile];
  (*pTokenizer)->Tokenize(Content, ezGlobalLog::GetInstance());

  return EZ_SUCCESS;
}


ezResult ezPreprocessor::HandleInclude(const TokenStream& Tokens, ezUInt32 uiCurToken, ezUInt32 uiDirectiveToken, TokenStream& TokenOutput)
{
  EZ_ASSERT(m_FileLocatorCallback != nullptr, "File locator callback has not been set");

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

    CombineTokensToString(PathTokens, 0, sPath);

    // remove all whitespace at the end (this could be part of a comment, so not tokenized as whitespace)
    while (sPath.EndsWith(" ") || sPath.EndsWith("\t"))
      sPath.Shrink(0, 1);

    // there must always be a > at the end, although it could be a separate token or part of a comment
    // so we check the string, instead of the tokens
    if (sPath.EndsWith(">"))
      sPath.Shrink(0, 1);
    else
    {
      PP_LOG(Error, "Invalid include path '%s'", Tokens[uiAccepted], sPath.GetData());
      return EZ_FAILURE;
    }
  }

  if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
  {
    PP_LOG0(Error, "Expected end-of-line", Tokens[uiCurToken]);
    return EZ_FAILURE;
  }

  EZ_ASSERT(!m_sCurrentFileStack.IsEmpty(), "Implementation error.");

  ezString sOtherFile;

  if (m_FileLocatorCallback(m_sCurrentFileStack.PeekBack().m_sFileName.GetData(), sPath.GetData(), IncType, sOtherFile).Failed())
  {
    PP_LOG(Error, "#include file '%s' could not be located", Tokens[uiAccepted], sPath.GetData());
    return EZ_FAILURE;
  }

  // if this has been included before, and contains a #pragma once, do not include it again
  if (m_PragmaOnce.Find(sPath.GetData()).IsValid())
    return EZ_SUCCESS;

  if (ProcessFile(sPath.GetData(), TokenOutput).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

