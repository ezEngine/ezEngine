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
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const override;
  virtual const char* GetTypeCategory(const ezRTTI* pRtti) const override;

private:
  virtual ezStatus InternalCanAdd(
    const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const override;

  ezUInt32 CountNodesOfType(ezVisualShaderNodeType::Enum type) const;
};
