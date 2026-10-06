#pragma once

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>

class ezSaveDocumentTask final : public ezTask
{
public:
  ezSaveDocumentTask();
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
  ezAfterSaveDocumentTask();
  ~ezAfterSaveDocumentTask();

  ezDocument* m_document = nullptr;
  ezDocument::AfterSaveCallback m_callback;

  /// The group this task belongs to. A newer save may have replaced the document's active save task in the meantime.
  ezTaskGroupID m_OwnGroup;

  virtual void Execute() override;
};
