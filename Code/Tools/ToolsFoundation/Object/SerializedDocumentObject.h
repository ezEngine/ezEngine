#pragma once

#include <ToolsFoundation/Serialization/SerializedObjectBase.h>

class ezDocumentObjectBase;
class ezIReflectedTypeAccessor;

class EZ_TOOLSFOUNDATION_DLL ezSerializedDocumentObjectWriter : public ezSerializedObjectWriterBase
{
public:
  ezSerializedDocumentObjectWriter(const ezDocumentObjectBase* pObject);
  ~ezSerializedDocumentObjectWriter();

  virtual ezUuid GetGuid() const override;
  virtual ezUuid GetParentGuid() const override;
  virtual const char* GetType(ezStringBuilder& builder) const override;

  virtual void GatherProperties(ezObjectSerializationContext& context) const override;

private:
  const ezDocumentObjectBase* m_pObject;
};


class EZ_TOOLSFOUNDATION_DLL ezSerializedDocumentObjectReader : public ezSerializedObjectReaderBase
{
public:
  ezSerializedDocumentObjectReader(ezDocumentObjectBase* pObject);
  ~ezSerializedDocumentObjectReader();

  virtual void SetGuid(const ezUuid& guid) override;
  virtual void SetParentGuid(const ezUuid& guid) override;
  virtual void SetProperty(const char* szName, const ezVariant& value) override;
  virtual void SubGroupChanged(const ezHybridArray<ezString, 8>& stack) override;

private:
  ezDocumentObjectBase* m_pObject;
  ezIReflectedTypeAccessor* m_pAccessor;
  ezUuid m_ParentGuid;
};


