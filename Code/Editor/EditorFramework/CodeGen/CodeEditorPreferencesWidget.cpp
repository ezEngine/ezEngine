#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CodeEditorPreferencesWidget.moc.h>

ezQtCodeEditorPreferencesWidget::ezQtCodeEditorPreferencesWidget()
  : ezQtPropertyTypeWidget(true)
{
  QLabel* versionText = new QLabel(ezMakeQString("Code Editor to be used when clicking on stack traces. {file} and {line} will be replaced depending on the context."));
  versionText->setWordWrap(true);
  m_pGroupLayout->addWidget(versionText);
}

ezQtCodeEditorPreferencesWidget::~ezQtCodeEditorPreferencesWidget() = default;