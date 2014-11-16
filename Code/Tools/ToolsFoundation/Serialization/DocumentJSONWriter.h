#pragma once

#include <ToolsFoundation/Serialization/DocumentWriterBase.h>
#include <Foundation/IO/ExtendedJSONWriter.h>

class ezExtendedJSONWriter;

class EZ_TOOLSFOUNDATION_DLL ezDocumentJSONWriter : public ezDocumentWriterBase
{
public:
  ezDocumentJSONWriter(ezStreamWriterBase* pOutput);
  virtual ~ezDocumentJSONWriter();

  virtual void StartGroup(const char* szName) override;
  virtual void EndGroup() override;

  virtual void WriteObject(const ezSerializedObjectWriterBase& object) override;
  virtual void EndDocument() override;

private:
  virtual void PushSubGroup(const char* szGroupName) override;
  virtual void PopSubGroup() override;
  virtual void AddProperty(const char* szName, const ezVariant& value) override;

private:
  bool m_bInGroup;
  ezHybridArray<ezString, 8> m_SubGroupStack;
  ezExtendedJSONWriter m_Writer;
};

