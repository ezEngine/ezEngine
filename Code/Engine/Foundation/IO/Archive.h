#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/IO/MemoryStream.h>


class ezArchiveSerializer
{
public:

  virtual void Serialize(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pReference) const
  {
  }

private:

};

class EZ_FOUNDATION_DLL ezArchiveWriter : public ezChunkStreamWriter
{
public:
  ezArchiveWriter(ezStreamWriterBase& stream);

  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override;

  void WriteReflectedObject(const ezReflectedClass* pReflected);

  void BeginTypedObject(const ezRTTI* pRtti, const void* pReference);

  void EndTypedObject();

  void WriteObjectReference(const void* pReference);

  virtual void EndStream() override;

  void RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer);

  //virtual void BeginChunk(const char* szName, ezUInt32 uiVersion) override;

  //virtual void EndChunk() override;

private:

  ezArchiveSerializer* GetTypeSerializer(const ezRTTI* pRttiBase);

  void StoreType(const ezRTTI* pRtti);

  struct TypeInfo
  {
    ezUInt16 m_uiTypeID;
    ezUInt32 m_uiObjects;
  };

  ezMap<const ezRTTI*, ezArchiveSerializer*> m_TypeSerializers;

  ezMap<const ezRTTI*, TypeInfo> m_WrittenTypes;
  ezDeque<const ezRTTI*> m_RttiToWrite;

  ezStreamWriterBase& m_OutputStream;

  struct Temp
  {
    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamWriter m_Writer;

    Temp() : m_Writer(&m_Storage) { }
  };

  ezDeque<Temp> m_Temp;

  ezMemoryStreamStorage m_StorageTemp;
  ezMemoryStreamWriter m_WriterTemp;

  ezMap<const void*, ezUInt32> m_ObjectReferences;
};


class EZ_FOUNDATION_DLL ezArchiveReader : public ezChunkStreamReader
{
public:
  ezArchiveReader(ezStreamReaderBase& stream);

  //virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override;

  virtual void BeginStream() override;

  virtual void EndStream() override;

  struct RttiData
  {
    ezUInt16 m_uiTypeID;
    ezString m_sTypeName;
    ezUInt32 m_uiTypeVersion;
    ezUInt32 m_uiObjectCount;
    const ezRTTI* m_pRTTI;
  };

  const ezDynamicArray<RttiData>& GetArchiveRttiData() const { return m_Types; }

private:

  ezDynamicArray<RttiData> m_Types;
  ezStreamReaderBase& m_InputStream;
};