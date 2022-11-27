#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class ezExposedParametersAttribute;
class ezExposedParameterCommandAccessor;

/// \brief Default state provider handling variant maps with the ezExposedParametersAttribute set. Reflects the default value defined in the ezExposedParameter.
class EZ_EDITORFRAMEWORK_DLL ezExposedParametersDefaultStateProvider : public ezDefaultStateProvider
{
public:
  static ezSharedPtr<ezDefaultStateProvider> CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);
  ezExposedParametersDefaultStateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  virtual ezInt32 GetRootDepth() const override;
  virtual ezColorGammaUB GetBackgroundColor() const override;
  virtual ezString GetStateProviderName() const override { return "Exposed Parameters"; }

  virtual ezVariant GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff) override;

  virtual bool IsDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus RevertProperty(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;

private:
  const ezDocumentObject* m_pObject = nullptr;
  const ezAbstractProperty* m_pProp = nullptr;
  const ezExposedParametersAttribute* m_pAttrib = nullptr;
  const ezAbstractProperty* m_pParameterSourceProp = nullptr;
};
