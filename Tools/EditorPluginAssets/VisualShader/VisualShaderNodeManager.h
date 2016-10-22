#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezVisualShaderPin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderPin, ezPin);
public:

  ezVisualShaderPin(Type type, const char* szName, const ezDocumentObject* pObject, const ezRTTI* pDataType, const ezColorGammaUB& color);

  const ezRTTI* GetDataType() const { return m_pDataType; }
  const ezColorGammaUB& GetColor() const { return m_Color; }

private:
  const ezRTTI* m_pDataType;
  ezColorGammaUB m_Color;
};

class ezVisualShaderNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  virtual ezStatus InternalCanConnect(const ezPin* pSource, const ezPin* pTarget) const override;

};
