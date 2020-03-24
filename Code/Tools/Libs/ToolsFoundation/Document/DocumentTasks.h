#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>

class ezSaveDocumentTask final : public ezTask
{
public:
  ezSaveDocumentTask(ezTask::OnTaskFinished onTaskFinished);
  ~ezSaveDocumentTask();

  ezDeferredFileWriter file;
  ezAbstractObjectGraph headerGraph;
  ezAbstractObjectGraph objectGraph;
  ezAbstractObjectGraph typesGraph;
  ezDocument* m_document = nullptr;

  virtual void Execute() override;
};

class ezAfterSaveDocumentTask final : public ezTask
{
public:
  ezAfterSaveDocumentTask(ezTask::OnTaskFinished onTaskFinished);
  ~ezAfterSaveDocumentTask();

  ezDocument* m_document = nullptr;
  ezDocument::AfterSaveCallback m_callback;

  virtual void Execute() override;
};
