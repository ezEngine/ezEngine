#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class ezVisualScriptInstance;
class ezVisualScriptExecutionContext;

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptNodeDescription
{
  /// \brief Native node types for visual script graphs.
  /// Editor only types are not supported at runtime and will be replaced by the visual script compiler during asset transform.
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
      GetReflectedProperty,
      SetReflectedProperty,
      InplaceCoroutine,
      GetScriptOwner,
      SendMessage,

      FirstBuiltin,

      Builtin_Constant,    // Editor only
      Builtin_GetVariable, // Editor only
      Builtin_SetVariable,
      Builtin_IncVariable,
      Builtin_DecVariable,
      Builtin_TempVariable,

      Builtin_Branch,
      Builtin_Switch,
      Builtin_WhileLoop,          // Editor only
      Builtin_ForLoop,            // Editor only
      Builtin_ForEachLoop,        // Editor only
      Builtin_ReverseForEachLoop, // Editor only
      Builtin_Break,              // Editor only
      Builtin_Jump,               // Editor only

      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_CompareExec, // Editor only
      Builtin_IsValid,
      Builtin_Select,

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,
      Builtin_Expression,

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_String_Format,
      Builtin_ToHashedString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_MakeArray,
      Builtin_Array_GetElement,
      Builtin_Array_SetElement,
      Builtin_Array_GetCount,
      Builtin_Array_IsEmpty,
      Builtin_Array_Clear,
      Builtin_Array_Contains,
      Builtin_Array_IndexOf,
      Builtin_Array_Insert,
      Builtin_Array_PushBack,
      Builtin_Array_PushBackRange,
      Builtin_Array_Remove,
      Builtin_Array_RemoveAt,

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
    EZ_ALWAYS_INLINE static bool IsLoop(Enum type) { return type >= Builtin_WhileLoop && type <= Builtin_ReverseForEachLoop; }

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

  ezVariant m_Value;

  void AppendUserDataName(ezStringBuilder& out_sResult) const;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptGraphDescription : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezVisualScriptGraphDescription);

public:
  ezVisualScriptGraphDescription();
  ~ezVisualScriptGraphDescription();

  static ezResult Serialize(ezArrayPtr<const ezVisualScriptNodeDescription> nodes, const ezVisualScriptDataDescription& localDataDesc, ezStreamWriter& inout_stream);
  ezResult Deserialize(ezStreamReader& inout_stream, const ezVisualScriptDataDescription& instanceDataDesc, const ezVisualScriptDataDescription& constantDataDesc);

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

    T* Init(ezUInt8 uiCount, ezUInt32 uiAlignment, ezUInt8*& inout_pAdditionalData);
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

    DataOffset* GetInputDataOffsets();
    DataOffset* GetOutputDataOffsets();

    template <typename T>
    static constexpr ezUInt32 GetUserDataAlignment();

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    T& InitUserData(ezUInt8*& inout_pAdditionalData, ezUInt32 uiByteSize = sizeof(T), ezUInt32 uiAlignment = GetUserDataAlignment<T>());
  };

  const Node* GetNode(ezUInt32 uiIndex) const;

  bool IsCoroutine() const;
  ezScriptMessageDesc GetMessageDesc() const;

  const ezSharedPtr<const ezVisualScriptDataDescription>& GetLocalDataDesc() const;

private:
  ezArrayPtr<const Node> m_Nodes;
  ezBlob m_Storage;

  ezSharedPtr<const ezVisualScriptDataDescription> m_pLocalDataDesc;
};


class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptExecutionContext
{
public:
  ezVisualScriptExecutionContext(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc, ezAllocator* pAllocator);
  ~ezVisualScriptExecutionContext();

  void Initialize(ezVisualScriptInstance& inout_instance, ezArrayPtr<ezVariant> arguments);
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

  ezVisualScriptDataStorage m_LocalDataStorage;
  ezVisualScriptDataStorage* m_DataStorage[DataOffset::Source::Count] = {};

  ezScriptCoroutine* m_pCurrentCoroutine = nullptr;
};

struct ezVisualScriptSendMessageMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Direct,    ///< Directly send the message to the target game object
    Recursive, ///< Send the message to the target game object and its children
    Event,     ///< Send the message as event. \sa ezGameObject::SendEventMessage()

    Default = Direct
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_VISUALSCRIPTPLUGIN_DLL, ezVisualScriptSendMessageMode);

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
