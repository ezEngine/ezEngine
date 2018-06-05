#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/Messages/EventMessage.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMessageNodes.h>

//////////////////////////////////////////////////////////////////////////
/// ezVisualScriptResource
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptResource, 2, ezRTTIDefaultAllocator<ezVisualScriptResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptResource::ezVisualScriptResource() : ezResource<ezVisualScriptResource, ezVisualScriptResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezVisualScriptResource::~ezVisualScriptResource()
{
}

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
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  {
    ezResourceHandleReadContext context;
    context.BeginReadingFromStream(Stream);
    context.BeginRestoringHandles(Stream);

    m_Descriptor.Load(*Stream);

    context.EndReadingFromStream(Stream);
    context.EndRestoringHandles();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezVisualScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezVisualScriptResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezVisualScriptResource::CreateResource(const ezVisualScriptResourceDescriptor& descriptor)
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

void ezVisualScriptResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 5, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion < 4)
    return;

  ezUInt32 uiNumNodes = 0;
  ezUInt32 uiNumExecCon = 0;
  ezUInt32 uiNumDataCon = 0;
  ezUInt32 uiNumProps = 0;

  stream >> uiNumNodes;
  stream >> uiNumExecCon;
  stream >> uiNumDataCon;
  stream >> uiNumProps;

  m_Nodes.SetCount(uiNumNodes);
  m_ExecutionPaths.SetCountUninitialized(uiNumExecCon);
  m_DataPaths.SetCountUninitialized(uiNumDataCon);
  m_Properties.SetCount(uiNumProps);

  ezStringBuilder sType;
  for (auto& node : m_Nodes)
  {
    stream >> sType;

    node.m_isMsgSender = 0;
    node.m_isMsgHandler = 0;

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

    stream >> node.m_uiFirstProperty;
    stream >> node.m_uiNumProperties;
  }

  for (auto& con : m_ExecutionPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
    stream >> con.m_uiInputPin;
  }

  for (auto& con : m_DataPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
    stream >> con.m_uiOutputPinType;
    stream >> con.m_uiInputPin;
    stream >> con.m_uiInputPinType;
  }

  for (auto& prop : m_Properties)
  {
    stream >> prop.m_sName;
    stream >> prop.m_Value;
  }

  // Version 5
  if (uiVersion >= 5)
  {
    ezUInt32 num;

    stream >> num;
    m_BoolParameters.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      stream >> m_BoolParameters[i].m_sName;
      stream >> m_BoolParameters[i].m_Value;
    }

    stream >> num;
    m_NumberParameters.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      stream >> m_NumberParameters[i].m_sName;
      stream >> m_NumberParameters[i].m_Value;
    }
  }

  PrecomputeMessageHandlers();
}

void ezVisualScriptResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 5;

  stream << uiVersion;

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  const ezUInt32 uiNumExecCon = m_ExecutionPaths.GetCount();
  const ezUInt32 uiNumDataCon = m_DataPaths.GetCount();
  const ezUInt32 uiNumProps = m_Properties.GetCount();

  stream << uiNumNodes;
  stream << uiNumExecCon;
  stream << uiNumDataCon;
  stream << uiNumProps;

  ezStringBuilder sType;

  for (const auto& node : m_Nodes)
  {
    if (node.m_pType != nullptr)
    {
      sType  = node.m_pType->GetTypeName();
    }
    else
    {
      sType = node.m_sTypeName;
    }

    if (node.m_isMsgSender)
      sType.Append("<send>");
    else if (node.m_isMsgHandler)
      sType.Append("<handle>");

    stream << sType;

    stream << node.m_uiFirstProperty;
    stream << node.m_uiNumProperties;
  }

  for (const auto& con : m_ExecutionPaths)
  {
    stream << con.m_uiSourceNode;
    stream << con.m_uiTargetNode;
    stream << con.m_uiOutputPin;
    stream << con.m_uiInputPin;
  }

  for (const auto& con : m_DataPaths)
  {
    stream << con.m_uiSourceNode;
    stream << con.m_uiTargetNode;
    stream << con.m_uiOutputPin;
    stream << con.m_uiOutputPinType;
    stream << con.m_uiInputPin;
    stream << con.m_uiInputPinType;
  }

  for (const auto& prop : m_Properties)
  {
    stream << prop.m_sName;
    stream << prop.m_Value;
  }

  // Version 5
  {
    stream << m_BoolParameters.GetCount();
    for (const auto& param : m_BoolParameters)
    {
      stream << param.m_sName;
      stream << param.m_Value;
    }

    stream << m_NumberParameters.GetCount();
    for (const auto& param : m_NumberParameters)
    {
      stream << param.m_sName;
      stream << param.m_Value;
    }
  }
}

void ezVisualScriptResourceDescriptor::PrecomputeMessageHandlers()
{
  for (ezUInt32 uiNode = 0; uiNode < m_Nodes.GetCount(); ++uiNode)
  {
    auto& node = m_Nodes[uiNode];
    const ezRTTI* pType = node.m_pType;

    ezVisualScriptNode* pNode = nullptr;

    if (pType->IsDerivedFrom<ezEventMessage>())
    {
      // TODO: just do the generic node logic here without allocating the node
      ezVisualScriptNode_GenericEvent* pEvent = ezVisualScriptNode_GenericEvent::GetStaticRTTI()->GetAllocator()->Allocate<ezVisualScriptNode_GenericEvent>();
      pNode = pEvent;

      pEvent->m_sEventType = pType->GetTypeName();
    }
    else if (pType->IsDerivedFrom<ezVisualScriptNode>())
    {
      pNode = pType->GetAllocator()->Allocate<ezVisualScriptNode>();
    }
    else
    {
      continue;
    }

    const ezInt32 iMsgID = pNode->HandlesMessagesWithID();

    if (iMsgID >= 0)
    {
      m_MessageHandlers.Insert(iMsgID, (ezUInt16)uiNode);
    }

    pType->GetAllocator()->Deallocate(pNode);
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptResource);

