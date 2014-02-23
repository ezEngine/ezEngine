#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_FileWidget.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>

class ezFileWidget : public QDockWidget, public Ui_FileWidget
{
public:
  Q_OBJECT

public:
  ezFileWidget(QWidget* parent = 0);

  static ezFileWidget* s_pWidget;

private slots:

  virtual void on_SpinLimitToRecent_valueChanged(int val);
  virtual void on_SpinMinDuration_valueChanged(double val);
  virtual void on_LineFilterByName_textChanged();
  virtual void on_ComboThread_currentIndexChanged(int state);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  enum FileOpState
  {
    None,
    OpenReading,
    OpenWriting,
    ClosedReading,
    ClosedWriting,
    OpenReadingFailed,
    OpenWritingFailed,
    FileExists,
    FileExistsFailed,
    FileDelete,
    FileDeleteFailed,
    FileCopy,
    FileCopyFailed,
    CreateDirs,
    CreateDirsFailed,
    FileStat,
    FileStatFailed,
    FileCasing,
    FileCasingFailed,
  };

  struct FileOpData
  {
    ezString m_sFile;
    FileOpState m_State;
    ezTime m_StartTime;
    ezTime m_BlockedDuration;
    ezUInt64 m_uiBytesAccessed;
    ezUInt8 m_uiThreadTypes; // 1 = Main, 2 = Task: Loading, 4 = Other

    FileOpData()
    {
      m_State = None;
      m_uiBytesAccessed = 0;
      m_uiThreadTypes = 0;
    }
  };

  QTableWidgetItem* GetStateString(FileOpState State) const;

  ezInt32 m_iMaxID;
  ezTime m_LastTableUpdate;
  bool m_bUpdateTable;
  ezHashTable<ezUInt32, FileOpData> m_FileOps;
};


