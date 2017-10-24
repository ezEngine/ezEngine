#include <PCH.h>
#include <GameEngine/Components/MaterialAnimComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>

EZ_BEGIN_COMPONENT_TYPE(ezMaterialAnimComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new ezAssetBrowserAttribute("PropertyAnim")),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMaterialAnimComponent::ezMaterialAnimComponent()
{
}

void ezMaterialAnimComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPropertyAnim;
  s << m_hMaterial;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void ezMaterialAnimComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPropertyAnim;
  s >> m_hMaterial;
}

void ezMaterialAnimComponent::SetPropertyAnimFile(const char* szFile)
{
  ezPropertyAnimResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPropertyAnimResource>(szFile);
  }

  SetPropertyAnim(hResource);
}

const char* ezMaterialAnimComponent::GetPropertyAnimFile() const
{
  if (!m_hPropertyAnim.IsValid())
    return "";

  return m_hPropertyAnim.GetResourceID();
}

void ezMaterialAnimComponent::SetPropertyAnim(const ezPropertyAnimResourceHandle& hMaterialAnim)
{
  m_hPropertyAnim = hMaterialAnim;
}


void ezMaterialAnimComponent::SetMaterialFile(const char* szFile)
{
  ezMaterialResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }

  SetMaterial(hResource);
}


const char* ezMaterialAnimComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}


void ezMaterialAnimComponent::SetMaterial(const ezMaterialResourceHandle& hResource)
{
  m_hMaterial = hResource;
}

void ezMaterialAnimComponent::Update()
{
  if (!m_hPropertyAnim.IsValid() || !m_hMaterial.IsValid())
    return;

  ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  m_CurAnimTime += tDiff;

  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial);

  if (pMaterial->IsMissingResource())
    return;

  ezResourceLock<ezPropertyAnimResource> pAnim(m_hPropertyAnim);

  if (pAnim->IsMissingResource())
    return;

  /// \todo IsFallbackResource ...
  //if (pMaterial->IsFallbackResource())
  //  return;

  auto pDesc = pAnim->GetDescriptor();

  for (ezUInt32 i = 0; i < pDesc->m_Animations.GetCount(); ++i)
  {
    const auto& anim = pDesc->m_Animations[i];

    switch (anim.m_Target)
    {
    case ezPropertyAnimTarget::Color:
      {
        ezResourceLock<ezColorGradientResource> pColor(anim.m_hColorCurve);

        ezColor color;
        pColor->GetDescriptor().m_Gradient.Evaluate((float)m_CurAnimTime.GetSeconds(), color);

        pMaterial->SetParameter(anim.m_sPropertyName.GetData(), color);
      }
      break;

    case ezPropertyAnimTarget::Number:
      {
        ezResourceLock<ezCurve1DResource> pCurve(anim.m_hNumberCurve);

        const double value = pCurve->GetDescriptor().m_Curves[0].Evaluate(m_CurAnimTime.GetSeconds());

        pMaterial->SetParameter(anim.m_sPropertyName.GetData(), value);
      }
      break;

    case ezPropertyAnimTarget::VectorX:
    case ezPropertyAnimTarget::VectorY:
    case ezPropertyAnimTarget::VectorZ:
    case ezPropertyAnimTarget::VectorW:
      {
        // OMG combinatorial EXPLOSION! aaaaahhh! (aka I need a utility function for this)
      }
      break;
    }
  }
}


