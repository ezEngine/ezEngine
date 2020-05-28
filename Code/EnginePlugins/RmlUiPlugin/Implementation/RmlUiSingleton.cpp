#include <RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

ezResult ezRmlUiConfiguration::Save(const char* szFile) const
{
  ezFileWriter file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);

  writer.BeginObject("Fonts");
  for (auto& font : m_Fonts)
  {
    ezOpenDdlUtils::StoreString(writer, font);
  }
  writer.EndObject();

  return EZ_SUCCESS;
}

ezResult ezRmlUiConfiguration::Load(const char* szFile)
{
  EZ_LOG_BLOCK("ezWorldModuleConfig::Load()");

  m_Fonts.Clear();

  ezFileReader file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    ezLog::Error("Failed to parse RmlUi config file '{0}'", szFile);
    return EZ_FAILURE;
  }

  const ezOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pChild = pTree->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Fonts"))
    {
      for (const ezOpenDdlReaderElement* pFont = pChild->GetFirstChild(); pFont != nullptr; pFont = pFont->GetSibling())
      {
        m_Fonts.PushBack(pFont->GetPrimitivesString()[0]);
      }
    }
  }

  return EZ_SUCCESS;
}

bool ezRmlUiConfiguration::operator==(const ezRmlUiConfiguration& rhs) const
{
  return m_Fonts == rhs.m_Fonts;
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezRmlUi);

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pExtractor = EZ_DEFAULT_NEW(ezRmlUiInternal::Extractor);
  Rml::Core::SetRenderInterface(m_pExtractor.Borrow());

  m_pFileInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::FileInterface);
  Rml::Core::SetFileInterface(m_pFileInterface.Borrow());

  m_pSystemInterface = EZ_DEFAULT_NEW(ezRmlUiInternal::SystemInterface);
  Rml::Core::SetSystemInterface(m_pSystemInterface.Borrow());

  Rml::Core::Initialise();

  const char* szFile = ":project/RmlUiConfig.ddl";
  if (m_Config.Load(szFile).Failed())
  {
    ezLog::Warning("No valid RmlUi configuration file available in '{0}'.", szFile);
    return;
  }

  for (auto& font : m_Config.m_Fonts)
  {
    if (Rml::Core::LoadFontFace(font.GetData()) == false)
    {
      ezLog::Warning("Failed to load font face '{0}'.", font);
    }
  }
}

ezRmlUi::~ezRmlUi()
{
  Rml::Core::Shutdown();
}

ezRmlUiContext* ezRmlUi::CreateContext(const char* szName, const ezVec2U32& initialSize)
{
  m_Contexts.PushBack(EZ_DEFAULT_NEW(ezRmlUiContext, szName, initialSize));

  return m_Contexts.PeekBack().Borrow();
}

void ezRmlUi::DeleteContext(ezRmlUiContext* pContext)
{
  for (ezUInt32 i = 0; i < m_Contexts.GetCount(); ++i)
  {
    if (m_Contexts[i] == pContext)
    {
      m_Contexts.RemoveAtAndCopy(i);
      break;
    }
  }
}

void ezRmlUi::ExtractContext(ezRmlUiContext& context, ezMsgExtractRenderData& msg)
{
  if (context.m_pDocument == nullptr)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  EZ_LOCK(m_ExtractionMutex);

  context.ExtractRenderData(*m_pExtractor);

  if (context.m_pRenderData != nullptr)
  {
    msg.AddRenderData(context.m_pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}
