#include <FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

ezStreamWriter& operator<<(ezStreamWriter& Stream, const char* szValue)
{
  ezStringView szView(szValue);
  Stream.WriteString(szView);

  return Stream;
}

// ezStringBuilder

ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezStringBuilder& sValue)
{
  Stream.WriteString(sValue.GetView());
  return Stream;
}

ezStreamReader& operator>>(ezStreamReader& Stream, ezStringBuilder& sValue)
{
  Stream.ReadString(sValue);
  return Stream;
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperations);

