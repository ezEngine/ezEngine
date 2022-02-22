#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// \brief Default state provider that reflects the default state defined in the prefab template.
class EZ_GUIFOUNDATION_DLL ezPrefabDefaultStateProvider : public ezDefaultStateProvider
{
public:
  static ezSharedPtr<ezDefaultStateProvider> CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  ezPrefabDefaultStateProvider(const ezUuid& rootObjectGuid, const ezUuid& createFromPrefab, const ezUuid& prefabSeedGuid, ezInt32 iRootDepth);
  virtual ezInt32 GetRootDepth() const override;
  virtual ezColorGammaUB GetBackgroundColor() const override;
  virtual ezString GetStateProviderName() const override { return "Prefab"; }

  virtual ezVariant GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff) override;

private:
  const ezUuid m_rootObjectGuid;
  const ezUuid m_createFromPrefab;
  const ezUuid m_prefabSeedGuid;
  ezInt32 m_iRootDepth = 0;
};
