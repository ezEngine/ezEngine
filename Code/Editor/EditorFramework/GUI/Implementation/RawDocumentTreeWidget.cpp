#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

/// Delegate object for the scene graph filter model.
///
/// Handles matching game objects against the search text by checking component type names
/// and, when the "ref:" keyword is used, whether any component property references a specific asset GUID.
class ezGameObjectFilter
{
public:
  bool Filter(QModelIndex index, const ezSearchPatternFilter& filter)
  {
    if (filter.GetSearchText() != m_sLastFilterText)
    {
      ParseFilter(filter.GetSearchText());
    }

    const ezQtDocumentTreeModel* pModel = qobject_cast<const ezQtDocumentTreeModel*>(index.model());
    if (pModel == nullptr)
      return false;

    const ezDocumentObject* pObj = pModel->GetObject(index);
    ezObjectAccessorBase* pAcc = pModel->GetDocumentTree()->GetDocument()->GetObjectAccessor();
    ezVariant comp;

    const ezInt32 iNum = pAcc->GetCountByName(pObj, "Components");
    for (ezInt32 i = 0; i < iNum; ++i)
    {
      if (pAcc->GetValueByName(pObj, "Components", comp, i).Failed())
        continue;

      const ezDocumentObject* pCompObj = pAcc->GetObject(comp.Get<ezUuid>());

      if (m_bGuidSearch)
      {
        if (ComponentReferencesGuid(pCompObj, m_SearchGuid))
          return true;
      }
      else
      {
        if (filter.PassesFilters(pCompObj->GetType()->GetTypeName()))
          return true;
      }
    }

    return false;
  }

private:
  void ParseFilter(const ezString& sText)
  {
    m_sLastFilterText = sText;
    m_bGuidSearch = false;

    const char* szRef = ezStringUtils::FindSubString_NoCase(sText.GetData(), "ref:");
    if (szRef != nullptr)
    {
      const char* szGuid = szRef + strlen("ref:");
      if (ezConversionUtils::IsStringUuid(szGuid))
      {
        m_SearchGuid = ezConversionUtils::ConvertStringToUuid(szGuid);
        m_bGuidSearch = true;
      }
    }
  }

  static bool ComponentReferencesGuid(const ezDocumentObject* pCompObj, const ezUuid& searchGuid)
  {
    const ezIReflectedTypeAccessor& acc = pCompObj->GetTypeAccessor();

    ezDynamicArray<const ezAbstractProperty*> properties;
    acc.GetType()->GetAllProperties(properties);

    for (const ezAbstractProperty* pProp : properties)
    {
      if (pProp->GetAttributeByType<ezAssetBrowserAttribute>() == nullptr)
        continue;

      const auto propVarType = pProp->GetSpecificType()->GetVariantType();
      if (propVarType != ezVariantType::String && propVarType != ezVariantType::StringView)
        continue;

      switch (pProp->GetCategory())
      {
        case ezPropertyCategory::Member:
        {
          const ezVariant val = acc.GetValue(pProp->GetPropertyName());
          if (val.CanConvertTo<ezString>())
          {
            const ezString sVal = val.ConvertTo<ezString>();
            if (ezConversionUtils::IsStringUuid(sVal) && ezConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
              return true;
          }
          break;
        }
        case ezPropertyCategory::Array:
        {
          const ezInt32 iCount = acc.GetCount(pProp->GetPropertyName());
          for (ezInt32 i = 0; i < iCount; ++i)
          {
            const ezVariant val = acc.GetValue(pProp->GetPropertyName(), i);
            if (val.CanConvertTo<ezString>())
            {
              const ezString sVal = val.ConvertTo<ezString>();
              if (ezConversionUtils::IsStringUuid(sVal) && ezConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
                return true;
            }
          }
          break;
        }
        case ezPropertyCategory::Map:
        {
          ezDynamicArray<ezVariant> keys;
          acc.GetKeys(pProp->GetPropertyName(), keys);
          for (const ezVariant& key : keys)
          {
            const ezVariant val = acc.GetValue(pProp->GetPropertyName(), key);
            if (val.CanConvertTo<ezString>())
            {
              const ezString sVal = val.ConvertTo<ezString>();
              if (ezConversionUtils::IsStringUuid(sVal) && ezConversionUtils::ConvertStringToUuid(sVal) == searchGuid)
                return true;
            }
          }
          break;
        }
        default:
          break;
      }
    }
    return false;
  }

  ezString m_sLastFilterText;
  bool m_bGuidSearch = false;
  ezUuid m_SearchGuid;
};

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* pParent)
  : ezQtItemView<QTreeView>(pParent)
{
  setObjectName("ezQtDocumentTreeView");
}

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel, ezSelectionManager* pSelection)
  : ezQtItemView<QTreeView>(pParent)
{
  setObjectName("ezQtDocumentTreeView");

  Initialize(pDocument, std::move(pModel), pSelection);
}

void ezQtDocumentTreeView::Initialize(ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel, ezSelectionManager* pSelection)
{
  m_pDocument = pDocument;
  m_pModel = std::move(pModel);
  m_pSelectionManager = pSelection;
  if (m_pSelectionManager == nullptr)
  {
    // If no selection manager is provided, fall back to the default selection.
    m_pSelectionManager = m_pDocument->GetSelectionManager();
  }

  m_pGameObjectFilter = std::make_unique<ezGameObjectFilter>();
  m_pFilterModel.reset(new ezQtTreeSearchFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());
  m_pFilterModel->SetCustomFilterFunc(ezMakeDelegate(&ezGameObjectFilter::Filter, m_pGameObjectFilter.get()));

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pFilterModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);
  setUniformRowHeights(true);

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
              SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr,
    "signal/slot connection failed");
  m_pSelectionManager->m_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));

  ezSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject = nullptr;
  e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
  SelectionEventHandler(e);
}

ezQtDocumentTreeView::~ezQtDocumentTreeView()
{
  m_pSelectionManager->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));
}

void ezQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  ezDeque<const ezDocumentObject*> sel;

  foreach (QModelIndex index, selection)
  {
    if (index.isValid() && index.column() == 0)
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const ezDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((ezSelectionManager*)m_pSelectionManager)->SetSelection(sel);
}

void ezQtDocumentTreeView::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case ezSelectionManagerEvent::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;

    case ezSelectionManagerEvent::Type::SelectionSet:
    case ezSelectionManagerEvent::Type::ObjectAdded:
    case ezSelectionManagerEvent::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      QModelIndex currentIndex;
      for (const ezDocumentObject* pObject : m_pSelectionManager->GetSelection())
      {
        currentIndex = m_pModel->ComputeModelIndex(pObject);
        currentIndex = m_pFilterModel->mapFromSource(currentIndex);

        if (currentIndex.isValid())
          selection.select(currentIndex, currentIndex);
      }
      if (currentIndex.isValid())
      {
        // We need to change the current index as well because the current index can trigger side effects. E.g. deleting the current index row triggers a selection change event.
        selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::SelectCurrent);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;

    case ezSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
      break;
  }
}

void ezQtDocumentTreeView::EnsureLastSelectedItemVisible()
{
  if (m_pSelectionManager->GetSelection().IsEmpty())
    return;

  const ezDocumentObject* pObject = m_pSelectionManager->GetSelection().PeekBack();
  EZ_ASSERT_DEBUG(m_pModel->GetDocumentTree()->GetDocument() == pObject->GetDocumentObjectManager()->GetDocument(), "Selection is from a different document.");

  auto index = m_pModel->ComputeModelIndex(pObject);
  index = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void ezQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void ezQtDocumentTreeView::SetAllowDeleteObjects(bool bAllow)
{
  m_bAllowDeleteObjects = bAllow;
}

bool ezQtDocumentTreeView::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;

    if (pEvent->type() == QEvent::KeyPress && keyEvent == QKeySequence::Delete)
    {
      if (m_bAllowDeleteObjects)
      {
        m_pDocument->DeleteSelectedObjects();
      }
      pEvent->accept();
      return true;
    }
  }

  return QTreeView::event(pEvent);
}
