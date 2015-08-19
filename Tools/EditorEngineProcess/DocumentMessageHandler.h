#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Serialization/RttiConverter.h>


class ezDocumentMessageContext : public ezRttiConverterContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;

private:


};


class ezDocumentMessageHandler
{
public:


private:


};