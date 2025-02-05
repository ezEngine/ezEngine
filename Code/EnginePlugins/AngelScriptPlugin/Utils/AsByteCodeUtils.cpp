#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>

class ezAsWriteStream : public asIBinaryStream
{
public:
  int Write(const void* pPtr, asUINT size)
  {
    m_pBuffer->PushBackRange(ezConstByteArrayPtr((const ezUInt8*)pPtr, size));
    return size;
  }

  int Read(void* pPtr, asUINT size)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }

  ezDynamicArray<ezUInt8>* m_pBuffer = nullptr;
};

class ezAsReadStream : public asIBinaryStream
{
public:
  int Write(const void* pPtr, asUINT size)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }

  int Read(void* pPtr, asUINT size)
  {
    const ezUInt32 uiReadSize = ezMath::Min(size, m_Buffer.GetCount() - m_uiReadPos);

    ezMemoryUtils::RawByteCopy(pPtr, m_Buffer.GetPtr() + m_uiReadPos, uiReadSize);

    m_uiReadPos += uiReadSize;
    return uiReadSize;
  }

  ezUInt32 m_uiReadPos = 0;
  ezArrayPtr<ezUInt8> m_Buffer;
};

void ezAngelScriptUtils::SaveByteCode(asIScriptModule* pModule, ezDynamicArray<ezUInt8>& out_byteCode)
{
  ezAsWriteStream stream;
  stream.m_pBuffer = &out_byteCode;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pModule->SaveByteCode(&stream, false); // TODO AS: strip debug info ? (flag?)
#else
  pModule->SaveByteCode(&stream, true);
#endif
}

asIScriptModule* ezAngelScriptUtils::LoadFromByteCode(asIScriptEngine* pEngine, ezStringView sModuleName, ezArrayPtr<ezUInt8> byteCode)
{
  ezAsReadStream stream;
  stream.m_Buffer = byteCode;

  ezStringBuilder tmp;
  asIScriptModule* pModule = pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (pModule->LoadByteCode(&stream) < 0)
  {
    return nullptr;
  }

  return pModule;
}
