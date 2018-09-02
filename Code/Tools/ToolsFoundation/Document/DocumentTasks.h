#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>

class ezSaveDocumentTask : public ezTask
{
public:
  ezSaveDocumentTask();

  ezDeferredFileWriter file;
  ezAbstractObjectGraph headerGraph;
  ezAbstractObjectGraph objectGraph;
  ezAbstractObjectGraph typesGraph;
  ezDocument* m_document = nullptr;

  virtual void Execute() override;
};

class ezAfterSaveDocumentTask : public ezTask
{
public:
  ezAfterSaveDocumentTask();

  ezDocument* m_document = nullptr;
  ezDocument::AfterSaveCallback m_callback;

  virtual void Execute() override;
};
