#include <Foundation/PCH.h>
#include <Foundation/Memory/EndianHelper.h>

void ezEndianHelper::SwitchStruct(void* pDataPointer, const char* szFormat)
{
  EZ_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  EZ_ASSERT_DEBUG(szFormat != nullptr && strlen(szFormat) > 0, "Struct format description necessary!");

  ezUInt8* pWorkPointer = static_cast<ezUInt8*>(pDataPointer);
  char cCurrentElement = *szFormat;

  while (cCurrentElement != '\0')
  {
    switch (cCurrentElement)
    {
      case 'c':
      case 'b':
        pWorkPointer++;
        break;

      case 's':
      case 'w':
        {
        ezUInt16* pWordElement = reinterpret_cast<ezUInt16*>(pWorkPointer);
        *pWordElement = Switch(*pWordElement);
        pWorkPointer += sizeof(ezUInt16);
        }
        break;

      case 'd':
        {
        ezUInt32* pDWordElement = reinterpret_cast<ezUInt32*>(pWorkPointer);
        *pDWordElement = Switch(*pDWordElement);
        pWorkPointer += sizeof(ezUInt32);
        }
        break;

      case 'q':
        {
        ezUInt64* pQWordElement = reinterpret_cast<ezUInt64*>(pWorkPointer);
        *pQWordElement = Switch(*pQWordElement);
        pWorkPointer += sizeof(ezUInt64);
        }
        break;

    }

    szFormat++;
    cCurrentElement = *szFormat;
  }
}

void ezEndianHelper::SwitchStructs(void* pDataPointer, const char* szFormat, ezUInt32 uiStride, ezUInt32 uiCount)
{
  EZ_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  EZ_ASSERT_DEBUG(szFormat != nullptr && strlen(szFormat) > 0, "Struct format description necessary!");
  EZ_ASSERT_DEBUG(uiStride > 0, "Struct size necessary!");

  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    SwitchStruct(pDataPointer, szFormat);
    pDataPointer = ezMemoryUtils::AddByteOffset(pDataPointer, uiStride);
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_EndianHelper);

