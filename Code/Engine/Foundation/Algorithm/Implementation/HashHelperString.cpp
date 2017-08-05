#include <PCH.h>
#include <Foundation/Algorithm/HashHelperString.h>

//static
ezUInt32 ezHashHelperString_NoCase::Hash(const char* szValue)
{
  ezHybridArray<char, 256> temp;
  ezUInt32 uiElemCount = (ezUInt32)strlen(szValue);
  temp.SetCountUninitialized(uiElemCount);
  ezMemoryUtils::Copy(temp.GetData(), szValue, uiElemCount);
  uiElemCount = ezStringUtils::ToLowerString(temp.GetData(), temp.GetData() + uiElemCount);
  return ezHashing::MurmurHash((void*)temp.GetData(), uiElemCount);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Algorithm_Implementation_HashHelperString);
