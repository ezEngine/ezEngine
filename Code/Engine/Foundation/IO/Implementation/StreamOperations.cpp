#include <PCH.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

ezStreamWriter& operator << (ezStreamWriter& Stream, const char* szValue)
{
  ezUInt32 uiLength = 0;

  if (szValue != nullptr)
    uiLength = static_cast<ezUInt32>(strlen(szValue));

  Stream << uiLength;

  if (uiLength > 0)
    Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(szValue), uiLength);

  return Stream;
}

// ezStringBuilder

ezStreamWriter& operator << (ezStreamWriter& Stream, const ezStringBuilder& sValue)
{
  return Stream << sValue.GetData();
}

ezStreamReader& operator >> (ezStreamReader& Stream, ezStringBuilder& sValue)
{
  ezUInt32 uiLength = 0;

  Stream >> uiLength;

  if (uiLength > 0)
  {
    ezHybridArray<ezUInt8, 256> sTemp;
    sTemp.SetCountUninitialized(uiLength + 1);

    Stream.ReadBytes(&sTemp[0], uiLength);
    sTemp[uiLength] = '\0';

    sValue = reinterpret_cast<const char*>(&sTemp[0]);
  }
  else
    sValue.Clear();

  return Stream;
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperations);

