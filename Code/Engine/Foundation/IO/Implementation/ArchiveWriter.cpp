#include <Foundation/PCH.h>
#include <Foundation/IO/Archive.h>

ezArchiveWriter::ezArchiveWriter(ezStreamWriterBase& stream) : ezChunkStreamWriter(m_WriterTemp),
  m_OutputStream(stream),
  m_WriterTemp(&m_StorageTemp)
{
}

ezResult ezArchiveWriter::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  if (!m_Temp.IsEmpty())
  {
    return m_Temp.PeekBack().m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
  }

  return ezChunkStreamWriter::WriteBytes(pWriteBuffer, uiBytesToWrite);
}

void ezArchiveWriter::RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer)
{
  m_TypeSerializers[pRttiBase] = pSerializer;
}

ezArchiveSerializer* ezArchiveWriter::GetTypeSerializer(const ezRTTI* pRttiBase)
{
  bool bExisted = false;
  auto it = m_TypeSerializers.FindOrAdd(pRttiBase, &bExisted); // this will insert nullptr if not found

  if (bExisted)
    return it.Value();

  // if we have a base type use that serializer, if not, nullptr will stay in there
  if (pRttiBase->GetParentType() != nullptr)
  {
    it.Value() = GetTypeSerializer(pRttiBase->GetParentType());
  }

  return it.Value();
}

void ezArchiveWriter::AddObjectWrittenState(const void* pReference)
{
  m_WrittenObjects.Insert(pReference);
}

bool ezArchiveWriter::HasObjectBeenWritten(const void* pReference) const
{
  return m_WrittenObjects.Find(pReference).IsValid();
}

void ezArchiveWriter::ResetObjectWrittenState()
{
  m_WrittenObjects.Clear();
}

void ezArchiveWriter::StoreType(const ezRTTI* pRtti)
{
  bool bExisted = false;

  auto it = m_WrittenTypes.FindOrAdd(pRtti, &bExisted);

  it.Value().m_uiObjects++;

  if (bExisted)
    return;

  const ezUInt16 uiTypeID = m_WrittenTypes.GetCount(); // can never be 0

  it.Value().m_uiTypeID = uiTypeID; // store a unique type ID

  // store type information of all parents
  if (pRtti->GetParentType())
    StoreType(pRtti->GetParentType());

  m_RttiToWrite.PushBack(pRtti);
}

void ezArchiveWriter::WriteObjectReference(const void* pReference)
{
  bool bExisted = false;
  auto it = m_ObjectReferences.FindOrAdd(pReference, &bExisted);

  if (!bExisted)
    it.Value() = m_ObjectReferences.GetCount();

  // write the reference ID
  WriteBytes(&it.Value(), sizeof(ezUInt32));
}

void ezArchiveWriter::WriteReflectedObject(const ezReflectedClass* pReflected)
{
  BeginTypedObject(pReflected->GetDynamicRTTI(), pReflected);

  pReflected->Serialize(*this);

  EndTypedObject();
}

void ezArchiveWriter::BeginTypedObject(const ezRTTI* pRtti, const void* pReference)
{
  // check whether the type data is already stored
  StoreType(pRtti);

  // store that we have written this object
  AddObjectWrittenState(pReference);

  // write the object's type ID
  const ezUInt16 uiTypeID = m_WrittenTypes[pRtti].m_uiTypeID;
  WriteBytes(&uiTypeID, sizeof(ezUInt16));

  WriteObjectReference(pReference);

  m_Temp.PushBack();

  ezArchiveSerializer* pSerializer = GetTypeSerializer(pRtti);

  if (pSerializer != nullptr)
  {
    pSerializer->Serialize(m_Temp.PeekBack().m_Writer, pRtti, pReference);
  }
}

void ezArchiveWriter::EndTypedObject()
{
  ezMemoryStreamStorage& storage = m_Temp.PeekBack().m_Storage;
  const ezUInt32 uiObjectSize = storage.GetStorageSize();

  if (m_Temp.GetCount() > 1)
  {
    ezMemoryStreamWriter& writer = m_Temp[m_Temp.GetCount() - 2].m_Writer;

    // write the size of the object
    writer << uiObjectSize;

    // write the temp data
    writer.WriteBytes(storage.GetData(), uiObjectSize);
  }
  else
  {
    ezChunkStreamWriter::WriteBytes(&uiObjectSize, sizeof(ezUInt32));
    ezChunkStreamWriter::WriteBytes(storage.GetData(), uiObjectSize);
  }

  m_Temp.PopBack();
}

void ezArchiveWriter::EndStream()
{
  EZ_ASSERT(m_Temp.GetCount() == 0, "BeginTypedObject / EndTypedObject has not been called in tandem (%i)", m_Temp.GetCount());

  ezChunkStreamWriter::EndStream();

  const ezUInt8 uiVersion = ezArchiveVersion::CurrentVersion;

  m_OutputStream << "ezArchive";
  m_OutputStream << uiVersion;

  const ezUInt32 uiMaxReferences = m_ObjectReferences.GetCount();
  m_OutputStream << uiMaxReferences;

  // write all RTTI information
  {
    const ezUInt32 uiRttiCount = m_RttiToWrite.GetCount();
    m_OutputStream << uiRttiCount;

    for (ezUInt32 i = 0; i < m_RttiToWrite.GetCount(); ++i)
    {
      m_OutputStream << m_WrittenTypes[m_RttiToWrite[i]].m_uiTypeID;
      m_OutputStream << m_RttiToWrite[i]->GetTypeName();
      m_OutputStream << m_RttiToWrite[i]->GetTypeVersion();
      m_OutputStream << m_WrittenTypes[m_RttiToWrite[i]].m_uiObjects;
    }
  }

  // write all data
  {
    m_OutputStream << m_StorageTemp.GetStorageSize();

    m_OutputStream.WriteBytes(m_StorageTemp.GetData(), m_StorageTemp.GetStorageSize());
  }
}
