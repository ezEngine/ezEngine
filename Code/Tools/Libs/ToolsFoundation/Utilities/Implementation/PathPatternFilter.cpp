#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

void ezPathPattern::Configure(const ezStringView text0)
{
  ezStringView text = text0;

  text.Trim(" \t\r\n");

  const bool bStart = text.StartsWith("*");
  const bool bEnd = text.EndsWith("*");

  text.Trim("*");
  m_sString = text;

  if (bStart && bEnd)
    m_MatchType = MatchType::Contains;
  else if (bStart)
    m_MatchType = MatchType::EndsWith;
  else if (bEnd)
    m_MatchType = MatchType::StartsWith;
  else
    m_MatchType = MatchType::Exact;
}

bool ezPathPattern::Matches(const ezStringView text) const
{
  switch (m_MatchType)
  {
    case MatchType::Exact:
      return text.IsEqual_NoCase(m_sString.GetView());
    case MatchType::StartsWith:
      return text.StartsWith_NoCase(m_sString);
    case MatchType::EndsWith:
      return text.EndsWith_NoCase(m_sString);
    case MatchType::Contains:
      return text.FindSubString_NoCase(m_sString) != nullptr;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}

//////////////////////////////////////////////////////////////////////////

bool ezPathPatternFilter::PassesFilters(ezStringView text) const
{
  for (const auto& filter : m_IncludePatterns)
  {
    // if any include pattern matches, that overrides the exclude patterns
    if (filter.Matches(text))
      return true;
  }

  for (const auto& filter : m_ExcludePatterns)
  {
    // no include pattern matched, but any exclude pattern matches -> filter out
    if (filter.Matches(text))
      return false;
  }

  // no filter matches at all -> include by default
  return true;
}

void ezPathPatternFilter::AddFilter(ezStringView sText, bool bIncludeFilter)
{
  ezStringBuilder text = sText;
  text.MakeCleanPath();
  text.Trim(" \t\r\n");

  if (text.IsEmpty() || text.StartsWith("//"))
    return;

  if (!text.StartsWith("*") && !text.StartsWith("/"))
    text.Prepend("/");

  if (bIncludeFilter)
    m_IncludePatterns.ExpandAndGetRef().Configure(text);
  else
    m_ExcludePatterns.ExpandAndGetRef().Configure(text);
}

ezResult ezPathPatternFilter::ReadConfigFile(const char* szFile, const ezDynamicArray<ezString>& preprocessorDefines)
{
  ezStringBuilder content;

  ezPreprocessor pp;
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);

  for (const auto& def : preprocessorDefines)
  {
    pp.AddCustomDefine(def).IgnoreResult();
  }

  // keep comments, because * and / can form a multi-line comment, and then we could lose vital information
  // instead only allow single-line comments and filter those out in AddFilter().
  if (pp.Process(szFile, content, true, true).Failed())
    return EZ_FAILURE;

  ezDynamicArray<ezStringView> lines;

  content.Split(false, lines, "\n", "\r");

  bool bIncludeFilter = false;

  for (auto line : lines)
  {
    if (line.IsEqual_NoCase("[INCLUDE]"))
    {
      bIncludeFilter = true;
      continue;
    }

    if (line.IsEqual_NoCase("[EXCLUDE]"))
    {
      bIncludeFilter = false;
      continue;
    }

    AddFilter(line, bIncludeFilter);
  }

  return EZ_SUCCESS;
}
