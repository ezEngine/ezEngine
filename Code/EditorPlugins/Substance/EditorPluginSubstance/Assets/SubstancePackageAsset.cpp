#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>

#include <qsettings.h>
#include <qxmlstream.h>

namespace
{
  ezResult GetSbsContent(ezStringView sAbsolutePath, ezStringBuilder& out_sContent)
  {
    ezFileReader fileReader;
    EZ_SUCCEED_OR_RETURN(fileReader.Open(sAbsolutePath));

    out_sContent.ReadAll(fileReader);
    return EZ_SUCCESS;
  }

  ezResult ReadUntilStartElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      EZ_IGNORE_UNUSED(tokenType);

      if (inout_reader.isStartElement() && inout_reader.name() == QLatin1StringView(szName))
        return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  ezResult ReadUntilEndElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      EZ_IGNORE_UNUSED(tokenType);

      if (inout_reader.isEndElement() && inout_reader.name() == QLatin1StringView(szName))
        return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  template <typename T>
  T GetValueAttribute(QXmlStreamReader& inout_reader)
  {
    ezString s(inout_reader.attributes().value("v").toUtf8().data());
    if constexpr (std::is_same_v<T, ezString>)
    {
      return s;
    }
    else
    {
      ezVariant v = s;
      return v.ConvertTo<T>();
    }
  }

  static const char* s_szSubstanceUsageMapping[] = {
    "",                 // Unknown,

    "baseColor",        // BaseColor,
    "emissive",         // Emissive,
    "height",           // Height,
    "metallic",         // Metallic,
    "mask",             // Mask,
    "normal",           // Normal,
    "ambientOcclusion", // Occlusion,
    "opacity",          // Opacity,
    "roughness",        // Roughness,
  };

  static_assert(EZ_ARRAY_SIZE(s_szSubstanceUsageMapping) == ezSubstanceUsage::Count);

  static ezUInt8 s_substanceNumChannelsMapping[] = {
    1, // Unknown,

    3, // BaseColor,
    3, // Emissive,
    1, // Height,
    1, // Metallic,
    1, // Mask,
    3, // Normal,
    1, // Occlusion,
    1, // Opacity,
    1, // Roughness,
  };

  static_assert(EZ_ARRAY_SIZE(s_szSubstanceUsageMapping) == ezSubstanceUsage::Count);


  ezSubstanceUsage::Enum GetUsage(QXmlStreamReader& inout_reader)
  {
    ezString s = GetValueAttribute<ezString>(inout_reader);
    for (ezUInt32 i = 0; i < ezSubstanceUsage::Count; ++i)
    {
      if (s.IsEqual_NoCase(s_szSubstanceUsageMapping[i]))
      {
        return static_cast<ezSubstanceUsage::Enum>(i);
      }
    }

    return ezSubstanceUsage::Unknown;
  }

  ezResult ParseGraphOutput(QXmlStreamReader& inout_reader, ezUInt32 uiGraphUid, ezSubstanceGraphOutput& out_graphOutput)
  {
    EZ_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graphoutput"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graphOutput.m_sName = GetValueAttribute<ezString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        ezUInt32 outputUid = GetValueAttribute<ezUInt32>(inout_reader);
        ezUInt64 seed = ezUInt64(uiGraphUid) << 32ull | outputUid;
        out_graphOutput.m_Uuid = ezUuid::MakeStableUuidFromInt(seed);
      }
      else if (inout_reader.name() == QLatin1StringView("attributes"))
      {
        if (ReadUntilStartElement(inout_reader, "label").Succeeded())
        {
          out_graphOutput.m_sLabel = GetValueAttribute<ezString>(inout_reader);
          EZ_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "label"));
        }
      }
      else if (inout_reader.name() == QLatin1StringView("usages"))
      {
        if (ReadUntilStartElement(inout_reader, "name").Succeeded())
        {
          out_graphOutput.m_Usage = GetUsage(inout_reader);
          out_graphOutput.m_uiNumChannels = s_substanceNumChannelsMapping[out_graphOutput.m_Usage];
          EZ_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "usage"));
        }
      }

      inout_reader.skipCurrentElement();
    }

    return EZ_SUCCESS;
  }

  struct Option
  {
    ezString m_sName;
    ezString m_sValue;
  };

  ezResult ParseOption(QXmlStreamReader& inout_reader, Option& out_option)
  {
    EZ_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("option"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("name"))
      {
        out_option.m_sName = GetValueAttribute<ezString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("value"))
      {
        out_option.m_sValue = GetValueAttribute<ezString>(inout_reader);
      }

      inout_reader.skipCurrentElement();
    }

    return EZ_SUCCESS;
  }

  ezResult ParseGraph(QXmlStreamReader& inout_reader, ezSubstanceGraph& out_graph)
  {
    EZ_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graph"), "");

    ezUInt32 uiGraphUid = 0;
    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graph.m_sName = GetValueAttribute<ezString>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        uiGraphUid = GetValueAttribute<ezUInt32>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("graphOutputs"))
      {
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("graphoutput"))
          {
            auto& graphOutput = out_graph.m_Outputs.ExpandAndGetRef();
            EZ_SUCCEED_OR_RETURN(ParseGraphOutput(inout_reader, uiGraphUid, graphOutput));
          }
        }
      }
      else if (inout_reader.name() == QLatin1StringView("options"))
      {
        Option option;
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("option"))
          {
            EZ_SUCCEED_OR_RETURN(ParseOption(inout_reader, option));

            if (option.m_sName == "defaultParentSize")
            {
              ezStringView sValue = option.m_sValue;
              ezUInt32 tmp = 0;
              const char* szLastPos = nullptr;
              EZ_SUCCEED_OR_RETURN(ezConversionUtils::StringToUInt(sValue, tmp, &szLastPos));
              out_graph.m_uiOutputWidth = static_cast<ezUInt8>(tmp);

              if (*szLastPos != 'x')
                return EZ_FAILURE;

              sValue = ezStringView(szLastPos + 1);
              EZ_SUCCEED_OR_RETURN(ezConversionUtils::StringToUInt(sValue, tmp));
              out_graph.m_uiOutputHeight = static_cast<ezUInt8>(tmp);
            }
            else if (option.m_sName.StartsWith("export/fromGraph/outputs/"))
            {
              const char* szLastSlash = option.m_sName.FindLastSubString("/");
              ezStringView sOutputIdentifier = ezStringView(szLastSlash + 1);

              bool bEnabled = false;
              EZ_SUCCEED_OR_RETURN(ezConversionUtils::StringToBool(option.m_sValue, bEnabled));

              for (auto& output : out_graph.m_Outputs)
              {
                if (output.m_sName == sOutputIdentifier)
                {
                  output.m_bEnabled = bEnabled;
                }
              }
            }
          }
        }
      }
      else
      {
        inout_reader.skipCurrentElement();
      }
    }

    return EZ_SUCCESS;
  }

  ezResult ReadDependencies(ezStringView sSbsFile, ezSet<ezString>& out_dependencies)
  {
    ezStringBuilder sAbsolutePath = sSbsFile;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePath))
    {
      return EZ_FAILURE;
    }

    ezStringBuilder sFileContent;
    EZ_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

    QXmlStreamReader reader(sFileContent.GetData());
    EZ_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "dependencies"));

    while (reader.readNextStartElement())
    {
      if (reader.name() != QLatin1StringView("dependency"))
      {
        reader.skipCurrentElement();
        continue;
      }

      if (ReadUntilStartElement(reader, "filename").Failed())
        continue;

      ezString sDependency = GetValueAttribute<ezString>(reader);
      if (sDependency.EndsWith(".sbs") && sDependency.StartsWith("sbs://") == false)
      {
        ezStringBuilder sFullPath;
        if (sDependency.IsAbsolutePath())
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
        else
        {
          sFullPath = sSbsFile.GetFileDirectory();
          sFullPath.AppendPath(sDependency);
          sFullPath.MakeCleanPath();
        }

        if (out_dependencies.Contains(sFullPath) == false)
        {
          out_dependencies.Insert(sFullPath);

          EZ_SUCCEED_OR_RETURN(ReadDependencies(sFullPath, out_dependencies));
        }
      }

      EZ_SUCCEED_OR_RETURN(ReadUntilEndElement(reader, "dependency"));
    }

    return EZ_SUCCESS;
  }

  ezResult GetInstallationPath(ezStringBuilder& out_sPath)
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
    static ezUntrackedString s_CachedPath;
    if (s_CachedPath.IsEmpty() == false)
    {
      out_sPath = s_CachedPath;
      return EZ_SUCCESS;
    }

    auto CheckPath = [&](ezStringView sPath)
    {
      ezStringBuilder path = sPath;
      path.AppendPath("sbscooker.exe");

      if (ezOSFile::ExistsFile(path))
      {
        s_CachedPath = sPath;
        out_sPath = sPath;
        return true;
      }

      return false;
    };

    ezStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
    if (CheckPath(sPath))
    {
      return EZ_SUCCESS;
    }

    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{e9e3d6d9-3023-41c7-b223-11d8fdd691b9}_is1", QSettings::NativeFormat);
    sPath = ezStringView(settings.value("InstallLocation").toString().toUtf8());

    if (CheckPath(sPath))
    {
      return EZ_SUCCESS;
    }

    ezLog::Error("Installation of Substance Designer could not be located.");
    return EZ_FAILURE;
#endif

    return EZ_FAILURE;
  }

  ezStatus RunSbsCooker(const char* szSbsFile, const char* szOutputPath)
  {
    ezStringBuilder sToolPath;
    EZ_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbscooker");

    QStringList arguments;

    arguments << "--inputs";
    arguments << szSbsFile;

    arguments << "--output-path";
    arguments << szOutputPath;

    arguments << "--no-optimization";

    EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 180, ezLog::GetThreadLocalLogSystem(), ezLogMsgType::InfoMsg));

    return ezStatus(EZ_SUCCESS);
  }

  ezStatus RunSbsRender(const char* szSbsarFile, const char* szGraph, const char* szGraphOutput, const char* szOutputName, const char* szOutputPath, ezUInt8 uiOutputWidth, ezUInt8 uiOutputHeight)
  {
    ezStringBuilder sToolPath;
    EZ_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbsrender");

    ezStringBuilder sTmp;

    QStringList arguments;
    arguments << "render";

    arguments << "--engine";
    arguments << "d3d11pc";

    arguments << "--input";
    arguments << szSbsarFile;

    arguments << "--input-graph";
    arguments << szGraph;

    if (ezStringUtils::IsNullOrEmpty(szGraphOutput) == false)
    {
      arguments << "--input-graph-output";
      arguments << szGraphOutput;
    }

    if (ezStringUtils::IsNullOrEmpty(szOutputName) == false)
    {
      arguments << "--output-name";
      arguments << szOutputName;
    }

    arguments << "--output-path";
    arguments << szOutputPath;

    sTmp.SetFormat("$outputsize@{},{}", uiOutputWidth, uiOutputHeight);
    arguments << "--set-value";
    arguments << sTmp.GetData();

    EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 180, ezLog::GetThreadLocalLogSystem()));

    return ezStatus(EZ_SUCCESS);
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSubstanceUsage, 1)
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Unknown),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::BaseColor),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Emissive),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Height),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Metallic),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Mask),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Normal),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Occlusion),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Opacity),
  EZ_ENUM_CONSTANT(ezSubstanceUsage::Roughness),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSubstanceGraphOutput, ezNoBase, 1, ezRTTIDefaultAllocator<ezSubstanceGraphOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Label", m_sLabel),
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezSubstanceUsage, m_Usage),
    EZ_MEMBER_PROPERTY("NumChannels", m_uiNumChannels)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 4)),
    EZ_ENUM_MEMBER_PROPERTY("CompressionMode", ezTexConvCompressionMode, m_CompressionMode)->AddAttributes(new ezDefaultValueAttribute(ezTexConvCompressionMode::High)),
    EZ_ENUM_MEMBER_PROPERTY("MipmapMode", ezTexConvMipmapMode, m_MipmapMode),
    EZ_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    EZ_MEMBER_PROPERTY("Uuid", m_Uuid)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSubstanceGraph, ezNoBase, 1, ezRTTIDefaultAllocator<ezSubstanceGraph>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("OutputWidth", m_uiOutputWidth)->AddAttributes(new ezClampValueAttribute(4, 12)),
    EZ_MEMBER_PROPERTY("OutputHeight", m_uiOutputHeight)->AddAttributes(new ezClampValueAttribute(4, 12)),
    EZ_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstancePackageAssetProperties, 1, ezRTTIDefaultAllocator<ezSubstancePackageAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SubstanceFile", m_sSubstancePackage)->AddAttributes(new ezFileBrowserAttribute("Select Substance File", "*.sbs")),
    EZ_MEMBER_PROPERTY("OutputPattern", m_sOutputPattern)->AddAttributes(new ezDefaultValueAttribute(ezStringView("$(graph)_$(label)"))),
    EZ_ARRAY_MEMBER_PROPERTY("Graphs", m_Graphs)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstancePackageAssetMetaData, 1, ezRTTIDefaultAllocator<ezSubstancePackageAssetMetaData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("OutputUuids", m_OutputUuids),
    EZ_ARRAY_MEMBER_PROPERTY("OutputNames", m_OutputNames)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstancePackageAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSubstancePackageAssetDocument::ezSubstancePackageAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument(sDocumentPath, ezAssetDocEngineConnection::None)
{
  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSubstancePackageAssetDocument::OnPropertyChanged, this));
}

ezSubstancePackageAssetDocument::~ezSubstancePackageAssetDocument()
{
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSubstancePackageAssetDocument::OnPropertyChanged, this));
}

void ezSubstancePackageAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezSubstancePackageAssetProperties* pProp = GetProperties();

  // Dependencies
  {
    pInfo->m_TransformDependencies.Insert(pProp->m_sSubstancePackage);

    ReadDependencies(pProp->m_sSubstancePackage, pInfo->m_TransformDependencies).IgnoreResult();
  }

  // Outputs
  {
    ezStringBuilder sName;

    auto pMetaData = ezGetStaticRTTI<ezSubstancePackageAssetMetaData>()->GetAllocator()->Allocate<ezSubstancePackageAssetMetaData>();
    for (auto& graph : pProp->m_Graphs)
    {
      if (graph.m_bEnabled == false)
        continue;

      for (auto& output : graph.m_Outputs)
      {
        if (output.m_bEnabled == false)
          continue;

        GenerateOutputName(graph, output, sName);
        if (pMetaData->m_OutputNames.Contains(sName))
        {
          ezLog::Error("A substance texture named '{}' already exists.", sName);
          continue;
        }

        pMetaData->m_OutputUuids.PushBack(output.m_Uuid);
        pMetaData->m_OutputNames.PushBack(sName);
      }
    }

    pInfo->m_MetaInfo.PushBack(pMetaData);
  }
}

ezTransformStatus ezSubstancePackageAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& assetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezSubstancePackageAssetProperties* pProp = GetProperties();
  ezStringBuilder sAbsolutePackagePath = pProp->m_sSubstancePackage;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath))
  {
    return ezStatus(ezFmt("Couldn't make path absolute: '{0};", sAbsolutePackagePath));
  }

  EZ_SUCCEED_OR_RETURN(UpdateGraphOutputs(sAbsolutePackagePath, transformFlags.IsSet(ezTransformFlags::BackgroundProcessing) == false));

  ezStringBuilder sTempDir;
  EZ_SUCCEED_OR_RETURN(GetTempDir(sTempDir));
  EZ_SUCCEED_OR_RETURN(ezOSFile::CreateDirectoryStructure(sTempDir));

  EZ_SUCCEED_OR_RETURN(RunSbsCooker(sAbsolutePackagePath, sTempDir));

  ezStringView sPackageName = sAbsolutePackagePath.GetFileName();

  ezStringBuilder sSbsarPath = sTempDir;
  sSbsarPath.AppendPath(sPackageName);
  sSbsarPath.Append(".sbsar");

  ezStringBuilder sOutputName, sPngPath, sTargetFile;
  auto& textureTypeDesc = static_cast<const ezSubstancePackageAssetDocumentManager*>(GetDocumentManager())->GetTextureTypeDesc();
  const bool bUpdateThumbnail = pAssetProfile == ezAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();
  auto pAssetConfig = pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>();

  for (auto& graph : pProp->m_Graphs)
  {
    if (graph.m_bEnabled == false)
      continue;

    EZ_SUCCEED_OR_RETURN(RunSbsRender(sSbsarPath, graph.m_sName, nullptr, nullptr, sTempDir, graph.m_uiOutputWidth, graph.m_uiOutputHeight));

    for (auto& output : graph.m_Outputs)
    {
      if (output.m_bEnabled == false)
        continue;

      sPngPath = sTempDir;
      sPngPath.AppendPath(sPackageName);
      sPngPath.Append("_", graph.m_sName, "_", output.m_sName, ".png");

      GenerateOutputName(graph, output, sOutputName);
      sTargetFile = ezStringView(GetDocumentPath()).GetFileDirectory();
      sTargetFile.AppendPath(sOutputName);
      ezString sAbsTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(&textureTypeDesc, sTargetFile, "", pAssetProfile);

      ezString sThumbnailFile = GetAssetDocumentManager()->GenerateResourceThumbnailPath(sTargetFile);
      EZ_SUCCEED_OR_RETURN(RunTexConv(sPngPath, sAbsTargetFile, assetHeader, output, sThumbnailFile, pAssetConfig));
    }
  }

  return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, assetHeader, transformFlags);
}

void ezSubstancePackageAssetDocument::OnPropertyChanged(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet && e.m_sProperty == "SubstanceFile")
  {
    ezStringBuilder sAbsolutePackagePath = e.m_NewValue.Get<ezString>();
    GetProperties()->m_sSubstancePackage = sAbsolutePackagePath;

    bool bSuccess = ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath) &&
                    UpdateGraphOutputs(sAbsolutePackagePath, true).Succeeded();
    if (bSuccess == false)
    {
      ezLog::Error("Substance package not found or invalid '{}'", sAbsolutePackagePath);
    }
  }
}

ezResult ezSubstancePackageAssetDocument::GetTempDir(ezStringBuilder& out_sTempDir) const
{
  auto szDocumentPath = GetDocumentPath();
  const ezString sDataDir = ezAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  ezStringBuilder sRelativePath(szDocumentPath);
  EZ_SUCCEED_OR_RETURN(sRelativePath.MakeRelativeTo(sDataDir));

  out_sTempDir.Set(sDataDir, "/AssetCache/Temp/", sRelativePath.GetFileDirectory());
  out_sTempDir.MakeCleanPath();
  return EZ_SUCCESS;
}

void ezSubstancePackageAssetDocument::GenerateOutputName(const ezSubstanceGraph& graph, const ezSubstanceGraphOutput& graphOutput, ezStringBuilder& out_sOutputName) const
{
  out_sOutputName = GetProperties()->m_sOutputPattern;
  out_sOutputName.ReplaceAll("$(graph)", graph.m_sName);
  out_sOutputName.ReplaceAll("$(name)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(identifier)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(label)", graphOutput.m_sLabel);

  ezStringBuilder sUsage;
  ezReflectionUtils::EnumerationToString(graphOutput.m_Usage, sUsage, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
  out_sOutputName.ReplaceAll("$(usage)", sUsage);
}

ezTransformStatus ezSubstancePackageAssetDocument::UpdateGraphOutputs(ezStringView sAbsolutePath, bool bAllowPropertyModifications)
{
  ezStringBuilder sFileContent;
  EZ_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

  ezHybridArray<ezSubstanceGraph, 2> graphs;

  QXmlStreamReader reader(sFileContent.GetData());
  EZ_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "content"));

  while (reader.atEnd() == false)
  {
    auto tokenType = reader.readNext();
    EZ_IGNORE_UNUSED(tokenType);

    if (reader.isStartElement() && reader.name() == QLatin1StringView("graph"))
    {
      ezSubstanceGraph graph;
      EZ_SUCCEED_OR_RETURN(ParseGraph(reader, graph));

      if (graph.m_sName.StartsWith("_") == false)
      {
        graphs.PushBack(std::move(graph));
      }
    }
  }

  // Transfer enabled state, label and usage
  ezSubstancePackageAssetProperties* pProp = GetProperties();
  for (auto& newGraph : graphs)
  {
    ezSubstanceGraph* pExistingGraph = nullptr;
    for (auto& existingGraph : pProp->m_Graphs)
    {
      if (newGraph.m_sName == existingGraph.m_sName)
      {
        newGraph.m_bEnabled = existingGraph.m_bEnabled;
        pExistingGraph = &existingGraph;
        break;
      }
    }

    if (pExistingGraph == nullptr)
      continue;

    for (auto& newOutput : newGraph.m_Outputs)
    {
      ezSubstanceGraphOutput* pExistingOutput = nullptr;
      for (auto& existingOutput : pExistingGraph->m_Outputs)
      {
        if (newOutput.m_sName == existingOutput.m_sName)
        {
          newOutput.m_bEnabled = existingOutput.m_bEnabled;
          newOutput.m_CompressionMode = existingOutput.m_CompressionMode;
          newOutput.m_uiNumChannels = existingOutput.m_uiNumChannels;
          newOutput.m_MipmapMode = existingOutput.m_MipmapMode;
          newOutput.m_bPreserveAlphaCoverage = existingOutput.m_bPreserveAlphaCoverage;
          newOutput.m_Usage = existingOutput.m_Usage;
          newOutput.m_sLabel = existingOutput.m_sLabel;
          break;
        }
      }
    }
  }

  if (pProp->m_Graphs != graphs)
  {
    if (!bAllowPropertyModifications)
    {
      return ezTransformStatus(ezTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Graphs");

    pProp->m_Graphs = std::move(graphs);

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }

  return ezStatus(EZ_SUCCESS);
}

static const char* s_szTexConvUsageMapping[] = {
  "Auto",      // Unknown,

  "Color",     // BaseColor,
  "Color",     // Emissive,
  "Linear",    // Height,
  "Linear",    // Metallic,
  "Linear",    // Mask,
  "NormalMap", // Normal,
  "Linear",    // Occlusion,
  "Linear",    // Opacity,
  "Linear",    // Roughness,
};

static_assert(EZ_ARRAY_SIZE(s_szTexConvUsageMapping) == ezSubstanceUsage::Count);

static const char* s_szTexConvCompressionMapping[] = {
  "None",
  "Medium",
  "High",
};

static_assert(EZ_ARRAY_SIZE(s_szTexConvCompressionMapping) == ezTexConvCompressionMode::High + 1);

static const char* s_szTexConvMipMapMapping[] = {
  "None",
  "Linear",
  "Kaiser",
};

static_assert(EZ_ARRAY_SIZE(s_szTexConvMipMapMapping) == ezTexConvMipmapMode::Kaiser + 1);

ezStatus ezSubstancePackageAssetDocument::RunTexConv(const char* szInputFile, const char* szTargetFile, const ezAssetFileHeader& assetHeader, const ezSubstanceGraphOutput& graphOutput, ezStringView sThumbnailFile, const ezTextureAssetProfileConfig* pAssetConfig)
{
  QStringList arguments;
  ezStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << ezConversionUtils::ToString(assetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const ezUInt64 uiHash64 = assetHeader.GetFileHash();
    const ezUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const ezUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", ezArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", ezArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }

  arguments << "-in0";
  arguments << szInputFile;

  arguments << "-out";
  arguments << szTargetFile;

  if (sThumbnailFile.IsEmpty() == false)
  {
    // Thumbnail
    const ezStringView sDir = sThumbnailFile.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnailFile.GetData(temp));
  }

  arguments << "-compression";
  arguments << s_szTexConvCompressionMapping[graphOutput.m_CompressionMode];

  arguments << "-mipmaps";
  arguments << s_szTexConvMipMapMapping[graphOutput.m_MipmapMode];

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-usage";
  arguments << s_szTexConvUsageMapping[graphOutput.m_Usage];

  switch (graphOutput.m_uiNumChannels)
  {
    case 1:
      arguments << "-r";
      arguments << "in0.r";
      break;

    case 2:
      arguments << "-rg";
      arguments << "in0.rg";
      break;

    case 3:
      arguments << "-rgb";
      arguments << "in0.rgb";
      break;

    default:
      arguments << "-rgba";
      arguments << "in0.rgba";
      break;
  }

  if (graphOutput.m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    arguments << "0.5";
  }

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("ezTexConv", arguments, 180, ezLog::GetThreadLocalLogSystem()));

  if (sThumbnailFile.IsEmpty() == false)
  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnailFile, thumbnailInfo);
  }

  return ezStatus(EZ_SUCCESS);
}
