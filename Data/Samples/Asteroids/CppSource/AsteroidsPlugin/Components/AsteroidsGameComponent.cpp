#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/Components/AsteroidComponent.h>
#include <AsteroidsPlugin/Components/AsteroidsGameComponent.h>
#include <AsteroidsPlugin/Components/PickupComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Random.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

namespace
{
  /// The values for the three asteroid sizes, from large to small.
  struct AsteroidSize
  {
    float m_fRadius;
    float m_fMinSpeed;
    float m_fMaxSpeed;
    ezUInt32 m_uiPoints;
    float m_fShake;
  };

  constexpr AsteroidSize s_AsteroidSizes[] = {
    {5.0f, 4.0f, 9.0f, 20, 0.5f},
    {2.8f, 7.0f, 14.0f, 50, 0.3f},
    {1.6f, 10.0f, 19.0f, 100, 0.15f},
  };

  constexpr ezUInt32 s_uiPointsForExtraLife = 10000;
  constexpr ezUInt32 s_uiPointsForPickup = 250;
  constexpr ezUInt32 s_uiMaxAsteroidsPerWave = 11;
  constexpr ezUInt32 s_uiTitleScreenAsteroids = 5;
  constexpr float s_fMaxCameraShake = 1.6f;

  constexpr double s_fRespawnDelay = 1.0;
  constexpr double s_fNextWaveDelay = 2.5;
  constexpr double s_fRestartDelay = 1.0;
  constexpr double s_fSpawnInvulnerability = 2.5;

  /// The radius for which the explosion effect was authored, its 'Scale' parameter is relative to this.
  constexpr float s_fExplosionBaseRadius = 3.0f;

  /// The values of the HUD's 'Phase' entry, in the order of the Phase enum. UI/Hud.rml decides what to show for each.
  constexpr const char* s_PhaseNames[] = {"Title", "Playing", "Respawning", "WaveCleared", "GameOver"};

  bool Overlap(const ezVec3& vPosA, float fRadiusA, const ezVec3& vPosB, float fRadiusB)
  {
    return (vPosA - vPosB).GetLengthSquared() < ezMath::Square(fRadiusA + fRadiusB);
  }

  /// Deleted components stay in their manager until the end of the frame, but are not active anymore.
  template <typename ManagerType>
  void DeleteOwnersOfAllComponents(ezWorld* pWorld)
  {
    for (auto it = pWorld->GetOrCreateComponentManager<ManagerType>()->GetComponents(); it.IsValid(); ++it)
    {
      if (it->IsActive())
        pWorld->DeleteObjectDelayed(it->GetOwner()->GetHandle());
    }
  }

  void LoadPrefabs(ezArrayPtr<const ezString> paths, ezDynamicArray<ezPrefabResourceHandle>& out_prefabs)
  {
    out_prefabs.Clear();

    for (const ezString& sPath : paths)
    {
      if (sPath.IsEmpty())
        continue;

      ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(sPath);
      ezResourceManager::PreloadResource(hPrefab);
      out_prefabs.PushBack(hPrefab);
    }
  }

  ezVec3 RandomDirection2D(ezRandom& ref_rng)
  {
    const ezAngle angle = ezAngle::MakeFromDegree((float)ref_rng.DoubleMinMax(0.0, 360.0));
    return ezVec3(ezMath::Cos(angle), ezMath::Sin(angle), 0.0f);
  }
} // namespace

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(AsteroidsGameComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("ShipPrefab", m_hShipPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("AsteroidPrefabs", m_AsteroidPrefabs)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("PickupPrefabs", m_PickupPrefabs)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("WaveClearedSound", m_sWaveClearedSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("GameOverSound", m_sGameOverSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("PickupChance", m_fPickupChance)->AddAttributes(new ezDefaultValueAttribute(0.12f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("StartLives", m_uiStartLives)->AddAttributes(new ezDefaultValueAttribute(3)),
    EZ_MEMBER_PROPERTY("StartAsteroids", m_uiStartAsteroids)->AddAttributes(new ezDefaultValueAttribute(3)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Asteroids"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

AsteroidsGameComponent::AsteroidsGameComponent() = default;
AsteroidsGameComponent::~AsteroidsGameComponent() = default;

void AsteroidsGameComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hShipPrefab;
  s.WriteArray(m_AsteroidPrefabs).AssertSuccess();
  s.WriteArray(m_PickupPrefabs).AssertSuccess();
  s << m_sWaveClearedSound;
  s << m_sGameOverSound;
  s << m_fPickupChance;
  s << m_uiStartLives;
  s << m_uiStartAsteroids;
}

void AsteroidsGameComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_hShipPrefab;
  s.ReadArray(m_AsteroidPrefabs).AssertSuccess();
  s.ReadArray(m_PickupPrefabs).AssertSuccess();
  s >> m_sWaveClearedSound;
  s >> m_sGameOverSound;
  s >> m_fPickupChance;
  s >> m_uiStartLives;
  s >> m_uiStartAsteroids;
}

void AsteroidsGameComponent::MoveAndWrap(ezGameObject* pObject, const ezVec3& vVelocity, float fRadius)
{
  const ezWorld* pWorld = pObject->GetWorld();

  ezVec3 vPosition = pObject->GetLocalPosition() + vVelocity * pWorld->GetClock().GetTimeDiff().AsFloatInSeconds();

  if (const AsteroidsGameComponentManager* pManager = pWorld->GetComponentManager<AsteroidsGameComponentManager>())
  {
    auto it = pManager->GetComponents();
    if (it.IsValid())
    {
      const float fLimitX = it->m_vHalfExtents.x + fRadius;
      const float fLimitY = it->m_vHalfExtents.y + fRadius;

      if (ezMath::Abs(vPosition.x) > fLimitX)
        vPosition.x = ezMath::Sign(vPosition.x) * -fLimitX;

      if (ezMath::Abs(vPosition.y) > fLimitY)
        vPosition.y = ezMath::Sign(vPosition.y) * -fLimitY;
    }
  }

  pObject->SetLocalPosition(vPosition);
}

ezGameObject* AsteroidsGameComponent::SpawnPrefab(ezWorld* pWorld, const ezPrefabResourceHandle& hPrefab, const ezTransform& transform)
{
  if (!hPrefab.IsValid())
    return nullptr;

  ezHybridArray<ezGameObject*, 4> created;
  ezPrefabInstantiationOptions options;
  options.m_pCreatedRootObjectsOut = &created;

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::BlockTillLoaded);
  pPrefab->InstantiatePrefab(*pWorld, transform, options);

  return created.IsEmpty() ? nullptr : created[0];
}

void AsteroidsGameComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezBlackboardComponent* pBlackboard = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pBlackboard))
  {
    pBlackboard->EnsureInitialized();
    m_pHud = pBlackboard->GetBoard();
  }

  ezGameObject* pCamera = nullptr;
  if (GetWorld()->TryGetObjectWithGlobalKey(ezTempHashedString("MainCamera"), pCamera))
  {
    m_hCamera = pCamera->GetHandle();
    m_vCameraBasePosition = pCamera->GetLocalPosition();
  }

  // make sure spawning something for the first time doesn't cause a hiccup during gameplay
  if (m_hShipPrefab.IsValid())
    ezResourceManager::PreloadResource(m_hShipPrefab);

  LoadPrefabs(m_AsteroidPrefabs, m_hAsteroidPrefabs);
  LoadPrefabs(m_PickupPrefabs, m_hPickupPrefabs);

  UpdatePlayfield();

  // the title screen has some asteroids floating around, but no ship
  SpawnAsteroidsAtEdge(s_uiTitleScreenAsteroids, 1.0f);

  SetPhase(Phase::Title);
}

void AsteroidsGameComponent::Update()
{
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  m_PhaseTimer -= tDiff;

  UpdatePlayfield();
  UpdatePhase();
  UpdateCollisions();
  UpdateCameraShake(tDiff.AsFloatInSeconds());
  UpdateHud();
}

void AsteroidsGameComponent::UpdatePlayfield()
{
  // The playfield is exactly what the camera sees at Z=0.
  // That depends on the window size, so wide screens get a wider playfield.
  for (const ezViewHandle& hView : ezRenderWorld::GetMainViews())
  {
    ezView* pView = nullptr;
    if (!ezRenderWorld::TryGetView(hView, pView) || pView->GetWorld() != GetWorld())
      continue;

    // while playing inside the editor, the viewport of the editor shows the same world
    if (pView->GetCameraUsageHint() != ezCameraUsageHint::MainView)
      continue;

    const ezCamera* pCamera = pView->GetCamera();
    const ezVec2 vViewportSize(pView->GetViewport().width, pView->GetViewport().height);

    if (!pCamera->IsPerspective() || vViewportSize.y <= 0.0f)
      continue;

    const float fAspectRatio = vViewportSize.x / vViewportSize.y;
    const float fHeight = pCamera->GetCenterPosition().z;

    m_vHalfExtents.x = fHeight * ezMath::Tan(pCamera->GetFovX(fAspectRatio) * 0.5f);
    m_vHalfExtents.y = fHeight * ezMath::Tan(pCamera->GetFovY(fAspectRatio) * 0.5f);
    return;
  }
}

void AsteroidsGameComponent::UpdateCameraShake(float fTimeDiff)
{
  ezGameObject* pCamera = nullptr;
  if (!GetWorld()->TryGetObject(m_hCamera, pCamera))
    return;

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  const double fOffset = ezMath::Square(m_fShake) * s_fMaxCameraShake;
  const ezVec3 vOffset((float)rng.DoubleMinMax(-fOffset, fOffset), (float)rng.DoubleMinMax(-fOffset, fOffset), 0.0f);

  pCamera->SetLocalPosition(m_vCameraBasePosition + vOffset);

  m_fShake = ezMath::Max(0.0f, m_fShake - fTimeDiff * 1.5f);
}

void AsteroidsGameComponent::SetPhase(Phase phase)
{
  m_Phase = phase;

  if (m_pHud != nullptr)
  {
    m_pHud->SetEntryValue("Phase", s_PhaseNames[(ezUInt32)phase]);
  }

  switch (phase)
  {
    case Phase::Respawning:
      m_PhaseTimer = ezTime::MakeFromSeconds(s_fRespawnDelay);
      break;

    case Phase::WaveCleared:
      m_PhaseTimer = ezTime::MakeFromSeconds(s_fNextWaveDelay);
      break;

    case Phase::GameOver:
      m_PhaseTimer = ezTime::MakeFromSeconds(s_fRestartDelay);
      break;

    default:
      break;
  }
}

void AsteroidsGameComponent::UpdatePhase()
{
  const bool bFirePressed = ezInputManager::GetInputActionState("Game", "Fire") == ezKeyState::Pressed;

  switch (m_Phase)
  {
    case Phase::Title:
      if (bFirePressed)
      {
        StartNewGame();
      }
      break;

    case Phase::Playing:
      break;

    case Phase::Respawning:
      if (m_PhaseTimer.IsZeroOrNegative() && bFirePressed)
      {
        SpawnShip();
        SetPhase(Phase::Playing);
      }
      break;

    case Phase::WaveCleared:
      if (m_PhaseTimer.IsZeroOrNegative())
      {
        ++m_uiWave;
        StartWave();
        SetPhase(Phase::Playing);
      }
      break;

    case Phase::GameOver:
      if (m_PhaseTimer.IsZeroOrNegative() && bFirePressed)
      {
        StartNewGame();
      }
      break;
  }
}

void AsteroidsGameComponent::StartNewGame()
{
  ClearObjects();

  m_uiScore = 0;
  m_uiLives = m_uiStartLives;
  m_uiWave = 1;
  m_uiNextExtraLife = s_uiPointsForExtraLife;

  SpawnShip();
  StartWave();
  SetPhase(Phase::Playing);
}

void AsteroidsGameComponent::StartWave()
{
  const ezUInt32 uiNumAsteroids = ezMath::Min(m_uiStartAsteroids + m_uiWave - 1, s_uiMaxAsteroidsPerWave);
  const float fSpeedFactor = 1.0f + 0.08f * ezMath::Min<ezUInt32>(m_uiWave - 1, 10);

  SpawnAsteroidsAtEdge(uiNumAsteroids, fSpeedFactor);
}

void AsteroidsGameComponent::SpawnAsteroidsAtEdge(ezUInt32 uiNumAsteroids, float fSpeedFactor)
{
  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  const AsteroidSize& size = s_AsteroidSizes[0];

  // asteroids always come in from the edge of the screen, so nothing appears right on top of the player
  const float fEdgeX = m_vHalfExtents.x + size.m_fRadius;
  const float fEdgeY = m_vHalfExtents.y + size.m_fRadius;

  for (ezUInt32 i = 0; i < uiNumAsteroids; ++i)
  {
    ezVec3 vPosition;
    if (rng.Bool())
      vPosition.Set(rng.Bool() ? fEdgeX : -fEdgeX, (float)rng.DoubleMinMax(-fEdgeY, fEdgeY), 0.0f);
    else
      vPosition.Set((float)rng.DoubleMinMax(-fEdgeX, fEdgeX), rng.Bool() ? fEdgeY : -fEdgeY, 0.0f);

    const float fSpeed = (float)rng.DoubleMinMax(size.m_fMinSpeed, size.m_fMaxSpeed) * fSpeedFactor;

    SpawnAsteroid(0, vPosition, RandomDirection2D(rng) * fSpeed);
  }
}

void AsteroidsGameComponent::UpdateCollisions()
{
  // Spawning new asteroids while iterating over the manager would invalidate the iterators, so first collect all hits.
  ezDynamicArray<AsteroidHit> hits;

  UpdateProjectileHits(hits);
  UpdateShipCollisions(hits);

  for (const AsteroidHit& hit : hits)
  {
    DestroyAsteroid(hit.m_pAsteroid, hit.m_vDirection);
  }

  CheckWaveCleared();
}

void AsteroidsGameComponent::UpdateProjectileHits(ezDynamicArray<AsteroidHit>& inout_hits)
{
  ezWorld* pWorld = GetWorld();

  auto* pAsteroids = pWorld->GetOrCreateComponentManager<AsteroidComponentManager>();
  auto* pProjectiles = pWorld->GetOrCreateComponentManager<ProjectileComponentManager>();

  for (auto itProjectile = pProjectiles->GetComponents(); itProjectile.IsValid(); ++itProjectile)
  {
    if (!itProjectile->IsActive())
      continue;

    const ezVec3 vPosition = itProjectile->GetOwner()->GetGlobalPosition();

    for (auto itAsteroid = pAsteroids->GetComponents(); itAsteroid.IsValid(); ++itAsteroid)
    {
      if (!itAsteroid->IsActive() || itAsteroid->m_bDestroyed)
        continue;

      if (Overlap(vPosition, itProjectile->m_fRadius, itAsteroid->GetOwner()->GetGlobalPosition(), itAsteroid->GetRadius()))
      {
        itAsteroid->m_bDestroyed = true;
        inout_hits.PushBack(AsteroidHit{itAsteroid, itProjectile->m_vVelocity});

        pWorld->DeleteObjectDelayed(itProjectile->GetOwner()->GetHandle());
        break;
      }
    }
  }
}

void AsteroidsGameComponent::UpdateShipCollisions(ezDynamicArray<AsteroidHit>& inout_hits)
{
  ezWorld* pWorld = GetWorld();

  ezGameObject* pShipObject = nullptr;
  ShipComponent* pShip = nullptr;
  if (!pWorld->TryGetObject(m_hShip, pShipObject) || !pShipObject->TryGetComponentOfBaseType(pShip))
    return;

  const ezVec3 vShipPos = pShipObject->GetGlobalPosition();

  auto* pPickups = pWorld->GetOrCreateComponentManager<PickupComponentManager>();

  for (auto itPickup = pPickups->GetComponents(); itPickup.IsValid(); ++itPickup)
  {
    if (!itPickup->IsActive())
      continue;

    if (Overlap(vShipPos, pShip->m_fRadius, itPickup->GetOwner()->GetGlobalPosition(), itPickup->m_fRadius))
    {
      if (itPickup->m_Type == PickupType::Shield)
        pShip->GiveShield();
      else
        pShip->GiveTripleShot();

      AddScore(s_uiPointsForPickup);
      PlaySound(itPickup->m_sCollectSound);
      pWorld->DeleteObjectDelayed(itPickup->GetOwner()->GetHandle());
    }
  }

  if (pShip->IsInvulnerable())
    return;

  auto* pAsteroids = pWorld->GetOrCreateComponentManager<AsteroidComponentManager>();

  for (auto itAsteroid = pAsteroids->GetComponents(); itAsteroid.IsValid(); ++itAsteroid)
  {
    // the rocks are not perfectly round, so be a little forgiving
    if (!itAsteroid->IsActive() || itAsteroid->m_bDestroyed || !Overlap(vShipPos, pShip->m_fRadius, itAsteroid->GetOwner()->GetGlobalPosition(), itAsteroid->GetRadius() * 0.85f))
      continue;

    // ramming an asteroid also destroys that asteroid, just like shooting it would
    itAsteroid->m_bDestroyed = true;
    inout_hits.PushBack(AsteroidHit{itAsteroid, pShip->GetVelocity()});

    if (pShip->Hit())
      DestroyShip(pShip);

    break;
  }
}

void AsteroidsGameComponent::CheckWaveCleared()
{
  if (m_Phase != Phase::Playing)
    return;

  const auto* pAsteroids = GetWorld()->GetOrCreateComponentManager<AsteroidComponentManager>();

  for (auto it = pAsteroids->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && !it->m_bDestroyed)
      return;
  }

  PlaySound(m_sWaveClearedSound);
  SetPhase(Phase::WaveCleared);
}

void AsteroidsGameComponent::AddScore(ezUInt32 uiPoints)
{
  // asteroids also get destroyed on the title screen, and by projectiles that are still flying after the game has ended
  if (m_Phase == Phase::Title || m_Phase == Phase::GameOver)
    return;

  m_uiScore += uiPoints;
  m_uiHighScore = ezMath::Max(m_uiHighScore, m_uiScore);

  if (m_uiScore >= m_uiNextExtraLife)
  {
    ++m_uiLives;
    m_uiNextExtraLife += s_uiPointsForExtraLife;
  }
}

void AsteroidsGameComponent::DestroyAsteroid(AsteroidComponent* pAsteroid, const ezVec3& vHitDirection)
{
  const ezUInt8 uiSize = pAsteroid->GetSize();
  const AsteroidSize& size = s_AsteroidSizes[uiSize];
  const ezVec3 vPosition = pAsteroid->GetOwner()->GetGlobalPosition();
  const ezVec3 vVelocity = pAsteroid->GetVelocity();

  AddScore(size.m_uiPoints);
  m_fShake = ezMath::Min(1.0f, m_fShake + size.m_fShake);

  if (ezGameObject* pExplosion = SpawnPrefab(GetWorld(), pAsteroid->m_hExplosionPrefab, ezTransform(vPosition)))
  {
    // 'Scale' is an exposed parameter of the particle effect
    ezMsgSetFloatParameter msg;
    msg.m_sParameterName = "Scale";
    msg.m_fValue = size.m_fRadius / s_fExplosionBaseRadius;
    pExplosion->SendMessage(msg);
  }

  GetWorld()->DeleteObjectDelayed(pAsteroid->GetOwner()->GetHandle());

  TrySpawnPickup(vPosition);

  if (uiSize + 1 >= EZ_ARRAY_SIZE(s_AsteroidSizes))
    return;

  // the fragments fly apart sideways to the direction of the hit
  ezVec3 vSide(-vHitDirection.y, vHitDirection.x, 0.0f);
  vSide.NormalizeIfNotZero(ezVec3(0, 1, 0)).IgnoreResult();

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();
  const AsteroidSize& nextSize = s_AsteroidSizes[uiSize + 1];

  for (float fDirection : {1.0f, -1.0f})
  {
    const float fSpeed = (float)rng.DoubleMinMax(nextSize.m_fMinSpeed, nextSize.m_fMaxSpeed);
    SpawnAsteroid(uiSize + 1, vPosition + vSide * fDirection * nextSize.m_fRadius, vVelocity * 0.5f + vSide * fDirection * fSpeed);
  }
}

void AsteroidsGameComponent::DestroyShip(ShipComponent* pShip)
{
  SpawnPrefab(GetWorld(), pShip->m_hExplosionPrefab, ezTransform(pShip->GetOwner()->GetGlobalPosition()));
  GetWorld()->DeleteObjectDelayed(m_hShip);
  m_hShip.Invalidate();

  m_fShake = 1.0f;
  --m_uiLives;

  if (m_uiLives > 0)
  {
    SetPhase(Phase::Respawning);
  }
  else
  {
    PlaySound(m_sGameOverSound);
    SetPhase(Phase::GameOver);
  }
}

void AsteroidsGameComponent::TrySpawnPickup(const ezVec3& vPosition)
{
  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  if (m_Phase == Phase::Title || (float)rng.DoubleZeroToOneExclusive() >= m_fPickupChance)
    return;

  ezGameObject* pObject = SpawnRandomPrefab(m_hPickupPrefabs, vPosition);

  PickupComponent* pPickup = nullptr;
  if (pObject != nullptr && pObject->TryGetComponentOfBaseType(pPickup))
  {
    pPickup->m_vVelocity = RandomDirection2D(rng) * 3.0f;
  }
}

void AsteroidsGameComponent::SpawnShip()
{
  // start facing up
  const ezQuat qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90.0f));

  ezGameObject* pObject = SpawnPrefab(GetWorld(), m_hShipPrefab, ezTransform(ezVec3::MakeZero(), qRotation));
  if (pObject == nullptr)
    return;

  m_hShip = pObject->GetHandle();

  ShipComponent* pShip = nullptr;
  if (pObject->TryGetComponentOfBaseType(pShip))
  {
    pShip->SetInvulnerable(ezTime::MakeFromSeconds(s_fSpawnInvulnerability));
  }
}

void AsteroidsGameComponent::SpawnAsteroid(ezUInt8 uiSize, const ezVec3& vPosition, const ezVec3& vVelocity)
{
  ezGameObject* pObject = SpawnRandomPrefab(m_hAsteroidPrefabs, vPosition);

  AsteroidComponent* pAsteroid = nullptr;
  if (pObject != nullptr && pObject->TryGetComponentOfBaseType(pAsteroid))
  {
    pAsteroid->Launch(uiSize, s_AsteroidSizes[uiSize].m_fRadius, vVelocity);
  }
}

ezGameObject* AsteroidsGameComponent::SpawnRandomPrefab(ezArrayPtr<const ezPrefabResourceHandle> prefabs, const ezVec3& vPosition)
{
  if (prefabs.IsEmpty())
    return nullptr;

  const ezUInt32 uiIndex = GetWorld()->GetRandomNumberGenerator().UIntInRange(prefabs.GetCount());
  return SpawnPrefab(GetWorld(), prefabs[uiIndex], ezTransform(vPosition));
}

void AsteroidsGameComponent::ClearObjects()
{
  ezWorld* pWorld = GetWorld();

  for (auto it = pWorld->GetOrCreateComponentManager<AsteroidComponentManager>()->GetComponents(); it.IsValid(); ++it)
  {
    // they are only deleted at the end of the frame, until then they must not take part in the game anymore
    it->m_bDestroyed = true;
  }

  DeleteOwnersOfAllComponents<AsteroidComponentManager>(pWorld);
  DeleteOwnersOfAllComponents<ProjectileComponentManager>(pWorld);
  DeleteOwnersOfAllComponents<PickupComponentManager>(pWorld);

  if (!m_hShip.IsInvalidated())
  {
    pWorld->DeleteObjectDelayed(m_hShip);
    m_hShip.Invalidate();
  }
}

void AsteroidsGameComponent::PlaySound(const ezString& sSound)
{
  if (!sSound.IsEmpty())
  {
    ezSoundInterface::PlaySound(GetWorld(), sSound, GetOwner()->GetGlobalTransform(), 1.0f, 1.0f, false).IgnoreResult();
  }
}

void AsteroidsGameComponent::UpdateHud()
{
  if (m_pHud == nullptr)
    return;

  // the data binding only updates the UI when an entry changes its value, so it is fine to set these every frame
  m_pHud->SetEntryValue("Score", (ezInt32)m_uiScore);
  m_pHud->SetEntryValue("HighScore", (ezInt32)m_uiHighScore);
  m_pHud->SetEntryValue("Wave", (ezInt32)m_uiWave);
  m_pHud->SetEntryValue("Lives", (ezInt32)m_uiLives);
}
