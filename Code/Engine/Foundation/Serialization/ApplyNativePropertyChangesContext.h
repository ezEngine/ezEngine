#pragma once

#include <Foundation/Serialization/RttiConverter.h>

/// \brief The ezApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those of the ezAbstractObjectGraph that was passed in. This allows native changes to be tracked and applied to the object graph at a later point.
/// \sa ezAbstractObjectGraph::ModifyNodeViaNativeCounterpart
class EZ_FOUNDATION_DLL ezApplyNativePropertyChangesContext : public ezRttiConverterContext
{
public:
  ezApplyNativePropertyChangesContext(ezRttiConverterContext& source, const ezAbstractObjectGraph& originalGraph);

  virtual ezUuid GenerateObjectGuid(const ezUuid& parentGuid, const ezAbstractProperty* pProp, ezVariant index, void* pObject) const override;

private:
  ezRttiConverterContext& m_NativeContext;
  const ezAbstractObjectGraph& m_OriginalGraph;
};
