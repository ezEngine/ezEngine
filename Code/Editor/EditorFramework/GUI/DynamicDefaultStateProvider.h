#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class ezDynamicDefaultValueAttribute;
class ezPropertyPath;

/// \brief Retrieves the dynamic default state of an object or container attributed with ezDynamicDefaultValueAttribute from an asset's meta data.
class EZ_EDITORFRAMEWORK_DLL ezDynamicDefaultStateProvider : public ezDefaultStateProvider
{
public:
  static ezSharedPtr<ezDefaultStateProvider> CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  ezDynamicDefaultStateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezDocumentObject* pClassObject, const ezDocumentObject* pRootObject, const ezAbstractProperty* pRootProp, ezInt32 iRootDepth);

  virtual ezInt32 GetRootDepth() const override;
  virtual ezColorGammaUB GetBackgroundColor() const override;
  virtual ezString GetStateProviderName() const override { return "Dynamic"; }

  virtual ezVariant GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff) override;

private:
  const ezReflectedClass* GetMetaInfo(ezObjectAccessorBase* pAccessor) const;
  const ezResult CreatePath(ezObjectAccessorBase* pAccessor, const ezReflectedClass* pMeta, ezPropertyPath& propertyPath, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant());

  const ezDocumentObject* m_pObject = nullptr;
  const ezDocumentObject* m_pClassObject = nullptr;
  const ezDocumentObject* m_pRootObject = nullptr;
  const ezAbstractProperty* m_pRootProp = nullptr;
  ezInt32 m_iRootDepth = 0;
  const ezDynamicDefaultValueAttribute* m_pAttrib = nullptr;
  const ezAbstractProperty* m_pClassSourceProp = nullptr;
  const ezRTTI* m_pClassType = nullptr;
  const ezAbstractProperty* m_pClassProperty = nullptr;
};
