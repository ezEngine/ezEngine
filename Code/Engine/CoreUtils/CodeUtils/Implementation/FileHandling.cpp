#include <CoreUtils/PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

void ezPreprocessor::SetLogInterface(ezLogInterface* pLog)
{
  m_pLog = pLog;
}

void ezPreprocessor::SetFileOpenCallback(FileOpenCB cb)
{
  m_FileOpenCallback = cb;
}

ezResult ezPreprocessor::OpenFile(const char* szFile, ezTokenizer** pTokenizer)
{
  EZ_ASSERT(m_FileOpenCallback != NULL, "OpenFile callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_FileCache.Find(szFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value();
    return EZ_SUCCESS;
  }

  ezDynamicArray<ezUInt8> Content;
  if (m_FileOpenCallback(szFile, Content).Failed())
    return EZ_FAILURE;

  *pTokenizer = &m_FileCache[szFile];
  (*pTokenizer)->Tokenize(Content, ezGlobalLog::GetInstance());

  return EZ_SUCCESS;
}
