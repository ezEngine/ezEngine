#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class ezPropertySerializationContext;

class EZ_TOOLSFOUNDATION_DLL ezSerializedObjectWriterBase
{
public:
  virtual ezUuid GetGuid() const = 0;
  virtual ezUuid GetParentGuid() const = 0;
  virtual const char* GetType(ezStringBuilder& builder) const = 0;

  virtual void GatherProperties(ezPropertySerializationContext& context) const = 0;
};


class EZ_TOOLSFOUNDATION_DLL ezSerializedObjectReaderBase
{
public:
  virtual void SetGuid(const ezUuid& guid) = 0;
  virtual void SetParentGuid(const ezUuid& guid) = 0;
  virtual void SetProperty(const char* szName, const ezVariant& value) = 0;
  virtual void SubGroupChanged(const ezHybridArray<ezString, 8>& stack) = 0;
};


