#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

ezVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(ezVisualScriptNodeDescription::Type::Enum nodeType, ezVisualScriptDataType::Enum dataType);

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "EntryCall_Coroutine",
    "MessageHandler",
    "MessageHandler_Coroutine",
    "ReflectedFunction",
    "GetReflectedProperty",
    "SetReflectedProperty",
    "InplaceCoroutine",
    "GetScriptOwner",
    "SendMessage",

    "", // FirstBuiltin,

    "Builtin_Constant",
    "Builtin_GetVariable",
    "Builtin_SetVariable",
    "Builtin_IncVariable",
    "Builtin_DecVariable",

    "Builtin_Branch",
    "Builtin_Switch",
    "Builtin_Loop",

    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_IsValid",
    "Builtin_Select",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_String_Format",
    "Builtin_ToHashedString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_MakeArray",

    "Builtin_TryGetComponentOfBaseType",

    "Builtin_StartCoroutine",
    "Builtin_StopCoroutine",
    "Builtin_StopAllCoroutines",
    "Builtin_WaitForAll",
    "Builtin_WaitForAny",
    "Builtin_Yield",

    "", // LastBuiltin,
  };
  static_assert(EZ_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)ezVisualScriptNodeDescription::Type::Count);

  template <typename T>
  ezResult WriteNodeArray(ezArrayPtr<T> a, ezStreamWriter& inout_stream)
  {
    ezUInt16 uiCount = static_cast<ezUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
ezVisualScriptNodeDescription::Type::Enum ezVisualScriptNodeDescription::Type::GetConversionType(ezVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Bool - ezVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Byte - ezVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Int - ezVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Int64 - ezVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Float - ezVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (ezVisualScriptDataType::Double - ezVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (ezVisualScriptDataType::IsNumber(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - ezVisualScriptDataType::Bool));

  if (targetDataType == ezVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == ezVisualScriptDataType::HashedString)
    return Builtin_ToHashedString;

  if (targetDataType == ezVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  EZ_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* ezVisualScriptNodeDescription::Type::GetName(Enum type)
{
  EZ_ASSERT_DEBUG(type >= 0 && type < EZ_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void ezVisualScriptNodeDescription::AppendUserDataName(ezStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptGraphDescription::ezVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

ezVisualScriptGraphDescription::~ezVisualScriptGraphDescription() = default;

static const ezTypeVersion s_uiVisualScriptGraphDescriptionVersion = 2;

// static
ezResult ezVisualScriptGraphDescription::Serialize(ezArrayPtr<const ezVisualScriptNodeDescription> nodes, const ezVisualScriptDataDescription& localDataDesc, ezStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  ezDefaultMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter stream(&streamStorage);
  ezUInt32 additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      EZ_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        ezUInt32 uiSize = 0;
        ezUInt32 uiAlignment = 0;
        EZ_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const ezUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  EZ_SUCCEED_OR_RETURN(streamStorage.CopyToStream(inout_stream));

  EZ_SUCCEED_OR_RETURN(localDataDesc.Serialize(inout_stream));

  return EZ_SUCCESS;
}

ezResult ezVisualScriptGraphDescription::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);
  if (uiVersion < 2)
  {
    ezLog::Error("Invalid visual script desc version. Expected >= 2 but got {}. Visual Script needs re-export", uiVersion);
    return EZ_FAILURE;
  }

  {
    ezUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  ezUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = ezMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  ezUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    EZ_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    EZ_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    EZ_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      EZ_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  ezSharedPtr<ezVisualScriptDataDescription> pLocalDataDesc = EZ_DEFAULT_NEW(ezVisualScriptDataDescription);
  EZ_SUCCEED_OR_RETURN(pLocalDataDesc->Deserialize(inout_stream));
  m_pLocalDataDesc = pLocalDataDesc;

  return EZ_SUCCESS;
}

ezScriptMessageDesc ezVisualScriptGraphDescription::GetMessageDesc() const
{
  auto pEntryNode = GetNode(0);
  EZ_ASSERT_DEBUG(pEntryNode != nullptr &&
                      pEntryNode->m_Type == ezVisualScriptNodeDescription::Type::MessageHandler ||
                    pEntryNode->m_Type == ezVisualScriptNodeDescription::Type::MessageHandler_Coroutine ||
                    pEntryNode->m_Type == ezVisualScriptNodeDescription::Type::SendMessage,
    "Entry node is invalid or not a message handler");

  auto& userData = pEntryNode->GetUserData<NodeUserData_TypeAndProperties>();

  ezScriptMessageDesc desc;
  desc.m_pType = userData.m_pType;
  desc.m_Properties = ezMakeArrayPtr(userData.m_Properties, userData.m_uiNumProperties);
  return desc;
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptExecutionContext::ezVisualScriptExecutionContext(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

ezVisualScriptExecutionContext::~ezVisualScriptExecutionContext()
{
  Deinitialize();
}

void ezVisualScriptExecutionContext::Initialize(ezVisualScriptInstance& inout_instance, ezVisualScriptDataStorage& inout_localDataStorage, ezArrayPtr<ezVariant> arguments)
{
  m_pInstance = &inout_instance;

  m_DataStorage[DataOffset::Source::Local] = &inout_localDataStorage;
  m_DataStorage[DataOffset::Source::Instance] = inout_instance.GetInstanceDataStorage();
  m_DataStorage[DataOffset::Source::Constant] = inout_instance.GetConstantDataStorage();

  auto pNode = m_pDesc->GetNode(0);
  EZ_ASSERT_DEV(ezVisualScriptNodeDescription::Type::IsEntry(pNode->m_Type), "Invalid entry node");

  for (ezUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
}

void ezVisualScriptExecutionContext::Deinitialize()
{
  // 0x1 is a marker value to indicate that we are in a yield
  if (m_pCurrentCoroutine > reinterpret_cast<ezScriptCoroutine*>(0x1))
  {
    auto pModule = m_pInstance->GetWorld()->GetOrCreateModule<ezScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(m_pCurrentCoroutine->GetHandle());
    m_pCurrentCoroutine = nullptr;
  }
}

ezVisualScriptExecutionContext::ExecResult ezVisualScriptExecutionContext::Execute(ezTime deltaTimeSinceLastExecution)
{
  EZ_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_uiExecutionCounter;
  m_DeltaTimeSinceLastExecution = deltaTimeSinceLastExecution;

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
    ExecResult result = pNode->m_Function(*this, *pNode);
    if (result.m_NextExecAndState < ExecResult::State::Completed)
    {
      return result;
    }

    m_uiCurrentNode = pNode->GetExecutionIndex(result.m_NextExecAndState);
    m_pCurrentCoroutine = nullptr;

    pNode = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ExecResult::RunNext(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVisualScriptSendMessageMode, 1)
  EZ_ENUM_CONSTANTS(ezVisualScriptSendMessageMode::Direct, ezVisualScriptSendMessageMode::Recursive, ezVisualScriptSendMessageMode::Event)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on
