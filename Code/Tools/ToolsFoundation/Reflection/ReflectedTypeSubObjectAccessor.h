#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class ezReflectedClass;


class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeSubObjectAccessor : public ezIReflectedTypeAccessor
{
public:
  ezReflectedTypeSubObjectAccessor(const ezRTTI* pRtti);
  void SetSubAccessor(ezIReflectedTypeAccessor* pAcc, const ezPropertyPath& subPath);

  virtual const ezVariant GetValue(const ezPropertyPath& path) const override;
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value) override;

private:
  ezIReflectedTypeAccessor* m_pAcc;
  ezPropertyPath m_SubPath;
};

