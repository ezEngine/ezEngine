#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <QPushButton>

class QMenu;
class ezDocument;
class ezDynamicStringEnum;
class ezQtSearchableMenu;

/// A push button with a searchable drop-down menu that lets the user pick a value from an ezDynamicStringEnum.
///
/// Emits ValueSelected once the user picks a value. If the enum supports editing (storage file or edit command), an
/// entry to edit the available values is added as well. This is the shared building block used wherever a dynamic
/// string enum has to be edited, e.g. the property grid widget for ezDynamicStringEnumAttribute.
class EZ_EDITORFRAMEWORK_DLL ezQtDynamicStringEnumMenuButton : public QPushButton
{
  Q_OBJECT;

public:
  explicit ezQtDynamicStringEnumMenuButton(QWidget* pParent = nullptr);

  /// Selects which dynamic string enum the menu presents.
  void SetEnum(ezStringView sEnumName);
  ezDynamicStringEnum* GetEnum() const { return m_pEnum; }

  /// The document is passed along when refreshing values and when invoking the enum's edit command. May be null.
  void SetDocument(const ezDocument* pDocument) { m_pDocument = pDocument; }

  /// Updates the text shown on the button to the currently selected value.
  void SetCurrentValue(ezStringView sValue);

Q_SIGNALS:
  void ValueSelected(const QString& sValue);

private Q_SLOTS:
  void onMenuAboutToShow();

private:
  const ezDocument* m_pDocument = nullptr;
  ezDynamicStringEnum* m_pEnum = nullptr;
  QMenu* m_pMenu = nullptr;
  ezQtSearchableMenu* m_pSearchableMenu = nullptr;
  ezString m_sEnumName;

  static ezMap<ezString, QString> s_LastSearch;
};
