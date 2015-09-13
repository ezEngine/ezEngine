#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class ezReflectedClass;


class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeSubObjectAccessor : public ezIReflectedTypeAccessor
{
public:
  ezReflectedTypeSubObjectAccessor(const ezRTTI* pRtti, ezDocumentObjectBase* pOwner);
  void SetSubAccessor(ezIReflectedTypeAccessor* pAcc, const ezPropertyPath& subPath);

  virtual const ezVariant GetValue(const ezPropertyPath& path, ezVariant index = ezVariant()) const override;
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index = ezVariant()) override;

  virtual ezInt32 GetCount(const ezPropertyPath& path) const override;
  virtual bool GetKeys(const ezPropertyPath& path, ezHybridArray<ezVariant, 16>& out_keys) const override;

  virtual bool InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value) override;
  virtual bool RemoveValue(const ezPropertyPath& path, ezVariant index) override;
  virtual bool MoveValue(const ezPropertyPath& path, ezVariant oldIndex, ezVariant newIndex) override;

  virtual ezVariant GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const override;

private:
  ezIReflectedTypeAccessor* m_pAcc;
  ezPropertyPath m_SubPath;
};

