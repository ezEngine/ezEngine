#include <RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
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

struct ezRmlUi::Data
{
  ezMutex m_ExtractionMutex;
  ezRmlUiInternal::Extractor m_Extractor;

  ezRmlUiInternal::FileInterface m_FileInterface;
  ezRmlUiInternal::SystemInterface m_SystemInterface;

  ezRmlUiInternal::ContextInstancer m_ContextInstancer;
  ezRmlUiInternal::EventListenerInstancer m_EventListenerInstancer;

  ezDynamicArray<ezRmlUiContext*> m_Contexts;

  ezRmlUiConfiguration m_Config;
};

ezRmlUi::ezRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pData = EZ_DEFAULT_NEW(Data);

  Rml::SetRenderInterface(&m_pData->m_Extractor);
  Rml::SetFileInterface(&m_pData->m_FileInterface);
  Rml::SetSystemInterface(&m_pData->m_SystemInterface);

  Rml::Initialise();

  Rml::Factory::RegisterContextInstancer(&m_pData->m_ContextInstancer);
  Rml::Factory::RegisterEventListenerInstancer(&m_pData->m_EventListenerInstancer);

  const char* szFile = ":project/RmlUiConfig.ddl";
  if (m_pData->m_Config.Load(szFile).Failed())
  {
    ezLog::Warning("No valid RmlUi configuration file available in '{0}'.", szFile);
    return;
  }

  for (auto& font : m_pData->m_Config.m_Fonts)
  {
    if (Rml::LoadFontFace(font.GetData()) == false)
    {
      ezLog::Warning("Failed to load font face '{0}'.", font);
    }
  }
}

ezRmlUi::~ezRmlUi()
{
  Rml::Shutdown();
}

ezRmlUiContext* ezRmlUi::CreateContext(const char* szName, const ezVec2U32& initialSize)
{
  ezRmlUiContext* pContext = static_cast<ezRmlUiContext*>(Rml::CreateContext(szName, Rml::Vector2i(initialSize.x, initialSize.y)));

  m_pData->m_Contexts.PushBack(pContext);

  return pContext;
}

void ezRmlUi::DeleteContext(ezRmlUiContext* pContext)
{
  m_pData->m_Contexts.RemoveAndCopy(pContext);

  Rml::RemoveContext(pContext->GetName());
}

bool ezRmlUi::AnyContextWantsInput()
{
  for (auto pContext : m_pData->m_Contexts)
  {
    if (pContext->WantsInput())
      return true;
  }

  return false;
}

void ezRmlUi::ExtractContext(ezRmlUiContext& context, ezMsgExtractRenderData& msg)
{
  if (context.HasDocument() == false)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  EZ_LOCK(m_pData->m_ExtractionMutex);

  context.ExtractRenderData(m_pData->m_Extractor);

  if (context.m_pRenderData != nullptr)
  {
    msg.AddRenderData(context.m_pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}
