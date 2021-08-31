#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_CVarWidget.h>
#include <QItemDelegate>
#include <QPointer>
#include <QWidget>

class QStandardItemModel;
class QSortFilterProxyModel;
class ezQtCVarModel;
class ezQtCVarWidget;

class ezQtCVarItemDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  explicit ezQtCVarItemDelegate(QObject* parent = nullptr)
    : QItemDelegate(parent)
  {
  }

  virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

  ezQtCVarModel* m_pModel = nullptr;
};

class ezQtCVarModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtCVarModel(ezQtCVarWidget* owner);
  ~ezQtCVarModel();

  void BeginResetModel();
  void EndResetModel();

public: // QAbstractItemModel interface
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

public:
  struct Entry
  {
    ezString m_sFullName;
    QString m_sDisplayString;
    Entry* m_pParentEntry = nullptr;
    ezDynamicArray<Entry*> m_ChildEntries;

    QString m_sPlugin;      // in which plugin a CVar is defined
    QString m_sDescription; // CVar description text
    ezVariant m_Value;
  };

  Entry* CreateEntry(const char* name);

  ezQtCVarWidget* m_pOwner = nullptr;
  ezDynamicArray<Entry*> m_RootEntries;
  ezDeque<Entry> m_AllEntries;
};

/// \brief Data used by ezQtCVarWidget to represent CVar states
struct EZ_GUIFOUNDATION_DLL ezCVarWidgetData
{
  mutable bool m_bNewEntry = true;

  ezString m_sPlugin;      // in which plugin a CVar is defined
  ezString m_sDescription; // CVar description text
  ezUInt8 m_uiType = 0;    // ezCVarType

  // 'union' over the different possible CVar types
  bool m_bValue = false;
  float m_fValue = 0.0f;
  ezInt32 m_iValue = 0;
  ezString m_sValue;
};

/// \brief Displays CVar values in a table and allows to modify them.
class EZ_GUIFOUNDATION_DLL ezQtCVarWidget : public QWidget, public Ui_CVarWidget
{
  Q_OBJECT

public:
  ezQtCVarWidget(QWidget* parent);
  ~ezQtCVarWidget();

  /// \brief Clears the table
  void Clear();

  /// \brief Recreates the full UI. This is necessary when elements were added or removed.
  void RebuildCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars);

  /// \brief Updates the existing UI. This is sufficient if values changed only.
  void UpdateCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars);

Q_SIGNALS:
  void onBoolChanged(const char* szCVar, bool newValue);
  void onFloatChanged(const char* szCVar, float newValue);
  void onIntChanged(const char* szCVar, int newValue);
  void onStringChanged(const char* szCVar, const char* newValue);

private Q_SLOTS:
  void SearchTextChanged(const QString& text);

private:
  QPointer<ezQtCVarModel> m_pItemModel;
  QPointer<QSortFilterProxyModel> m_pFilterModel;
  QPointer<ezQtCVarItemDelegate> m_pItemDelegate;
};
