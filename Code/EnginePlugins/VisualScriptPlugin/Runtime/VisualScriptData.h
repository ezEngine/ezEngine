#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptDataDescription : public ezRefCounted
{
  struct DataOffset
  {
    EZ_DECLARE_POD_TYPE();

    struct EZ_VISUALSCRIPTPLUGIN_DLL Source
    {
      enum Enum
      {
        Local,
        Instance,
        Constant,

        Count
      };

      static const char* GetName(Enum source);
    };

    enum
    {
      BYTE_OFFSET_BITS = 24,
      TYPE_BITS = 6,
      SOURCE_BITS = 2,
      INVALID_BYTE_OFFSET = EZ_BIT(BYTE_OFFSET_BITS) - 1
    };

    EZ_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = INVALID_BYTE_OFFSET;
      m_uiType = ezVisualScriptDataType::Invalid;
      m_uiSource = Source::Local;
    }

    EZ_ALWAYS_INLINE DataOffset(ezUInt32 uiOffset, ezVisualScriptDataType::Enum dataType, Source::Enum source)
    {
      m_uiByteOffset = uiOffset;
      m_uiType = dataType;
      m_uiSource = source;
    }

    EZ_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != INVALID_BYTE_OFFSET &&
             m_uiType != ezVisualScriptDataType::Invalid;
    }

    EZ_ALWAYS_INLINE ezVisualScriptDataType::Enum GetType() const { return static_cast<ezVisualScriptDataType::Enum>(m_uiType); }
    EZ_ALWAYS_INLINE Source::Enum GetSource() const { return static_cast<Source::Enum>(m_uiSource); }
    EZ_ALWAYS_INLINE bool IsLocal() const { return m_uiSource == Source::Local; }
    EZ_ALWAYS_INLINE bool IsInstance() const { return m_uiSource == Source::Instance; }
    EZ_ALWAYS_INLINE bool IsConstant() const { return m_uiSource == Source::Constant; }

    EZ_ALWAYS_INLINE ezResult Serialize(ezStreamWriter& inout_stream) const { return inout_stream.WriteDWordValue(this); }
    EZ_ALWAYS_INLINE ezResult Deserialize(ezStreamReader& inout_stream) { return inout_stream.ReadDWordValue(this); }

    ezUInt32 m_uiByteOffset : BYTE_OFFSET_BITS;
    ezUInt32 m_uiType : TYPE_BITS;
    ezUInt32 m_uiSource : SOURCE_BITS;
  };

  struct OffsetAndCount
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStartOffset = 0;
    ezUInt32 m_uiCount = 0;
  };

  OffsetAndCount m_PerTypeInfo[ezVisualScriptDataType::Count];
  ezUInt32 m_uiStorageSizeNeeded = 0;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  void Clear();
  void CalculatePerTypeStartOffsets();
  void CheckOffset(DataOffset dataOffset, const ezRTTI* pType) const;

  DataOffset GetOffset(ezVisualScriptDataType::Enum dataType, ezUInt32 uiIndex, DataOffset::Source::Enum source) const;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptDataStorage : public ezRefCounted
{
public:
  ezVisualScriptDataStorage(const ezSharedPtr<const ezVisualScriptDataDescription>& pDesc);
  ~ezVisualScriptDataStorage();

  const ezVisualScriptDataDescription& GetDesc() const;

  bool IsAllocated() const;
  void AllocateStorage();
  void DeallocateStorage();

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  using DataOffset = ezVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  ezTypedPointer GetPointerData(DataOffset dataOffset, ezUInt32 uiExecutionCounter) const;

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType, ezUInt32 uiExecutionCounter);

  ezVariant GetDataAsVariant(DataOffset dataOffset, const ezRTTI* pExpectedType, ezUInt32 uiExecutionCounter) const;
  void SetDataFromVariant(DataOffset dataOffset, const ezVariant& value, ezUInt32 uiExecutionCounter);

private:
  ezSharedPtr<const ezVisualScriptDataDescription> m_pDesc;
  ezBlob m_Storage;
};

struct ezVisualScriptInstanceData
{
  ezVisualScriptDataDescription::DataOffset m_DataOffset;
  ezVariant m_DefaultValue;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

using ezVisualScriptInstanceDataMapping = ezRefCountedContainer<ezHashTable<ezHashedString, ezVisualScriptInstanceData>>;

#include <VisualScriptPlugin/Runtime/VisualScriptData_inl.h>
