#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void MakeSubfunctionName(const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject, ezStringBuilder& out_sName)
  {
    ezVariant sNameProperty = pObject->GetTypeAccessor().GetValue("Name");
    ezUInt32 uiHash = ezHashHelper<ezUuid>::Hash(pObject->GetGuid());

    out_sName.SetFormat("{}_{}_{}", pEntryObject != nullptr ? ezVisualScriptNodeManager::GetNiceFunctionName(pEntryObject) : "", sNameProperty, ezArgU(uiHash, 8, true, 16));
  }

  ezVisualScriptDataType::Enum FinalizeDataType(ezVisualScriptDataType::Enum dataType)
  {
    ezVisualScriptDataType::Enum result = dataType;
    if (result == ezVisualScriptDataType::EnumValue || result == ezVisualScriptDataType::BitflagValue)
      result = ezVisualScriptDataType::Int64;

    return result;
  }

  using FillUserDataFunction = ezResult (*)(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject);

  static ezResult FillUserData_CoroutineMode(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("CoroutineMode");
    return EZ_SUCCESS;
  }

  static ezResult FillUserData_ReflectedPropertyOrFunction(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
      inout_astNode.m_sTargetTypeName.Assign(pNodeDesc->m_pTargetType->GetTypeName());

    ezVariantArray propertyNames;
    for (auto& pProp : pNodeDesc->m_TargetProperties)
    {
      ezHashedString sPropertyName;
      sPropertyName.Assign(pProp->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return EZ_SUCCESS;
  }

  static ezResult FillUserData_DynamicReflectedProperty(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    auto pTargetType = ezVisualScriptTypeDeduction::GetReflectedType(pObject);
    auto pTargetProperty = ezVisualScriptTypeDeduction::GetReflectedProperty(pObject);
    if (pTargetType == nullptr || pTargetProperty == nullptr)
      return EZ_FAILURE;

    inout_astNode.m_sTargetTypeName.Assign(pTargetType->GetTypeName());

    ezVariantArray propertyNames;
    {
      ezHashedString sPropertyName;
      sPropertyName.Assign(pTargetProperty->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return EZ_SUCCESS;
  }

  static ezResult FillUserData_ConstantValue(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Value");
    inout_astNode.m_DeductedDataType = ezVisualScriptDataType::FromVariantType(inout_astNode.m_Value.GetType());
    return EZ_SUCCESS;
  }

  static ezResult FillUserData_VariableName(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Name");

    ezStringView sName = inout_astNode.m_Value.Get<ezString>().GetView();

    ezVariant defaultValue;
    if (static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager())->GetVariableDefaultValue(ezTempHashedString(sName), defaultValue).Failed())
    {
      ezLog::Error("Invalid variable named '{}'", sName);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  static ezResult FillUserData_Switch(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    inout_astNode.m_DeductedDataType = ezVisualScriptDataType::Int64;

    ezVariantArray casesVarArray;

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
    {
      ezHybridArray<ezReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
      ezReflectionUtils::GetEnumKeysAndValues(pNodeDesc->m_pTargetType, enumKeysAndValues, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& keyAndValue : enumKeysAndValues)
      {
        casesVarArray.PushBack(keyAndValue.m_iValue);
      }
    }
    else
    {
      ezVariant casesVar = pObject->GetTypeAccessor().GetValue("Cases");
      casesVarArray = casesVar.Get<ezVariantArray>();
      for (auto& caseVar : casesVarArray)
      {
        if (caseVar.IsA<ezString>())
        {
          inout_astNode.m_DeductedDataType = ezVisualScriptDataType::HashedString;
          caseVar = ezTempHashedString(caseVar.Get<ezString>()).GetHash();
        }
        else if (caseVar.IsA<ezHashedString>())
        {
          inout_astNode.m_DeductedDataType = ezVisualScriptDataType::HashedString;
          caseVar = caseVar.Get<ezHashedString>().GetHash();
        }
      }
    }

    inout_astNode.m_Value = casesVarArray;
    return EZ_SUCCESS;
  }

  static ezResult FillUserData_Builtin_Compare(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Operator");
    return EZ_SUCCESS;
  }

  static ezResult FillUserData_Builtin_Expression(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    auto pManager = static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

    ezHybridArray<const ezVisualScriptPin*, 16> pins;

    ezHybridArray<ezExpression::StreamDesc, 8> inputs;
    pManager->GetInputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& input = inputs.ExpandAndGetRef();
      input.m_sName.Assign(pPin->GetName());
      input.m_DataType = ezVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    ezHybridArray<ezExpression::StreamDesc, 8> outputs;
    pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& output = outputs.ExpandAndGetRef();
      output.m_sName.Assign(pPin->GetName());
      output.m_DataType = ezVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    ezString sExpressionSource = pObject->GetTypeAccessor().GetValue("Expression").Get<ezString>();

    ezExpressionParser parser;
    ezExpressionParser::Options options = {};
    ezExpressionAST ast;
    EZ_SUCCEED_OR_RETURN(parser.Parse(sExpressionSource, inputs, outputs, options, ast));

    ezExpressionCompiler compiler;
    ezExpressionByteCode byteCode;
    EZ_SUCCEED_OR_RETURN(compiler.Compile(ast, byteCode));

    inout_astNode.m_Value = byteCode;

    return EZ_SUCCESS;
  }

  static ezResult FillUserData_Builtin_TryGetComponentOfBaseType(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    auto typeName = pObject->GetTypeAccessor().GetValue("TypeName");
    const ezRTTI* pType = ezRTTI::FindTypeByName(typeName.Get<ezString>());
    if (pType == nullptr)
    {
      ezLog::Error("Invalid type '{}' for GameObject::TryGetComponentOfBaseType node.", typeName);
      return EZ_FAILURE;
    }

    inout_astNode.m_sTargetTypeName.Assign(pType->GetTypeName());
    return EZ_SUCCESS;
  }

  static ezResult FillUserData_Builtin_StartCoroutine(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    EZ_SUCCEED_OR_RETURN(FillUserData_CoroutineMode(inout_astNode, pCompiler, pObject, pEntryObject));

    auto pManager = static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    ezHybridArray<const ezVisualScriptPin*, 16> pins;
    pManager->GetOutputExecutionPins(pObject, pins);

    const ezUInt32 uiCoroutineBodyIndex = 1;
    auto connections = pManager->GetConnections(*pins[uiCoroutineBodyIndex]);
    if (connections.IsEmpty() == false)
    {
      ezStringBuilder sFunctionName;
      MakeSubfunctionName(pObject, pEntryObject, sFunctionName);

      ezStringBuilder sFullName;
      sFullName.Set(pCompiler->GetCompiledModule().m_sScriptClassName, "::", sFunctionName, "<Coroutine>");

      inout_astNode.m_sTargetTypeName.Assign(sFullName);

      return pCompiler->AddFunction(sFunctionName, connections[0]->GetTargetPin().GetParent(), pObject);
    }

    return EZ_SUCCESS;
  }

  static FillUserDataFunction s_TypeToFillUserDataFunctions[] = {
    nullptr,                                         // Invalid,
    &FillUserData_CoroutineMode,                     // EntryCall,
    &FillUserData_CoroutineMode,                     // EntryCall_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction,       // MessageHandler,
    &FillUserData_ReflectedPropertyOrFunction,       // MessageHandler_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction,       // ReflectedFunction,
    &FillUserData_DynamicReflectedProperty,          // GetReflectedProperty,
    &FillUserData_DynamicReflectedProperty,          // SetReflectedProperty,
    &FillUserData_ReflectedPropertyOrFunction,       // InplaceCoroutine,
    nullptr,                                         // GetOwner,
    &FillUserData_ReflectedPropertyOrFunction,       // SendMessage,

    nullptr,                                         // FirstBuiltin,

    &FillUserData_ConstantValue,                     // Builtin_Constant,
    &FillUserData_VariableName,                      // Builtin_GetVariable,
    &FillUserData_VariableName,                      // Builtin_SetVariable,
    &FillUserData_VariableName,                      // Builtin_IncVariable,
    &FillUserData_VariableName,                      // Builtin_DecVariable,
    nullptr,                                         // Builtin_TempVariable,

    nullptr,                                         // Builtin_Branch,
    &FillUserData_Switch,                            // Builtin_Switch,
    nullptr,                                         // Builtin_WhileLoop,
    nullptr,                                         // Builtin_ForLoop,
    nullptr,                                         // Builtin_ForEachLoop,
    nullptr,                                         // Builtin_ReverseForEachLoop,
    nullptr,                                         // Builtin_Break,
    nullptr,                                         // Builtin_Jump,

    nullptr,                                         // Builtin_And,
    nullptr,                                         // Builtin_Or,
    nullptr,                                         // Builtin_Not,
    &FillUserData_Builtin_Compare,                   // Builtin_Compare,
    &FillUserData_Builtin_Compare,                   // Builtin_CompareExec,
    nullptr,                                         // Builtin_IsValid,
    nullptr,                                         // Builtin_Select,

    nullptr,                                         // Builtin_Add,
    nullptr,                                         // Builtin_Subtract,
    nullptr,                                         // Builtin_Multiply,
    nullptr,                                         // Builtin_Divide,
    &FillUserData_Builtin_Expression,                // Builtin_Expression,

    nullptr,                                         // Builtin_ToBool,
    nullptr,                                         // Builtin_ToByte,
    nullptr,                                         // Builtin_ToInt,
    nullptr,                                         // Builtin_ToInt64,
    nullptr,                                         // Builtin_ToFloat,
    nullptr,                                         // Builtin_ToDouble,
    nullptr,                                         // Builtin_ToString,
    nullptr,                                         // Builtin_String_Format,
    nullptr,                                         // Builtin_ToHashedString,
    nullptr,                                         // Builtin_ToVariant,
    nullptr,                                         // Builtin_Variant_ConvertTo,

    nullptr,                                         // Builtin_MakeArray
    nullptr,                                         // Builtin_Array_GetElement,
    nullptr,                                         // Builtin_Array_SetElement,
    nullptr,                                         // Builtin_Array_GetCount,
    nullptr,                                         // Builtin_Array_IsEmpty,
    nullptr,                                         // Builtin_Array_Clear,
    nullptr,                                         // Builtin_Array_Contains,
    nullptr,                                         // Builtin_Array_IndexOf,
    nullptr,                                         // Builtin_Array_Insert,
    nullptr,                                         // Builtin_Array_PushBack,
    nullptr,                                         // Builtin_Array_Remove,
    nullptr,                                         // Builtin_Array_RemoveAt,

    &FillUserData_Builtin_TryGetComponentOfBaseType, // Builtin_TryGetComponentOfBaseType

    &FillUserData_Builtin_StartCoroutine,            // Builtin_StartCoroutine,
    nullptr,                                         // Builtin_StopCoroutine,
    nullptr,                                         // Builtin_StopAllCoroutines,
    nullptr,                                         // Builtin_WaitForAll,
    nullptr,                                         // Builtin_WaitForAny,
    nullptr,                                         // Builtin_Yield,

    nullptr,                                         // LastBuiltin,
  };

  static_assert(EZ_ARRAY_SIZE(s_TypeToFillUserDataFunctions) == ezVisualScriptNodeDescription::Type::Count);

  ezResult FillUserData(ezVisualScriptCompiler::AstNode& inout_astNode, ezVisualScriptCompiler* pCompiler, const ezDocumentObject* pObject, const ezDocumentObject* pEntryObject)
  {
    if (pObject == nullptr)
      return EZ_SUCCESS;

    auto nodeType = inout_astNode.m_Type;
    EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_TypeToFillUserDataFunctions), "Out of bounds access");
    auto func = s_TypeToFillUserDataFunctions[nodeType];

    if (func != nullptr)
    {
      EZ_SUCCEED_OR_RETURN(func(inout_astNode, pCompiler, pObject, pEntryObject));
    }

    return EZ_SUCCESS;
  }

} // namespace

//////////////////////////////////////////////////////////////////////////

ezVisualScriptCompiler::CompiledModule::CompiledModule()
  : m_ConstantDataStorage(ezSharedPtr<ezVisualScriptDataDescription>(&m_ConstantDataDesc, nullptr))
{
  // Prevent the data desc from being deleted by fake shared ptr above
  m_ConstantDataDesc.AddRef();
}

ezResult ezVisualScriptCompiler::CompiledModule::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_ASSERT_DEV(m_sScriptClassName.IsEmpty() == false, "Invalid script class name");

  ezStringDeduplicationWriteContext stringDedup(inout_stream);

  ezChunkStreamWriter chunk(stringDedup.Begin());
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Header", 1);
    chunk << m_sBaseClassName;
    chunk << m_sScriptClassName;
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("ConstantData", 1);
    EZ_SUCCEED_OR_RETURN(m_ConstantDataDesc.Serialize(chunk));
    EZ_SUCCEED_OR_RETURN(m_ConstantDataStorage.Serialize(chunk));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("InstanceData", 1);
    EZ_SUCCEED_OR_RETURN(m_InstanceDataDesc.Serialize(chunk));
    EZ_SUCCEED_OR_RETURN(chunk.WriteHashTable(m_InstanceDataMapping.m_Content));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    for (auto& function : m_Functions)
    {
      chunk << function.m_sName;
      chunk << function.m_Type;
      chunk << function.m_CoroutineCreationMode;

      EZ_SUCCEED_OR_RETURN(ezVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, function.m_LocalDataDesc, chunk));
    }

    chunk.EndChunk();
  }

  chunk.EndStream();

  return stringDedup.End();
}

//////////////////////////////////////////////////////////////////////////

// static
ezUInt32 ezVisualScriptCompiler::ConnectionHasher::Hash(const Connection& c)
{
  ezUInt32 uiHashes[] = {
    ezHashHelper<void*>::Hash(c.m_pSource),
    ezHashHelper<void*>::Hash(c.m_pTarget),
    ezHashHelper<ezUInt32>::Hash(c.m_Type),
    ezHashHelper<ezUInt32>::Hash(c.m_uiSourcePinIndex),
  };
  return ezHashingUtils::xxHash32(uiHashes, sizeof(uiHashes));
}

// static
bool ezVisualScriptCompiler::ConnectionHasher::Equal(const Connection& a, const Connection& b)
{
  return a.m_pSource == b.m_pSource &&
         a.m_pTarget == b.m_pTarget &&
         a.m_Type == b.m_Type &&
         a.m_uiSourcePinIndex == b.m_uiSourcePinIndex;
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptCompiler::ezVisualScriptCompiler() = default;
ezVisualScriptCompiler::~ezVisualScriptCompiler() = default;

void ezVisualScriptCompiler::InitModule(ezStringView sBaseClassName, ezStringView sScriptClassName)
{
  m_Module.m_sBaseClassName = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

ezResult ezVisualScriptCompiler::AddFunction(ezStringView sName, const ezDocumentObject* pEntryObject, const ezDocumentObject* pParentObject)
{
  if (m_pManager == nullptr)
  {
    m_pManager = static_cast<const ezVisualScriptNodeManager*>(pEntryObject->GetDocumentObjectManager());
  }
  EZ_ASSERT_DEV(m_pManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      ezLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return EZ_FAILURE;
    }
  }

  AstNode* pEntryAstNode = BuildAST(pEntryObject);
  if (pEntryAstNode == nullptr)
    return EZ_FAILURE;

  auto& function = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;
  function.m_Type = pEntryAstNode->m_Type;

  {
    auto pObjectWithCoroutineMode = pParentObject != nullptr ? pParentObject : pEntryObject;
    auto mode = pObjectWithCoroutineMode->GetTypeAccessor().GetValue("CoroutineMode");
    if (mode.IsA<ezInt64>())
    {
      function.m_CoroutineCreationMode = static_cast<ezScriptCoroutineCreationMode::Enum>(mode.Get<ezInt64>());
    }
    else
    {
      function.m_CoroutineCreationMode = ezScriptCoroutineCreationMode::AllowOverlap;
    }
  }

  m_EntryAstNodes.PushBack(pEntryAstNode);
  EZ_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryAstNodes.GetCount(), "");

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::Compile(ezStringView sDebugAstOutputPath)
{
  for (ezUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto& function = m_Module.m_Functions[i];
    AstNode* pEntryAstNode = m_EntryAstNodes[i];

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    EZ_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_Replaced");

    EZ_SUCCEED_OR_RETURN(InlineConstants(pEntryAstNode));
    EZ_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));
    EZ_SUCCEED_OR_RETURN(InlineVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_TypeConv");

    EZ_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_FlattenedExec");

    EZ_SUCCEED_OR_RETURN(FillDataOutputConnections(pEntryAstNode));
    EZ_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));
    EZ_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_PinIdToDataDesc.Clear();
  }

  EZ_SUCCEED_OR_RETURN(FinalizeConstantData());

  return EZ_SUCCESS;
}

ezUInt32 ezVisualScriptCompiler::GetPinId(const ezVisualScriptPin* pPin)
{
  ezUInt32 uiId = 0;
  if (pPin != nullptr && m_PinToId.TryGetValue(pPin, uiId))
    return uiId;

  uiId = m_uiNextPinId++;
  if (pPin != nullptr)
  {
    m_PinToId.Insert(pPin, uiId);
  }
  return uiId;
}

ezVisualScriptCompiler::DataOutput& ezVisualScriptCompiler::GetDataOutput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_Outputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_Outputs[dataInput.m_uiSourcePinIndex];
  }

  EZ_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

ezVisualScriptCompiler::AstNode& ezVisualScriptCompiler::CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, ezVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution)
{
  auto& node = m_AstNodes.ExpandAndGetRef();
  node.m_Type = type;
  node.m_DeductedDataType = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

void ezVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, ezUInt8 uiSourcePinIndex, ezVisualScriptDataType::Enum dataType)
{
  auto& dataInput = node.m_Inputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = pSourceNode;
  dataInput.m_uiId = GetPinId(nullptr);
  dataInput.m_uiSourcePinIndex = uiSourcePinIndex;
  dataInput.m_DataType = dataType;
}

void ezVisualScriptCompiler::AddDataOutput(AstNode& node, ezVisualScriptDataType::Enum dataType)
{
  auto& dataOutput = node.m_Outputs.ExpandAndGetRef();
  dataOutput.m_uiId = GetPinId(nullptr);
  dataOutput.m_DataType = dataType;
}

ezVisualScriptCompiler::DefaultInput ezVisualScriptCompiler::GetDefaultPointerInput(const ezRTTI* pDataType)
{
  DefaultInput defaultInput;
  if (m_DefaultInputs.TryGetValue(pDataType, defaultInput) == false)
  {
    if (pDataType == ezGetStaticRTTI<ezGameObject>() || pDataType == ezGetStaticRTTI<ezGameObjectHandle>())
    {
      auto& getOwnerNode = CreateAstNode(ezVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, ezVisualScriptDataType::TypedPointer);
      AddDataOutput(getOwnerNode, ezVisualScriptDataType::GameObject);

      defaultInput.m_pSourceNode = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 1;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
    else if (pDataType == ezGetStaticRTTI<ezWorld>())
    {
      auto& getOwnerNode = CreateAstNode(ezVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, ezVisualScriptDataType::TypedPointer);

      defaultInput.m_pSourceNode = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 0;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
  }

  return defaultInput;
}

ezVisualScriptCompiler::AstNode* ezVisualScriptCompiler::CreateConstantNode(const ezVariant& value)
{
  ezVisualScriptDataType::Enum valueDataType = ezVisualScriptDataType::FromVariantType(value.GetType());

  auto& constantNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Constant, valueDataType, true);
  constantNode.m_Value = value;

  AddDataOutput(constantNode, valueDataType);

  return &constantNode;
}

ezVisualScriptCompiler::AstNode* ezVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = ezUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return &jumpNode;
}

ezVisualScriptCompiler::DataOffset ezVisualScriptCompiler::GetInstanceDataOffset(ezHashedString sName, ezVisualScriptDataType::Enum dataType)
{
  ezVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    EZ_ASSERT_DEBUG(dataType < ezVisualScriptDataType::Count, "Invalid data type");
    auto& offsetAndCount = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType = dataType;
    instanceData.m_DataOffset.m_uiSource = DataOffset::Source::Instance;
    ++offsetAndCount.m_uiCount;

    m_pManager->GetVariableDefaultValue(sName, instanceData.m_DefaultValue).AssertSuccess();

    m_Module.m_InstanceDataMapping.m_Content.Insert(sName, instanceData);
  }

  return instanceData.m_DataOffset;
}

ezVisualScriptCompiler::AstNode* ezVisualScriptCompiler::BuildAST(const ezDocumentObject* pEntryObject)
{
  m_DefaultInputs.Clear();

  ezHashTable<const ezDocumentObject*, AstNode*> objectToAstNode;
  ezHybridArray<const ezVisualScriptPin*, 16> pins;

  auto CreateAstNodeFromObject = [&](const ezDocumentObject* pObject) -> AstNode*
  {
    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    EZ_ASSERT_DEV(pNodeDesc != nullptr, "Invalid node type");

    auto& astNode = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
    if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
      return nullptr;

    objectToAstNode.Insert(pObject, &astNode);

    return &astNode;
  };

  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (ezVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(ezVisualScriptNodeDescription::Type::EntryCall);
    astNode.m_Next.PushBack(pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  ezHybridArray<const ezDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryObject);

  while (nodeStack.IsEmpty() == false)
  {
    const ezDocumentObject* pObject = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pAstNode = nullptr;
    EZ_VERIFY(objectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (ezVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_pManager->GetInputDataPins(pObject, pins);
    ezUInt32 uiNextInputPinIndex = 0;

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.IsExecutionPin())
        continue;

      AstNode* pAstNodeToAddInput = pAstNode;
      bool bArrayInput = false;
      if (pinDesc.m_bReplaceWithArray && pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
      {
        const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
        if (pProp == nullptr)
          return nullptr;

        if (pProp->GetCategory() == ezPropertyCategory::Array)
        {
          auto pMakeArrayAstNode = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          AddDataOutput(*pMakeArrayAstNode, ezVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, ezVisualScriptDataType::Array);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        if (pPin->GetDynamicPinProperty() != pinDesc.m_sDynamicPinProperty)
          break;

        auto connections = m_pManager->GetConnections(*pPin);
        if (pPin->IsRequired() && connections.IsEmpty())
        {
          ezLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return nullptr;
        }

        ezVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == ezVisualScriptDataType::Invalid)
        {
          ezLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return nullptr;
        }

        auto& dataInput = pAstNodeToAddInput->m_Inputs.ExpandAndGetRef();
        dataInput.m_uiId = GetPinId(pPin);
        dataInput.m_DataType = bArrayInput ? ezVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (ezVisualScriptDataType::IsPointer(dataInput.m_DataType))
          {
            auto defaultInput = GetDefaultPointerInput(pPin->GetDataType());
            if (defaultInput.m_pSourceNode != nullptr)
            {
              dataInput.m_pSourceNode = defaultInput.m_pSourceNode;
              dataInput.m_uiSourcePinIndex = defaultInput.m_uiSourcePinIndex;
            }
          }
          else
          {
            ezStringView sPropertyName = pPin->HasDynamicPinProperty() ? pPin->GetDynamicPinProperty() : pPin->GetName();

            ezVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);
            if (value.IsValid() && pPin->HasDynamicPinProperty())
            {
              EZ_ASSERT_DEBUG(value.IsA<ezVariantArray>(), "Implementation error");
              value = value.Get<ezVariantArray>()[pPin->GetElementIndex()];
            }

            if (value.IsA<ezUuid>())
            {
              value = 0;
            }

            ezVisualScriptDataType::Enum valueDataType = ezVisualScriptDataType::FromVariantType(value.GetType());
            if (dataInput.m_DataType != ezVisualScriptDataType::Variant)
            {
              value = value.ConvertTo(ezVisualScriptDataType::GetVariantType(dataInput.m_DataType));
              if (value.IsValid() == false)
              {
                ezLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), ezVisualScriptDataType::GetName(valueDataType), ezVisualScriptDataType::GetName(dataInput.m_DataType));
                return nullptr;
              }
            }

            dataInput.m_pSourceNode = CreateConstantNode(value);
            dataInput.m_uiSourcePinIndex = 0;
          }
        }
        else
        {
          auto& sourcePin = static_cast<const ezVisualScriptPin&>(connections[0]->GetSourcePin());
          const ezDocumentObject* pSourceObject = sourcePin.GetParent();

          AstNode* pSourceAstNode;
          if (objectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
          {
            pSourceAstNode = CreateAstNodeFromObject(pSourceObject);
            if (pSourceAstNode == nullptr)
              return nullptr;

            nodeStack.PushBack(pSourceObject);
          }

          ezVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
          if (sourceDataType == ezVisualScriptDataType::Invalid)
          {
            ezLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
            return nullptr;
          }

          if (sourcePin.CanConvertTo(*pPin) == false)
          {
            ezLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
            return nullptr;
          }

          dataInput.m_pSourceNode = pSourceAstNode;
          dataInput.m_uiSourcePinIndex = sourcePin.GetDataPinIndex();
        }

        ++uiNextInputPinIndex;
      }
    }

    m_pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& dataOutput = pAstNode->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = GetPinId(pPin);
      dataOutput.m_DataType = FinalizeDataType(pPin->GetResolvedScriptDataType());
    }

    m_pManager->GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_pManager->GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_Next.PushBack(nullptr);
        continue;
      }

      EZ_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const ezDocumentObject* pNextNode = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (objectToAstNode.TryGetValue(pNextNode, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextNode);
        if (pNextAstNode == nullptr)
          return nullptr;

        nodeStack.PushBack(pNextNode);
      }

      pAstNode->m_Next.PushBack(pNextAstNode);
    }
  }

  return pEntryAstNode;
}

void ezVisualScriptCompiler::MarkAsCoroutine(AstNode* pEntryAstNode)
{
  switch (pEntryAstNode->m_Type)
  {
    case ezVisualScriptNodeDescription::Type::EntryCall:
      pEntryAstNode->m_Type = ezVisualScriptNodeDescription::Type::EntryCall_Coroutine;
      break;
    case ezVisualScriptNodeDescription::Type::MessageHandler:
      pEntryAstNode->m_Type = ezVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
      break;
    case ezVisualScriptNodeDescription::Type::EntryCall_Coroutine:
    case ezVisualScriptNodeDescription::Type::MessageHandler_Coroutine:
      // Already a coroutine
      break;
      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

ezResult ezVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  EZ_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
    [&](Connection& connection)
    {
      AstNode* pNode = connection.m_pTarget;

      if (ezVisualScriptNodeDescription::Type::IsLoop(pNode->m_Type))
      {
        if (ReplaceLoop(connection).Failed())
          return VisitorResult::Error;
      }

      return VisitorResult::Continue;
    }));

  return TraverseExecutionConnections(pEntryAstNode,
    [&](Connection& connection)
    {
      AstNode* pNode = connection.m_pTarget;

      if (pNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_CompareExec)
      {
        const auto& dataInputA = pNode->m_Inputs[0];
        const auto& dataInputB = pNode->m_Inputs[1];

        auto& compareNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, pNode->m_DeductedDataType, false);
        compareNode.m_Value = pNode->m_Value;
        AddDataInput(compareNode, dataInputA.m_pSourceNode, dataInputA.m_uiSourcePinIndex, dataInputA.m_DataType);
        AddDataInput(compareNode, dataInputB.m_pSourceNode, dataInputB.m_uiSourcePinIndex, dataInputB.m_DataType);
        AddDataOutput(compareNode, ezVisualScriptDataType::Bool);

        auto& branchNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Branch);
        AddDataInput(branchNode, &compareNode, 0, ezVisualScriptDataType::Bool);
        branchNode.m_Next.PushBack(pNode->m_Next[0]);
        branchNode.m_Next.PushBack(pNode->m_Next[1]);

        compareNode.m_Next.PushBack(&branchNode);
        EZ_ASSERT_DEBUG(connection.m_pSource != nullptr, "");
        connection.m_pSource->m_Next[connection.m_uiSourcePinIndex] = &compareNode;
        connection.m_pTarget = &branchNode;
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::ReplaceLoop(Connection& connection)
{
  AstNode* pLoopInitStart = nullptr;
  AstNode* pLoopInitEnd = nullptr;
  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd = nullptr;
  AstNode* pLoopIncrementStart = nullptr;
  AstNode* pLoopIncrementEnd = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex = nullptr;

  AstNode* pLoopNode = connection.m_pTarget;
  AstNode* pLoopBody = pLoopNode->m_Next[0];
  AstNode* pLoopCompleted = pLoopNode->m_Next[1];
  auto loopType = pLoopNode->m_Type;

  if (loopType == ezVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    pLoopConditionEnd = pLoopNode->m_Inputs[0].m_pSourceNode;
    pLoopConditionEnd->m_bImplicitExecution = false;

    EZ_SUCCEED_OR_RETURN(InlineConstants(pLoopConditionEnd));
    EZ_SUCCEED_OR_RETURN(InsertTypeConversions(pLoopConditionEnd));
    EZ_SUCCEED_OR_RETURN(InlineVariables(pLoopConditionEnd));

    ezHybridArray<AstNode*, 64> nodeStack;
    EZ_SUCCEED_OR_RETURN(BuildDataStack(pLoopConditionEnd, nodeStack));

    if (nodeStack.IsEmpty())
    {
      pLoopConditionStart = pLoopConditionEnd;
    }
    else
    {
      for (auto pDataNode : nodeStack)
      {
        pDataNode->m_bImplicitExecution = false;
      }

      pLoopConditionStart = nodeStack.PeekBack();

      AstNode* pLastDataNode = nodeStack[0];
      pLastDataNode->m_Next.PushBack(pLoopConditionEnd);
    }
  }
  else if (loopType == ezVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_Inputs[0];
    auto& lastIndexInput = pLoopNode->m_Inputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_ToInt, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, firstIndexInput.m_pSourceNode, firstIndexInput.m_uiSourcePinIndex, firstIndexInput.m_DataType);
      AddDataOutput(*pLoopInitStart, ezVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, ezVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = ezInt64(ezComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, lastIndexInput.m_pSourceNode, lastIndexInput.m_uiSourcePinIndex, lastIndexInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrementStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Add, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, ezVisualScriptDataType::Int);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = ezVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }
  }
  else if (loopType == ezVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse = (loopType == ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto& arrayInput = pLoopNode->m_Inputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopInitStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopInitStart, ezVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Subtract, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, CreateConstantNode(1), 0, ezVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitEnd, ezVisualScriptDataType::Int);

      pLoopInitStart->m_Next.PushBack(pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_ToInt, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, CreateConstantNode(0), 0, ezVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitStart, ezVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    if (isReverse)
    {
      pLoopConditionStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, ezVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = ezInt64(ezComparisonOperator::GreaterEqual);
      AddDataInput(*pLoopConditionStart, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, CreateConstantNode(0), 0, ezVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopConditionStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Int);

      pLoopConditionEnd = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, ezVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = ezInt64(ezComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, ezVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, ezVisualScriptDataType::Bool);

      pLoopConditionStart->m_Next.PushBack(pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? ezVisualScriptNodeDescription::Type::Builtin_Subtract : ezVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrementStart = &CreateAstNode(incType, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, ezVisualScriptDataType::Int);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      AddDataInput(*pLoopIncrementStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = ezVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }

    pLoopElement = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetElement, ezVisualScriptDataType::Invalid, true);
    AddDataInput(*pLoopElement, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
    AddDataInput(*pLoopElement, pLoopIndex, 0, ezVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, ezVisualScriptDataType::Variant);
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  pLoopNode->m_Inputs.Clear();
  pLoopNode->m_Next.Clear();

  {
    auto& branchNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Branch);
    AddDataInput(branchNode, pLoopConditionEnd, 0, ezVisualScriptDataType::Bool);

    if (pLoopConditionStart->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Constant)
    {
      pLoopConditionStart = &branchNode;
    }
    else
    {
      pLoopConditionEnd->m_bImplicitExecution = false;
      pLoopConditionEnd->m_Next.PushBack(&branchNode);
    }

    branchNode.m_Next.PushBack(pLoopBody);      // True -> LoopBody
    branchNode.m_Next.PushBack(pLoopCompleted); // False -> Completed
    pLoopConditionEnd = &branchNode;
  }

  if (pLoopInitStart != nullptr)
  {
    connection.m_pSource->m_Next[connection.m_uiSourcePinIndex] = pLoopInitStart;
    pLoopInitEnd->m_Next.PushBack(pLoopConditionStart);
  }
  else
  {
    connection.m_pSource->m_Next[connection.m_uiSourcePinIndex] = pLoopConditionStart;
  }

  AstNode* pJumpNode = CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrementStart != nullptr)
  {
    pLoopIncrementEnd->m_Next.PushBack(pJumpNode);
    pJumpNode = pLoopIncrementStart;
  }

  EZ_SUCCEED_OR_RETURN(TraverseAllConnections(pLoopBody,
    [&](Connection& connection)
    {
      if (connection.m_pSource == nullptr)
      {
        connection.m_pSource = pLoopConditionEnd;
        connection.m_uiSourcePinIndex = 0;
      }

      if (ezVisualScriptNodeDescription::Type::IsLoop(connection.m_pTarget->m_Type))
      {
        if (ReplaceLoop(connection).Failed())
          return VisitorResult::Error;
      }

      if (connection.m_Type == ConnectionType::Data && connection.m_pTarget->m_bImplicitExecution == false)
        return VisitorResult::Skip;

      AstNode* pNode = connection.m_pTarget;

      if (pNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Break)
      {
        connection.m_pSource->m_Next[connection.m_uiSourcePinIndex] = pLoopCompleted;
        return VisitorResult::Continue;
      }

      for (auto& pNext : pNode->m_Next)
      {
        if (pNext == nullptr)
        {
          pNext = pJumpNode;
        }
      }

      for (auto& dataInput : pNode->m_Inputs)
      {
        if (loopType == ezVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
            loopType == ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
        {
          if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
          {
            dataInput.m_pSourceNode = pLoopElement;
          }
          else if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 1)
          {
            dataInput.m_pSourceNode = pLoopIndex;
            dataInput.m_uiSourcePinIndex = 0;
          }
        }
        else
        {
          if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
          {
            dataInput.m_pSourceNode = pLoopIndex;
          }
        }
      }

      return VisitorResult::Continue;
    }));

  connection.m_pSource = pLoopConditionStart;
  connection.m_pTarget = pLoopConditionEnd;
  connection.m_uiSourcePinIndex = 0;

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_Type == ConnectionType::Data)
      {
        auto& dataInput = connection.m_pSource->m_Inputs[connection.m_uiSourcePinIndex];
        auto& dataOutput = GetDataOutput(dataInput);

        if (dataOutput.m_DataType != dataInput.m_DataType)
        {
          auto nodeType = ezVisualScriptNodeDescription::Type::GetConversionType(dataInput.m_DataType);

          auto& astNode = CreateAstNode(nodeType, dataOutput.m_DataType, true);
          AddDataInput(astNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, dataOutput.m_DataType);
          AddDataOutput(astNode, dataInput.m_DataType);

          dataInput.m_pSourceNode = &astNode;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::InlineConstants(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      auto pCurrentNode = connection.m_pTarget;
      for (auto& dataInput : pCurrentNode->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        if (pSourceNode == nullptr)
          continue;

        if (pSourceNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Constant)
        {
          auto dataType = pSourceNode->m_DeductedDataType;

          ezUInt32 uiIndex = ezInvalidIndex;
          if (m_ConstantDataToIndex.TryGetValue(pSourceNode->m_Value, uiIndex) == false)
          {
            auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
            uiIndex = offsetAndCount.m_uiCount;
            ++offsetAndCount.m_uiCount;

            m_ConstantDataToIndex.Insert(pSourceNode->m_Value, uiIndex);
          }

          DataDesc dataDesc;
          dataDesc.m_DataOffset = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
          m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::InlineVariables(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      auto pCurrentNode = connection.m_pTarget;
      for (auto& dataInput : pCurrentNode->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        if (pSourceNode == nullptr)
          continue;

        if (pSourceNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_GetVariable)
        {
          auto& dataOutput = pSourceNode->m_Outputs[0];

          ezHashedString sName;
          sName.Assign(pSourceNode->m_Value.Get<ezString>());

          DataDesc dataDesc;
          dataDesc.m_DataOffset = GetInstanceDataOffset(sName, dataOutput.m_DataType);
          m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      if (pCurrentNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_SetVariable ||
          pCurrentNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_IncVariable ||
          pCurrentNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_DecVariable)
      {
        ezHashedString sName;
        sName.Assign(pCurrentNode->m_Value.Get<ezString>());

        DataDesc dataDesc;
        dataDesc.m_DataOffset = GetInstanceDataOffset(sName, pCurrentNode->m_DeductedDataType);

        if (pCurrentNode->m_Type != ezVisualScriptNodeDescription::Type::Builtin_SetVariable)
        {
          if (pCurrentNode->m_Inputs.IsEmpty())
          {
            AddDataInput(*pCurrentNode, nullptr, 0, pCurrentNode->m_DeductedDataType);
          }

          m_PinIdToDataDesc.Insert(pCurrentNode->m_Inputs[0].m_uiId, dataDesc);
        }

        {
          if (pCurrentNode->m_Outputs.IsEmpty())
          {
            AddDataOutput(*pCurrentNode, pCurrentNode->m_DeductedDataType);
          }

          m_PinIdToDataDesc.Insert(pCurrentNode->m_Outputs[0].m_uiId, dataDesc);
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, ezDynamicArray<AstNode*>& out_Stack)
{
  ezHashSet<const AstNode*> visitedNodes;
  out_Stack.Clear();

  EZ_SUCCEED_OR_RETURN(TraverseDataConnections(
    pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_pTarget->m_bImplicitExecution == false)
        return VisitorResult::Skip;

      if (visitedNodes.Insert(connection.m_pTarget))
      {
        // If the node was already visited, remove it again so it is moved to the top of the stack
        out_Stack.RemoveAndCopy(connection.m_pTarget);
      }

      out_Stack.PushBack(connection.m_pTarget);

      return VisitorResult::Continue;
    },
    false));

  // Make unique
  ezHashTable<AstNode*, AstNode*> oldToNewNodes;
  for (ezUInt32 i = out_Stack.GetCount(); i > 0; --i)
  {
    auto& pDataNode = out_Stack[i - 1];

    if (pDataNode->m_Next.IsEmpty())
    {
      // remap inputs to new nodes
      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pNewNode = nullptr;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pNewNode))
        {
          dataInput.m_pSourceNode = pNewNode;
        }
      }
    }
    else
    {
      auto& newDataNode = CreateAstNode(pDataNode->m_Type, pDataNode->m_DeductedDataType, pDataNode->m_bImplicitExecution);
      newDataNode.m_sTargetTypeName = pDataNode->m_sTargetTypeName;
      newDataNode.m_Value = pDataNode->m_Value;

      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pSourceNode = dataInput.m_pSourceNode;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pSourceNode) == false)
        {
          EZ_ASSERT_DEBUG(dataInput.m_pSourceNode == nullptr || dataInput.m_pSourceNode->m_bImplicitExecution == false, "");
        }

        AddDataInput(newDataNode, pSourceNode, dataInput.m_uiSourcePinIndex, dataInput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Inputs.PeekBack().m_uiId, dataDesc);
        }
      }

      for (auto& dataOutput : pDataNode->m_Outputs)
      {
        AddDataOutput(newDataNode, dataOutput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Outputs.PeekBack().m_uiId, dataDesc);
        }
      }

      oldToNewNodes.Insert(pDataNode, &newDataNode);
      pDataNode = &newDataNode;
    }
  }

  // Connect next execution
  if (out_Stack.GetCount() > 1)
  {
    AstNode* pLastDataNode = out_Stack.PeekBack();
    for (ezUInt32 i = out_Stack.GetCount() - 1; i > 0; --i)
    {
      auto& pDataNode = out_Stack[i - 1];
      pLastDataNode->m_Next.PushBack(pDataNode);
      pLastDataNode = pDataNode;
    }
  }

  // Remap inputs
  for (auto& dataInput : pEntryAstNode->m_Inputs)
  {
    if (dataInput.m_pSourceNode != nullptr)
    {
      oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, dataInput.m_pSourceNode);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  ezHybridArray<Connection, 64> allExecConnections;

  EZ_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      allExecConnections.PushBack(connection);
      return VisitorResult::Continue;
    }));

  ezHybridArray<AstNode*, 64> nodeStack;
  ezHashTable<AstNode*, AstNode*> nodeToFirstDataNode;

  for (const auto& connection : allExecConnections)
  {
    AstNode* pFirstDataNode = nullptr;
    if (nodeToFirstDataNode.TryGetValue(connection.m_pTarget, pFirstDataNode) == false)
    {
      if (BuildDataStack(connection.m_pTarget, nodeStack).Failed())
        return EZ_FAILURE;

      if (nodeStack.IsEmpty() == false)
      {
        pFirstDataNode = nodeStack.PeekBack();

        AstNode* pLastDataNode = nodeStack[0];
        pLastDataNode->m_Next.PushBack(connection.m_pTarget);
      }
    }

    if (pFirstDataNode != nullptr)
    {
      connection.m_pSource->m_Next[connection.m_uiSourcePinIndex] = pFirstDataNode;
    }
    nodeToFirstDataNode.Insert(connection.m_pTarget, pFirstDataNode);
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::FillDataOutputConnections(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_Type == ConnectionType::Data)
      {
        auto& dataInput = connection.m_pSource->m_Inputs[connection.m_uiSourcePinIndex];
        auto& dataOutput = GetDataOutput(dataInput);

        EZ_ASSERT_DEBUG(dataInput.m_pSourceNode == connection.m_pTarget, "");
        if (dataOutput.m_TargetNodes.Contains(connection.m_pSource) == false)
        {
          dataOutput.m_TargetNodes.PushBack(connection.m_pSource);
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, ezVisualScriptDataDescription& inout_localDataDesc)
{
  ezDynamicArray<DataOffset> freeDataOffsets;

  return TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      // Outputs first so we don't end up using the same data as input and output
      for (auto& dataOutput : connection.m_pTarget->m_Outputs)
      {
        if (m_PinIdToDataDesc.Contains(dataOutput.m_uiId))
          continue;

        if (dataOutput.m_TargetNodes.IsEmpty() == false)
        {
          DataOffset dataOffset;
          dataOffset.m_uiType = dataOutput.m_DataType;

          for (ezUInt32 i = 0; i < freeDataOffsets.GetCount(); ++i)
          {
            auto freeDataOffset = freeDataOffsets[i];
            if (freeDataOffset.m_uiType == dataOffset.m_uiType)
            {
              dataOffset = freeDataOffset;
              freeDataOffsets.RemoveAtAndSwap(i);
              break;
            }
          }

          if (dataOffset.IsValid() == false)
          {
            EZ_ASSERT_DEBUG(dataOffset.GetType() < ezVisualScriptDataType::Count, "Invalid data type");
            auto& offsetAndCount = inout_localDataDesc.m_PerTypeInfo[dataOffset.m_uiType];
            dataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
            ++offsetAndCount.m_uiCount;
          }

          DataDesc dataDesc;
          dataDesc.m_DataOffset = dataOffset;
          dataDesc.m_uiUsageCounter = dataOutput.m_TargetNodes.GetCount();
          m_PinIdToDataDesc.Insert(dataOutput.m_uiId, dataDesc);
        }
      }

      for (auto& dataInput : connection.m_pTarget->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId) || dataInput.m_pSourceNode == nullptr)
          continue;

        auto& dataOutput = GetDataOutput(dataInput);
        DataDesc* pDataDesc = nullptr;
        m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, pDataDesc);
        if (pDataDesc == nullptr)
        {
          ezLog::Error("Internal Compiler Error: Local variable for output id {} is not yet assigned.", dataOutput.m_uiId);
          return VisitorResult::Error;
        }

        --pDataDesc->m_uiUsageCounter;
        if (pDataDesc->m_uiUsageCounter == 0 && pDataDesc->m_DataOffset.IsLocal())
        {
          freeDataOffsets.PushBack(pDataDesc->m_DataOffset);
        }

        // Make a copy first because Insert() might re-allocate and the pointer might point to dead memory afterwards.
        DataDesc dataDesc = *pDataDesc;
        m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::BuildNodeDescriptions(AstNode* pEntryAstNode, ezDynamicArray<ezVisualScriptNodeDescription>& out_NodeDescriptions)
{
  ezHashTable<const AstNode*, ezUInt32> astNodeToNodeDescIndices;
  out_NodeDescriptions.Clear();

  auto CreateNodeDesc = [&](const AstNode& astNode, ezUInt32& out_uiNodeDescIndex) -> ezResult
  {
    out_uiNodeDescIndex = out_NodeDescriptions.GetCount();

    auto& nodeDesc = out_NodeDescriptions.ExpandAndGetRef();
    nodeDesc.m_Type = astNode.m_Type;
    nodeDesc.m_DeductedDataType = astNode.m_DeductedDataType;
    nodeDesc.m_sTargetTypeName = astNode.m_sTargetTypeName;
    nodeDesc.m_Value = astNode.m_Value;

    for (auto& dataInput : astNode.m_Inputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc);
      nodeDesc.m_InputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_Outputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc);
      nodeDesc.m_OutputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex);
    return EZ_SUCCESS;
  };

  ezUInt32 uiNodeDescIndex = 0;
  EZ_SUCCEED_OR_RETURN(CreateNodeDesc(*pEntryAstNode, uiNodeDescIndex));

  return TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      ezUInt32 uiCurrentIndex = 0;
      if (astNodeToNodeDescIndices.TryGetValue(connection.m_pTarget, uiCurrentIndex) == false)
      {
        return VisitorResult::Skip;
      }

      auto pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
      if (pNodeDesc->m_ExecutionIndices.GetCount() == connection.m_pTarget->m_Next.GetCount())
      {
        return VisitorResult::Continue;
      }

      for (auto pNextAstNode : connection.m_pTarget->m_Next)
      {
        if (pNextAstNode == nullptr)
        {
          pNodeDesc->m_ExecutionIndices.PushBack(static_cast<ezUInt16>(ezInvalidIndex));
        }
        else if (pNextAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Jump)
        {
          ezUInt64 uiPtr = pNextAstNode->m_Value.Get<ezUInt64>();
          AstNode* pTargetAstNode = *reinterpret_cast<AstNode**>(&uiPtr);

          ezUInt32 uiNextIndex = 0;
          if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiNextIndex) == false)
            return VisitorResult::Error;

          pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
        }
        else
        {
          ezUInt32 uiNextIndex = 0;
          if (astNodeToNodeDescIndices.TryGetValue(pNextAstNode, uiNextIndex) == false)
          {
            if (CreateNodeDesc(*pNextAstNode, uiNextIndex).Failed())
              return VisitorResult::Error;

            // array might have been resized, fetch node desc again
            pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
          }

          pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  m_ReportedConnections.Clear();
  ezHybridArray<AstNode*, 64> nodeStack;

  {
    Connection connection = {nullptr, pEntryAstNode, ConnectionType::Execution, ezInvalidIndex};
    auto res = func(connection);
    if (res == VisitorResult::Skip || res == VisitorResult::Stop)
      return EZ_SUCCESS;
    if (res == VisitorResult::Error)
      return EZ_FAILURE;

    if (connection.m_pTarget != nullptr)
    {
      nodeStack.PushBack(connection.m_pTarget);
    }
  }

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pSourceAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (ezUInt32 i = 0; i < pSourceAstNode->m_Next.GetCount(); ++i)
    {
      auto pTargetAstNode = pSourceAstNode->m_Next[i];
      EZ_ASSERT_DEBUG(pTargetAstNode != pSourceAstNode, "");

      if (pTargetAstNode == nullptr)
        continue;

      Connection connection = {pSourceAstNode, pTargetAstNode, ConnectionType::Execution, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return EZ_SUCCESS;
      if (res == VisitorResult::Error)
        return EZ_FAILURE;

      if (connection.m_pTarget != nullptr)
      {
        nodeStack.PushBack(connection.m_pTarget);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/, bool bClearReportedConnections /*= true*/)
{
  if (bClearReportedConnections)
  {
    m_ReportedConnections.Clear();
  }

  ezHybridArray<AstNode*, 64> nodeStack;

  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (ezUInt32 i = 0; i < pCurrentAstNode->m_Inputs.GetCount(); ++i)
    {
      auto& dataInput = pCurrentAstNode->m_Inputs[i];

      if (dataInput.m_pSourceNode == nullptr)
        continue;

      Connection connection = {pCurrentAstNode, dataInput.m_pSourceNode, ConnectionType::Data, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return EZ_SUCCESS;
      if (res == VisitorResult::Error)
        return EZ_FAILURE;

      if (connection.m_pTarget != nullptr)
      {
        nodeStack.PushBack(connection.m_pTarget);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  return TraverseExecutionConnections(
    pEntryAstNode,
    [&](Connection& connection)
    {
      auto res = func(connection);
      if (res != VisitorResult::Continue)
        return res;

      if (TraverseDataConnections(connection.m_pTarget, func, bDeduplicate, false).Failed())
        return VisitorResult::Error;

      return VisitorResult::Continue;
    },
    bDeduplicate);
}

ezResult ezVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataStorage.AllocateStorage(ezFoundation::GetDefaultAllocator());

  for (auto& it : m_ConstantDataToIndex)
  {
    const ezVariant& value = it.Key();
    ezUInt32 uiIndex = it.Value();

    auto scriptDataType = ezVisualScriptDataType::FromVariantType(value.GetType());
    if (scriptDataType == ezVisualScriptDataType::Invalid)
    {
      scriptDataType = ezVisualScriptDataType::Variant;
    }

    auto dataOffset = m_Module.m_ConstantDataDesc.GetOffset(scriptDataType, uiIndex, DataOffset::Source::Constant);

    m_Module.m_ConstantDataStorage.SetDataFromVariant(dataOffset, value, 0);
  }

  return EZ_SUCCESS;
}

void ezVisualScriptCompiler::DumpAST(AstNode* pEntryAstNode, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  ezDGMLGraph dgmlGraph;
  {
    ezHashTable<const AstNode*, ezUInt32> nodeCache;
    ezHashTable<ezUInt64, ezString> connectionCache;
    ezStringBuilder sb;

    TraverseAllConnections(pEntryAstNode,
      [&](const Connection& connection)
      {
        AstNode* pAstNode = connection.m_pTarget;

        ezUInt32 uiGraphNode = 0;
        if (nodeCache.TryGetValue(pAstNode, uiGraphNode) == false)
        {
          const char* szTypeName = ezVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
          sb = szTypeName;
          if (pAstNode->m_sTargetTypeName.IsEmpty() == false)
          {
            sb.Append("\n", pAstNode->m_sTargetTypeName);
          }
          if (pAstNode->m_DeductedDataType != ezVisualScriptDataType::Invalid)
          {
            sb.Append("\nDataType: ", ezVisualScriptDataType::GetName(pAstNode->m_DeductedDataType));
          }
          sb.AppendFormat("\nImplicitExec: {}", pAstNode->m_bImplicitExecution);
          if (pAstNode->m_Value.IsValid())
          {
            sb.AppendFormat("\nValue: {}", pAstNode->m_Value);
          }

          float colorX = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(ezHashingUtils::StringHash(szTypeName))).x();

          ezDGMLGraph::NodeDesc nd;
          nd.m_Color = ezColorScheme::LightUI(colorX);
          uiGraphNode = dgmlGraph.AddNode(sb, &nd);
          nodeCache.Insert(pAstNode, uiGraphNode);
        }

        if (connection.m_pSource != nullptr)
        {
          ezUInt32 uiPrevGraphNode = 0;
          EZ_VERIFY(nodeCache.TryGetValue(connection.m_pSource, uiPrevGraphNode), "");

          if (connection.m_Type == ConnectionType::Execution)
          {
            ezUInt64 uiConnectionKey = uiPrevGraphNode | ezUInt64(uiGraphNode) << 32;
            ezString& sLabel = connectionCache[uiConnectionKey];

            ezStringBuilder sb = sLabel;
            if (sb.IsEmpty() == false)
            {
              sb.Append(" + ");
            }
            sb.AppendFormat("Exec{}", connection.m_uiSourcePinIndex);
            sLabel = sb;
          }
          else
          {
            ezUInt64 uiConnectionKey = uiGraphNode | ezUInt64(uiPrevGraphNode) << 32;
            ezString& sLabel = connectionCache[uiConnectionKey];

            auto& dataInput = connection.m_pSource->m_Inputs[connection.m_uiSourcePinIndex];
            auto& dataOutput = GetDataOutput(dataInput);

            ezStringBuilder sb = sLabel;
            if (sb.IsEmpty() == false)
            {
              sb.Append(" + ");
            }
            sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, ezVisualScriptDataType::GetName(dataOutput.m_DataType), dataOutput.m_uiId, connection.m_uiSourcePinIndex, ezVisualScriptDataType::GetName(dataInput.m_DataType), dataInput.m_uiId);
            sLabel = sb;
          }
        }

        return VisitorResult::Continue;
      })
      .IgnoreResult();

    for (auto& it : connectionCache)
    {
      ezUInt32 uiSource = it.Key() & 0xFFFFFFFF;
      ezUInt32 uiTarget = it.Key() >> 32;

      dgmlGraph.AddConnection(uiSource, uiTarget, it.Value());
    }
  }

  ezStringView sExt = sOutputPath.GetFileExtension();
  ezStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  ezDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    ezLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    ezLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}

void ezVisualScriptCompiler::DumpGraph(ezArrayPtr<const ezVisualScriptNodeDescription> nodeDescriptions, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  ezDGMLGraph dgmlGraph;
  {
    ezStringBuilder sTmp;
    for (auto& nodeDesc : nodeDescriptions)
    {
      ezStringView sTypeName = ezVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), ezVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_ConstantDataToIndex)
          {
            auto scriptDataType = ezVisualScriptDataType::FromVariantType(it.Key().GetType());
            if (scriptDataType == dataOffset.GetType() && it.Value() == dataOffset.m_uiByteOffset)
            {
              sTmp.AppendFormat(" ({})", it.Key());
              break;
            }
          }
        }
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        sTmp.AppendFormat("\n Output {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), ezVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);
      }

      float colorX = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(ezHashingUtils::StringHash(sTypeName))).x();

      ezDGMLGraph::NodeDesc nd;
      nd.m_Color = ezColorScheme::LightUI(colorX);

      dgmlGraph.AddNode(sTmp, &nd);
    }

    for (ezUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      for (auto uiNextIndex : nodeDescriptions[i].m_ExecutionIndices)
      {
        if (uiNextIndex == ezSmallInvalidIndex)
          continue;

        dgmlGraph.AddConnection(i, uiNextIndex);
      }
    }
  }

  ezStringView sExt = sOutputPath.GetFileExtension();
  ezStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  ezDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    ezLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    ezLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}
