#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <EditorPluginAngelScript/AngelScriptWindow/AngelScriptWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

static bool g_bRetrievedScriptInfos = false;
static QSet<QString> g_KeywordsBlue;
static QSet<QString> g_KeywordsPink;
static QSet<QString> g_KeywordsGreen;
static QSet<QString> g_BuiltIn;

//////////////////////////////////////////////////////////////////////////

ezQtAngelScriptAssetDocumentWindow::ezQtAngelScriptAssetDocumentWindow(ezAngelScriptAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  m_pAssetDoc = pDocument;

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "AngelScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "AngelScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AngelScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    m_pSourceLabel = new QTextEdit(this);
    m_pSourceLabel->setFont(font);
    m_pSourceLabel->setUndoRedoEnabled(false);
    m_pSourceLabel->setReadOnly(true);
    m_pSourceLabel->setTabStopDistance(m_pSourceLabel->tabStopDistance() / 4.0f);

    QFontMetricsF fm(m_pSourceLabel->font());
    auto stopWidth = 4 * fm.averageCharWidth();
    m_pSourceLabel->setTabStopDistance(ceil(stopWidth));

    connect(m_pSourceLabel, &QTextEdit::textChanged, this, &ezQtAngelScriptAssetDocumentWindow::onTextEditTextChanged);

    m_pHighlighter = new ASHighlighter(m_pSourceLabel->document());

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(this, pDocument);
    pCentral->setObjectName("AngelScriptView");
    pCentral->setWindowTitle("Script");
    pCentral->setWidget(m_pSourceLabel);

    m_pDockManager->setCentralWidget(pCentral);

    UpdateFileContentDisplay();
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AngelScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Angel Script Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator((ezAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_LastEdit = ezTime::MakeZero();
  m_EditTimer.setInterval(100);
  connect(&m_EditTimer, &QTimer::timeout, this, &ezQtAngelScriptAssetDocumentWindow::onEditTimer);

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAngelScriptAssetDocumentWindow::AssetEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler, this));

  FinishWindowCreation();

  RetrieveScriptInfos();
}

ezQtAngelScriptAssetDocumentWindow::~ezQtAngelScriptAssetDocumentWindow()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAngelScriptAssetDocumentWindow::AssetEventHandler, this));
}

void ezQtAngelScriptAssetDocumentWindow::DocumentObjectEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (!m_bIgnoreCodeChange && (e.m_sProperty == "Source" || e.m_sProperty == "Code"))
    {
      UpdateFileContentDisplay();
    }
  }
}

void ezQtAngelScriptAssetDocumentWindow::StoreInlineDocState()
{
  ezStringBuilder sContent = m_pSourceLabel->toPlainText().toUtf8().data();
  // sContent.ReplaceAll("\t", "    ");

  if (m_pAssetDoc->GetProperties()->m_CodeMode == ezAngelScriptCodeMode::Inline && m_pAssetDoc->GetProperties()->m_sCode != sContent)
  {
    m_bIgnoreCodeChange = true;
    EZ_SCOPE_EXIT(m_bIgnoreCodeChange = false);

    ezObjectCommandAccessor accessor(m_pAssetDoc->GetCommandHistory());
    accessor.StartTransaction("Edit Code");

    const ezDocumentObject* pProps = m_pAssetDoc->GetPropertyObject();
    if (pProps)
    {
      accessor.SetValueByName(pProps, "Code", sContent.GetView()).AssertSuccess();
    }

    accessor.FinishTransaction();
  }
}

void ezQtAngelScriptAssetDocumentWindow::onTextEditTextChanged()
{
  if (m_pAssetDoc->GetProperties()->m_CodeMode == ezAngelScriptCodeMode::Inline)
  {
    if (!m_EditTimer.isActive())
    {
      m_EditTimer.start();
    }

    m_LastEdit = ezTime::Now();
  }
}

void ezQtAngelScriptAssetDocumentWindow::onEditTimer()
{
  if (m_LastEdit > m_LastSave)
  {
    if (ezTime::Now() - m_LastSave > ezTime::Seconds(0.5))
    {
      m_LastSave = ezTime::Now();
      StoreInlineDocState();
    }
  }
  else
  {
    m_EditTimer.stop();
  }
}

void ezQtAngelScriptAssetDocumentWindow::AssetEventHandler(const ezAssetCuratorEvent& e)
{
  if (e.m_AssetGuid == m_pAssetDoc->GetGuid())
  {
    if (e.m_Type == ezAssetCuratorEvent::Type::AssetUpdated)
    {
      UpdateFileContentDisplay();
    }
  }
}

void ezQtAngelScriptAssetDocumentWindow::UpdateFileContentDisplay()
{
  if (m_pAssetDoc->GetProperties()->m_CodeMode == ezAngelScriptCodeMode::Inline)
  {
    const auto& sCode = m_pAssetDoc->GetProperties()->m_sCode;

    m_pSourceLabel->setText(sCode.GetData());
    m_pSourceLabel->setReadOnly(false);
  }
  else
  {

    m_pSourceLabel->setReadOnly(true);

    const auto& sFile = m_pAssetDoc->GetProperties()->m_sScriptFile;

    if (sFile.IsEmpty())
    {
      m_pSourceLabel->setText("No script file specified.");
      return;
    }

    ezFileReader file;
    if (file.Open(sFile).Failed())
    {
      m_pSourceLabel->setText(QString("Script file '%1' doesn't exist.").arg(sFile.GetData()));
      return;
    }

    ezStringBuilder content;
    content.ReadAll(file);

    m_pSourceLabel->setText(content.GetData());
  }
}

void ezQtAngelScriptAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "SyncExposedParams_Clear")
    {
      m_ExposedParams.Clear();
    }

    if (pMsg->m_sWhatToDo == "SyncExposedParams_Add")
    {
      auto& p = m_ExposedParams.ExpandAndGetRef();
      p.m_sName = pMsg->m_sPayload;
      p.m_DefaultValue = pMsg->m_PayloadValue;
    }

    if (pMsg->m_sWhatToDo == "SyncExposedParams_Finish")
    {
      ezCommandHistory* history = m_pAssetDoc->GetCommandHistory();

      ezObjectCommandAccessor accessor(history);

      auto pProps = m_pAssetDoc->GetPropertyObject();

      const ezInt32 uiNum = accessor.GetCountByName(pProps, "Parameters");

      bool bAnyChange = (uiNum != m_ExposedParams.GetCount());

      ezStringBuilder sDecl;

      for (ezInt32 ui = 0; ui < uiNum; ++ui)
      {
        const ezDocumentObject* pArgObj = accessor.GetChildObjectByName(pProps, "Parameters", ui);

        ezVariant name, expose, def;
        accessor.GetValueByName(pArgObj, "Name", name).AssertSuccess();
        accessor.GetValueByName(pArgObj, "Expose", expose).AssertSuccess();
        accessor.GetValueByName(pArgObj, "DefaultValue", def).AssertSuccess();

        const ezString sName = name.ConvertTo<ezString>();
        bool bExpose = expose.ConvertTo<bool>();
        bool bFound = false;

        for (auto& ep : m_ExposedParams)
        {
          if (ep.m_sName == sName)
          {
            bFound = true;
            ep.m_bExpose = bExpose;

            if (ep.m_DefaultValue != def)
            {
              bAnyChange = true;
            }

            break;
          }
        }

        if (!bFound)
        {
          bAnyChange = true;
        }
      }

      if (bAnyChange)
      {
        accessor.StartTransaction("Sync Parameters");

        // clear the entire array
        accessor.ClearByName(pProps, "Parameters").AssertSuccess();

        // and fill it again
        for (ezUInt32 clip = 0; clip < m_ExposedParams.GetCount(); ++clip)
        {
          ezUuid newItemGuid = ezUuid::MakeUuid();
          accessor.AddObjectByName(pProps, "Parameters", -1, ezGetStaticRTTI<ezAngelScriptParameter>(), newItemGuid).AssertSuccess();

          const ezDocumentObject* pNewItem = accessor.GetObject(newItemGuid);

          sDecl.SetFormat("{} {} = {}", ezAngelScriptUtils::VariantTypeToString(m_ExposedParams[clip].m_DefaultValue.GetType()), m_ExposedParams[clip].m_sName, m_ExposedParams[clip].m_DefaultValue);

          accessor.SetValueByName(pNewItem, "Name", m_ExposedParams[clip].m_sName).AssertSuccess();
          accessor.SetValueByName(pNewItem, "Declaration", sDecl.GetData()).AssertSuccess();
          accessor.SetValueByName(pNewItem, "Expose", m_ExposedParams[clip].m_bExpose).AssertSuccess();
          accessor.SetValueByName(pNewItem, "DefaultValue", m_ExposedParams[clip].m_DefaultValue).AssertSuccess();
        }

        accessor.FinishTransaction();
      }
    }
  }
}

static void ReadSet(ezStringView sBasePath, ezStringView sFile, QSet<QString>& inout_Set)
{
  const ezStringBuilder fullPath(sBasePath, "/", sFile);

  ezFileReader file;
  if (file.Open(fullPath).Failed())
    return;

  ezStringBuilder tmp;
  tmp.ReadAll(file);

  ezDynamicArray<ezStringView> lines;
  tmp.Split(false, lines, "\n", "\r");

  for (ezStringView line : lines)
  {
    line.Trim();

    if (!line.IsEmpty())
    {
      inout_Set.insert(ezMakeQString(line));
    }
  }
}

void ezQtAngelScriptAssetDocumentWindow::RetrieveScriptInfos()
{
  if (g_bRetrievedScriptInfos)
    return;

  g_bRetrievedScriptInfos = true;

  ezStringBuilder sBasePath = ezToolsProject::GetSingleton()->GetProjectDataFolder();
  sBasePath.AppendPath("AngelScript");

  ezDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "RetrieveScriptInfos";
  msg.m_sValue = sBasePath;
  GetDocument()->SendMessageToEngine(&msg);

  ReadSet(sBasePath, "Types.txt", g_KeywordsGreen);
  ReadSet(sBasePath, "Namespaces.txt", g_KeywordsGreen);
  ReadSet(sBasePath, "GlobalFunctions.txt", g_BuiltIn);
  ReadSet(sBasePath, "Methods.txt", g_BuiltIn);
  ReadSet(sBasePath, "Properties.txt", g_BuiltIn);
}

ASHighlighter::ASHighlighter(QTextDocument* pParent)
  : QSyntaxHighlighter(pParent)
{
  // default color scheme
  m_Colors[ASEdit::Comment] = QColor("#6A8A35");
  m_Colors[ASEdit::Number] = QColor("#B5CEA8");
  m_Colors[ASEdit::String] = QColor("#CE916A");
  m_Colors[ASEdit::Operator] = QColor("#808000");
  m_Colors[ASEdit::KeywordBlue] = QColor("#569CCA");
  m_Colors[ASEdit::KeywordPink] = QColor("#C586C0");
  m_Colors[ASEdit::KeywordGreen] = QColor("#4EC9B0");
  m_Colors[ASEdit::BuiltIn] = QColor("#DCDCAA");

  g_KeywordsPink << "break";
  g_KeywordsPink << "case";
  g_KeywordsPink << "continue";
  g_KeywordsPink << "default";
  g_KeywordsPink << "do";
  g_KeywordsPink << "for";
  g_KeywordsPink << "return";
  g_KeywordsPink << "switch";
  g_KeywordsPink << "while";
  g_KeywordsPink << "if";
  g_KeywordsPink << "else";
  g_KeywordsPink << "in";
  g_KeywordsPink << "out";
  g_KeywordsPink << "inout";
  g_KeywordsPink << "is";
  g_KeywordsPink << "ezAngelScriptClass";

  g_KeywordsBlue << "function";
  g_KeywordsBlue << "funcdef";
  g_KeywordsBlue << "import";
  g_KeywordsBlue << "cast";
  g_KeywordsBlue << "this";
  g_KeywordsBlue << "void";
  g_KeywordsBlue << "true";
  g_KeywordsBlue << "false";
  g_KeywordsBlue << "null";
  g_KeywordsBlue << "class";
  g_KeywordsBlue << "struct";
  g_KeywordsBlue << "const";
  g_KeywordsBlue << "enum";
  g_KeywordsBlue << "private";
  g_KeywordsBlue << "protected";
  g_KeywordsBlue << "auto";
  g_KeywordsBlue << "explicit";
  g_KeywordsBlue << "external";
  g_KeywordsBlue << "final";
  g_KeywordsBlue << "namespace";
  g_KeywordsBlue << "interface";
  g_KeywordsBlue << "mixin";
  g_KeywordsBlue << "abstract";
  g_KeywordsBlue << "not";
  g_KeywordsBlue << "and";
  g_KeywordsBlue << "or";
  g_KeywordsBlue << "xor";
  g_KeywordsBlue << "override";
  g_KeywordsBlue << "property";
  g_KeywordsBlue << "shared";
  g_KeywordsBlue << "super";
  g_KeywordsBlue << "try";
  g_KeywordsBlue << "catch";
  g_KeywordsBlue << "typedef";

  g_KeywordsGreen << "bool";
  g_KeywordsGreen << "float";
  g_KeywordsGreen << "double";
  g_KeywordsGreen << "int";
  g_KeywordsGreen << "int8";
  g_KeywordsGreen << "int16";
  g_KeywordsGreen << "int32";
  g_KeywordsGreen << "int64";
  g_KeywordsGreen << "uint8";
  g_KeywordsGreen << "uint16";
  g_KeywordsGreen << "uint32";
  g_KeywordsGreen << "uint64";
}


void ASHighlighter::highlightBlock(const QString& text)
{
  // parsing state
  enum
  {
    Start = 0,
    Number = 1,
    Identifier = 2,
    String = 3,
    Comment = 4,
    Regex = 5
  };

  QList<int> bracketPositions;

  int blockState = previousBlockState();
  int bracketLevel = blockState >> 4;
  int state = blockState & 15;
  if (blockState < 0)
  {
    bracketLevel = 0;
    state = Start;
  }

  int start = 0;
  int i = 0;
  while (i <= text.length())
  {
    QChar ch = (i < text.length()) ? text.at(i) : QChar();
    QChar next = (i < text.length() - 1) ? text.at(i + 1) : QChar();

    switch (state)
    {

      case Start:
        start = i;
        if (ch.isSpace())
        {
          ++i;
        }
        else if (ch.isDigit())
        {
          ++i;
          state = Number;
        }
        else if (ch.isLetter() || ch == '_')
        {
          ++i;
          state = Identifier;
        }
        else if (ch == '\'' || ch == '\"')
        {
          ++i;
          state = String;
        }
        else if (ch == '/' && next == '*')
        {
          ++i;
          ++i;
          state = Comment;
        }
        else if (ch == '/' && next == '/')
        {
          i = text.length();
          setFormat(start, text.length(), m_Colors[ASEdit::Comment]);
        }
        else if (ch == '/' && next != '*')
        {
          ++i;
          state = Regex;
        }
        else
        {
          if (!QString("(){}[]").contains(ch))
            setFormat(start, 1, m_Colors[ASEdit::Operator]);
          if (ch == '{' || ch == '}')
          {
            bracketPositions += i;
            if (ch == '{')
              bracketLevel++;
            else
              bracketLevel--;
          }
          ++i;
          state = Start;
        }
        break;

      case Number:
        if (ch.isSpace() || !ch.isDigit())
        {
          setFormat(start, i - start, m_Colors[ASEdit::Number]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Identifier:
        if (ch.isSpace() || !(ch.isDigit() || ch.isLetter() || ch == '_'))
        {
          QString token = text.mid(start, i - start).trimmed();
          if (g_KeywordsBlue.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordBlue]);
          if (g_KeywordsPink.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordPink]);
          if (g_KeywordsGreen.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::KeywordGreen]);
          else if (g_BuiltIn.contains(token))
            setFormat(start, i - start, m_Colors[ASEdit::BuiltIn]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case String:
        if (ch == text.at(start))
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_Colors[ASEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      case Comment:
        if (ch == '*' && next == '/')
        {
          ++i;
          ++i;
          setFormat(start, i - start, m_Colors[ASEdit::Comment]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Regex:
        if (ch == '/')
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_Colors[ASEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      default:
        state = Start;
        break;
    }
  }

  if (state == Comment)
    setFormat(start, text.length(), m_Colors[ASEdit::Comment]);
  else
    state = Start;

  if (!bracketPositions.isEmpty())
  {
    ASBlockData* blockData = reinterpret_cast<ASBlockData*>(currentBlock().userData());
    if (!blockData)
    {
      blockData = new ASBlockData;
      currentBlock().setUserData(blockData);
    }
    blockData->bracketPositions = bracketPositions;
  }

  blockState = (state & 15) | (bracketLevel << 4);
  setCurrentBlockState(blockState);
}
