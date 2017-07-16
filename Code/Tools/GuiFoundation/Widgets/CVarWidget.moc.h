#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Basics.h>
#include <Code/Tools/GuiFoundation/ui_CVarWidget.h>
#include <QWidget>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>

/// \brief Data used by ezQtCVarWidget to represent CVar states
struct EZ_GUIFOUNDATION_DLL ezCVarWidgetData
{
  mutable ezInt32 m_iTableRow = -1; // updated by ezQtCVarWidget::RebuildCVarUI

  ezString m_sPlugin; // in which plugin a CVar is defined
  ezString m_sDescription; // CVar description text
  ezUInt8 m_uiType = 0; // ezCVarType

  // 'union' over the different possible CVar types
  bool m_bValue = false;
  float m_fValue = 0.0f;
  ezString m_sValue;
  ezInt32 m_iValue = 0;
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

signals:
  void onBoolChanged(const char* szCVar, bool newValue);
  void onFloatChanged(const char* szCVar, float newValue);
  void onIntChanged(const char* szCVar, int newValue);
  void onStringChanged(const char* szCVar, const char* newValue);

private slots:
  void BoolChanged(int index);
  void FloatChanged(double d);
  void IntChanged(int i);
  void StringChanged(const QString& val);
};
