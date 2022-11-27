#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// \brief This is the fall back default state provider which handles the default state set via the ezDefaultAttribute on the reflected type.
class EZ_GUIFOUNDATION_DLL ezAttributeDefaultStateProvider : public ezDefaultStateProvider
{
public:
  static ezSharedPtr<ezDefaultStateProvider> CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  virtual ezInt32 GetRootDepth() const override;
  virtual ezColorGammaUB GetBackgroundColor() const override;
  virtual ezString GetStateProviderName() const override { return "Attribute"; }

  virtual ezVariant GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff) override;
};
