#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

// preloading assets is considered to be the vast majority of scene loading
constexpr float fCollectionPreloadPiece = 0.9f;

ezSceneLoadUtility::ezSceneLoadUtility() = default;
ezSceneLoadUtility::~ezSceneLoadUtility() = default;

void ezSceneLoadUtility::StartSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollectionFile)
{
  EZ_ASSERT_DEV(m_LoadingState == LoadingState::NotStarted, "Can't reuse an ezSceneLoadUtility.");

  EZ_LOG_BLOCK("StartSceneLoading");

  m_LoadingState = LoadingState::Ongoing;

  m_sRequestedFile = sSceneFile;
  ezStringBuilder sFinalSceneFile = sSceneFile;

  if (sFinalSceneFile.IsEmpty())
  {
    LoadingFailed("No scene file specified.");
    return;
  }

  ezLog::Info("Loading scene '{}'.", sSceneFile);

  if (sFinalSceneFile.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    sFinalSceneFile.MakeRelativeTo(ezGameApplication::GetGameApplicationInstance()->GetAppProjectPath()).IgnoreResult();
  }

  if (sFinalSceneFile.HasExtension("ezScene") || sFinalSceneFile.HasExtension("ezPrefab"))
  {
    if (sFinalSceneFile.IsAbsolutePath())
    {
      if (ezFileSystem::ResolvePath(sFinalSceneFile, nullptr, &sFinalSceneFile).Failed())
      {
        LoadingFailed(ezFmt("Scene path is not located in any data directory: '{}'", sFinalSceneFile));
        return;
      }
    }

    // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
    sFinalSceneFile.Prepend("AssetCache/Common/");

    if (sFinalSceneFile.HasExtension("ezScene"))
      sFinalSceneFile.ChangeFileExtension("ezBinScene");
    else
      sFinalSceneFile.ChangeFileExtension("ezBinPrefab");
  }

  if (sFinalSceneFile != sSceneFile)
  {
    ezLog::Dev("Redirecting scene file from '{}' to '{}'", sSceneFile, sFinalSceneFile);
  }

  m_sRedirectedFile = sFinalSceneFile;

  if (!sPreloadCollectionFile.IsEmpty())
  {
    m_hPreloadCollection = ezResourceManager::LoadResource<ezCollectionResource>(ezString(sPreloadCollectionFile));
  }
}

ezUniquePtr<ezWorld> ezSceneLoadUtility::RetrieveLoadedScene()
{
  EZ_ASSERT_DEV(m_LoadingState == LoadingState::FinishedSuccessfully, "Can't retrieve a scene when loading hasn't finished successfully.");

  m_LoadingState = LoadingState::FinishedAndRetrieved;

  m_pWorld->SetWorldSimulationEnabled(true);

  return std::move(m_pWorld);
}

void ezSceneLoadUtility::LoadingFailed(const ezFormatString& reason)
{
  EZ_ASSERT_DEV(m_LoadingState == LoadingState::Ongoing, "Invalid loading state");
  m_LoadingState = LoadingState::Failed;

  ezStringBuilder tmp;
  m_sFailureReason = reason.GetText(tmp);
}

void ezSceneLoadUtility::TickSceneLoading()
{
  switch (m_LoadingState)
  {
    case LoadingState::FinishedSuccessfully:
    case LoadingState::Failed:
      return;

    default:
      break;
  }

  // update our current loading progress
  {
    m_fLoadingProgress = fCollectionPreloadPiece;

    // if we have a collection, preload that first
    if (m_hPreloadCollection.IsValid())
    {
      m_fLoadingProgress = 0.0f;

      ezResourceLock<ezCollectionResource> pCollection(m_hPreloadCollection, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

      if (pCollection.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        if (pCollection->PreloadResources())
        {
          EZ_REPORT_FAILURE("Failed to start preloading all resources.");
        }

        float progress = 0.0f;
        if (pCollection->IsLoadingFinished(&progress))
        {
          m_fLoadingProgress = fCollectionPreloadPiece;
        }
        else
        {
          m_fLoadingProgress = progress * fCollectionPreloadPiece;
        }
      }
    }

    // if preloading the collection is finished (or we just don't have one) add the world instantiation progress
    if (m_fLoadingProgress == fCollectionPreloadPiece)
    {
      m_fLoadingProgress += m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
    }
  }

  // as long as we are still pre-loading assets from the collection, don't do anything else
  if (m_fLoadingProgress < fCollectionPreloadPiece)
    return;

  // if we haven't created a world yet, do so now, and set up an instantiation context
  if (m_pWorld == nullptr)
  {
    EZ_LOG_BLOCK("LoadObjectGraph", m_sRedirectedFile);

    ezWorldDesc desc(m_sRedirectedFile);
    m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);
    m_pWorld->SetWorldSimulationEnabled(false);

    EZ_LOCK(m_pWorld->GetWriteMarker());

    if (m_FileReader.Open(m_sRedirectedFile).Failed())
    {
      LoadingFailed("Failed to open the file.");
      return;
    }
    else
    {
      // Read and skip the asset file header
      ezAssetFileHeader header;
      header.Read(m_FileReader).AssertSuccess();

      char szSceneTag[16];
      m_FileReader.ReadBytes(szSceneTag, sizeof(char) * 16);

      if (!ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16))
      {
        LoadingFailed("The given file isn't an object-graph file.");
        return;
      }

      if (m_WorldReader.ReadWorldDescription(m_FileReader).Failed())
      {
        LoadingFailed("Error reading world description.");
        return;
      }

      // TODO: make frame time configurable ?
      m_pInstantiationContext = m_WorldReader.InstantiateWorld(*m_pWorld, nullptr, ezTime::MakeFromMilliseconds(1), &m_InstantiationProgress);
    }
  }
  else if (m_pInstantiationContext)
  {
    ezWorldReader::InstantiationContextBase::StepResult res = m_pInstantiationContext->Step();

    if (res == ezWorldReader::InstantiationContextBase::StepResult::ContinueNextFrame)
    {
      // TODO: can we finish the world instantiation without updating the entire world?
      // E.g. only finish component instantiation?
      // also we may want to step the world only with a very small (and fixed!) time-step

      EZ_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();
    }
    else if (res == ezWorldReader::InstantiationContextBase::StepResult::Finished)
    {
      // TODO: ticking twice seems to fix some Jolt physics issues
      EZ_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();

      m_pInstantiationContext = nullptr;
      m_LoadingState = LoadingState::FinishedSuccessfully;
    }

    m_fLoadingProgress = fCollectionPreloadPiece + m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid code path.");
  }
}
