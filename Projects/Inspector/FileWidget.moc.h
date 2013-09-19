#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_FileWidget.h>
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
    CreateDirsFailed
  };

  struct FileOpData
  {
    ezString m_sFile;
    FileOpState m_State;
    //ezTime m_StartTime;
    ezTime m_BlockedDuration;
    ezUInt64 m_uiBytesAccessed;

    FileOpData()
    {
      m_State = None;
      m_uiBytesAccessed = 0;
    }
  };

  const char* GetStateString(FileOpState State) const;

  bool m_bUpdateTable;
  ezHashTable<ezUInt32, FileOpData> m_FileOps;
};


