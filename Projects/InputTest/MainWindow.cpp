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

  ComboMapController1->setCurrentIndex(ezInputDeviceXBox360::GetDevice()->GetPhysicalToVirtualControllerMapping(0) + 1);
  ComboMapController2->setCurrentIndex(ezInputDeviceXBox360::GetDevice()->GetPhysicalToVirtualControllerMapping(1) + 1);
  ComboMapController3->setCurrentIndex(ezInputDeviceXBox360::GetDevice()->GetPhysicalToVirtualControllerMapping(2) + 1);
  ComboMapController4->setCurrentIndex(ezInputDeviceXBox360::GetDevice()->GetPhysicalToVirtualControllerMapping(3) + 1);

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

  CheckClipCursor->setChecked(ezInputDeviceWindows::GetDevice()->GetClipMouseCursor());
  CheckShowCursor->setChecked(ezInputDeviceWindows::GetDevice()->GetShowMouseCursor());

  ComboMapController1->setEnabled(ezInputDeviceXBox360::GetDevice()->IsControllerConnected(0));
  ComboMapController2->setEnabled(ezInputDeviceXBox360::GetDevice()->IsControllerConnected(1));
  ComboMapController3->setEnabled(ezInputDeviceXBox360::GetDevice()->IsControllerConnected(2));
  ComboMapController4->setEnabled(ezInputDeviceXBox360::GetDevice()->IsControllerConnected(3));

  if (ezInputDeviceXBox360::GetDevice()->IsControllerConnected(0))
    Controller1_Connected->setText(QString::fromUtf8("Controller 1 is connected."));
  else
    Controller1_Connected->setText(QString::fromUtf8("Controller 1 is NOT connected."));

  if (ezInputDeviceXBox360::GetDevice()->IsControllerConnected(1))
    Controller2_Connected->setText(QString::fromUtf8("Controller 2 is connected."));
  else
    Controller2_Connected->setText(QString::fromUtf8("Controller 2 is NOT connected."));

  if (ezInputDeviceXBox360::GetDevice()->IsControllerConnected(2))
    Controller3_Connected->setText(QString::fromUtf8("Controller 3 is connected."));
  else
    Controller3_Connected->setText(QString::fromUtf8("Controller 3 is NOT connected."));

  if (ezInputDeviceXBox360::GetDevice()->IsControllerConnected(3))
    Controller4_Connected->setText(QString::fromUtf8("Controller 4 is connected."));
  else
    Controller4_Connected->setText(QString::fromUtf8("Controller 4 is NOT connected."));

  update();
}

void ezMainWindow::on_CheckClipCursor_clicked()
{
  ezInputDeviceWindows::GetDevice()->SetClipMouseCursor(!CheckClipCursor->isChecked());
}

void ezMainWindow::on_CheckShowCursor_clicked()
{
  ezInputDeviceWindows::GetDevice()->SetShowMouseCursor(!CheckShowCursor->isChecked());
}

void ezMainWindow::on_ComboMapController1_currentIndexChanged(int index)
{
  ezInputDeviceXBox360::GetDevice()->SetPhysicalToVirtualControllerMapping(0, index - 1);
}

void ezMainWindow::on_ComboMapController2_currentIndexChanged(int index)
{
  ezInputDeviceXBox360::GetDevice()->SetPhysicalToVirtualControllerMapping(1, index - 1);
}

void ezMainWindow::on_ComboMapController3_currentIndexChanged(int index)
{
  ezInputDeviceXBox360::GetDevice()->SetPhysicalToVirtualControllerMapping(2, index - 1);
}

void ezMainWindow::on_ComboMapController4_currentIndexChanged(int index)
{
  ezInputDeviceXBox360::GetDevice()->SetPhysicalToVirtualControllerMapping(3, index - 1);
}


