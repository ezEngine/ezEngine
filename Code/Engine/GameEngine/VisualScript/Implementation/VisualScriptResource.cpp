#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>

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
  EZ_ASSERT_DEV(uiVersion == 4, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion != 4)
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

  PrecomputeMessageHandlers();
}

void ezVisualScriptResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 4;

  stream << uiVersion;

  const ezUInt32 uiNumNodes = m_Nodes.GetCount();
  const ezUInt32 uiNumExecCon = m_ExecutionPaths.GetCount();
  const ezUInt32 uiNumDataCon = m_DataPaths.GetCount();
  const ezUInt32 uiNumProps = m_Properties.GetCount();

  stream << uiNumNodes;
  stream << uiNumExecCon;
  stream << uiNumDataCon;
  stream << uiNumProps;

  for (const auto& node : m_Nodes)
  {
    if (node.m_pType != nullptr)
    {
      stream << node.m_pType->GetTypeName();
    }
    else
    {
      stream << node.m_sTypeName;
    }

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
}

void ezVisualScriptResourceDescriptor::PrecomputeMessageHandlers()
{
  for (ezUInt32 uiNode = 0; uiNode < m_Nodes.GetCount(); ++uiNode)
  {
    auto& node = m_Nodes[uiNode];
    const ezRTTI* pType = node.m_pType;

    if (!pType->IsDerivedFrom<ezVisualScriptNode>())
      continue;

    // TODO: this is a bit inefficient, we create each node type to call a virtual function on it
    // instead we should do this once for every type derived from ezVisualScriptNode and cache the result
    ezVisualScriptNode* pNode = static_cast<ezVisualScriptNode*>(pType->GetAllocator()->Allocate());

    const ezInt32 iMsgID = pNode->HandlesMessagesWithID();

    if (iMsgID >= 0)
    {
      m_MessageHandlers.Insert(iMsgID, (ezUInt16)uiNode);
    }

    pType->GetAllocator()->Deallocate(pNode);
  }
}
