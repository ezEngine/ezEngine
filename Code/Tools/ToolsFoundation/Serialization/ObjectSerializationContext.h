#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezDocumentWriterBase;
class ezVariant;

class EZ_TOOLSFOUNDATION_DLL ezPropertySerializationContext
{
public:
  ezPropertySerializationContext(ezDocumentWriterBase& writer);

  void PushSubGroup(const char* szGroupName);
  void PopSubGroup();
  void AddProperty(const char* szName, const ezVariant& value);
  void AddProperty(ezPropertyPath& path, const ezVariant& value);

private:
  friend class ezDocumentWriterBase;

  ezDocumentWriterBase* m_pWriter;
};


