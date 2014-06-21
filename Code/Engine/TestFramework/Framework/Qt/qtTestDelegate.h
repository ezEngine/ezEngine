#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <QStyledItemDelegate>

class ezQtTestFramework;

/// \brief Delegate for ezQtTestModel which shows bars for the test durations.
class EZ_TEST_DLL ezQtTestDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  ezQtTestDelegate(QObject* pParent);
  virtual ~ezQtTestDelegate();

public: //QStyledItemDelegate interface
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif

