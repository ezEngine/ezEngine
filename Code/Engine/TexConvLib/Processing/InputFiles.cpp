#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezResult ezTexConvProcessor::LoadInputImages()
{
  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    ezLog::Error("No input images have been specified.");
    return EZ_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    ezLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return EZ_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
    return EZ_SUCCESS;

  m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

  for (const auto& file : m_Descriptor.m_InputFiles)
  {
    auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
    if (img.LoadFrom(file).Failed())
    {
      ezLog::Error("Could not load input file '{0}'.", file);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}
