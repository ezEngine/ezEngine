#pragma once

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct ezVisualShaderPinDescriptor;

class ezVisualShaderPin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderPin, ezPin);

public:
  ezVisualShaderPin(Type type, const ezVisualShaderPinDescriptor* pDescriptor, const ezDocumentObject* pObject);

  const ezRTTI* GetDataType() const;
  const ezString& GetTooltip() const;
  const ezVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const ezVisualShaderPinDescriptor* m_pDescriptor;
};

class ezVisualShaderNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetNodeCreationTemplates(ezDynamicArray<ezNodeCreationTemplate>& out_templates) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const override;

private:
  virtual ezStatus InternalCanAdd(
    const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const override;

  ezUInt32 CountNodesOfType(ezVisualShaderNodeType::Enum type) const;
};
