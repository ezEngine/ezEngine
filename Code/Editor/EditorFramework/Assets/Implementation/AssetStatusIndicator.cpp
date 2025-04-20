#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>

ezQtAssetStatusIndicator::ezQtAssetStatusIndicator(ezAssetDocument* pDoc, QWidget* pParent)
  : QFrame(pParent)
{
  m_pAsset = pDoc;

  setContentsMargins(0, 0, 0, 0);
  setLayout(new QHBoxLayout());

  layout()->setContentsMargins(0, 0, 0, 0);

  m_pLabel = new QPushButton();
  m_pLabel->setFlat(true);
  connect(m_pLabel, &QPushButton::clicked, this, &ezQtAssetStatusIndicator::onClick);

  layout()->addWidget(m_pLabel);

  m_pHelp = new QPushButton();
  connect(m_pHelp, &QPushButton::clicked, this, &ezQtAssetStatusIndicator::onHelp);
  m_pHelp->setFlat(true);
  m_pHelp->setIcon(QIcon(":/GuiFoundation/Icons/Help.svg"));
  m_pHelp->setMaximumWidth(32);
  m_pHelp->setToolTip(ezMakeQString(ezTranslateTooltip("Asset.Help")));
  layout()->addWidget(m_pHelp);

  m_pAsset->m_EventsOne.AddEventHandler(ezMakeDelegate(&ezQtAssetStatusIndicator::DocumentEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetStatusIndicator::AssetEventHandler, this));

  UpdateDisplay();
}

ezQtAssetStatusIndicator::~ezQtAssetStatusIndicator()
{
  m_pAsset->m_EventsOne.RemoveEventHandler(ezMakeDelegate(&ezQtAssetStatusIndicator::DocumentEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetStatusIndicator::AssetEventHandler, this));
}

void ezQtAssetStatusIndicator::DocumentEventHandler(const ezDocumentEvent& e)
{
  if (e.m_Type == ezDocumentEvent::Type::ModifiedChanged)
  {
    UpdateDisplay();
  }
}

void ezQtAssetStatusIndicator::AssetEventHandler(const ezAssetCuratorEvent& e)
{
  if (e.m_AssetGuid == m_pAsset->GetGuid())
  {
    if (e.m_Type == ezAssetCuratorEvent::Type::AssetUpdated)
    {
      UpdateDisplay();
    }
  }
}

void ezQtAssetStatusIndicator::UpdateDisplay()
{
  // states:
  // all good
  // document modified - but live preview
  // document modified - no preview
  // saved - waiting for background transform
  // saved - needs manual transform (bg off)
  // transform error

  if (m_pAsset->IsModified())
  {
    auto flags = m_pAsset->GetAssetFlags();
    const bool bTransformOnSave = flags.IsSet(ezAssetDocumentFlags::AutoTransformOnSave);
    const bool bBgRunning = ezAssetProcessor::GetSingleton()->GetProcessTaskState() == ezAssetProcessor::ProcessTaskState::Running;

    // no flag for live preview available (ignore)

    if (bTransformOnSave || bBgRunning)
    {
      m_pLabel->setText("Asset Modified: Click to Save");
      m_Action = Action::Save;
    }
    else
    {
      m_pLabel->setText("Asset Modified: Click to Transform");
      m_Action = Action::Transform;
    }

    m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
  }
  else
  {
    auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());
    switch (assetInfo->m_pAssetInfo->m_TransformState)
    {
      case ezAssetInfo::TransformState::UpToDate:
      case ezAssetInfo::TransformState::NeedsThumbnail:
      {
        m_pLabel->setText("Asset State: All Good");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetOk.svg"));
        m_Action = Action::None;
        break;
      }

      case ezAssetInfo::TransformState::NeedsImport:
      case ezAssetInfo::TransformState::NeedsTransform:
      {
        const bool bBgRunning = ezAssetProcessor::GetSingleton()->GetProcessTaskState() == ezAssetProcessor::ProcessTaskState::Running;

        if (bBgRunning)
        {
          m_pLabel->setText("Waiting for Transform: Click to Force");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetNeedsTransform.svg"));
        }
        else
        {
          m_pLabel->setText("Asset Changed: Click to Transform");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
        }

        m_Action = Action::Transform;
        break;
      }

      case ezAssetInfo::TransformState::TransformError:
      case ezAssetInfo::TransformState::MissingTransformDependency:
      case ezAssetInfo::TransformState::MissingThumbnailDependency:
      case ezAssetInfo::TransformState::MissingPackageDependency:
      case ezAssetInfo::TransformState::CircularDependency:
        m_pLabel->setText("Asset Error: Click for Details");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetFailedTransform.svg"));
        m_Action = Action::ShowErrors;
        break;

      default:
        break;
    }
  }
}

void ezQtAssetStatusIndicator::onClick(bool)
{
  switch (m_Action)
  {
    case Action::Save:
      m_pAsset->SaveDocument().IgnoreResult();
      break;

    case Action::None:
      // also transform in this case
      [[fallthrough]];

    case Action::Transform:
      m_pAsset->TransformAsset(ezTransformFlags::TriggeredManually | ezTransformFlags::ForceTransform);
      break;

    case Action::ShowErrors:
    {
      auto assetInfo = ezAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());

      ezStringBuilder output;
      output.Set("Asset transform failed.\n\n");

      if (!assetInfo->m_pAssetInfo->m_LogEntries.IsEmpty())
      {
        output.Append("Errors:\n\n");

        for (const ezLogEntry& logEntry : assetInfo->m_pAssetInfo->m_LogEntries)
        {
          output.AppendFormat("{}\n", logEntry.m_sMsg);
        }
      }

      auto getNiceName = [](const ezString& sDep) -> ezStringBuilder
      {
        if (ezConversionUtils::IsStringUuid(sDep))
        {
          ezUuid guid = ezConversionUtils::ConvertStringToUuid(sDep);
          auto assetInfoDep = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
          if (assetInfoDep)
          {
            return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          }

          ezUInt64 uiLow;
          ezUInt64 uiHigh;
          guid.GetValues(uiLow, uiHigh);
          ezStringBuilder sTmp;
          sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

          return sTmp;
        }

        return sDep;
      };

      ezSet<ezString> missingDeps;

      if (!assetInfo->m_pAssetInfo->m_MissingTransformDeps.IsEmpty())
      {
        for (const ezString& dep : assetInfo->m_pAssetInfo->m_MissingTransformDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingPackageDeps.IsEmpty())
      {
        for (const ezString& dep : assetInfo->m_pAssetInfo->m_MissingPackageDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingThumbnailDeps.IsEmpty())
      {
        for (const ezString& dep : assetInfo->m_pAssetInfo->m_MissingThumbnailDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!missingDeps.IsEmpty())
      {
        output.Append("Missing Dependencies:\n\n");

        for (const ezString& dep : missingDeps)
        {
          output.AppendFormat("{}\n", dep);
        }
      }

      ezQtUiServices::GetSingleton()->MessageBoxInformation(output);

      break;
    }
  }
}

void ezQtAssetStatusIndicator::onHelp(bool)
{
  ezStringView sType = m_pAsset->GetDocumentTypeName();
  ezString sURL = ezTranslateHelpURL(sType);

  if (!sURL.IsEmpty())
  {
    QDesktopServices::openUrl(QUrl(ezMakeQString(sURL)));
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("There is no known online documentation for the asset type '{}'.\n\nPlease report this to the developers.", sType));
  }
}
