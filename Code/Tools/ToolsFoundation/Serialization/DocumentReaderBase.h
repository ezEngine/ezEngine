#pragma once

#include <ToolsFoundation/Basics.h>

class ezStreamReaderBase;
class ezUuid;
class ezStringBuilder;
class ezSerializedObjectReaderBase;

class EZ_TOOLSFOUNDATION_DLL ezDocumentReaderBase
{
public:
  ezDocumentReaderBase(ezStreamReaderBase* pInput) : m_pInput(pInput) {}
  virtual ~ezDocumentReaderBase() { m_pInput = nullptr; }

  virtual bool OpenGroup(const char* szName) = 0;
  virtual bool PeekNextObject(ezUuid& out_objectGuid, ezStringBuilder& out_sType, ezUuid& out_parentGuid) = 0;
  virtual void ReadObject(ezSerializedObjectReaderBase& object) = 0;

protected:
  ezStreamReaderBase* m_pInput;
};
