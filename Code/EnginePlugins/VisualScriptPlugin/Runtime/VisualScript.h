#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class ezVisualScriptInstance;
class ezVisualScriptExecutionContext;

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptNodeDescription
{
  struct EZ_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      EntryCall_Coroutine,
      MessageHandler,
      MessageHandler_Coroutine,
      ReflectedFunction,
      InplaceCoroutine,
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

      Builtin_StartCoroutine,
      Builtin_StopCoroutine,
      Builtin_StopAllCoroutines,
      Builtin_WaitForAll,
      Builtin_WaitForAny,
      Builtin_Yield,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    EZ_ALWAYS_INLINE static bool IsEntry(Enum type) { return type >= EntryCall && type <= MessageHandler_Coroutine; }

    EZ_ALWAYS_INLINE static bool MakesOuterCoroutine(Enum type) { return type == InplaceCoroutine || (type >= Builtin_WaitForAll && type <= Builtin_Yield); }

    EZ_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(ezVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  using DataOffset = ezVisualScriptDataDescription::DataOffset;

  ezEnum<Type> m_Type;
  ezEnum<ezVisualScriptDataType> m_DeductedDataType;
  ezSmallArray<ezUInt16, 4> m_ExecutionIndices;
  ezSmallArray<DataOffset, 4> m_InputDataOffsets;
  ezSmallArray<DataOffset, 2> m_OutputDataOffsets;

  ezHashedString m_sTargetTypeName;
  ezHashedString m_sTargetPropertyName;

  ezEnum<ezComparisonOperator> m_ComparisonOperator;
  ezEnum<ezScriptCoroutineCreationMode> m_CoroutineCreationMode;

  void AppendUserDataName(ezStringBuilder& out_sResult) const;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptGraphDescription : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezVisualScriptGraphDescription);

public:
  ezVisualScriptGraphDescription();
  ~ezVisualScriptGraphDescription();

  static ezResult Serialize(ezArrayPtr<const ezVisualScriptNodeDescription> nodes, const ezVisualScriptDataDescription& localDataDesc, ezStreamWriter& inout_stream);
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

  struct ExecResult
  {
    struct State
    {
      enum Enum
      {
        Completed = 0,
        ContinueLater = -1,

        Error = -100,
      };
    };

    static EZ_ALWAYS_INLINE ExecResult Completed() { return {0}; }
    static EZ_ALWAYS_INLINE ExecResult RunNext(int iExecSlot) { return {iExecSlot}; }
    static EZ_ALWAYS_INLINE ExecResult ContinueLater(ezTime maxDelay) { return {State::ContinueLater, maxDelay}; }
    static EZ_ALWAYS_INLINE ExecResult Error() { return {State::Error}; }

    int m_NextExecAndState = 0;
    ezTime m_MaxDelay = ezTime::MakeZero();
  };

  struct Node;
  using ExecuteFunction = ExecResult (*)(ezVisualScriptExecutionContext& inout_context, const Node& node);
  using DataOffset = ezVisualScriptDataDescription::DataOffset;
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

  const Node* GetNode(ezUInt32 uiIndex) const;

  bool IsCoroutine() const;

  const ezSharedPtr<const ezVisualScriptDataDescription>& GetLocalDataDesc() const;

private:
  ezArrayPtr<const Node> m_Nodes;
  ezBlob m_Storage;

  ezSharedPtr<const ezVisualScriptDataDescription> m_pLocalDataDesc;
};



class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptExecutionContext
{
public:
  ezVisualScriptExecutionContext(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc);
  ~ezVisualScriptExecutionContext();

  void Initialize(ezVisualScriptInstance& inout_instance, ezVisualScriptDataStorage& inout_localDataStorage, ezArrayPtr<ezVariant> arguments);
  void Deinitialize();

  using ExecResult = ezVisualScriptGraphDescription::ExecResult;
  ExecResult Execute(ezTime deltaTimeSinceLastExecution);

  ezVisualScriptInstance& GetInstance() { return *m_pInstance; }

  using DataOffset = ezVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  ezTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType = nullptr);

  ezVariant GetDataAsVariant(DataOffset dataOffset, const ezRTTI* pExpectedType) const;
  void SetDataFromVariant(DataOffset dataOffset, const ezVariant& value);

  ezScriptCoroutine* GetCurrentCoroutine() { return m_pCurrentCoroutine; }
  void SetCurrentCoroutine(ezScriptCoroutine* pCoroutine);

  ezTime GetDeltaTimeSinceLastExecution();

private:
  ezSharedPtr<const ezVisualScriptGraphDescription> m_pDesc;
  ezVisualScriptInstance* m_pInstance = nullptr;
  ezUInt32 m_uiCurrentNode = 0;
  ezUInt32 m_uiExecutionCounter = 0;
  ezTime m_DeltaTimeSinceLastExecution;

  ezVisualScriptDataStorage* m_DataStorage[DataOffset::Source::Count] = {};

  ezScriptCoroutine* m_pCurrentCoroutine = nullptr;
};

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
