#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

ezQtCVarWidget::ezQtCVarWidget(QWidget* parent)
  : QWidget(parent)
{
  setupUi(this);

  m_pItemModel = new ezQtCVarModel(this);

  m_pFilterModel = new QSortFilterProxyModel(this);
  m_pFilterModel->setSourceModel(m_pItemModel);

  m_pItemDelegate = new ezQtCVarItemDelegate(this);
  m_pItemDelegate->m_pModel = m_pItemModel;

  CVarsView->setModel(m_pFilterModel);
  CVarsView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  CVarsView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  CVarsView->setHeaderHidden(false);
  CVarsView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged | QAbstractItemView::EditTrigger::SelectedClicked);
  CVarsView->setItemDelegateForColumn(1, m_pItemDelegate);
}

ezQtCVarWidget::~ezQtCVarWidget() {}

void ezQtCVarWidget::Clear()
{
  clearFocus();
  m_pItemModel->BeginResetModel();
  m_pItemModel->EndResetModel();
}

void ezQtCVarWidget::RebuildCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars)
{
  UpdateCVarUI(cvars);

  CVarsView->expandAll();
  CVarsView->resizeColumnToContents(0);
  CVarsView->resizeColumnToContents(1);
}

void ezQtCVarWidget::UpdateCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars)
{
  int row = 0;

  m_pItemModel->BeginResetModel();

  for (auto it = cvars.GetIterator(); it.IsValid(); ++it, ++row)
  {
    it.Value().m_bNewEntry = false;

    auto item = m_pItemModel->CreateEntry(it.Key().GetData());
    item->m_sDescription = it.Value().m_sDescription;
    item->m_sPlugin = it.Value().m_sPlugin;

    switch (it.Value().m_uiType)
    {
      case ezCVarType::Bool:
        item->m_Value = it.Value().m_bValue;
        break;
      case ezCVarType::Float:
        item->m_Value = it.Value().m_fValue;
        break;
      case ezCVarType::Int:
        item->m_Value = it.Value().m_iValue;
        break;
      case ezCVarType::String:
        item->m_Value = it.Value().m_sValue;
        break;
    }
  }

  m_pItemModel->EndResetModel();
}

void ezQtCVarWidget::BoolChanged(int index)
{
  const QComboBox* pValue = qobject_cast<QComboBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const bool newValue = (pValue->currentIndex() == 0);

  Q_EMIT onBoolChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::FloatChanged()
{
  const QDoubleSpinBox* pValue = qobject_cast<QDoubleSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const float newValue = (float)pValue->value();

  Q_EMIT onFloatChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::IntChanged()
{
  const QSpinBox* pValue = qobject_cast<QSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const int newValue = (int)pValue->value();

  Q_EMIT onIntChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::StringChanged()
{
  const QLineEdit* pValue = qobject_cast<QLineEdit*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const QString newValue = pValue->text();

  Q_EMIT onStringChanged(cvar.toUtf8().data(), newValue.toUtf8().data());
}

ezQtCVarModel::ezQtCVarModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
}

ezQtCVarModel::~ezQtCVarModel() = default;

void ezQtCVarModel::BeginResetModel()
{
  beginResetModel();
  m_RootEntries.Clear();
  m_AllEntries.Clear();
}

void ezQtCVarModel::EndResetModel()
{
  endResetModel();
}

QVariant ezQtCVarModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Name";

      case 1:
        return "Value";

      case 2:
        return "Description";

      case 3:
        return "Plugin";
    }
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

bool ezQtCVarModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
  if (index.column() == 1 && role == Qt::EditRole)
  {
    ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(index.internalId());

    switch (e->m_Value.GetType())
    {
      case ezVariantType::Bool:
        e->m_Value = value.toBool();
        break;
      case ezVariantType::Int32:
        e->m_Value = value.toInt();
        break;
      case ezVariantType::Float:
        e->m_Value = value.toFloat();
        break;
      case ezVariantType::String:
        e->m_Value = value.toString().toUtf8().data();
        break;
      default:
        break;
    }
  }

  return QAbstractItemModel::setData(index, value, role);
}

QVariant ezQtCVarModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(index.internalId());


  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
      case 0:
        return e->m_sDisplayString;

      case 1:
        return e->m_Value.ConvertTo<ezString>().GetData();

      case 2:
        return e->m_sDescription;

      case 3:
        return e->m_sPlugin;
    }
  }

  if (role == Qt::DecorationRole && index.column() == 0)
  {
    if (e->m_Value.IsValid())
    {
      return ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.png");
    }
  }

  if (role == Qt::ToolTipRole && index.column() == 0)
  {
    if (e->m_Value.IsValid())
    {
      return e->m_sDescription + " | " + e->m_sPlugin;
    }
  }

  if (role == Qt::EditRole && index.column() == 1)
  {
    switch (e->m_Value.GetType())
    {
      case ezVariantType::Bool:
        return e->m_Value.Get<bool>();
      case ezVariantType::Int32:
        return e->m_Value.Get<ezInt32>();
      case ezVariantType::Float:
        return e->m_Value.Get<float>();
      case ezVariantType::String:
        return e->m_Value.Get<ezString>().GetData();
      default:
        break;
    }
  }
  return QVariant();
}

Qt::ItemFlags ezQtCVarModel::flags(const QModelIndex& index) const
{
  if (index.column() == 1)
  {
    ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(index.internalId());

    if (e->m_Value.IsValid())
    {
      return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable;
    }
  }

  return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled;
}

QModelIndex ezQtCVarModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(parent.internalId());
    return createIndex(row, column, const_cast<ezQtCVarModel::Entry*>(e->m_ChildEntries[row]));
  }
  else
  {
    return createIndex(row, column, const_cast<ezQtCVarModel::Entry*>(m_RootEntries[row]));
  }
}

QModelIndex ezQtCVarModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(index.internalId());

  if (e->m_pParentEntry == nullptr)
    return QModelIndex();

  ezQtCVarModel::Entry* p = e->m_pParentEntry;

  for (ezUInt32 row = 0; row < p->m_ChildEntries.GetCount(); ++row)
  {
    if (p->m_ChildEntries[row] == e)
    {
      return createIndex(row, index.column(), p);
    }
  }

  return QModelIndex();
}

int ezQtCVarModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(parent.internalId());

    return (int)e->m_ChildEntries.GetCount();
  }
  else
  {
    return (int)m_RootEntries.GetCount();
  }
}

int ezQtCVarModel::columnCount(const QModelIndex& index /*= QModelIndex()*/) const
{
  return 2;
}

ezQtCVarModel::Entry* ezQtCVarModel::CreateEntry(const char* name)
{
  ezStringBuilder tmp = name;
  ezStringBuilder tmp2;

  ezHybridArray<ezStringView, 8> pieces;
  tmp.Split(false, pieces, ".", "_");

  ezDynamicArray<Entry*>* vals = &m_RootEntries;
  Entry* parentEntry = nullptr;

  for (ezUInt32 p = 0; p < pieces.GetCount(); ++p)
  {
    QString piece = pieces[p].GetData(tmp2);
    for (ezUInt32 v = 0; v < vals->GetCount(); ++v)
    {
      if ((*vals)[v]->m_sDisplayString == piece)
      {
        parentEntry = (*vals)[v];
        vals = &((*vals)[v]->m_ChildEntries);
        goto found;
      }
    }

    {
      auto& newItem = m_AllEntries.ExpandAndGetRef();
      newItem.m_sDisplayString = piece;
      newItem.m_pParentEntry = parentEntry;

      vals->PushBack(&newItem);

      parentEntry = &newItem;
      vals = &newItem.m_ChildEntries;
    }
  found:;
  }

  return parentEntry;
}

QModelIndex ezQtCVarModel::ComputeFullIndex(const QModelIndex& idx)
{
  if (idx.parent().isValid())
  {
    return index(idx.row(), idx.column(), ComputeFullIndex(idx.parent()));
  }
  else
  {
    return index(idx.row(), idx.column());
  }
}

QWidget* ezQtCVarItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& idx) const
{
  auto index = m_pModel->ComputeFullIndex(idx);
  ezQtCVarModel::Entry* e = reinterpret_cast<ezQtCVarModel::Entry*>(index.internalPointer());

  if (!e->m_Value.IsValid())
    return nullptr;

  if (e->m_Value.IsA<bool>())
  {
    QComboBox* ret = new QComboBox(parent);
    ret->addItem("true");
    ret->addItem("false");
    return ret;
  }

  if (e->m_Value.IsA<ezInt32>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    ret->setValidator(new QIntValidator(ret));
    return ret;
  }

  if (e->m_Value.IsA<float>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    auto val = new QDoubleValidator(ret);
    val->setDecimals(3);
    ret->setValidator(val);
    return ret;
  }

  if (e->m_Value.IsA<ezString>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    return ret;
  }

  return nullptr;
}

void ezQtCVarItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  QVariant value = index.model()->data(index, Qt::EditRole);

  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    pLine->setText(value.toString());
    pLine->selectAll();
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    pLine->setCurrentIndex(value.toBool() ? 0 : 1);
  }
}

void ezQtCVarItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    model->setData(index, pLine->text(), Qt::EditRole);
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    model->setData(index, pLine->currentText(), Qt::EditRole);
  }
}
