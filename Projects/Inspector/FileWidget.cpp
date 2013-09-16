#include <Inspector/FileWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <Foundation/IO/OSFile.h>
#include <qgraphicsitem.h>

ezFileWidget* ezFileWidget::s_pWidget = NULL;

ezFileWidget::ezFileWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  //QTransform t = graphicsView->transform ();
  //t.scale (1, 1);
  //graphicsView->setTransform (t);
  
  graphicsView->setScene(&m_Scene);

  graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

  ResetStats();

  //m_Scene.addRect(0, 0, 50, 10, QPen(), QBrush(QColor::fromRgb(255, 0, 0)));

  graphicsView->setSceneRect(QRectF (0, 0, 100, 10));
  graphicsView->fitInView   (QRectF (0, 0, 100, 10));

}

void ezFileWidget::ResetStats()
{
  m_Scene.clear();
}

static QColor s_Colors[9] =
{
  QColor(255, 106,   0), // orange
  QColor(182, 255,   0), // lime green
  QColor(255,   0, 255), // pink
  QColor(  0, 148, 255), // light blue
  QColor(255,   0,   0), // red
  QColor(  0, 255, 255), // turqoise
  QColor(178,   0, 255), // purple
  QColor(  0,  38, 255), // dark blue
  QColor( 72,   0, 255), // lilac
};


void ezFileWidget::ProcessTelemetry(void* pUnuseed)
{
  //if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  static double fPos = 0;
  static int iColor = 0;

  while (ezTelemetry::RetrieveMessage('FILE', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'OPEN':
      {
        ezString sFile;
        ezUInt8 uiMode = 0;
        bool bSuccess = false;
        double dTime = 0.0f;

        Msg.GetReader() >> sFile;
        Msg.GetReader() >> uiMode;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        dTime *= 10000;
        dTime += 1.0;

        int iNextColor = (iColor + 1) % 9;
        QGraphicsRectItem* pRect = s_pWidget->m_Scene.addRect(fPos, 0, dTime, 10, QPen(s_Colors[iColor]), QBrush(s_Colors[iNextColor]));
        iColor = iNextColor;

        fPos += dTime;

        pRect->setPen(QPen(Qt::NoPen));

        s_pWidget->graphicsView->setSceneRect(QRectF (0, 0, ezMath::Max(100.0, fPos), 10));
        s_pWidget->graphicsView->fitInView   (QRectF (0, 0, ezMath::Max(100.0, fPos), 10));

        const char* szRead = "Read";
        const char* szWrite = "Write";
        const char* szAppend = "Append";
        const char* szSuccess = "Success";
        const char* szFailure = "Failure";
        const char* szMode = NULL;

        switch (uiMode)
        {
        case ezFileMode::Append:
          szMode = szAppend;
          break;
        case ezFileMode::Read:
          szMode = szRead;
          break;
        case ezFileMode::Write:
          szMode = szWrite;
          break;
        }

        ezStringBuilder sOut;
        sOut.Format("OpenFile (%s): '%s' -> %s (%.3f sec)", szMode, sFile.GetData(), bSuccess ? szSuccess : szFailure, dTime);
        //ezMainWindow::s_pWidget->Log(sOut.GetData());
      }
      break;
    case 'CLOS':
      {
        ezString sFile;
        ezUInt8 uiMode = 0;
        double dTime;

        Msg.GetReader() >> sFile;
        Msg.GetReader() >> uiMode;
        Msg.GetReader() >> dTime;

        const char* szRead = "Read";
        const char* szWrite = "Write";
        const char* szAppend = "Append";
        const char* szMode = NULL;

        switch (uiMode)
        {
        case ezFileMode::Append:
          szMode = szAppend;
          break;
        case ezFileMode::Read:
          szMode = szRead;
          break;
        case ezFileMode::Write:
          szMode = szWrite;
          break;
        }

        //ezStringBuilder sOut;
        //sOut.Format("CloseFile (%s): '%s' (%.3f sec)", szMode, sFile.GetData(), dTime);
        //ezMainWindow::s_pWidget->Log(sOut.GetData());
      }
      break;
    case 'WRIT':
      {
        ezString sFile;
        ezUInt64 uiSize;
        bool bSuccess = false;
        double dTime = 0;

        Msg.GetReader() >> sFile;
        Msg.GetReader() >> uiSize;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        const char* szSuccess = "Success";
        const char* szFailure = "Failure";

        //ezStringBuilder sOut;
        //sOut.Format("WriteFile (%llu Bytes): '%s' -> %s (%.3f sec)", uiSize, sFile.GetData(), bSuccess ? szSuccess : szFailure, dTime);
        //ezMainWindow::s_pWidget->Log(sOut.GetData());
      }
      break;
    case 'READ':
      {
        ezString sFile;
        ezUInt64 uiSize;
        ezUInt64 uiRead;
        double dTime = 0;

        Msg.GetReader() >> sFile;
        Msg.GetReader() >> uiSize;
        Msg.GetReader() >> uiRead;
        Msg.GetReader() >> dTime;

        //ezStringBuilder sOut;
        //sOut.Format("ReadFile (%llu Bytes): '%s' -> %llu Bytes Read (%.3f sec)", uiSize, sFile.GetData(), uiRead, dTime);
        //ezMainWindow::s_pWidget->Log(sOut.GetData());
      }
      break;
    }
  }

}
