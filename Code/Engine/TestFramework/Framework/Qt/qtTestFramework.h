#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/TestFramework.h>
#include <QObject>

/// \brief Derived ezTestFramework which signals the GUI to update whenever a new tests result comes in.
class EZ_TEST_DLL ezQtTestFramework : public QObject, public ezTestFramework
{
  Q_OBJECT
public:
  ezQtTestFramework(const char* szTestName, const char* szAbsTestDir, int argc, const char** argv);
  virtual ~ezQtTestFramework();

private:
  ezQtTestFramework(ezQtTestFramework&);
  void operator=(ezQtTestFramework&);

signals:
  void TestResultReceived(qint32 iTestIndex, qint32 iSubTestIndex);

protected:
  virtual void OutputImpl(ezTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(ezInt32 iSubTestIndex, bool bSuccess, double fDuration) override;
};

#endif

