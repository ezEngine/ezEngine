#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezParticleBehavior_SizeCurvePatch_1_2 : public ezGraphPatch
{
public:
  ezParticleBehavior_SizeCurvePatch_1_2()
    : ezGraphPatch("ezParticleBehaviorFactory_SizeCurve", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("SizeCurve", "SharedSizeCurve");
    pNode->RenameProperty("BaseSize", "SizeCurveOffset");
    pNode->RenameProperty("CurveScale", "SizeCurveScale");

    // Set the curve source to "Shared" for backward compatibility
    // In older versions, there was only the shared curve option
    pNode->AddProperty("ChangeSizeWith", (ezInt32)1);
  }
};

ezParticleBehavior_SizeCurvePatch_1_2 g_ezParticleBehavior_SizeCurvePatch_1_2;

//////////////////////////////////////////////////////////////////////////

/// Migrates wind influence and rise speed from Velocity behavior to new Wind and Move behaviors
class ezParticleBehavior_Velocity_1_2 : public ezGraphPatch
{
public:
  ezParticleBehavior_Velocity_1_2()
    : ezGraphPatch("ezParticleBehaviorFactory_Velocity", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Read the wind influence value from the old Velocity behavior
    auto* pWindInfluence = pNode->FindProperty("WindInfluence");
    const float fWindInfluence = (pWindInfluence && pWindInfluence->m_Value.IsFloatingPoint()) ? pWindInfluence->m_Value.ConvertTo<float>() : 0.0f;

    // Read the rise speed value from the old Velocity behavior
    auto* pRiseSpeed = pNode->FindProperty("RiseSpeed");
    const float fRiseSpeed = (pRiseSpeed && pRiseSpeed->m_Value.IsFloatingPoint()) ? pRiseSpeed->m_Value.ConvertTo<float>() : 0.0f;

    // Only create new behaviors if we have something to migrate
    if (fWindInfluence <= 0.0f && fRiseSpeed == 0.0f)
      return;

    // Find the parent system descriptor node
    ezAbstractObjectNode* pSystemNode = nullptr;
    for (auto it = pGraph->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      ezAbstractObjectNode* pCandidate = it.Value();
      if (pCandidate->GetType() == "ezParticleSystemDescriptor")
      {
        // Check if this system contains our velocity behavior
        auto* pBehaviors = pCandidate->FindProperty("Behaviors");
        if (pBehaviors && pBehaviors->m_Value.IsA<ezVariantArray>())
        {
          const auto& behaviors = pBehaviors->m_Value.Get<ezVariantArray>();
          for (const auto& behaviorVar : behaviors)
          {
            if (behaviorVar.IsA<ezUuid>() && behaviorVar.Get<ezUuid>() == pNode->GetGuid())
            {
              pSystemNode = pCandidate;
              break;
            }
          }
        }
        if (pSystemNode)
          break;
      }
    }

    if (pSystemNode == nullptr)
      return;

    // Get the behaviors array to add new behaviors to
    auto* pBehaviors = pSystemNode->FindProperty("Behaviors");
    if (!pBehaviors || !pBehaviors->m_Value.IsA<ezVariantArray>())
      return;

    ezVariantArray behaviors = pBehaviors->m_Value.Get<ezVariantArray>();

    // Create a new Wind behavior node if wind influence is greater than 0
    if (fWindInfluence > 0.0f)
    {
      ezUuid windBehaviorGuid = ezUuid::MakeUuid();
      ezAbstractObjectNode* pWindNode = pGraph->AddNode(windBehaviorGuid, "ezParticleBehavior_Wind", 1);
      pWindNode->AddProperty("WindInfluence", fWindInfluence);
      behaviors.PushBack(windBehaviorGuid);
    }

    // Create a new Move behavior node if rise speed is non-zero
    if (fRiseSpeed != 0.0f)
    {
      ezUuid moveBehaviorGuid = ezUuid::MakeUuid();
      ezAbstractObjectNode* pMoveNode = pGraph->AddNode(moveBehaviorGuid, "ezParticleBehavior_Move", 1);

      // Set Z-axis movement to constant mode with the rise speed value
      pMoveNode->AddProperty("MoveZ_Mode", (ezInt32)0); // ezMovementMode::Constant = 0
      pMoveNode->AddProperty("MoveZ_Speed", fRiseSpeed);

      behaviors.PushBack(moveBehaviorGuid);
    }

    // Update the behaviors array with the new behaviors
    pBehaviors->m_Value = behaviors;
  }
};

ezParticleBehavior_Velocity_1_2 g_ezParticleBehavior_Velocity_1_2;

//////////////////////////////////////////////////////////////////////////

/// Migrates ColorGradient behavior from old hGradient to new GradientSource system
class ezParticleBehavior_ColorGradient_2_3 : public ezGraphPatch
{
public:
  ezParticleBehavior_ColorGradient_2_3()
    : ezGraphPatch("ezParticleBehaviorFactory_ColorGradient", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Check if there's an old hGradient property (from version 2)
    auto* pGradientHandle = pNode->FindProperty("Gradient");
    if (pGradientHandle)
    {
      // Rename the old property to the new SharedGradient name
      pNode->RenameProperty("Gradient", "SharedGradient");

      // Set GradientSource to SharedGradient (1) for backward compatibility
      pNode->AddProperty("GradientSource", (ezInt32)1);
    }
    else
    {
      // If no old property exists, default to CustomGradient (0)
      pNode->AddProperty("GradientSource", (ezInt32)0);
    }
  }
};

ezParticleBehavior_ColorGradient_2_3 g_ezParticleBehavior_ColorGradient_2_3;

//////////////////////////////////////////////////////////////////////////

/// Migrates RandomColor initializer from old hGradient to new GradientSource system
class ezParticleInitializer_RandomColor_2_3 : public ezGraphPatch
{
public:
  ezParticleInitializer_RandomColor_2_3()
    : ezGraphPatch("ezParticleInitializerFactory_RandomColor", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Check if there's an old hGradient property (from version 2)
    auto* pGradientHandle = pNode->FindProperty("Gradient");
    if (pGradientHandle)
    {
      // Rename the old property to the new SharedGradient name
      pNode->RenameProperty("Gradient", "SharedGradient");

      // Set GradientSource to SharedGradient (1) for backward compatibility
      pNode->AddProperty("GradientSource", (ezInt32)1);
    }
    else
    {
      // If no old property exists, default to CustomGradient (0)
      pNode->AddProperty("GradientSource", (ezInt32)0);
    }
  }
};

ezParticleInitializer_RandomColor_2_3 g_ezParticleInitializer_RandomColor_2_3;

//////////////////////////////////////////////////////////////////////////

/// The particle module classes used to be called factories, when there was an additional class for the instance of each module.
class ezParticleModuleRenamePatch : public ezGraphPatch
{
public:
  ezParticleModuleRenamePatch(const char* szOldName, const char* szNewName, ezUInt32 uiCurrentVersion)
    : ezGraphPatch(szOldName, uiCurrentVersion + 1)
    , m_szNewName(szNewName)
    , m_uiCurrentVersion(uiCurrentVersion)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass(m_szNewName, m_uiCurrentVersion);
  }

private:
  const char* m_szNewName;
  ezUInt32 m_uiCurrentVersion;
};

static ezParticleModuleRenamePatch g_ParticleModuleRenamePatches[] = {
  {"ezParticleBehaviorFactory_Attract", "ezParticleBehavior_Attract", 1},
  {"ezParticleBehaviorFactory_Bounds", "ezParticleBehavior_Bounds", 1},
  {"ezParticleBehaviorFactory_BoundsSphere", "ezParticleBehavior_BoundsSphere", 1},
  {"ezParticleBehaviorFactory_ColorGradient", "ezParticleBehavior_ColorGradient", 3},
  {"ezParticleBehaviorFactory_Expression", "ezParticleBehavior_Expression", 1},
  {"ezParticleBehaviorFactory_FadeOut", "ezParticleBehavior_FadeOut", 1},
  {"ezParticleBehaviorFactory_Flies", "ezParticleBehavior_Flies", 1},
  {"ezParticleBehaviorFactory_Gravity", "ezParticleBehavior_Gravity", 1},
  {"ezParticleBehaviorFactory_Move", "ezParticleBehavior_Move", 1},
  {"ezParticleBehaviorFactory_Opacity", "ezParticleBehavior_Opacity", 2},
  {"ezParticleBehaviorFactory_PullAlong", "ezParticleBehavior_PullAlong", 1},
  {"ezParticleBehaviorFactory_Raycast", "ezParticleBehavior_Raycast", 1},
  {"ezParticleBehaviorFactory_SizeCurve", "ezParticleBehavior_SizeCurve", 2},
  {"ezParticleBehaviorFactory_Turbulence", "ezParticleBehavior_Turbulence", 1},
  {"ezParticleBehaviorFactory_Velocity", "ezParticleBehavior_Velocity", 2},
  {"ezParticleBehaviorFactory_Wind", "ezParticleBehavior_Wind", 1},
  {"ezParticleEmitterFactory_Burst", "ezParticleEmitter_Burst", 1},
  {"ezParticleEmitterFactory_Continuous", "ezParticleEmitter_Continuous", 1},
  {"ezParticleEmitterFactory_Distance", "ezParticleEmitter_Distance", 1},
  {"ezParticleEmitterFactory_OnEvent", "ezParticleEmitter_OnEvent", 1},
  {"ezParticleEventReactionFactory_Effect", "ezParticleEventReaction_Effect", 1},
  {"ezParticleEventReactionFactory_Prefab", "ezParticleEventReaction_Prefab", 1},
  {"ezParticleFinalizerFactory_Age", "ezParticleFinalizer_Age", 1},
  {"ezParticleFinalizerFactory_ApplyVelocity", "ezParticleFinalizer_ApplyVelocity", 1},
  {"ezParticleFinalizerFactory_LastPosition", "ezParticleFinalizer_LastPosition", 1},
  {"ezParticleFinalizerFactory_Volume", "ezParticleFinalizer_Volume", 1},
  {"ezParticleInitializerFactory_BoxPosition", "ezParticleInitializer_BoxPosition", 1},
  {"ezParticleInitializerFactory_CylinderPosition", "ezParticleInitializer_CylinderPosition", 2},
  {"ezParticleInitializerFactory_MeshPosition", "ezParticleInitializer_MeshPosition", 1},
  {"ezParticleInitializerFactory_RandomColor", "ezParticleInitializer_RandomColor", 3},
  {"ezParticleInitializerFactory_RandomRotationSpeed", "ezParticleInitializer_RandomRotationSpeed", 2},
  {"ezParticleInitializerFactory_RandomSize", "ezParticleInitializer_RandomSize", 3},
  {"ezParticleInitializerFactory_SpherePosition", "ezParticleInitializer_SpherePosition", 2},
  {"ezParticleInitializerFactory_VelocityCone", "ezParticleInitializer_VelocityCone", 2},
  {"ezParticleTypeEffectFactory", "ezParticleTypeEffect", 1},
  {"ezParticleTypeLightFactory", "ezParticleTypeLight", 1},
  {"ezParticleTypeMeshFactory", "ezParticleTypeMesh", 1},
  {"ezParticleTypePointFactory", "ezParticleTypePoint", 1},
  {"ezParticleTypeQuadFactory", "ezParticleTypeQuad", 2},
  {"ezParticleTypeTrailFactory", "ezParticleTypeTrail", 1},
};
