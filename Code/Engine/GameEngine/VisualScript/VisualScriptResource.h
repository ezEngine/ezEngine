#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Reflection/Reflection.h>

typedef ezTypedResourceHandle<class ezVisualScriptResource> ezVisualScriptResourceHandle;

/// \brief Describes a visual script graph (node types and connections)
struct EZ_GAMEENGINE_DLL ezVisualScriptResourceDescriptor
{
  void Load(ezStreamReader& stream);
  void Save(ezStreamWriter& stream) const;

  struct Node
  {
    const ezRTTI* m_pType = nullptr; ///< Cached resolved type pointer after loading
    ezString m_sTypeName; ///< This is what gets written to the file if m_pType is null.

    /// \todo Properties
  };

  struct ExecutionConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiSourceNode;
    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiOutputPin;
  };

  struct DataConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiSourceNode;
    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiOutputPin;
    ezUInt8 m_uiInputPin;
  };

  ezDynamicArray<Node> m_Nodes;
  ezDynamicArray<ExecutionConnection> m_ExecutionPaths;
  ezDynamicArray<DataConnection> m_DataPaths;
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
  virtual ezResourceLoadDesc CreateResource(const ezVisualScriptResourceDescriptor& descriptor) override;

private:
  ezVisualScriptResourceDescriptor m_Descriptor;
};

