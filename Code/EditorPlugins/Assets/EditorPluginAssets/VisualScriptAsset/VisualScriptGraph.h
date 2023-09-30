#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

struct ezVisualScriptPinDescriptor;

class ezVisualScriptPin_Legacy : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptPin_Legacy, ezPin);

public:
  ezVisualScriptPin_Legacy(Type type, const ezVisualScriptPinDescriptor* pDescriptor, const ezDocumentObject* pObject);

  const ezString& GetTooltip() const;
  const ezVisualScriptPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const ezVisualScriptPinDescriptor* m_pDescriptor;
};

class ezVisualScriptConnection_Legacy : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptConnection_Legacy, ezReflectedClass);
};

class ezVisualScriptNodeManager_Legacy : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;
  virtual const ezRTTI* GetConnectionType() const override;
  virtual ezStringView GetTypeCategory(const ezRTTI* pRtti) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const override;
};
