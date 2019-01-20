#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/ArrayMap.h>

typedef ezTypedResourceHandle<class ezVisualScriptResource> ezVisualScriptResourceHandle;

/// \brief Describes a visual script graph (node types and connections)
struct EZ_GAMEENGINE_DLL ezVisualScriptResourceDescriptor
{
  void Load(ezStreamReader& stream);
  void Save(ezStreamWriter& stream) const;
  void PrecomputeMessageHandlers();

  struct Node
  {
    Node()
    {
      m_isMsgSender = 0;
      m_isMsgHandler = 0;
    }

    ezString m_sTypeName; ///< This is what gets written to the file if m_pType is null.
    const ezRTTI* m_pType = nullptr; ///< Cached resolved type pointer after loading

    ezUInt16 m_uiFirstProperty = 0;
    ezUInt8 m_uiNumProperties = 0;
    ezUInt8 m_isMsgSender : 1;
    ezUInt8 m_isMsgHandler : 1;
  };

  struct ExecutionConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiSourceNode;
    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiOutputPin;
    ezUInt8 m_uiInputPin;
  };

  struct DataConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiSourceNode;
    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiOutputPin;
    ezUInt8 m_uiOutputPinType; // ezVisualScriptDataPinType
    ezUInt8 m_uiInputPin;
    ezUInt8 m_uiInputPinType; // ezVisualScriptDataPinType
  };

  struct Property
  {
    ezString m_sName;
    ezVariant m_Value;
  };

  struct LocalParameterBool
  {
    ezHashedString m_sName;
    bool m_Value = false;
  };

  struct LocalParameterNumber
  {
    ezHashedString m_sName;
    double m_Value = 0;
  };

  ezDynamicArray<Node> m_Nodes;
  ezDynamicArray<ExecutionConnection> m_ExecutionPaths;
  ezDynamicArray<DataConnection> m_DataPaths;
  ezArrayMap<ezMessageId, ezUInt16> m_MessageHandlers;
  ezDeque<Property> m_Properties;
  ezDynamicArray<LocalParameterBool> m_BoolParameters;
  ezDynamicArray<LocalParameterNumber> m_NumberParameters;
};

class EZ_GAMEENGINE_DLL ezVisualScriptResource : public ezResource<ezVisualScriptResource, ezVisualScriptResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptResource, ezResourceBase);

public:
  ezVisualScriptResource();
  ~ezVisualScriptResource();

  const ezVisualScriptResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(ezVisualScriptResourceDescriptor&& descriptor) override;

private:
  ezVisualScriptResourceDescriptor m_Descriptor;
};

