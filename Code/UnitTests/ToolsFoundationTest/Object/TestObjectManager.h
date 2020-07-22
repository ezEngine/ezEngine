#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>


class ezTestDocumentObjectManager : public ezDocumentObjectManager
{
public:
  ezTestDocumentObjectManager();
  ~ezTestDocumentObjectManager();
};


class ezTestDocument : public ezDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestDocument, ezDocument);

public:
  ezTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror = false);
  ~ezTestDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  void ApplyNativePropertyChangesToObjectManager(ezDocumentObject* pObject);
  virtual ezDocumentInfo* CreateDocumentInfo() override;

  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;



private:
  bool m_bUseIPCObjectMirror;
};
