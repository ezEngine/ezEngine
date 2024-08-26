#pragma once

#ifdef EZ_USE_QT

#  include <QObject>
#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

/// \brief Derived ezTestFramework which signals the GUI to update whenever a new tests result comes in.
class EZ_TEST_DLL ezQtTestFramework : public QObject, public ezTestFramework
{
  Q_OBJECT
public:
  ezQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~ezQtTestFramework();

private:
  ezQtTestFramework(ezQtTestFramework&);
  void operator=(ezQtTestFramework&);

Q_SIGNALS:
  void TestResultReceived(qint32 testIndex, qint32 subTestIndex);

protected:
  virtual void OutputImpl(ezTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration) override;
  virtual void SetSubTestStatusImpl(ezUInt32 uiSubTestIndex, const char* szStatus) override;
};

#endif
