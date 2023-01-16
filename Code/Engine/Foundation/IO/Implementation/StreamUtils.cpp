#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void ezStreamUtils::ReadAllAndAppend(ezStreamReader& stream, ezDynamicArray<ezUInt8>& destination)
{
  ezUInt8 temp[1024 * 4];

  while (true)
  {
    const ezUInt32 uiRead = (ezUInt32)stream.ReadBytes(temp, EZ_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    destination.PushBackRange(ezArrayPtr<ezUInt8>(temp, uiRead));
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamUtils);
