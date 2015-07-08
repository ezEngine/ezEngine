#pragma once

#include <Foundation/Basics.h>
#include <qlistwidget.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezObjectCreatorList : public QListWidget
{
public:
  Q_OBJECT

public:
  ezObjectCreatorList(const ezDocumentObjectManager* pManager, QWidget* parent);
  ~ezObjectCreatorList();

  virtual QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const override;
  virtual QStringList mimeTypes () const override;

private:
  void TypeChanged(const ezPhantomTypeChange& data);

private:
  const ezDocumentObjectManager* m_pManager;
};


