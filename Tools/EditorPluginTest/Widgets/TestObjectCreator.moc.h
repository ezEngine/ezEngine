#pragma once

#include <Foundation/Basics.h>
#include <qlistwidget.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezTestObjectCreatorWidget : public QListWidget
{
public:
  Q_OBJECT

public:
  ezTestObjectCreatorWidget(const ezDocumentObjectManagerBase* pManager, QWidget* parent);
  ~ezTestObjectCreatorWidget();

  virtual QMimeData * mimeData ( const QList<QListWidgetItem *> items ) const override;
  virtual QStringList mimeTypes () const override;

private:
  void TypeChanged(const ezReflectedTypeChange& data);

private:
  const ezDocumentObjectManagerBase* m_pManager;


  /// \todo Broken delegates
  ezDelegate<void(const ezReflectedTypeChange&)> m_DelegateTypeChanged;
};


