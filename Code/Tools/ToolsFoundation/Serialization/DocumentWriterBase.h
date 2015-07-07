#pragma once

#include <ToolsFoundation/Basics.h>

class ezStreamWriterBase;
class ezSerializedObjectWriterBase;
class ezVariant;

class EZ_TOOLSFOUNDATION_DLL ezDocumentWriterBase
{
public:
  ezDocumentWriterBase(ezStreamWriterBase* pOutput) { m_pOutput = pOutput; }
  virtual ~ezDocumentWriterBase() { m_pOutput = nullptr; }

  virtual void StartGroup(const char* szName) = 0;
  virtual void EndGroup() = 0;

  virtual void WriteObject(const ezSerializedObjectWriterBase& object) = 0;
  virtual void EndDocument() = 0;

private:
  friend class ezPropertySerializationContext;
  // Used by ezPropertySerializationContext
  virtual void PushSubGroup(const char* szGroupName) = 0;
  virtual void PopSubGroup() = 0;
  virtual void AddProperty(const char* szName, const ezVariant& value) = 0;

protected:
  ezStreamWriterBase* m_pOutput;
};


