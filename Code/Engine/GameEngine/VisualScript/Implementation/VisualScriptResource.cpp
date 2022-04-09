#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Messages/EventMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptBasicNodes.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

//////////////////////////////////////////////////////////////////////////
/// ezVisualScriptResource
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptResource, 2, ezRTTIDefaultAllocator<ezVisualScriptResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezVisualScriptResource);
// clang-format on

ezVisualScriptResource::ezVisualScriptResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezVisualScriptResource::~ezVisualScriptResource() = default;

ezResourceLoadDesc ezVisualScriptResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezVisualScriptResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezVisualScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezVisualScriptResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezVisualScriptResource, ezVisualScriptResourceDescriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////
/// ezVisualScriptResourceDescriptor
//////////////////////////////////////////////////////////////////////////

void ezVisualScriptResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion < 7)
    return;

  ezUInt32 uiNumNodes = 0;
  ezUInt32 uiNumExecCon = 0;
  ezUInt32 uiNumDataCon = 0;
  ezUInt32 uiNumProps = 0;

  inout_stream >> uiNumNodes;
  inout_stream >> uiNumExecCon;
  inout_stream >> uiNumDataCon;
  inout_stream >> uiNumProps;

  m_Nodes.SetCount(uiNumNodes);
  m_ExecutionPaths.SetCountUninitialized(uiNumExecCon);
  m_DataPaths.SetCountUninitialized(uiNumDataCon);
  m_Properties.SetCount(uiNumProps);

  ezStringBuilder sType;
  for (auto& node : m_Nodes)
  {
    inout_stream >> sType;

    node.m_isMsgSender = 0;
    node.m_isMsgHandler = 0;
    node.m_isFunctionCall = 0;

    if (sType.EndsWith("<call>"))
    {
      node.m_isFunctionCall = 1;

      // remove the <call> part (leave full class name and function name in m_sTypeName
      sType.Shrink(0, 6);
      node.m_sTypeName = sType;

      const char* szColon = sType.FindLastSubString("::");
      sType.SetSubString_FromTo(sType.GetData(), szColon);

      node.m_pType = ezRTTI::FindTypeByName(sType);
    }
    else
    {
      if (sType.EndsWith("<send>"))
      {
        sType.Shrink(0, 6);
        node.m_isMsgSender = 1;
      }
      else if (sType.EndsWith("<handle>"))
      {
        sType.Shrink(0, 8);
        node.m_isMsgHandler = 1;
      }

      node.m_sTypeName = sType;
      node.m_pType = ezRTTI::FindTypeByName(sType);
    }

    inout_stream >> node.m_uiFirstProperty;
    inout_stream >> node.m_uiNumProperties;
  }

  for (auto& con : m_ExecutionPaths)
  {
    inout_stream >> con.m_uiSourceNode;
    inout_stream >> con.m_uiTargetNode;
    inout_stream >> con.m_uiOutputPin;
    inout_stream >> con.m_uiInputPin;
  }

  for (auto& con : m_DataPaths)
  {
    inout_stream >> con.m_uiSourceNode;
    inout_stream >> con.m_uiTargetNode;
    inout_stream >> con.m_uiOutputPin;
    inout_stream >> con.m_uiOutputPinType;
    inout_stream >> con.m_uiInputPin;
    inout_stream >> con.m_uiInputPinType;
  }

  for (auto& prop : m_Properties)
  {
    inout_stream >> prop.m_sName;
    inout_stream >> prop.m_Value;

    if (uiVersion >= 6)
    {
      inout_stream >> prop.m_iMappingIndex;
    }
  }

  // Version 5
  if (uiVersion >= 5)
  {
    ezUInt32 num;

    inout_stream >> num;
    m_BoolParameters.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      inout_stream >> m_BoolParameters[i].m_sName;
      inout_stream >> m_BoolParameters[i].m_Value;
    }

    inout_stream >> num;
    m_NumberParameters.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      inout_stream >> m_NumberParameters[i].m_sName;
      inout_stream >> m_NumberParameters[i].m_Value;
    }
  }

  // Version 8
  if (uiVersion >= 8)
  {
    ezUInt32 num;

    inout_stream >> num;
    m_StringParameters.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      inout_stream >> m_StringParameters[i].m_sName;
      inout_stream >> m_StringParameters[i].m_sValue;
    }
  }

  PrecomputeMessageHandlers();
}

void ezVisualScriptResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 8;

  inout_stream << uiVersion;

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  const ezUInt32 uiNumExecCon = m_ExecutionPaths.GetCount();
  const ezUInt32 uiNumDataCon = m_DataPaths.GetCount();
  const ezUInt32 uiNumProps = m_Properties.GetCount();

  inout_stream << uiNumNodes;
  inout_stream << uiNumExecCon;
  inout_stream << uiNumDataCon;
  inout_stream << uiNumProps;

  ezStringBuilder sType;

  for (const auto& node : m_Nodes)
  {
    if (node.m_pType != nullptr)
    {
      sType = node.m_pType->GetTypeName();
    }
    else
    {
      sType = node.m_sTypeName;
    }

    if (node.m_isMsgSender)
      sType.Append("<send>");
    else if (node.m_isMsgHandler)
      sType.Append("<handle>");
    else if (node.m_isFunctionCall)
      sType.Append("<call>");

    inout_stream << sType;

    inout_stream << node.m_uiFirstProperty;
    inout_stream << node.m_uiNumProperties;
  }

  for (const auto& con : m_ExecutionPaths)
  {
    inout_stream << con.m_uiSourceNode;
    inout_stream << con.m_uiTargetNode;
    inout_stream << con.m_uiOutputPin;
    inout_stream << con.m_uiInputPin;
  }

  for (const auto& con : m_DataPaths)
  {
    inout_stream << con.m_uiSourceNode;
    inout_stream << con.m_uiTargetNode;
    inout_stream << con.m_uiOutputPin;
    inout_stream << con.m_uiOutputPinType;
    inout_stream << con.m_uiInputPin;
    inout_stream << con.m_uiInputPinType;
  }

  for (const auto& prop : m_Properties)
  {
    inout_stream << prop.m_sName;
    inout_stream << prop.m_Value;

    // Version 6
    inout_stream << prop.m_iMappingIndex;
  }

  // Version 5
  {
    inout_stream << m_BoolParameters.GetCount();
    for (const auto& param : m_BoolParameters)
    {
      inout_stream << param.m_sName;
      inout_stream << param.m_Value;
    }

    inout_stream << m_NumberParameters.GetCount();
    for (const auto& param : m_NumberParameters)
    {
      inout_stream << param.m_sName;
      inout_stream << param.m_Value;
    }
  }

  // Version 8
  {
    inout_stream << m_StringParameters.GetCount();
    for (const auto& param : m_StringParameters)
    {
      inout_stream << param.m_sName;
      inout_stream << param.m_sValue;
    }
  }
}

void ezVisualScriptResourceDescriptor::PrecomputeMessageHandlers()
{
  for (ezUInt32 uiNode = 0; uiNode < m_Nodes.GetCount(); ++uiNode)
  {
    auto& node = m_Nodes[uiNode];
    const ezRTTI* pType = node.m_pType;
    if (pType == nullptr)
      continue;

    if (!pType)
      continue;

    ezUniquePtr<ezVisualScriptNode> pNode;

    if (pType->IsDerivedFrom<ezMessage>() && node.m_isMsgHandler)
    {
      auto pHandler = ezVisualScriptNode_MessageHandler::GetStaticRTTI()->GetAllocator()->Allocate<ezVisualScriptNode_MessageHandler>();
      pHandler->m_pMessageTypeToHandle = pType;

      pNode = pHandler;
    }
    else if (pType->IsDerivedFrom<ezVisualScriptNode>())
    {
      pNode = pType->GetAllocator()->Allocate<ezVisualScriptNode>();

      AssignNodeProperties(*pNode, node);
    }
    else
    {
      continue;
    }

    const ezInt32 iMsgID = pNode->HandlesMessagesWithID();

    if (iMsgID >= 0)
    {
      m_MessageHandlers.Insert(static_cast<ezUInt16>(iMsgID), static_cast<ezUInt16>(uiNode));
    }
  }
}

void ezVisualScriptResourceDescriptor::AssignNodeProperties(ezVisualScriptNode& ref_node, const Node& properties) const
{
  for (ezUInt32 i = 0; i < properties.m_uiNumProperties; ++i)
  {
    const ezUInt32 uiProp = properties.m_uiFirstProperty + i;
    const auto& prop = m_Properties[uiProp];

    ezAbstractProperty* pAbstract = ref_node.GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
    if (pAbstract->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);
    ezReflectionUtils::SetMemberPropertyValue(pMember, &ref_node, prop.m_Value);
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptResource);
