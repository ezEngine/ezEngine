#pragma once

#include <ToolsFoundation/Serialization/SerializedObjectBase.h>

class ezIReflectedTypeAccessor;

class EZ_TOOLSFOUNDATION_DLL ezSerializedTypeAccessorObjectWriter : public ezSerializedObjectWriterBase
{
public:
  ezSerializedTypeAccessorObjectWriter(const ezIReflectedTypeAccessor* pObject);
  ~ezSerializedTypeAccessorObjectWriter();

  virtual ezUuid GetGuid() const override;
  virtual ezUuid GetParentGuid() const override;
  virtual const char* GetType(ezStringBuilder& builder) const override;

  virtual void GatherProperties(ezObjectSerializationContext& context) const override;

private:
  const ezIReflectedTypeAccessor* m_pObject;
};


class EZ_TOOLSFOUNDATION_DLL ezSerializedTypeAccessorObjectReader : public ezSerializedObjectReaderBase
{
public:
  ezSerializedTypeAccessorObjectReader(ezIReflectedTypeAccessor* pObject);
  ~ezSerializedTypeAccessorObjectReader();

  virtual void SetGuid(const ezUuid& guid) override;
  virtual void SetParentGuid(const ezUuid& guid) override;
  virtual void SetProperty(const char* szName, const ezVariant& value) override;
  virtual void SubGroupChanged(const ezHybridArray<ezString, 8>& stack) override;

private:
  ezIReflectedTypeAccessor* m_pObject;
  bool m_bInTypeDataGroup;
};


