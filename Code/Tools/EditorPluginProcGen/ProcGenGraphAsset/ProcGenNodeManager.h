#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <Foundation/Configuration/Singleton.h>

class ezProcGenNodeRegistry
{
  EZ_DECLARE_SINGLETON(ezProcGenNodeRegistry);

public:
  ezProcGenNodeRegistry();

  const ezRTTI* GetBaseType() const { return m_pBaseType; }
  const ezRTTI* GetLayerOutputType() const { return m_pLayerOutputType; }

  void UpdateNodeTypes();

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ProceduralPlacement);

  const ezRTTI* m_pBaseType;
  const ezRTTI* m_pLayerOutputType;
};

class ezProcGenNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;
  //virtual const char* GetTypeCategory(const ezRTTI* pRtti) const override;

  virtual ezStatus InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const override;

private:
  //virtual ezConnection* InternalCreateConnection(const ezPin* pSource, const ezPin* pTarget) { return EZ_DEFAULT_NEW(ezVisualScriptConnection); }

};


