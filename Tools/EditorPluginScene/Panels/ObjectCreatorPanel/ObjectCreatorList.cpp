#include <PCH.h>
#include <EditorPluginScene/Panels/ObjectCreatorPanel/ObjectCreatorList.moc.h>
#include <EditorFramework/GUI/QtHelpers.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <QMimeData>

ezObjectCreatorList::ezObjectCreatorList(const ezDocumentObjectManager* pManager, QWidget* parent) : QListWidget(parent)
{
  m_pManager = pManager;

  ezPhantomRttiManager::m_TypeAddedEvent.AddEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));
  ezPhantomRttiManager::m_TypeChangedEvent.AddEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));
  ezPhantomRttiManager::m_TypeRemovedEvent.AddEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));

  TypeChanged(ezPhantomTypeChange());
}

ezObjectCreatorList::~ezObjectCreatorList()
{
  ezPhantomRttiManager::m_TypeAddedEvent.RemoveEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));
  ezPhantomRttiManager::m_TypeChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));
  ezPhantomRttiManager::m_TypeRemovedEvent.RemoveEventHandler(ezMakeDelegate(&ezObjectCreatorList::TypeChanged, this));
}

void ezObjectCreatorList::TypeChanged(const ezPhantomTypeChange& data)
{
  ezHybridArray<const ezRTTI*, 32> Types;
  m_pManager->GetCreateableTypes(Types);

  ezQtBlockSignals b(this);

  clear();

  for (ezUInt32 i = 0; i < Types.GetCount(); ++i)
  {
    addItem(QString::fromUtf8(Types[i]->GetTypeName()));
  }
  setDragEnabled(true);
}

QMimeData* ezObjectCreatorList::mimeData(const QList<QListWidgetItem *> items) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  stream << items[0]->text();

  mimeData->setData("application/ezEditor.ObjectCreator", encodedData);
  return mimeData;
}

QStringList ezObjectCreatorList::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.ObjectCreator";
  return types;
}

