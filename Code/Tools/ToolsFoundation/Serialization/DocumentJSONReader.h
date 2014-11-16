#pragma once

#include <ToolsFoundation/Serialization/DocumentReaderBase.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/Types/Variant.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentJSONReader : public ezDocumentReaderBase
{
public:
  ezDocumentJSONReader(ezStreamReaderBase* pInput);
  ~ezDocumentJSONReader();

  virtual bool OpenGroup(const char* szName) override;
  virtual bool PeekNextObject(ezUuid& out_objectGuid, ezStringBuilder& out_sType, ezUuid& out_parentGuid) override;
  virtual void ReadObject(ezSerializedObjectReaderBase& object) override;

private:
  void ReadObjectRecursive(const ezHashTable<ezString, ezVariant>::ConstIterator& it, ezSerializedObjectReaderBase& object, ezHybridArray<ezString, 8>& stack);


private:
  const ezVariantArray* m_pGroup;
  const ezVariantDictionary* m_pObject;
  ezUInt32 m_uiObjextIndex;
  ezExtendedJSONReader m_Reader;
};

