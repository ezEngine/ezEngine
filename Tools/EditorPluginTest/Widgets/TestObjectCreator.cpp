#include <PCH.h>
#include <EditorPluginTest/Widgets/TestObjectCreator.moc.h>
#include <EditorFramework/GUI/QtHelpers.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <QMimeData>

ezTestObjectCreatorWidget::ezTestObjectCreatorWidget(const ezDocumentObjectManagerBase* pManager, QWidget* parent) : QListWidget(parent)
{
  m_pManager = pManager;

  ezReflectedTypeManager::m_TypeAddedEvent.AddEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));
  ezReflectedTypeManager::m_TypeChangedEvent.AddEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));
  ezReflectedTypeManager::m_TypeRemovedEvent.AddEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));

  TypeChanged(ezReflectedTypeChange());
}

ezTestObjectCreatorWidget::~ezTestObjectCreatorWidget()
{
  ezReflectedTypeManager::m_TypeAddedEvent.RemoveEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));
  ezReflectedTypeManager::m_TypeChangedEvent.RemoveEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));
  ezReflectedTypeManager::m_TypeRemovedEvent.RemoveEventHandler(ezDelegate<void (const ezReflectedTypeChange&)>(&ezTestObjectCreatorWidget::TypeChanged, this));
}

void ezTestObjectCreatorWidget::TypeChanged(const ezReflectedTypeChange& data)
{
  ezHybridArray<ezReflectedTypeHandle, 32> Types;
  m_pManager->GetCreateableTypes(Types);

  ezQtBlockSignals b(this);

  clear();

  for (ezUInt32 i = 0; i < Types.GetCount(); ++i)
  {
    addItem(QString::fromUtf8(Types[i].GetType()->GetTypeName().GetData()));
  }
  setDragEnabled(true);
}

QMimeData* ezTestObjectCreatorWidget::mimeData(const QList<QListWidgetItem *> items) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  stream << items[0]->text();

  mimeData->setData("application/ezEditor.ObjectCreator", encodedData);
  return mimeData;
}

QStringList ezTestObjectCreatorWidget::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.ObjectCreator";
  return types;
}

