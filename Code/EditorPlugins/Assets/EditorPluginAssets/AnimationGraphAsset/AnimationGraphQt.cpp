#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>

ezQtAnimationGraphNode::ezQtAnimationGraphNode() = default;

void ezQtAnimationGraphNode::UpdateState()
{
  ezQtNode::UpdateState();

  ezStringBuilder sTitle;

  const ezRTTI* pRtti = GetObject()->GetType();

  ezVariant customTitle = GetObject()->GetTypeAccessor().GetValue("CustomTitle");
  if (customTitle.IsValid() && customTitle.CanConvertTo<ezString>())
  {
    sTitle = customTitle.ConvertTo<ezString>();
  }

  if (sTitle.IsEmpty())
  {
    if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
    {
      sTitle = pAttr->GetTitle();

      ezStringBuilder tmp, tmp2;
      ezVariant val;

      // replace enum properties with translated strings
      {
        ezHybridArray<const ezAbstractProperty*, 32> properties;
        pRtti->GetAllProperties(properties);

        for (const auto& prop : properties)
        {
          if (prop->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
          {
            val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

            ezReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<ezInt64>(), tmp);

            tmp2.Set("{", prop->GetPropertyName(), "}");
            sTitle.ReplaceAll(tmp2, ezTranslate(tmp));
          }
        }
      }

      // replace the rest
      while (true)
      {
        const char* szOpen = sTitle.FindSubString("{");

        if (szOpen == nullptr)
          break;

        const char* szClose = sTitle.FindSubString("}", szOpen);

        if (szClose == nullptr)
          break;

        tmp.SetSubString_FromTo(szOpen + 1, szClose);

        // three array indices should be enough for everyone
        if (tmp.TrimWordEnd("[0]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 0);
        else if (tmp.TrimWordEnd("[1]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 1);
        else if (tmp.TrimWordEnd("[2]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 2);
        else
          val = GetObject()->GetTypeAccessor().GetValue(tmp);

        if (val.IsValid())
        {

          tmp.SetFormat("{}", val);

          if (ezConversionUtils::IsStringUuid(tmp))
          {
            if (auto pAsset = ezAssetCurator::GetSingleton()->FindSubAsset(tmp))
            {
              tmp = pAsset->GetName();
            }
          }

          sTitle.ReplaceSubString(szOpen, szClose + 1, tmp);
        }
        else
        {
          sTitle.ReplaceSubString(szOpen, szClose + 1, "");
        }
      }
    }
  }

  sTitle.ReplaceAll("''", "");
  sTitle.ReplaceAll("\"\"", "");
  sTitle.ReplaceAll("  ", " ");
  sTitle.Trim(" ");

  if (sTitle.GetCharacterCount() > 30)
  {
    sTitle.Shrink(0, sTitle.GetCharacterCount() - 31);
    sTitle.Append("...");
  }

  if (!sTitle.IsEmpty())
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());
  }
}
