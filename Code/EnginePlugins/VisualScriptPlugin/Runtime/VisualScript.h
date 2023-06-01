#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class ezVisualScriptInstance;

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptNodeDescription
{
  struct EZ_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      MessageHandler,
      ReflectedFunction,
      GetScriptOwner,

      FirstBuiltin,

      Builtin_Branch,
      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_IsValid,

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_MakeArray,

      Builtin_TryGetComponentOfBaseType,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    EZ_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(ezVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  struct DataOffset
  {
    EZ_DECLARE_POD_TYPE();

    EZ_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = ezInvalidIndex;
      m_uiDataType = ezVisualScriptDataType::Invalid;
      m_uiIsConstant = 0;
    }

    EZ_ALWAYS_INLINE DataOffset(ezUInt32 uiOffset, ezVisualScriptDataType::Enum dataType, bool bIsConstant)
    {
      m_uiByteOffset = uiOffset;
      m_uiDataType = dataType;
      m_uiIsConstant = bIsConstant ? 1 : 0;
    }

    EZ_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != (EZ_BIT(24) - 1) &&
             m_uiDataType != ezVisualScriptDataType::Invalid;
    }

    ezUInt32 m_uiByteOffset : 24;
    ezUInt32 m_uiDataType : 7;
    ezUInt32 m_uiIsConstant : 1;
  };

  ezEnum<Type> m_Type;
  ezEnum<ezVisualScriptDataType> m_DeductedDataType;
  ezSmallArray<ezUInt16, 4> m_ExecutionIndices;
  ezSmallArray<DataOffset, 4> m_InputDataOffsets;
  ezSmallArray<DataOffset, 2> m_OutputDataOffsets;

  union
  {
    struct
    {
      const ezRTTI* m_pTargetType;
      const ezAbstractProperty* m_pTargetProperty;
    };

    ezComparisonOperator::Enum m_ComparisonOperator;

    ezUInt32 m_RawData[4] = {};
  } m_UserData;

  void AppendUserDataName(ezStringBuilder& out_sResult) const;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptGraphDescription
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezVisualScriptGraphDescription);

public:
  ezVisualScriptGraphDescription();
  ~ezVisualScriptGraphDescription();

  static ezResult Serialize(ezArrayPtr<const ezVisualScriptNodeDescription> nodes, ezStreamWriter& inout_stream);
  ezResult Deserialize(ezStreamReader& inout_stream);

  template <typename T, ezUInt32 Size>
  struct EmbeddedArrayOrPointer
  {
    union
    {
      T m_Embedded[Size] = {};
      T* m_Ptr;
    };

    static void AddAdditionalDataSize(ezArrayPtr<const T> a, ezUInt32& inout_uiAdditionalDataSize);
    static void AddAdditionalDataSize(ezUInt32 uiSize, ezUInt32 uiAlignment, ezUInt32& inout_uiAdditionalDataSize);

    T* Init(ezUInt8 uiCount, ezUInt8*& inout_pAdditionalData);
    ezResult ReadFromStream(ezUInt8& out_uiCount, ezStreamReader& inout_stream, ezUInt8*& inout_pAdditionalData);
  };

  struct ReturnValue
  {
    enum Enum
    {
      Completed = 0,
      ContinueNextFrame = -1,

      Error = -100,
    };
  };

  struct Node;
  using ExecuteFunction = int (*)(ezVisualScriptInstance& ref_instance, const Node& node);
  using DataOffset = ezVisualScriptNodeDescription::DataOffset;
  using ExecutionIndicesArray = EmbeddedArrayOrPointer<ezUInt16, 4>;
  using InputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 4>;
  using OutputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 2>;
  using UserDataArray = EmbeddedArrayOrPointer<ezUInt32, 4>;

  struct Node
  {
    ExecuteFunction m_Function = nullptr;
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    ezUInt32 m_uiPadding = 0;
#endif

    ExecutionIndicesArray m_ExecutionIndices;
    InputDataOffsetsArray m_InputDataOffsets;
    OutputDataOffsetsArray m_OutputDataOffsets;
    UserDataArray m_UserData;

    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    ezUInt8 m_NumExecutionIndices;
    ezUInt8 m_NumInputDataOffsets;
    ezUInt8 m_NumOutputDataOffsets;

    ezUInt16 m_UserDataByteSize;
    ezEnum<ezVisualScriptDataType> m_DeductedDataType;
    ezUInt8 m_Reserved = 0;

    ezUInt32 GetExecutionIndex(ezUInt32 uiSlot) const;
    DataOffset GetInputDataOffset(ezUInt32 uiSlot) const;
    DataOffset GetOutputDataOffset(ezUInt32 uiSlot) const;

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    void SetUserData(const T& data, ezUInt8*& inout_pAdditionalData);
  };

  const Node* GetNode(ezUInt32 uiIndex);

private:
  ezArrayPtr<const Node> m_Nodes;
  ezBlob m_Storage;
};

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptDataDescription : public ezRefCounted
{
  using DataOffset = ezVisualScriptNodeDescription::DataOffset;

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

  DataOffset GetOffset(ezVisualScriptDataType::Enum dataType, ezUInt32 uiIndex, bool bIsConstant) const;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptDataStorage : public ezRefCounted
{
public:
  using DataOffset = ezVisualScriptNodeDescription::DataOffset;

  ezVisualScriptDataStorage(const ezSharedPtr<const ezVisualScriptDataDescription>& pDesc);
  ~ezVisualScriptDataStorage();

  void AllocateStorage();
  void DeallocateStorage();

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  ezTypedPointer GetPointerData(DataOffset dataOffset, ezUInt32 uiExecutionCounter);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType, ezUInt32 uiExecutionCounter);

  ezVariant GetDataAsVariant(DataOffset dataOffset, ezVariantType::Enum expectedType, ezUInt32 uiExecutionCounter) const;
  void SetDataFromVariant(DataOffset dataOffset, const ezVariant& value, ezUInt32 uiExecutionCounter);

private:
  ezSharedPtr<const ezVisualScriptDataDescription> m_pDesc;
  ezBlob m_Storage;
};

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
