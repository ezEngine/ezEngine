#include <Inspector/MainWindow.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qmessagebox.h>

void ezMainWindow::ProcessTelemetry_General(void* pPassThrough)
{
  ezMainWindow* pWindow = (ezMainWindow*) pPassThrough;

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
        sOut.Format("Application Assert:\n\n%s\n\nFile: '%s'\nLine: %i\nFunction: '%s'\n\n'%s'", 
          sExpression.GetData(),
          sSourceFile.GetData(),
          uiLine,
          sFunction.GetData(),
          sMessage.GetData());

        QMessageBox::critical(pWindow, "Assertion", sOut.GetData());
      }
      break;
    case 'DATA':
      {
        ezString sPlatform;
        ezUInt32 uiCoreCount = 0;
        ezUInt32 uiMemorySize = 0;
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

