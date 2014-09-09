#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

/// \todo document, test

class ezArchiveSerializer
{
public:

  virtual void Serialize(ezStreamWriterBase& stream, const ezRTTI* pRtti, const void* pReference)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  virtual void* Deserialize(ezStreamReaderBase& stream, const ezRTTI* pRtti)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

private:

};

class EZ_FOUNDATION_DLL ezArchiveWriter : public ezChunkStreamWriter
{
public:
  ezArchiveWriter(ezStreamWriterBase& stream);

  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override;

  /// \brief Serializes a reflected object to the archive.
  ///
  /// This function internally calls BeginTypedObject() and EndTypedObject(). It also calls ezReflectedClass::Serialize on the given object.
  void WriteReflectedObject(const ezReflectedClass* pReflected);

  /// \brief Call this function when you want to write an object that has only static reflection information and that needs to be dynamically
  /// created at deserialization.
  ///
  /// This allows to serialize classes with their type information, such that at deserialization time the loader knows which type of object
  /// to create. If an object is derived from ezReflectedClass, you can use WriteReflectedObject() instead.
  ///
  /// If an object has reflection information, but at deserialization time it is clear which type of object will be loaded, it is not necessary
  /// to use this. For example an ezVec3 can be stored without any RTTI data.
  ///
  /// \note You may write objects nested. If you do and the deserialization is unable to create a parent object, the entire sub-tree
  ///       of nested objects will be skipped, even when the nested objects could be deserialized.
  void BeginTypedObject(const ezRTTI* pRtti, const void* pReference);

  /// \brief This needs to be called in conjunction with BeginTypedObject(), once the serialization of an object is finished.
  void EndTypedObject();

  /// \brief Writes a reference to the given object to the stream. Upon deserialization these references will be patched to
  /// the then existing object.
  ///
  /// You can store ANY pointer as a reference, even to objects that are not written through BeginTypedObject() and these references
  /// can also be restored, as long as you manually make sure that the reference information is passed to ezArchiveReader properly.
  void WriteObjectReference(const void* pReference);

  /// \brief Flags the given object as already written, which can be queried using HasObjectBeenWritten().
  ///
  /// This is not necessary to call manually, unless you want to do custom serialization logic for which this might be useful.
  void AddObjectWrittenState(const void* pReference);

  /// \brief Returns whether the given object reference has already been written before. This includes all objects written through
  /// BeginTypedObject() and WriteReflectedObject().
  ///
  /// This can be used to check whether some object has already been properly stored and whether it would be sufficient to
  /// only write a reference.
  ///
  /// \sa ResetObjectWrittenState()
  bool HasObjectBeenWritten(const void* pReference) const;

  /// \brief Resets the knowledge about which objects have been written already.
  ///
  /// This can be used to make sure that objects are written again to the same archive, which might be necessary when
  /// different chunks are written and should be independent from each other, so that a chunk can be skipped, but the next chunk
  /// duplicates all necessary data.
  void ResetObjectWrittenState();

  virtual void EndStream() override;

  void RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer);

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

  ezSet<const void*> m_WrittenObjects;
  ezMap<const void*, ezUInt32> m_ObjectReferences;
};


class EZ_FOUNDATION_DLL ezArchiveReader : public ezChunkStreamReader
{
public:
  ezArchiveReader(ezStreamReaderBase& stream);
  ~ezArchiveReader();

  void SetLogInterface(ezLogInterface* pLog) { m_pLog = pLog; }

  virtual void BeginStream() override;

  virtual void EndStream() override;

  struct RttiData
  {
    ezString m_sTypeName;
    ezUInt32 m_uiTypeVersion;
    ezUInt32 m_uiObjectCount;
    const ezRTTI* m_pRTTI;
  };

  const ezDynamicArray<RttiData>& GetArchiveRttiData() const { return m_Types; }

  ezUInt32 ReadObjectReference(void** ppReference);

  void SetObjectReferenceMapping(ezUInt32 uiObjectID, void* pNewObject);

  void* GetObjectReferenceMapping(ezUInt32 uiObjectID) const;

  void* ReadTypedObject();

  ezReflectedClass* ReadReflectedObject();

  void RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer);

  /// \brief Returns an array with pointers to all objects that were read using ReadReflectedObject().
  const ezDeque<ezReflectedClass*>& GetDeserializedObjects() const { return m_DeserializedReflected; }

  /// \brief Calls ezReflectedClass::OnDeserialized() on all objects that were read with ReadReflectedObject().
  void CallOnDeserialized();

  ezUInt32 GetStoredTypeVersion(const ezRTTI* pRtti) const;

  template<typename TYPE>
  ezUInt32 GetStoredTypeVersion() const
  {
    return GetStoredTypeVersion(ezGetStaticRTTI<TYPE>());
  }

private:
  void FinishReferenceMapping();

  void SetObjectReferenceMapping(ezUInt32 uiObjectID, void* pNewObject, ezUInt32 uiDataSize);

  bool m_bStreamFinished;
  ezLogInterface* m_pLog;

  ezArchiveSerializer* GetTypeSerializer(const ezRTTI* pRttiBase);

  ezMap<const ezRTTI*, ezArchiveSerializer*> m_TypeSerializers;

  struct ObjectRef
  {
    EZ_DECLARE_POD_TYPE();

    ObjectRef()
    {
      m_pObject = nullptr;
      m_uiDataSize = 0;
    }

    void* m_pObject;
    ezUInt32 m_uiDataSize;
  };

  ezDynamicArray<ObjectRef> m_ObjectIDtoPointer;

  struct ObjectMapping
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_ObjectID;
    void** m_pObjectWaiting;
  };

  ezDeque<ObjectMapping> m_ObjectsToMap;

  ezDynamicArray<RttiData> m_Types;
  ezStreamReaderBase& m_InputStream;

  ezDeque<ezReflectedClass*> m_DeserializedReflected;

  ezMap<const ezRTTI*, ezUInt32> m_StoredVersion;
};


// [internal]
enum ezArchiveVersion : ezUInt8
{
  InvalidVersion = 0,
  Version1,

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

