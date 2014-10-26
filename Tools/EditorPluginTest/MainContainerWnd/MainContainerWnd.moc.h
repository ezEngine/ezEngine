#pragma once

#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>

class ezMainContainerWnd : public ezContainerWindow
{
  Q_OBJECT

public:
  ezMainContainerWnd();

private slots:
  void OnMenuSettingsPlugins();

private:


};

