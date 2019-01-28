#include <PCH.h>

#include <TexConv2/TexConv2.h>

static const char* ToString(ezTexConvChannelValue::Enum e)
{
  switch (e)
  {
    case ezTexConvChannelValue::Red:
      return "Red";
    case ezTexConvChannelValue::Green:
      return "Green";
    case ezTexConvChannelValue::Blue:
      return "Blue";
    case ezTexConvChannelValue::Alpha:
      return "Alpha";
    case ezTexConvChannelValue::Black:
      return "Black";
    case ezTexConvChannelValue::White:
      return "White";

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return "";
}

ezResult ezTexConv2::ParseChannelMappings()
{
  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;

  EZ_SUCCEED_OR_RETURN(ParseChannelSliceMapping(-1));

  for (ezUInt32 slice = 0; slice < 64; ++slice)
  {
    const ezUInt32 uiPrevMappings = mappings.GetCount();

    EZ_SUCCEED_OR_RETURN(ParseChannelSliceMapping(slice));

    if (uiPrevMappings == mappings.GetCount())
    {
      // if no new mapping was found, don't try to find more
      break;
    }
  }

  if (!mappings.IsEmpty())
  {
    ezLog::Info("Custom output channel mapping:");
    for (ezUInt32 m = 0; m < mappings.GetCount(); ++m)
    {
      ezLog::Info("Slice {}, R -> Input file {}, {}", m, mappings[m].m_Channel[0].m_iInputImageIndex,
                  ToString(mappings[m].m_Channel[0].m_ChannelValue));
      ezLog::Info("Slice {}, G -> Input file {}, {}", m, mappings[m].m_Channel[1].m_iInputImageIndex,
                  ToString(mappings[m].m_Channel[1].m_ChannelValue));
      ezLog::Info("Slice {}, B -> Input file {}, {}", m, mappings[m].m_Channel[2].m_iInputImageIndex,
                  ToString(mappings[m].m_Channel[2].m_ChannelValue));
      ezLog::Info("Slice {}, A -> Input file {}, {}", m, mappings[m].m_Channel[3].m_iInputImageIndex,
                  ToString(mappings[m].m_Channel[3].m_ChannelValue));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseChannelSliceMapping(ezInt32 iSlice)
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;
  ezStringBuilder tmp, param;

  const ezUInt32 uiMappingIdx = iSlice < 0 ? 0 : iSlice;

  // input to output mappings
  {
    param = "-rgba";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, false));
    }

    param = "-rgb";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
    }

    param = "-rg";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
    }

    param = "-r";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, true));
    }

    param = "-g";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, true));
    }

    param = "-b";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, true));
    }

    param = "-a";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      EZ_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, true));
    }
  }
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseChannelMappingConfig(ezTexConvChannelMapping& out_Mapping, const char* cfg, ezInt32 iChannelIndex,
                                               bool bSingleChannel)
{
  out_Mapping.m_iInputImageIndex = -1;
  out_Mapping.m_ChannelValue = ezTexConvChannelValue::White;

  ezStringBuilder tmp = cfg;

  // '-r black' for setting it to zero
  if (tmp.IsEqual_NoCase("black"))
  {
    out_Mapping.m_ChannelValue = ezTexConvChannelValue::Black;
    return EZ_SUCCESS;
  }

  // 'r white' for setting it to 255
  if (tmp.IsEqual_NoCase("white"))
  {
    out_Mapping.m_ChannelValue = ezTexConvChannelValue::White;
    return EZ_SUCCESS;
  }

  // skip the 'in', if found
  // 'in' is optional, one can also write '-r 1.r' for '-r in1.r'
  if (tmp.StartsWith_NoCase("in"))
    tmp.Shrink(2, 0);

  if (tmp.StartsWith("."))
  {
    // no index given, e.g. '-r in.r'
    // in is equal to in0

    out_Mapping.m_iInputImageIndex = 0;
  }
  else
  {
    ezInt32 num = -1;
    const char* szLastPos = nullptr;
    if (ezConversionUtils::StringToInt(tmp, num, &szLastPos).Failed())
    {
      ezLog::Error("Could not parse channel mapping '{0}'", cfg);
      return EZ_FAILURE;
    }

    // valid index after the 'in'
    if (num >= 0 && num < (ezInt32)m_Processor.m_Descriptor.m_InputFiles.GetCount())
    {
      out_Mapping.m_iInputImageIndex = num;
    }
    else
    {
      ezLog::Error("Invalid channel mapping input file index '{0}'", num);
      return EZ_FAILURE;
    }

    ezStringBuilder dummy = szLastPos;

    // continue after the index
    tmp = dummy;
  }

  // no additional info, e.g. '-g in2' is identical to '-g in2.g' (same channel)
  if (tmp.IsEmpty())
  {
    out_Mapping.m_ChannelValue = (ezTexConvChannelValue::Enum)((ezInt32)ezTexConvChannelValue::Red + iChannelIndex);
    return EZ_SUCCESS;
  }

  if (!tmp.StartsWith("."))
  {
    ezLog::Error("Invalid channel mapping: Expected '.' after input file index in '{0}'", cfg);
    return EZ_FAILURE;
  }

  tmp.Shrink(1, 0);

  if (!bSingleChannel)
  {
    // in case of '-rgb in1.bgr' map r to b, g to g, b to r, etc.
    // in case of '-rgb in1.r' map everything to the same input
    if (tmp.GetCharacterCount() > 1)
      tmp.Shrink(iChannelIndex, 0);
  }

  // no additional info, e.g. '-rgb in2.rg'
  if (tmp.IsEmpty())
  {
    ezLog::Error("Invalid channel mapping: Too few channel identifiers '{0}'", cfg);
    return EZ_FAILURE;
  }

  {
    const ezUInt32 uiChar = tmp.GetIteratorFront().GetCharacter();

    if (uiChar == 'r')
    {
      out_Mapping.m_ChannelValue = ezTexConvChannelValue::Red;
    }
    else if (uiChar == 'g')
    {
      out_Mapping.m_ChannelValue = ezTexConvChannelValue::Green;
    }
    else if (uiChar == 'b')
    {
      out_Mapping.m_ChannelValue = ezTexConvChannelValue::Blue;
    }
    else if (uiChar == 'a')
    {
      out_Mapping.m_ChannelValue = ezTexConvChannelValue::Alpha;
    }
    else
    {
      ezLog::Error("Invalid channel mapping: Unexpected channel identifier in '{}'", cfg);
      return EZ_FAILURE;
    }

    tmp.Shrink(1, 0);
  }

  return EZ_SUCCESS;
}
