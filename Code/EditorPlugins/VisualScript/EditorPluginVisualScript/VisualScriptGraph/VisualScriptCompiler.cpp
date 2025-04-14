#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>
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

    nullptr,                                         // Builtin_Constant,
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

    // Reverse order so functions that are added later (e.g. coroutines) are loaded first
    for (ezUInt32 i = m_Functions.GetCount(); i-- > 0;)
    {
      auto& function = m_Functions[i];

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

ezVisualScriptCompiler::ezVisualScriptCompiler(ezVisualScriptNodeManager& ref_nodeManager)
  : m_NodeManager(ref_nodeManager)
{
}

ezVisualScriptCompiler::~ezVisualScriptCompiler() = default;

void ezVisualScriptCompiler::InitModule(ezStringView sBaseClassName, ezStringView sScriptClassName)
{
  m_Module.m_sBaseClassName = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

ezResult ezVisualScriptCompiler::AddFunction(ezStringView sName, const ezDocumentObject* pEntryObject, const ezDocumentObject* pParentObject)
{
  EZ_ASSERT_DEV(&m_NodeManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      ezLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return EZ_FAILURE;
    }
  }

  auto& function = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;

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

  m_EntryObjects.PushBack(pEntryObject);
  EZ_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryObjects.GetCount(), "");

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::Compile(ezStringView sDebugAstOutputPath)
{
  EZ_SUCCEED_OR_RETURN(BuildInstanceDataMapping());

  for (ezUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto& function = m_Module.m_Functions[i];
    const ezDocumentObject* pEntryObject = m_EntryObjects[i];

    AstNode* pEntryAstNode = BuildExecutionFlow(pEntryObject);
    if (pEntryAstNode == nullptr)
    {
      ezLog::Error("Failed to build execution flow for function '{}'", function.m_sName);
      return EZ_FAILURE;
    }

    function.m_Type = pEntryAstNode->m_Type;

    EZ_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    EZ_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_TypeConv");

    EZ_SUCCEED_OR_RETURN(AssignInstanceVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_InstanceVarsAssigned");

    EZ_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_Replaced");

    EZ_SUCCEED_OR_RETURN(CopyOutputsToInputs(pEntryAstNode));
    EZ_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_04_LocalVarsAssigned");

    EZ_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_CompilationState.Clear();
  }

  EZ_SUCCEED_OR_RETURN(FinalizeConstantData());

  return EZ_SUCCESS;
}

// Ast node creation
//////////////////////////////////////////////////////////////////////////

ezVisualScriptCompiler::AstNode& ezVisualScriptCompiler::CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, ezVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution /*= false*/)
{
  auto& node = m_AstNodes.ExpandAndGetRef();
  node.m_Type = type;
  node.m_DeductedDataType = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

ezVisualScriptCompiler::AstNode& ezVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = ezUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return jumpNode;
}

ezVisualScriptCompiler::AstNode* ezVisualScriptCompiler::CreateAstNodeFromObject(const ezDocumentObject* pObject, const ezVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const ezDocumentObject* pEntryObject, bool bImplicitOnly /*= false*/)
{
  if (pNodeDesc == nullptr)
  {
    pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  }
  EZ_ASSERT_DEV(pNodeDesc != nullptr && pNodeDesc->m_Type != ezVisualScriptNodeDescription::Type::GetScriptOwner, "Invalid node type");

  if (bImplicitOnly && !pNodeDesc->m_bImplicitExecution)
    return nullptr;

  auto& astNode = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(m_NodeManager.GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
  astNode.m_pObject = pObject;

  if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
    return nullptr;

  if (pNodeDesc->m_bImplicitExecution)
  {
    m_CompilationState.m_DataObjectToAstNode.Insert(pObject, &astNode);
  }
  else
  {
    m_CompilationState.m_ExecObjectToAstNode.Insert(pObject, &astNode);
  }

  return &astNode;
}

ezVisualScriptCompiler::DataInput ezVisualScriptCompiler::GetOrCreateDefaultPointerNode(const AstNode& node, const ezRTTI* pRtti)
{
  if (pRtti == nullptr)
  {
    pRtti = ezRTTI::FindTypeByNameHash(node.m_sTargetTypeName.GetHash());
  }

  const bool bIsGameObject = pRtti == ezGetStaticRTTI<ezGameObject>() || pRtti == ezGetStaticRTTI<ezGameObjectHandle>();
  const bool bIsWorld = pRtti == ezGetStaticRTTI<ezWorld>();

  if (bIsWorld || bIsGameObject)
  {
    DataInput dataInput;
    dataInput.m_pSourceNode = m_CompilationState.m_pGetScriptOwnerNode;
    dataInput.m_uiSourcePinIndex = bIsGameObject ? 1 : 0;
    dataInput.m_DataOffset = m_CompilationState.m_pGetScriptOwnerNode->m_DataOutputs[dataInput.m_uiSourcePinIndex].m_DataOffset;
    return dataInput;
  }

  return DataInput();
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

// Pins, inputs and outputs
//////////////////////////////////////////////////////////////////////////

void ezVisualScriptCompiler::AddConstantDataInput(AstNode& node, const ezVariant& value)
{
  ezVisualScriptDataType::Enum dataType = ezVisualScriptDataType::FromVariantType(value.GetType());

  ezUInt32 uiIndex = ezInvalidIndex;
  if (m_Module.m_ConstantDataToIndex.TryGetValue(value, uiIndex) == false)
  {
    auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
    uiIndex = offsetAndCount.m_uiCount;
    ++offsetAndCount.m_uiCount;

    m_Module.m_ConstantDataToIndex.Insert(value, uiIndex);
  }

  auto& dataInput = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = nullptr;
  dataInput.m_uiSourcePinIndex = 0;
  dataInput.m_DataOffset = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
}

ezResult ezVisualScriptCompiler::AddConstantDataInput(AstNode& node, const ezDocumentObject* pObject, const ezVisualScriptPin* pPin, ezVisualScriptDataType::Enum dataType)
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
  if (dataType != ezVisualScriptDataType::Variant)
  {
    value = value.ConvertTo(ezVisualScriptDataType::GetVariantType(dataType));
    if (value.IsValid() == false)
    {
      ezLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), ezVisualScriptDataType::GetName(valueDataType), ezVisualScriptDataType::GetName(dataType));
      return EZ_FAILURE;
    }
  }

  AddConstantDataInput(node, value);

  return EZ_SUCCESS;
}

void ezVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, ezUInt32 uiSourcePinIndex, ezVisualScriptDataType::Enum dataType)
{
  auto& dataInput = node.m_DataInputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = pSourceNode;
  dataInput.m_uiSourcePinIndex = uiSourcePinIndex;
  dataInput.m_DataOffset.m_uiType = dataType;
}

void ezVisualScriptCompiler::AddDataOutput(AstNode& node, ezVisualScriptDataType::Enum dataType)
{
  auto& dataOutput = node.m_DataOutputs.ExpandAndGetRef();
  dataOutput.m_DataOffset = DataOffset(m_CompilationState.m_uiNextLocalVarId++, dataType, DataOffset::Source::Local);
}

ezVisualScriptCompiler::DataOutput& ezVisualScriptCompiler::GetDataOutputFromInput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_DataOutputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_DataOutputs[dataInput.m_uiSourcePinIndex];
  }

  EZ_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

void ezVisualScriptCompiler::ConnectExecution(AstNode& sourceNode, AstNode& targetNode, ezUInt32 uiSourcePinIndex /*= ezInvalidIndex*/)
{
  if (uiSourcePinIndex == ezInvalidIndex)
  {
    // New connection
    ExecInput execInput;
    execInput.m_pSourceNode = &sourceNode;
    execInput.m_uiSourcePinIndex = sourceNode.m_ExecOutputs.GetCount();

    targetNode.m_ExecInputs.PushBack(execInput);

    ExecOutput execOutput;
    execOutput.m_pTargetNode = &targetNode;

    sourceNode.m_ExecOutputs.PushBack(execOutput);
  }
  else
  {
    ExecInput execInput;
    execInput.m_pSourceNode = &sourceNode;
    execInput.m_uiSourcePinIndex = uiSourcePinIndex;

    targetNode.m_ExecInputs.PushBack(execInput);

    sourceNode.m_ExecOutputs.EnsureCount(uiSourcePinIndex + 1);
    auto& execOutput = sourceNode.m_ExecOutputs[uiSourcePinIndex];
    execOutput.m_pTargetNode = &targetNode;
  }
}

void ezVisualScriptCompiler::DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, ezUInt32 uiSourcePinIndex)
{
  ExecInput execInputToRemove;
  execInputToRemove.m_pSourceNode = &sourceNode;
  execInputToRemove.m_uiSourcePinIndex = uiSourcePinIndex;
  EZ_VERIFY(targetNode.m_ExecInputs.RemoveAndCopy(execInputToRemove), "");

  sourceNode.m_ExecOutputs[uiSourcePinIndex].m_pTargetNode = nullptr;
}

void ezVisualScriptCompiler::ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  for (auto& execInput : node.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, firstNewNode, execInput.m_uiSourcePinIndex);
  }
  node.m_ExecInputs.Clear();

  ConnectExecution(lastNewNode, node);
}

void ezVisualScriptCompiler::ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode)
{
  EZ_ASSERT_DEV(node.m_ExecOutputs.GetCount() == 1, "This function only works for nodes with a single exec output pin");
  AstNode* pNodeAfter = node.m_ExecOutputs[0].m_pTargetNode;

  ConnectExecution(node, firstNewNode, 0);

  pNodeAfter->m_ExecInputs.Clear();
  ConnectExecution(lastNewNode, *pNodeAfter);
}

void ezVisualScriptCompiler::ReplaceExecution(AstNode& oldNode, AstNode& newNode)
{
  EZ_ASSERT_DEBUG(newNode.m_ExecInputs.IsEmpty() && newNode.m_ExecOutputs.IsEmpty(), "New node must not have any connections yet");

  for (auto& execInput : oldNode.m_ExecInputs)
  {
    ConnectExecution(*execInput.m_pSourceNode, newNode, execInput.m_uiSourcePinIndex);
  }
  oldNode.m_ExecInputs.Clear();

  for (ezUInt32 i = 0; i < oldNode.m_ExecOutputs.GetCount(); ++i)
  {
    AstNode* pTargetNode = oldNode.m_ExecOutputs[i].m_pTargetNode;
    if (pTargetNode != nullptr)
    {
      DisconnectExecution(oldNode, *pTargetNode, i);
      ConnectExecution(newNode, *pTargetNode, i);
    }
    else
    {
      newNode.m_ExecOutputs.PushBack({nullptr});
    }
  }
  oldNode.m_ExecOutputs.Clear();
}

ezVisualScriptCompiler::DataOffset ezVisualScriptCompiler::GetInstanceDataOffset(ezHashedString sName, ezVisualScriptDataType::Enum dataType)
{
  ezVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    ezLog::Error("Invalid variable named '{}'", sName);
    return DataOffset();
  }

  EZ_ASSERT_DEBUG(instanceData.m_DataOffset.m_uiType == dataType, "Data type mismatch");
  return instanceData.m_DataOffset;
}

// Compilation steps
//////////////////////////////////////////////////////////////////////////

ezResult ezVisualScriptCompiler::BuildInstanceDataMapping()
{
  ezHybridArray<ezVisualScriptVariable, 16> variables;
  m_NodeManager.GetAllVariables(variables);

  for (auto& variable : variables)
  {
    ezVisualScriptInstanceData instanceData;
    auto dataType = ezVisualScriptDataType::FromVariantType(variable.m_DefaultValue.GetType());

    auto& offsetAndCount = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType = dataType;
    instanceData.m_DataOffset.m_uiSource = DataOffset::Source::Instance;
    instanceData.m_DefaultValue = variable.m_DefaultValue;

    ++offsetAndCount.m_uiCount;

    m_Module.m_InstanceDataMapping.m_Content.Insert(variable.m_sName, instanceData);
  }

  return EZ_SUCCESS;
}

ezVisualScriptCompiler::AstNode* ezVisualScriptCompiler::BuildExecutionFlow(const ezDocumentObject* pEntryObject)
{
  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject, nullptr, pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (ezVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(ezVisualScriptNodeDescription::Type::EntryCall);
    ConnectExecution(astNode, *pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  ezHybridArray<const ezDocumentObject*, 64> objectStack;
  objectStack.PushBack(pEntryObject);

  while (objectStack.IsEmpty() == false)
  {
    const ezDocumentObject* pObject = objectStack.PeekBack();
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    EZ_VERIFY(m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (ezVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_NodeManager.GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_NodeManager.GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_ExecOutputs.PushBack({nullptr});
        continue;
      }

      EZ_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const ezDocumentObject* pNextObject = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pNextObject, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextObject, nullptr, pEntryObject);
        if (pNextAstNode == nullptr)
          return nullptr;

        objectStack.PushBack(pNextObject);
      }

      ConnectExecution(*pAstNode, *pNextAstNode);
    }
  }

  // Always add a GetScriptOwner node directly after the entry node
  {
    EZ_ASSERT_DEBUG(m_CompilationState.m_pGetScriptOwnerNode == nullptr, "");

    auto& getScriptOwnerNode = CreateAstNode(ezVisualScriptNodeDescription::Type::GetScriptOwner);
    AddDataOutput(getScriptOwnerNode, ezVisualScriptDataType::TypedPointer);
    AddDataOutput(getScriptOwnerNode, ezVisualScriptDataType::GameObject);
    AddDataOutput(getScriptOwnerNode, ezVisualScriptDataType::Component);

    m_CompilationState.m_pGetScriptOwnerNode = &getScriptOwnerNode;

    ExecuteAfter(*pEntryAstNode, getScriptOwnerNode, getScriptOwnerNode);
  }

  return pEntryAstNode;
}

ezResult ezVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, ezDynamicArray<AstNode*>& out_Stack)
{
  if (pEntryAstNode->m_pObject == nullptr)
    return EZ_SUCCESS;

  ezHybridArray<const ezVisualScriptPin*, 16> pins;

  struct ObjectContext
  {
    const ezDocumentObject* m_pObject = nullptr;
    const ezVisualScriptNodeRegistry::NodeDesc* m_pNodeDesc = nullptr;
  };

  ezHybridArray<ObjectContext, 32> objectStack;
  objectStack.PushBack({pEntryAstNode->m_pObject, ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pEntryAstNode->m_pObject->GetType())});

  // Start from scratch for each data stack
  m_CompilationState.m_DataObjectToAstNode.Clear();
  m_CompilationState.m_DataObjectToAstNode.Insert(pEntryAstNode->m_pObject, pEntryAstNode);

  while (objectStack.IsEmpty() == false)
  {
    auto& objCtx = objectStack.PeekBack();
    const ezDocumentObject* pObject = objCtx.m_pObject;
    const ezVisualScriptNodeRegistry::NodeDesc* pNodeDesc = objCtx.m_pNodeDesc;
    objectStack.PopBack();

    AstNode* pAstNode = nullptr;
    EZ_VERIFY(m_CompilationState.m_DataObjectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (pAstNode != pEntryAstNode)
    {
      out_Stack.PushBack(pAstNode);
    }

    m_NodeManager.GetInputDataPins(pObject, pins);
    ezUInt32 uiNextInputPinIndex = 0;

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
          return EZ_FAILURE;

        if (pProp->GetCategory() == ezPropertyCategory::Array)
        {
          auto pMakeArrayAstNode = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          pMakeArrayAstNode->m_pObject = pObject;
          AddDataOutput(*pMakeArrayAstNode, ezVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, ezVisualScriptDataType::Array);

          out_Stack.PushBack(pMakeArrayAstNode);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        if (pPin->GetDynamicPinProperty() != pinDesc.m_sDynamicPinProperty)
          break;

        ezVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == ezVisualScriptDataType::Invalid)
        {
          ezLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return EZ_FAILURE;
        }

        auto connections = m_NodeManager.GetConnections(*pPin);
        if (pPin->IsRequired() && targetDataType != ezVisualScriptDataType::GameObject && connections.IsEmpty())
        {
          ezLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return EZ_FAILURE;
        }

        auto dataInputType = bArrayInput ? ezVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (ezVisualScriptDataType::IsPointer(dataInputType))
          {
            auto defaultInput = GetOrCreateDefaultPointerNode(*pAstNode, pPin->GetDataType());
            AddDataInput(*pAstNodeToAddInput, defaultInput.m_pSourceNode, defaultInput.m_uiSourcePinIndex, defaultInput.m_DataOffset.GetType());

            if (defaultInput.m_pSourceNode != nullptr)
            {
              if (defaultInput.m_pSourceNode->m_ExecInputs.IsEmpty())
              {
                out_Stack.RemoveAndCopy(defaultInput.m_pSourceNode);
                out_Stack.PushBack(defaultInput.m_pSourceNode);
              }
            }
          }
          else
          {
            EZ_SUCCEED_OR_RETURN(AddConstantDataInput(*pAstNodeToAddInput, pObject, pPin, dataInputType));
          }
        }
        else
        {
          auto& sourcePin = static_cast<const ezVisualScriptPin&>(connections[0]->GetSourcePin());
          const ezDocumentObject* pSourceObject = sourcePin.GetParent();
          auto pSourceNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pSourceObject->GetType());

          if (pSourceNodeDesc->m_Type == ezVisualScriptNodeDescription::Type::GetScriptOwner)
          {
            AstNode* pSourceAstNode = m_CompilationState.m_pGetScriptOwnerNode;
            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
          else if (pSourceNodeDesc->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Constant)
          {
            ezVariant value = pSourceObject->GetTypeAccessor().GetValue("Value");
            AddConstantDataInput(*pAstNodeToAddInput, value);
          }
          else
          {
            AstNode* pSourceAstNode;
            if (m_CompilationState.m_DataObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
            {
              pSourceAstNode = CreateAstNodeFromObject(pSourceObject, pSourceNodeDesc, nullptr, true);
              if (pSourceAstNode != nullptr)
              {
                objectStack.PushBack({pSourceObject, pSourceNodeDesc});
              }
              else if (pSourceAstNode == nullptr)
              {
                if (m_CompilationState.m_ExecObjectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
                {
                  ezLog::Error("The source node '{}' is not executed in the current function.", GetNiceTypeName(pSourceObject));
                  return EZ_FAILURE;
                }
              }
            }
            else if (out_Stack.RemoveAndCopy(pSourceAstNode))
            {
              // If the source node was already added to the stack, we need to move it to the top to ensure correct execution order
              out_Stack.PushBack(pSourceAstNode);
            }

            ezVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
            if (sourceDataType == ezVisualScriptDataType::Invalid)
            {
              ezLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
              return EZ_FAILURE;
            }

            if (sourcePin.CanConvertTo(*pPin) == false)
            {
              ezLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
              return EZ_FAILURE;
            }

            AddDataInput(*pAstNodeToAddInput, pSourceAstNode, sourcePin.GetDataPinIndex(), dataInputType);
          }
        }

        ++uiNextInputPinIndex;
      }
    }

    m_NodeManager.GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      AddDataOutput(*pAstNode, FinalizeDataType(pPin->GetResolvedScriptDataType()));
    }
  }

  // Connect executions
  if (out_Stack.GetCount() > 1)
  {
    AstNode* pLastDataNode = out_Stack.PeekBack();
    for (ezUInt32 i = out_Stack.GetCount() - 1; i > 0; --i)
    {
      AstNode* pDataNode = out_Stack[i - 1];
      ConnectExecution(*pLastDataNode, *pDataNode);

      pLastDataNode = pDataNode;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      ezHybridArray<AstNode*, 32> dataStack;
      if (BuildDataStack(pAstNode, dataStack).Failed())
        return VisitorResult::Error;

      if (dataStack.IsEmpty())
        return VisitorResult::Continue;

      // Connect data stack
      AstNode* pFirstDataNode = dataStack.PeekBack();
      AstNode* pLastDataNode = dataStack[0];

      ExecuteBefore(*pAstNode, *pFirstDataNode, *pLastDataNode);

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnected() == false)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);

        auto inputDataType = dataInput.m_DataOffset.GetType();
        auto outputDataType = dataOutput.m_DataOffset.GetType();

        if (dataOutput.m_DataOffset.GetType() != inputDataType)
        {
          auto nodeType = ezVisualScriptNodeDescription::Type::GetConversionType(inputDataType);

          auto& conversionNode = CreateAstNode(nodeType, outputDataType, true);
          AddDataInput(conversionNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, outputDataType);
          AddDataOutput(conversionNode, inputDataType);

          dataInput.m_pSourceNode = &conversionNode;
          dataInput.m_uiSourcePinIndex = 0;

          ExecuteBefore(*pAstNode, conversionNode, conversionNode);
        }
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::ReplaceLoop(AstNode* pLoopNode)
{
  AstNode* pLoopInitStart = nullptr;
  AstNode* pLoopInitEnd = nullptr;

  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd = nullptr;

  DataInput conditionDataInput;
  conditionDataInput.m_uiSourcePinIndex = 0;
  conditionDataInput.m_DataOffset.m_uiType = ezVisualScriptDataType::Bool;

  AstNode* pLoopIncrement = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex = nullptr;

  AstNode* pLoopBody = pLoopNode->m_ExecOutputs[0].m_pTargetNode;
  AstNode* pLoopCompleted = pLoopNode->m_ExecOutputs[1].m_pTargetNode;

  auto loopType = pLoopNode->m_Type;

  if (loopType == ezVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    conditionDataInput = pLoopNode->m_DataInputs[0];

    if (conditionDataInput.m_pSourceNode != nullptr)
    {
      pLoopConditionEnd = conditionDataInput.m_pSourceNode;

      pLoopConditionStart = conditionDataInput.m_pSourceNode;
      while (pLoopConditionStart->m_ExecInputs.GetCount() == 1 && pLoopConditionStart->m_ExecInputs[0].m_pSourceNode->m_bImplicitExecution)
      {
        pLoopConditionStart = pLoopConditionStart->m_ExecInputs[0].m_pSourceNode;
      }
    }
  }
  else if (loopType == ezVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_DataInputs[0];
    auto& lastIndexInput = pLoopNode->m_DataInputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_ToInt, ezVisualScriptDataType::Int);
      pLoopInitStart->m_DataInputs.PushBack(firstIndexInput);
      AddDataOutput(*pLoopInitStart, ezVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;
      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, ezVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = ezInt64(ezComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, ezVisualScriptDataType::Int);
      pLoopConditionStart->m_DataInputs.PushBack(lastIndexInput);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrement = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Add, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }
  }
  else if (loopType == ezVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse = (loopType == ezVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto& arrayInput = pLoopNode->m_DataInputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopInitStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopInitStart, ezVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Subtract, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, ezVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitEnd, 1);
      AddDataOutput(*pLoopInitEnd, ezVisualScriptDataType::Int);

      ConnectExecution(*pLoopInitStart, *pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_ToInt, ezVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopInitStart, 0);
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
      AddConstantDataInput(*pLoopConditionStart, 0);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      pLoopConditionStart->m_DataInputs.PushBack(arrayInput);
      AddDataOutput(*pLoopConditionStart, ezVisualScriptDataType::Int);

      pLoopConditionEnd = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, ezVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = ezInt64(ezComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, ezVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, ezVisualScriptDataType::Bool);

      ConnectExecution(*pLoopConditionStart, *pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? ezVisualScriptNodeDescription::Type::Builtin_Subtract : ezVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrement = &CreateAstNode(incType, ezVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrement, pLoopIndex, 0, ezVisualScriptDataType::Int);
      AddConstantDataInput(*pLoopIncrement, 1);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      pLoopIncrement->m_DataInputs.PushBack(arrayInput);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrement->m_DataOutputs.ExpandAndGetRef();
      dataOutput.m_DataOffset = pLoopIndex->m_DataOutputs[0].m_DataOffset;
    }

    pLoopElement = &CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Array_GetElement, ezVisualScriptDataType::Invalid, true);
    pLoopElement->m_DataInputs.PushBack(arrayInput);
    AddDataInput(*pLoopElement, pLoopIndex, 0, ezVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, ezVisualScriptDataType::Variant);
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  {
    if (conditionDataInput.m_pSourceNode == nullptr && conditionDataInput.m_DataOffset.IsLocal())
    {
      conditionDataInput.m_pSourceNode = pLoopConditionEnd;
    }

    auto& branchNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Branch);
    branchNode.m_DataInputs.PushBack(conditionDataInput);

    ReplaceExecution(*pLoopNode, branchNode);

    if (pLoopConditionStart == nullptr)
    {
      pLoopConditionStart = &branchNode;
    }
    else if (pLoopConditionEnd->m_ExecOutputs.IsEmpty())
    {
      ExecuteBefore(branchNode, *pLoopConditionStart, *pLoopConditionEnd);
    }

    if (pLoopInitStart != nullptr)
    {
      ExecuteBefore(*pLoopConditionStart, *pLoopInitStart, *pLoopInitEnd);
    }

    if (pLoopElement != nullptr && pLoopBody != nullptr)
    {
      ExecuteBefore(*pLoopBody, *pLoopElement, *pLoopElement);
    }
  }

  AstNode* pJumpNode = &CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrement != nullptr)
  {
    ConnectExecution(*pLoopIncrement, *pJumpNode);
    pJumpNode = pLoopIncrement;
  }

  ezHybridArray<AstNode*, 8> nodesConnectedToBreak;

  if (TraverseAstDepthFirst(pLoopBody,
        [&](AstNode*& pAstNode)
        {
          EZ_ASSERT_DEV(ezVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) == false, "Nested Loops should have been resolved already");

          for (ezUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
          {
            auto& execOutput = pAstNode->m_ExecOutputs[i];
            if (execOutput.m_pTargetNode == nullptr)
            {
              ConnectExecution(*pAstNode, *pJumpNode, i);
            }
            else if (execOutput.m_pTargetNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Break)
            {
              // handle breaks after this traversal, otherwise we could end up traversing to nodes outside the loop
              nodesConnectedToBreak.PushBack(pAstNode);
            }
          }

          for (auto& dataInput : pAstNode->m_DataInputs)
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
        })
        .Failed())
  {
    return EZ_FAILURE;
  }

  for (auto pAstNode : nodesConnectedToBreak)
  {
    for (ezUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];
      if (execOutput.m_pTargetNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Break)
      {
        DisconnectExecution(*pAstNode, *execOutput.m_pTargetNode, i);
        if (pLoopCompleted != nullptr)
        {
          ConnectExecution(*pAstNode, *pLoopCompleted, i);
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  ezHybridArray<AstNode*, 64> unsupportedNodes;

  if (TraverseAstDepthFirst(pEntryAstNode,
        [&](AstNode*& pAstNode)
        {
          if (ezVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type) || pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_CompareExec)
          {
            EZ_ASSERT_DEBUG(unsupportedNodes.Contains(pAstNode) == false, "");
            unsupportedNodes.PushBack(pAstNode);
          }

          return VisitorResult::Continue;
        })
        .Failed())
  {
    return EZ_FAILURE;
  }

  // Replace unsupported nodes backwards so that we replace inner loops first, order doesn't matter for the other nodes
  for (ezUInt32 i = unsupportedNodes.GetCount(); i-- > 0;)
  {
    AstNode* pAstNode = unsupportedNodes[i];

    if (pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_CompareExec)
    {
      auto& compareNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Compare, pAstNode->m_DeductedDataType, true);
      compareNode.m_Value = pAstNode->m_Value;
      compareNode.m_DataInputs = pAstNode->m_DataInputs;
      AddDataOutput(compareNode, ezVisualScriptDataType::Bool);

      auto& branchNode = CreateAstNode(ezVisualScriptNodeDescription::Type::Builtin_Branch);
      AddDataInput(branchNode, &compareNode, 0, ezVisualScriptDataType::Bool);

      ReplaceExecution(*pAstNode, branchNode);
      ExecuteBefore(branchNode, compareNode, compareNode);
    }
    else if (ezVisualScriptNodeDescription::Type::IsLoop(pAstNode->m_Type))
    {
      EZ_SUCCEED_OR_RETURN(ReplaceLoop(pAstNode));
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::AssignInstanceVariables(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        EZ_ASSERT_DEBUG(pSourceNode != nullptr, "Data input source node should be always valid for local inputs");

        if (pSourceNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_GetVariable)
        {
          auto& dataOutput = GetDataOutputFromInput(dataInput);

          ezHashedString sName;
          sName.Assign(pSourceNode->m_Value.Get<ezString>());

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
          dataInput.m_DataOffset = GetInstanceDataOffset(sName, dataOutput.m_DataOffset.GetType());

          // Remove the GetVariable node from the execution flow
          if (pSourceNode->m_ExecOutputs.IsEmpty() == false)
          {
            AstNode* pNodeAfterGetVariable = pSourceNode->m_ExecOutputs[0].m_pTargetNode;
            pNodeAfterGetVariable->m_ExecInputs.Clear();

            for (auto& execInput : pSourceNode->m_ExecInputs)
            {
              ConnectExecution(*execInput.m_pSourceNode, *pNodeAfterGetVariable, execInput.m_uiSourcePinIndex);
            }

            pSourceNode->m_ExecInputs.Clear();
            pSourceNode->m_ExecOutputs.Clear();
          }
        }
      }

      if (pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_SetVariable ||
          pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_IncVariable ||
          pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_DecVariable)
      {
        ezHashedString sName;
        sName.Assign(pAstNode->m_Value.Get<ezString>());

        DataOffset dataOffset = GetInstanceDataOffset(sName, pAstNode->m_DeductedDataType);

        if (pAstNode->m_Type != ezVisualScriptNodeDescription::Type::Builtin_SetVariable)
        {
          if (pAstNode->m_DataInputs.IsEmpty())
          {
            auto& dataInput = pAstNode->m_DataInputs.ExpandAndGetRef();
            dataInput.m_DataOffset = dataOffset;
          }
        }

        EZ_ASSERT_DEBUG(pAstNode->m_DataOutputs.GetCount() > 0, "");
        auto& dataOutput = pAstNode->m_DataOutputs[0];
        dataOutput.m_DataOffset = dataOffset;
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, ezVisualScriptDataDescription& inout_localDataDesc)
{
  m_CompilationState.m_LiveLocalVars.Reserve(m_CompilationState.m_uiNextLocalVarId);
  for (ezUInt32 i = 0; i < m_CompilationState.m_uiNextLocalVarId; ++i)
  {
    auto& liveLocalVar = m_CompilationState.m_LiveLocalVars.ExpandAndGetRef();
    liveLocalVar.m_uiId = i;
  }

  ezUInt32 uiNodeIndex = 0;

  ezResult r = TraverseAstTopologicalOrder(pEntryAstNode,
    [&](const AstNode* pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        const ezUInt32 id = dataInput.m_DataOffset.m_uiByteOffset;
        auto& liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

        liveLocalVar.m_uiEnd = ezMath::Max(liveLocalVar.m_uiEnd, uiNodeIndex);
      }

      for (auto& dataOutput : pAstNode->m_DataOutputs)
      {
        if (dataOutput.IsValidAndLocal() == false)
          continue;

        const ezUInt32 id = dataOutput.m_DataOffset.m_uiByteOffset;
        auto& liveLocalVar = m_CompilationState.m_LiveLocalVars[id];

        liveLocalVar.m_uiId = id;
        liveLocalVar.m_DataOffset = dataOutput.m_DataOffset;
        liveLocalVar.m_uiStart = ezMath::Min(liveLocalVar.m_uiStart, uiNodeIndex);
      }

      ++uiNodeIndex;
      return VisitorResult::Continue;
    });
  if (r.Failed())
    return EZ_FAILURE;

  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort lifetime by start index
  m_CompilationState.m_LiveLocalVars.Sort(
    [](const LiveLocalVar& a, const LiveLocalVar& b)
    {
      if (a.m_uiStart != b.m_uiStart)
        return a.m_uiStart < b.m_uiStart;

      return a.m_uiId < b.m_uiId;
    });

  // Assign local vars
  ezHybridArray<LiveLocalVar, 64> activeIntervals;
  ezHybridArray<DataOffset, 64> freeDataOffsets;

  for (auto& liveInterval : m_CompilationState.m_LiveLocalVars)
  {
    // Expire old intervals with less comparison instead of less equal (as in the original paper) so we don't end up using the same data as input and output
    for (ezUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd < liveInterval.m_uiStart)
      {
        freeDataOffsets.PushBack(activeInterval.m_DataOffset);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Unused var
    if (liveInterval.m_uiEnd <= liveInterval.m_uiStart)
    {
      liveInterval.m_DataOffset = {};
      continue;
    }

    // Allocate local var
    {
      DataOffset dataOffset;
      dataOffset.m_uiType = liveInterval.m_DataOffset.m_uiType;

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

      liveInterval.m_DataOffset = dataOffset;
    }

    activeIntervals.PushBack(liveInterval);
  }

  // Sort by id and copy back to outputs and inputs
  m_CompilationState.m_LiveLocalVars.Sort([](const LiveLocalVar& a, const LiveLocalVar& b)
    { return a.m_uiId < b.m_uiId; });

  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnectedAndLocal() == false)
          continue;

        const ezUInt32 id = dataInput.m_DataOffset.m_uiByteOffset;
        EZ_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
        dataInput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
      }

      for (auto& dataOutput : pAstNode->m_DataOutputs)
      {
        if (dataOutput.IsValidAndLocal() == false)
          continue;

        const ezUInt32 id = dataOutput.m_DataOffset.m_uiByteOffset;
        EZ_ASSERT_DEBUG(m_CompilationState.m_LiveLocalVars[id].m_uiId == id, "");
        dataOutput.m_DataOffset = m_CompilationState.m_LiveLocalVars[id].m_DataOffset;
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::CopyOutputsToInputs(AstNode* pEntryAstNode)
{
  return TraverseAstDepthFirst(pEntryAstNode,
    [&](AstNode*& pAstNode)
    {
      for (auto& dataInput : pAstNode->m_DataInputs)
      {
        if (dataInput.IsConnected() == false)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);
        dataInput.m_DataOffset.m_uiByteOffset = dataOutput.m_DataOffset.m_uiByteOffset;
        dataInput.m_DataOffset.m_uiSource = dataOutput.m_DataOffset.m_uiSource;
        EZ_ASSERT_DEBUG(dataInput.m_DataOffset.GetType() == ezVisualScriptDataType::Variant || dataInput.m_DataOffset.GetType() == dataOutput.m_DataOffset.GetType(), "");
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

    for (auto& dataInput : astNode.m_DataInputs)
    {
      nodeDesc.m_InputDataOffsets.PushBack(dataInput.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_DataOutputs)
    {
      nodeDesc.m_OutputDataOffsets.PushBack(dataOutput.m_DataOffset);
    }

    nodeDesc.m_ExecutionIndices.SetCount(astNode.m_ExecOutputs.GetCount(), ezSmallInvalidIndex);

    EZ_VERIFY(astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex) == false, "");
    return EZ_SUCCESS;
  };

  return TraverseAstTopologicalOrder(pEntryAstNode,
    [&](const AstNode* pAstNode)
    {
      ezUInt32 uiTargetIndex = 0;

      // Jump nodes should not end up in the final node descriptions
      if (pAstNode->m_Type == ezVisualScriptNodeDescription::Type::Builtin_Jump)
      {
        ezUInt64 uiPtr = pAstNode->m_Value.Get<ezUInt64>();
        const AstNode* pTargetAstNode = *reinterpret_cast<const AstNode**>(&uiPtr);

        if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiTargetIndex) == false)
          return VisitorResult::Error;
      }
      else
      {
        if (CreateNodeDesc(*pAstNode, uiTargetIndex).Failed())
          return VisitorResult::Error;
      }

      for (auto& execInput : pAstNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode == nullptr)
          continue;

        ezUInt32 uiSourceIndex = 0;
        EZ_VERIFY(astNodeToNodeDescIndices.TryGetValue(execInput.m_pSourceNode, uiSourceIndex), "Topological sort failed");

        auto pSourceNodeDesc = &out_NodeDescriptions[uiSourceIndex];
        pSourceNodeDesc->m_ExecutionIndices[execInput.m_uiSourcePinIndex] = uiTargetIndex;
      }

      return VisitorResult::Continue;
    });
}

ezResult ezVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataStorage.AllocateStorage(ezFoundation::GetDefaultAllocator());

  for (auto& it : m_Module.m_ConstantDataToIndex)
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

ezResult ezVisualScriptCompiler::TraverseAstDepthFirst(AstNode* pEntryAstNode, ezDelegate<VisitorResult(AstNode*& pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return EZ_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  ezHybridArray<AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pOldAstNode = pAstNode;

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return EZ_FAILURE;

    // Check whether the node has been replaced by a new one and if so mark the new one as visited as well
    if (pAstNode != pOldAstNode)
    {
      EZ_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");
    }

    for (ezUInt32 i = 0; i < pAstNode->m_ExecOutputs.GetCount(); ++i)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Insert(execOutput.m_pTargetNode))
        continue;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      bool bExecInputFound = false;
      for (auto& execInput : execOutput.m_pTargetNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode == pAstNode && execInput.m_uiSourcePinIndex == i)
        {
          bExecInputFound = true;
          break;
        }
      }
      EZ_ASSERT_DEBUG(bExecInputFound, "Execution connection corrupted");
#endif

      nodeStack.PushBack(execOutput.m_pTargetNode);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVisualScriptCompiler::TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, ezDelegate<VisitorResult(const AstNode* pAstNode)> func)
{
  if (pEntryAstNode == nullptr)
    return EZ_SUCCESS;

  m_CompilationState.m_VisitedNodes.Clear();

  ezHybridArray<const AstNode*, 64> nodeStack;
  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    // Find next node with all execution dependencies visited
    const AstNode* pAstNode = nullptr;
    for (ezUInt32 i = nodeStack.GetCount(); i-- > 0;)
    {
      const AstNode* pCandidateAstNode = nodeStack[i];
      bool bAllVisited = true;
      for (auto& execInput : pCandidateAstNode->m_ExecInputs)
      {
        if (execInput.m_pSourceNode != nullptr && m_CompilationState.m_VisitedNodes.Contains(execInput.m_pSourceNode) == false)
        {
          bAllVisited = false;
          break;
        }
      }

      if (bAllVisited)
      {
        pAstNode = pCandidateAstNode;
        nodeStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (pAstNode == nullptr)
    {
      EZ_REPORT_FAILURE("Execution connection corrupted or loop detected");
      return EZ_FAILURE;
    }

    EZ_VERIFY(m_CompilationState.m_VisitedNodes.Insert(pAstNode) == false, "");

    VisitorResult r = func(pAstNode);
    if (r == VisitorResult::Error)
      return EZ_FAILURE;

    // Since we use a stack we need to iterate the execution outputs backwards to ensure that they are processed in order.
    // Strictly speaking this is not necessary but improves data locality especially for loops,
    // which in the end results in shorter variable lifetimes and thus in fewer local variables.
    for (ezUInt32 i = pAstNode->m_ExecOutputs.GetCount(); i-- > 0;)
    {
      auto& execOutput = pAstNode->m_ExecOutputs[i];

      if (execOutput.m_pTargetNode == nullptr)
        continue;
      if (m_CompilationState.m_VisitedNodes.Contains(execOutput.m_pTargetNode))
        continue;

      if (nodeStack.Contains(execOutput.m_pTargetNode) == false)
      {
        nodeStack.PushBack(execOutput.m_pTargetNode);
      }
    }
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
    ezUInt32 uiNodeIndex = 0;

    // First build all the dgml graph nodes
    TraverseAstTopologicalOrder(pEntryAstNode,
      [&](const AstNode* pAstNode)
      {
        ezStringView sTypeName = ezVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
        sb.SetFormat("{} (id: {})", sTypeName, uiNodeIndex);
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

        float colorX = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(ezHashingUtils::StringHash(sTypeName))).x();

        ezDGMLGraph::NodeDesc nd;
        nd.m_Color = ezColorScheme::LightUI(colorX);
        ezUInt32 uiGraphNode = dgmlGraph.AddNode(sb, &nd);
        EZ_VERIFY(nodeCache.Insert(pAstNode, uiGraphNode) == false, "");

        ++uiNodeIndex;
        return VisitorResult::Continue;
      })
      .IgnoreResult();

    // Then build all the connections
    for (auto it : nodeCache)
    {
      const AstNode* pAstNode = it.Key();
      const ezUInt32 uiGraphNode = it.Value();

      for (auto& execInput : pAstNode->m_ExecInputs)
      {
        ezUInt32 uiSourceGraphNode = 0;
        EZ_VERIFY(nodeCache.TryGetValue(execInput.m_pSourceNode, uiSourceGraphNode), "");

        ezUInt64 uiConnectionKey = uiSourceGraphNode | ezUInt64(uiGraphNode) << 32;
        ezString& sLabel = connectionCache[uiConnectionKey];

        ezStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        sb.AppendFormat("Exec{}", execInput.m_uiSourcePinIndex);
        sLabel = sb;
      }

      for (ezUInt32 i = 0; i < pAstNode->m_DataInputs.GetCount(); ++i)
      {
        auto& dataInput = pAstNode->m_DataInputs[i];
        if (dataInput.m_pSourceNode == nullptr)
          continue;

        // Exclude GetScriptOwner connections as they create too much noise
        if (dataInput.m_pSourceNode->m_Type == ezVisualScriptNodeDescription::Type::GetScriptOwner)
          continue;

        auto& dataOutput = GetDataOutputFromInput(dataInput);

        ezUInt32 uiSourceGraphNode = 0;
        EZ_VERIFY(nodeCache.TryGetValue(dataInput.m_pSourceNode, uiSourceGraphNode), "");

        ezUInt64 uiConnectionKey = uiSourceGraphNode | ezUInt64(uiGraphNode) << 32;
        ezString& sLabel = connectionCache[uiConnectionKey];

        ezStringBuilder sb = sLabel;
        if (sb.IsEmpty() == false)
        {
          sb.Append(" + ");
        }
        const ezUInt32 uiOutputId = dataOutput.m_DataOffset.m_uiByteOffset;
        const ezUInt32 uiInputId = dataInput.m_DataOffset.m_uiByteOffset;
        sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, ezVisualScriptDataType::GetName(dataOutput.m_DataOffset.GetType()), uiOutputId, i, ezVisualScriptDataType::GetName(dataInput.m_DataOffset.GetType()), uiInputId);
        sLabel = sb;
      }
    }

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
    for (ezUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      auto& nodeDesc = nodeDescriptions[i];

      ezStringView sTypeName = ezVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      sTmp.AppendFormat(" (id: {})", i);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), ezVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_Module.m_ConstantDataToIndex)
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

    for (ezUInt32 uiCurrentIndex = 0; uiCurrentIndex < nodeDescriptions.GetCount(); ++uiCurrentIndex)
    {
      auto& executionIndices = nodeDescriptions[uiCurrentIndex].m_ExecutionIndices;
      for (ezUInt32 uiExecIndex = 0; uiExecIndex < executionIndices.GetCount(); ++uiExecIndex)
      {
        const ezUInt32 uiNextIndex = executionIndices[uiExecIndex];
        if (uiNextIndex == ezSmallInvalidIndex)
          continue;

        sTmp.SetFormat("Exec{}", uiExecIndex);
        dgmlGraph.AddConnection(uiCurrentIndex, uiNextIndex, sTmp);
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
