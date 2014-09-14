#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

class ezArchiveSerializer;

/// \brief A chunk stream writer that supports storing reflected objects and pointer references.
///
/// ezArchiveWriter and ezArchiveReader implement a file format that is chunked (see ezChunkStreamWriter / ezChunkStreamReader)
/// and supports storing and restoring of reflected objects, as well as pointer references between objects.
///
/// When a reflected object is written to the archive, all necessary information to restore the object is written along with it.
/// When reading an archive, this information is used to restore the correct instance type. Version information for reflected
/// objects is also stored.
///
/// For types that cannot be restored without additional information, ezArchiveSerializer objects can be used to store custom
/// information and/or to recreate objects through specialized methods.
///
/// References (pointers) to objects can be stored as well, and will be restored by the reader when the stream is closed and
/// all referenced objects are known. References to objects that were not deserialized will be set to nullptr.
///
/// When a type is encountered during deserialization, that is unknown to the system, all objects of that type will be skipped,
/// including nested objects. This makes the archive serialization very robust in situations where custom types 
/// (e.g. through plugins) are used.
///
/// Additionally, since it builds on the chunk stream format, entire sections may be skipped during deserialization.
class EZ_FOUNDATION_DLL ezArchiveWriter : public ezChunkStreamWriter
{
public:
  /// \brief The constructor takes another stream to which all write requests are passed along.
  ezArchiveWriter(ezStreamWriterBase& stream);

  /// \brief Destructor
  ~ezArchiveWriter();

  /// \brief Overridden function. The same restrictions apply as for ezChunkStreamWriter::WriteBytes().
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override;

  /// \brief Serializes a reflected object to the archive.
  ///
  /// This function internally calls BeginTypedObject() and EndTypedObject(). It also calls ezReflectedClass::Serialize() on the given object.
  /// This should be the preferred method to serialize objects that are derived from ezReflectedClass, if its exact type is not
  /// known at deserialization time and needs to be read from the archive as well. WriteReflectedObject will write all
  /// necessary information to the archive to restore the exact object type later. It will also store the object's version
  /// number, so that the deserialization code can properly read the data.
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
  ///
  /// This function returns the integer token under which the reference is stored in the archive. This might be useful, if you want
  /// to store and restore custom references.
  ezUInt32 WriteObjectReference(const void* pReference);

  /// \brief Flags the given object as already written, which can be queried using HasObjectBeenWritten().
  ///
  /// This is not necessary to call manually, unless you want to do custom serialization logic for which this might be useful.
  /// This function is automatically called for all objects stored through BeginTypedObject() or WriteReflectedObject().
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

  /// \brief This must be called before writing any data to the stream.
  virtual void BeginStream() override;

  /// \brief This must be called once all writing to the archive is finished.
  virtual void EndStream() override;

  /// \brief Registers an ezArchiveSerializer to use for specific object types (and all derived classes).
  ///
  /// All serializers must be registered before the archive is written to.
  /// Every time a typed / reflected object is written to the archive, it is checked whether there is a serializer
  /// for this object type (or any of its base classes). If so, that serializer is called to store additional data
  /// to the stream, before the object itself is serialized.
  /// The ezArchiveReader must then also use a serializer to read that additional data and create the object of the proper type.
  /// This way one can store custom data per object which may be required to allocate the object.
  /// This mechanism can also be used to put a custom allocator in place, even though no additional data is required.
  /// E.g. if an object needs to be allocated through a special interface (e.g. ezWorld) and cannot be globally allocated
  /// the serializer can take care of that.
  void RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer);

private:

  ezArchiveSerializer* GetTypeSerializer(const ezRTTI* pRttiBase);

  void StoreType(const ezRTTI* pRtti);

  struct TypeInfo
  {
    ezUInt16 m_uiTypeID;
    ezUInt32 m_uiObjects;
  };

  bool m_bWritingFile;

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

/// \brief A chunk stream reader that supports restoring reflected objects and pointer references.
///
/// See ezArchiveWriter for more details.
class EZ_FOUNDATION_DLL ezArchiveReader : public ezChunkStreamReader
{
public:
  /// \brief The constructor takes another stream to which all read requests are passed along.
  ezArchiveReader(ezStreamReaderBase& stream);

  /// \brief Destructor
  ~ezArchiveReader();

  /// \brief The archive will output status information, warnings and errors through the given interface.
  void SetLogInterface(ezLogInterface* pLog) { m_pLog = pLog; }

  /// \brief Starts reading from the stream.
  ///
  /// After this call the RTTI information from the archive is available.
  virtual void BeginStream() override;

  /// \brief Finishes reading from the archive. This must be called once no more data should be read.
  ///
  /// All pointer-mappings for inter-object references are resolved at this time. Object references that were read
  /// previously and could not be resolved at that time will be patched up here. Therefore all pointers given to
  /// ReadObjectReference() must stay valid till after this call so that they can be written to.
  virtual void EndStream() override;

  /// \brief The RTTI data that is stored in the archive
  struct RttiData
  {
    ezString m_sTypeName;     ///< The name of the RTTI type
    ezUInt32 m_uiTypeVersion; ///< The version with which objects of this type were written to the archive
    ezUInt32 m_uiObjectCount; ///< How many objects of this type are stored in the archive in total
    const ezRTTI* m_pRTTI;    ///< The live ezRTTI object with the same type name. If the type in the archive is unknown, this will be nullptr
  };

  /// \brief Returns the RTTI data that was stored in the archive. This information is available right after the call to BeginStream()
  const ezDynamicArray<RttiData>& GetArchiveRttiData() const { return m_Types; }

  /// \brief Reads a reference to an object. The reference will be patched up once the information is available.
  ///
  /// This function reads a pointer to an object and stores it in the pointer whose address is given (therefore the parameter is a
  /// pointer to a pointer). However, many references will not be available at the time the reference is supposed to be read.
  /// Therefore the given pointer is stored and will be patched when EndStream() is called.
  /// A reference that cannot be patched, because the referenced object is never restored, will be set to nullptr.
  ///
  /// This function always returns an integer token for the requested reference. This can be used to set up the mapping manually
  /// via SetObjectReferenceMapping(). This might be necessary if you want to restore custom pointer references.
  ezUInt32 ReadObjectReference(void** ppReference);

  /// \brief Associates the given new pointer with a reference ID. All references read through ReadObjectReference() that return
  /// the given object ID will be patched up with the given pointer when archive reading is finished.
  void SetObjectReferenceMapping(ezUInt32 uiObjectID, void* pNewObject);

  /// \brief Returns the pointer value that is associated with the given object ID.
  void* GetObjectReferenceMapping(ezUInt32 uiObjectID) const;

  /// \brief Reads the RTTI information about a typed object and creates an object of that type. The instance is then returned.
  ///
  /// You need to use this function for objects that were written with ezArchiveWriter::BeginTypedObject().
  /// Will return nullptr, if the stored type cannot be created because the type is unknown or no allocator / serializer is available.
  ///
  /// The parameter \a out_pRtti optionally returns the RTTI information about the instance. If the type in the archive is unknown
  /// at runtime, this will be nullptr. The parameter \a out_pTypeID will return the internal type index. Use GetArchiveRttiData()
  /// to get more information about the type. This allows to at least know the type name of an object that failed to be created.
  void* ReadTypedObject(const ezRTTI** out_pRtti = nullptr, ezUInt16* out_pTypeID = nullptr);

  /// \brief Restores an object that was written with ezArchiveWriter::WriteReflectedObject().
  ///
  /// This internally calls ReadTypedObject(). If an object cannot be created due to an unknown type, it will be skipped.
  /// Once the object is created, ezReflectedClass::Deserialize() is called on it.
  /// It is also stored in the referenced objects map, so references to this instance can be restored.
  /// Additionally the instance is queued for the ezReflectedClass::OnDeserialized() callback. Once the entire archive has
  /// been read, you should call CallOnDeserialized(), so that all objects read through ReadReflectedObject() can do post
  /// deserialization work.
  ezReflectedClass* ReadReflectedObject();

  /// \brief Registers an ezArchiveSerializer to use for specific object types (and all derived classes).
  ///
  /// All serializers must be registered before the archive is read from.
  /// Every time a typed / reflected object is read, it is checked whether there is a serializer
  /// for this object type (or any of its base classes). If so, that serializer is called to read additional data
  /// from the stream, before the object itself is deserialized.
  ///
  /// \sa ezArchiveWriter::RegisterTypeSerializer()
  void RegisterTypeSerializer(const ezRTTI* pRttiBase, ezArchiveSerializer* pSerializer);

  /// \brief Returns an array with pointers to all objects that were read using ReadReflectedObject().
  const ezDeque<ezReflectedClass*>& GetDeserializedObjects() const { return m_DeserializedReflected; }

  /// \brief Calls ezReflectedClass::OnDeserialized() on all objects that were read with ReadReflectedObject().
  void CallOnDeserialized();

  /// \brief Returns the version with which objects of the given type were written to the archive.
  ///
  /// This is very important during deserialization, so that objects know how to interpret the data in the stream.
  /// Always call this in ezReflectedClass::Deserialize() with the own type.
  ezUInt32 GetStoredTypeVersion(const ezRTTI* pRtti) const;

  /// \brief Returns the version with which objects of the given type were written to the archive.
  ///
  /// This is very important during deserialization, so that objects know how to interpret the data in the stream.
  /// Always call this in ezReflectedClass::Deserialize() with the own type.
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

/// \brief Base class for custom helpers to store and restore reflected objects to and from an archive.
///
/// ezArchiveSerializer is used to store custom data to an archive before an instance of some type is serialized,
/// so that upon deserialization it can read that custom data first and then decide how to allocate the instance.
///
/// Note that a serializer can also simply be used to change how an instance is allocated, it does not necessarily
/// need to store data in the stream.
class ezArchiveSerializer
{
public:
  virtual ~ezArchiveSerializer() { }

  /// \brief Called by ezArchiveWriter when a (derived) type is serialized for which this serializer is registered.
  virtual void Serialize(ezArchiveWriter& stream, const ezRTTI* pRtti, const void* pReference)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  /// \brief Called by ezArchiveReader when a (derived) type is deserialized for which this serializer is registered.
  ///
  /// Either this function must read exactly the data that was written to the stream AND successfully create the instance,
  /// or it mustn't read a single byte and then return nullptr. In the latter case the object will be skipped by the 
  /// archive reader.
  /// Once it starts reading a byte, skipping of the object becomes impossible, so it must be fully and successfully
  /// created.
  virtual void* Deserialize(ezArchiveReader& stream, const ezRTTI* pRtti)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }
};

// [internal]
enum ezArchiveVersion : ezUInt8
{
  InvalidVersion = 0,
  Version1,
  // << insert new versions here >>

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

