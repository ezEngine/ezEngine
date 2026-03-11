#pragma once

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

struct ezVisualShaderPinDescriptor;

/// Visual graph pin for visual shader nodes.
///
/// Extends the base pin class with shader-specific metadata such as data type and tooltip information
/// derived from the pin descriptor.
class ezVisualShaderPin : public ezVisualGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderPin, ezVisualGraphPin);

public:
  ezVisualShaderPin(Type type, const ezVisualShaderPinDescriptor* pDescriptor, const ezDocumentObject* pObject);

  const ezRTTI* GetDataType() const;
  const ezString& GetTooltip() const;
  const ezVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const ezVisualShaderPinDescriptor* m_pDescriptor;
};

/// Object manager for visual shader graphs.
///
/// Manages the document representation of visual shader nodes and their connections.
/// Creates pins based on shader node type descriptors and validates connections based on data type compatibility.
/// Enforces constraints such as limiting the number of certain node types in a shader.
class ezVisualShaderNodeManager : public ezVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetNodeCreationTemplates(ezDynamicArray<ezVisualGraphNodeDesc>& out_templates) const override;

  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const override;

private:
  virtual ezStatus InternalCanAdd(
    const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const override;

  ezUInt32 CountNodesOfType(ezVisualShaderNodeType::Enum type) const;
};
