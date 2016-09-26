#pragma once

#include <GuiFoundation/Basics.h>
#include <QWidget>

class QLineEdit;
class QPushButton;

/// \brief A widget that contains a line edit for a search text and a button to clear the search text.
///
/// The clear button is only active when the line edit has text.
/// The text can be cleared by pressing ESC while the line edit has focus.
class EZ_GUIFOUNDATION_DLL ezQtSearchWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtSearchWidget(QWidget* pParent);

  /// \brief Passed through to the QLineEdit
  void setText(const QString& text);

  /// \brief Passed through from the QLineEdit
  QString text() const;

  /// \brief Passed through to the QLineEdit
  void setPlaceholderText(const QString& text);

signals:
  /// \brief Passed through from the QLineEdit
  void textChanged(const QString& text);

private slots:
  void onLineEditTextChanged(const QString& text);
  void onClearButtonClicked(bool checked = false);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  QLineEdit* m_pLineEdit;
  QPushButton* m_pClearButton;
};


