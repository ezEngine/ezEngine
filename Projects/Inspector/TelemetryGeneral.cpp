#include <Inspector/MainWindow.moc.h>
#include <Inspector/GeneralWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qmessagebox.h>

void ezGeneralWidget::ProcessTelemetry_General(void* pPassThrough)
{
  ezGeneralWidget* pWindow = (ezGeneralWidget*) pPassThrough;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('APP', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'ASRT':
      {
        ezString sSourceFile, sFunction, sExpression, sMessage;
        ezUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

        ezStringBuilder sOut;

        ezMainWindow::s_pWidget->Log("");
        ezMainWindow::s_pWidget->Log("<<< Application Assertion >>>");
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("    Expression: '%s'", sExpression.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("    Message: '%s'", sMessage.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("   File: '%s'", sSourceFile.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        sOut.Format("   Line: %i", uiLine);
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        sOut.Format("   In Function: '%s'", sFunction.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        ezMainWindow::s_pWidget->Log("");

        ezMainWindow::s_pWidget->Log(">>> Application Assertion <<<");
        ezMainWindow::s_pWidget->Log("");
      }
      break;
    case 'DATA':
      {
        ezString sPlatform;
        ezUInt32 uiCoreCount = 0;
        ezUInt64 uiMemorySize = 0;
        bool bIs64Bit = false;

        Msg.GetReader() >> sPlatform;
        Msg.GetReader() >> uiCoreCount;
        Msg.GetReader() >> uiMemorySize;
        Msg.GetReader() >> bIs64Bit;

        ezStringBuilder sInfo;
        sInfo.Format("%s %s, %i Cores, %.1f GB RAM", sPlatform.GetData(), bIs64Bit ? "64 Bit" : "32 Bit", uiCoreCount, uiMemorySize / 1024.0f / 1024.0f / 1024.0f);

        pWindow->LabelAppName->setText(sInfo.GetData());
      }
      break;
    }
  }
}

void ezGeneralWidget::on_ButtonConnect_clicked()
{
  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Input Server Name or IP Address", "", QLineEdit::Normal, "", &bOk);

  if (!bOk)
    return;

  if (ezTelemetry::ConnectToServer(sRes.toUtf8().data()) == EZ_SUCCESS)
  {
  }
}


