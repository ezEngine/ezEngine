#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>

//////////////////////////////////////////////////////////////////////////
/// ezVisualScriptResource
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptResource, 1, ezRTTIDefaultAllocator<ezVisualScriptResource>);
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
  EZ_ASSERT_DEV(uiVersion <= 2, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion > 2)
    return;

  ezUInt32 uiNumNodes = 0;
  ezUInt32 uiNumExecCon = 0;
  ezUInt32 uiNumDataCon = 0;
  ezUInt32 uiNumProps = 0;

  stream >> uiNumNodes;
  stream >> uiNumExecCon;
  stream >> uiNumDataCon;

  if (uiVersion >= 2)
  {
    stream >> uiNumProps;
  }

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

    if (uiVersion >= 2)
    {
      stream >> node.m_uiFirstProperty;
      stream >> node.m_uiNumProperties;
    }
    else
    {
      node.m_uiFirstProperty = 0;
      node.m_uiNumProperties = 0;
    }
  }

  for (auto& con : m_ExecutionPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
  }

  for (auto& con : m_DataPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
    stream >> con.m_uiInputPin;
  }

  for (auto& prop : m_Properties)
  {
    stream >> prop.m_sName;
    stream >> prop.m_Value;
  }
}

void ezVisualScriptResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;

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
  }

  for (const auto& con : m_DataPaths)
  {
    stream << con.m_uiSourceNode;
    stream << con.m_uiTargetNode;
    stream << con.m_uiOutputPin;
    stream << con.m_uiInputPin;
  }

  for (const auto& prop : m_Properties)
  {
    stream << prop.m_sName;
    stream << prop.m_Value;
  }
}
