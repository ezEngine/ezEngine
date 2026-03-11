#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QToolTip>

ezQtLayerAdapter::ezQtLayerAdapter(ezScene2Document* pDocument)
  : ezQtDocumentTreeModelAdapter(pDocument->GetSceneObjectManager(), ezGetStaticRTTI<ezSceneLayer>(), nullptr)
{
  m_pSceneDocument = pDocument;
  m_pSceneDocument->m_LayerEvents.AddEventHandler(
    ezMakeDelegate(&ezQtLayerAdapter::LayerEventHandler, this), m_LayerEventUnsubscriber);

  ezDocument::s_EventsAny.AddEventHandler(ezMakeDelegate(&ezQtLayerAdapter::DocumentEventHander, this), m_DocumentEventUnsubscriber);
}

ezQtLayerAdapter::~ezQtLayerAdapter()
{
  m_LayerEventUnsubscriber.Unsubscribe();
  m_DocumentEventUnsubscriber.Unsubscribe();
}

QVariant ezQtLayerAdapter::data(const ezDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case UserRoles::LayerGuid:
    {
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pObject, "Layer");
      return QVariant::fromValue(layerGuid);
    }
    break;
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pObject, "Layer");
      // Use curator to get name in case the layer is unloaded and there is no document to query.
      const ezAssetCurator::ezLockedSubAsset subAsset = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (subAsset.isValid())
      {
        if (iRole == Qt::ToolTipRole)
        {
          return ezMakeQString(subAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
        }
        ezStringBuilder sName = subAsset->GetName();
        QString sQtName = QString::fromUtf8(sName.GetData());
        if (ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
        {
          if (pLayer->IsModified())
          {
            sQtName += "*";
          }
        }
        return sQtName;
      }
      else
      {
        return QStringLiteral("Layer guid not found");
      }
    }
    break;

    case Qt::DecorationRole:
    {
      return ezQtUiServices::GetCachedIconResource(":/EditorPluginScene/Icons/Layer.svg");
    }
    break;
    case Qt::ForegroundRole:
    {
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pObject, "Layer");
      if (!m_pSceneDocument->IsLayerLoaded(layerGuid))
      {
        return QVariant();
      }
    }
    break;
    case Qt::FontRole:
    {
      QFont font;
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pObject, "Layer");
      if (m_pSceneDocument->GetActiveLayer() == layerGuid)
        font.setBold(true);
      return font;
    }
    break;
  }

  return QVariant();
}

bool ezQtLayerAdapter::setData(const ezDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

void ezQtLayerAdapter::LayerEventHandler(const ezScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case ezScene2LayerEvent::Type::LayerUnloaded:
    case ezScene2LayerEvent::Type::LayerLoaded:
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
    }
    break;
    case ezScene2LayerEvent::Type::ActiveLayerChanged:
    {
      QVector<int> v;
      v.push_back(Qt::FontRole);
      if (auto pObject = m_pSceneDocument->GetLayerObject(m_CurrentActiveLayer))
      {
        Q_EMIT dataChanged(pObject, v);
      }
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
      m_CurrentActiveLayer = e.m_layerGuid;
    }
    default:
      break;
  }
}

void ezQtLayerAdapter::DocumentEventHander(const ezDocumentEvent& e)
{
  if (e.m_Type == ezDocumentEvent::Type::DocumentSaved || e.m_Type == ezDocumentEvent::Type::ModifiedChanged)
  {
    const ezDocumentObject* pLayerObj = m_pSceneDocument->GetLayerObject(e.m_pDocument->GetGuid());
    if (pLayerObj)
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(pLayerObj, v);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtLayerDelegate::ezQtLayerDelegate(QObject* pParent, ezScene2Document* pDocument)
  : ezQtItemDelegate(pParent)
  , m_pDocument(pDocument)
{
}

bool ezQtLayerDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QRect visibleRect = GetVisibleIconRect(option);
  const QRect loadedRect = GetLoadedIconRect(option);
  if (pEvent->button() == Qt::MouseButton::LeftButton && (visibleRect.contains(pEvent->position().toPoint()) || loadedRect.contains(pEvent->position().toPoint())))
  {
    m_bPressed = true;
    pEvent->accept();
    return true;
  }
  return ezQtItemDelegate::mousePressEvent(pEvent, option, index);
}

bool ezQtLayerDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->position().toPoint()))
    {
      const ezUuid layerGuid = index.data(ezQtLayerAdapter::UserRoles::LayerGuid).value<ezUuid>();
      const bool bVisible = !m_pDocument->IsLayerVisible(layerGuid);
      m_pDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
    }
    else if (loadedRect.contains(pEvent->position().toPoint()))
    {
      const ezUuid layerGuid = index.data(ezQtLayerAdapter::UserRoles::LayerGuid).value<ezUuid>();
      if (layerGuid != m_pDocument->GetGuid())
      {
        ezLayerAction::ToggleLayerLoaded(m_pDocument, layerGuid);
      }
    }
    m_bPressed = false;
    pEvent->accept();
    return true;
  }
  return ezQtItemDelegate::mouseReleaseEvent(pEvent, option, index);
}

bool ezQtLayerDelegate::mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    return true;
  }
  return ezQtItemDelegate::mouseMoveEvent(pEvent, option, index);
}

void ezQtLayerDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  ezQtItemDelegate::paint(pPainter, opt, index);

  {
    const ezUuid layerGuid = index.data(ezQtLayerAdapter::UserRoles::LayerGuid).value<ezUuid>();
    if (layerGuid.IsValid())
    {
      {
        const QRect thumbnailRect = GetVisibleIconRect(opt);
        const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);

        if (bVisible)
        {
          ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsVisible.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsHidden.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }

      if (layerGuid != m_pDocument->GetGuid())
      {
        const QRect thumbnailRect = GetLoadedIconRect(opt);
        const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);

        if (bLoaded)
        {
          ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerLoaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerUnloaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }
    }
  }
}

QSize ezQtLayerDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  return ezQtItemDelegate::sizeHint(opt, index);
}

bool ezQtLayerDelegate::helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const ezUuid layerGuid = index.data(ezQtLayerAdapter::UserRoles::LayerGuid).value<ezUuid>();
  if (layerGuid.IsValid())
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->pos()))
    {
      const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bVisible ? "Hide Layer" : "Show Layer", pView);
      return true;
    }
    else if (loadedRect.contains(pEvent->pos()))
    {
      const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bLoaded ? "Unload Layer" : "Load Layer", pView);
      return true;
    }
  }
  return ezQtItemDelegate::helpEvent(pEvent, pView, option, index);
}

QRect ezQtLayerDelegate::GetVisibleIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height(), 0, 0, 0);
}

QRect ezQtLayerDelegate::GetLoadedIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height() * 2, 0, -opt.rect.height(), 0);
}

//////////////////////////////////////////////////////////////////////////

ezQtLayerModel::ezQtLayerModel(ezScene2Document* pDocument)
  : ezQtDocumentTreeModel(pDocument->GetSceneObjectManager(), pDocument->GetSettingsObject()->GetGuid())
  , m_pDocument(pDocument)
{
  m_sTargetContext = "layertree";
}
