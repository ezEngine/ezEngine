#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezParticleBehaviorFactory_SizeCurvePatch_1_2 : public ezGraphPatch
{
public:
  ezParticleBehaviorFactory_SizeCurvePatch_1_2()
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

ezParticleBehaviorFactory_SizeCurvePatch_1_2 g_ezParticleBehaviorFactory_SizeCurvePatch_1_2;

//////////////////////////////////////////////////////////////////////////

/// Migrates wind influence and rise speed from Velocity behavior to new Wind and Move behaviors
class ezParticleBehaviorFactory_Velocity_1_2 : public ezGraphPatch
{
public:
  ezParticleBehaviorFactory_Velocity_1_2()
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
      ezAbstractObjectNode* pWindNode = pGraph->AddNode(windBehaviorGuid, "ezParticleBehaviorFactory_Wind", 1);
      pWindNode->AddProperty("WindInfluence", fWindInfluence);
      behaviors.PushBack(windBehaviorGuid);
    }

    // Create a new Move behavior node if rise speed is non-zero
    if (fRiseSpeed != 0.0f)
    {
      ezUuid moveBehaviorGuid = ezUuid::MakeUuid();
      ezAbstractObjectNode* pMoveNode = pGraph->AddNode(moveBehaviorGuid, "ezParticleBehaviorFactory_Move", 1);

      // Set Z-axis movement to constant mode with the rise speed value
      pMoveNode->AddProperty("MoveZ_Mode", (ezInt32)0); // ezMovementMode::Constant = 0
      pMoveNode->AddProperty("MoveZ_Speed", fRiseSpeed);

      behaviors.PushBack(moveBehaviorGuid);
    }

    // Update the behaviors array with the new behaviors
    pBehaviors->m_Value = behaviors;
  }
};

ezParticleBehaviorFactory_Velocity_1_2 g_ezParticleBehaviorFactory_Velocity_1_2;
