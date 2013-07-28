#include <PCH.h>
#include <QtWidgets>
#include <InputTest/MainWindow.h>
#include <Core/Input/InputManager.h>
#include <InputWindows/InputDeviceWindows.h>
#include <InputXBox360/InputDeviceXBox.h>

ezMainWindow::ezMainWindow() : QMainWindow()
{
  setupUi(this);

  TableInput->setModel(&m_ListModel);

  ComboMapController1->blockSignals(true);
  ComboMapController2->blockSignals(true);
  ComboMapController3->blockSignals(true);
  ComboMapController4->blockSignals(true);

  ComboMapController1->addItem("<Deactivate>", QVariant(-1));
  ComboMapController1->addItem("Controller 1", QVariant(0));
  ComboMapController1->addItem("Controller 2", QVariant(1));
  ComboMapController1->addItem("Controller 3", QVariant(2));
  ComboMapController1->addItem("Controller 4", QVariant(3));

  ComboMapController2->addItem("<Deactivate>", QVariant(-1));
  ComboMapController2->addItem("Controller 1", QVariant(0));
  ComboMapController2->addItem("Controller 2", QVariant(1));
  ComboMapController2->addItem("Controller 3", QVariant(2));
  ComboMapController2->addItem("Controller 4", QVariant(3));

  ComboMapController3->addItem("<Deactivate>", QVariant(-1));
  ComboMapController3->addItem("Controller 1", QVariant(0));
  ComboMapController3->addItem("Controller 2", QVariant(1));
  ComboMapController3->addItem("Controller 3", QVariant(2));
  ComboMapController3->addItem("Controller 4", QVariant(3));

  ComboMapController4->addItem("<Deactivate>", QVariant(-1));
  ComboMapController4->addItem("Controller 1", QVariant(0));
  ComboMapController4->addItem("Controller 2", QVariant(1));
  ComboMapController4->addItem("Controller 3", QVariant(2));
  ComboMapController4->addItem("Controller 4", QVariant(3));

  ComboMapController1->setCurrentIndex(g_InputDeviceXBox360.GetPhysicalToVirtualControllerMapping(0) + 1);
  ComboMapController2->setCurrentIndex(g_InputDeviceXBox360.GetPhysicalToVirtualControllerMapping(1) + 1);
  ComboMapController3->setCurrentIndex(g_InputDeviceXBox360.GetPhysicalToVirtualControllerMapping(2) + 1);
  ComboMapController4->setCurrentIndex(g_InputDeviceXBox360.GetPhysicalToVirtualControllerMapping(3) + 1);

  ComboMapController1->blockSignals(false);
  ComboMapController2->blockSignals(false);
  ComboMapController3->blockSignals(false);
  ComboMapController4->blockSignals(false);
}

bool ezMainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
  MSG* msg = (MSG*) message;

  ezInputDeviceWindows* pDevice = (ezInputDeviceWindows*) ezInputManager::GetInputDeviceByName("MouseKeyboardWindows");

  if (pDevice)
    pDevice->WindowMessage(msg->hwnd, msg->message, msg->wParam, msg->lParam);

  return false;
}

void ezMainWindow::paintEvent(QPaintEvent* event)
{
  ezInputManager::Update();

  m_ListModel.layoutChanged();

  CheckClipCursor->setChecked(g_InputDeviceWindows.GetClipMouseCursor());
  CheckShowCursor->setChecked(g_InputDeviceWindows.GetShowMouseCursor());

  ComboMapController1->setEnabled(g_InputDeviceXBox360.IsControllerConnected(0));
  ComboMapController2->setEnabled(g_InputDeviceXBox360.IsControllerConnected(1));
  ComboMapController3->setEnabled(g_InputDeviceXBox360.IsControllerConnected(2));
  ComboMapController4->setEnabled(g_InputDeviceXBox360.IsControllerConnected(3));

  if (g_InputDeviceXBox360.IsControllerConnected(0))
    Controller1_Connected->setText(QString::fromUtf8("Controller 1 is connected."));
  else
    Controller1_Connected->setText(QString::fromUtf8("Controller 1 is NOT connected."));

  if (g_InputDeviceXBox360.IsControllerConnected(1))
    Controller2_Connected->setText(QString::fromUtf8("Controller 2 is connected."));
  else
    Controller2_Connected->setText(QString::fromUtf8("Controller 2 is NOT connected."));

  if (g_InputDeviceXBox360.IsControllerConnected(2))
    Controller3_Connected->setText(QString::fromUtf8("Controller 3 is connected."));
  else
    Controller3_Connected->setText(QString::fromUtf8("Controller 3 is NOT connected."));

  if (g_InputDeviceXBox360.IsControllerConnected(3))
    Controller4_Connected->setText(QString::fromUtf8("Controller 4 is connected."));
  else
    Controller4_Connected->setText(QString::fromUtf8("Controller 4 is NOT connected."));

  update();
}

void ezMainWindow::on_CheckClipCursor_clicked()
{
  g_InputDeviceWindows.SetClipMouseCursor(!CheckClipCursor->isChecked());
}

void ezMainWindow::on_CheckShowCursor_clicked()
{
  g_InputDeviceWindows.SetShowMouseCursor(!CheckShowCursor->isChecked());
}

void ezMainWindow::on_ComboMapController1_currentIndexChanged(int index)
{
  g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(0, index - 1);
}

void ezMainWindow::on_ComboMapController2_currentIndexChanged(int index)
{
  g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(1, index - 1);
}

void ezMainWindow::on_ComboMapController3_currentIndexChanged(int index)
{
  g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(2, index - 1);
}

void ezMainWindow::on_ComboMapController4_currentIndexChanged(int index)
{
  g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(3, index - 1);
}


