#pragma once
#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct ezVulkanDescriptorSetLayoutBinding
{
  enum ResourceType : ezUInt8
  {
    ConstantBuffer,
    ResourceView,
    UAV,
    Sampler,
  };

  EZ_DECLARE_POD_TYPE();
  ezStringView m_sName;                                                ///< Used to match the same descriptor use across multiple stages.
  ezUInt8 m_uiBinding = 0;                                             ///< Target descriptor binding slot.
  ezUInt8 m_uiVirtualBinding = 0;                                      ///< Virtual binding slot in the high level renderer interface.
  ezShaderResourceType::Enum m_ezType = ezShaderResourceType::Unknown; ///< EZ shader resource type, needed to find compatible fallback resources.
  ResourceType m_Type = ResourceType::ConstantBuffer;                  ///< Resource type, used to map to the correct EZ resource type.
  ezUInt16 m_uiDescriptorType = 0;                                     ///< Maps to vk::DescriptorType
  ezUInt32 m_uiDescriptorCount = 1;                                    ///< For now, this must be 1 as EZ does not support descriptor arrays right now.
  ezUInt32 m_uiWordOffset = 0;                                         ///< Offset of the location in the spirv code where the binding index is located to allow changing it at runtime.
};

struct ezVulkanDescriptorSetLayout
{
  ezUInt32 m_uiSet = 0;
  ezHybridArray<ezVulkanDescriptorSetLayoutBinding, 6> bindings;
};

struct ezVulkanVertexInputAttribute
{
  ezGALVertexAttributeSemantic::Enum m_eSemantic = ezGALVertexAttributeSemantic::Position;
  ezUInt8 m_uiLocation = 0;
  ezGALResourceFormat::Enum m_eFormat = ezGALResourceFormat::XYZFloat;
};

namespace ezSpirvMetaData
{
  constexpr ezUInt32 s_uiSpirvMetaDataMagicNumber = 0x4B565A45; //EZVK

  enum MetaDataVersion
  {
    Version1 = 1,
    Version2 = 2, ///< m_uiVirtualBinding, m_ezType added
    Version3 = 3, ///< Vertex input binding
  };

  void Write(ezStreamWriter& stream, const ezArrayPtr<ezUInt8>& shaderCode, const ezDynamicArray<ezVulkanDescriptorSetLayout>& sets, const ezDynamicArray<ezVulkanVertexInputAttribute>& vertexInputAttributes)
  {
    stream << s_uiSpirvMetaDataMagicNumber;
    stream.WriteVersion(MetaDataVersion::Version3);
    const ezUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const ezUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (ezUInt8 i = 0; i < uiSets; i++)
    {
      const ezVulkanDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;
      const ezUInt8 uiBindings = set.bindings.GetCount();
      stream << uiBindings;
      for (ezUInt8 j = 0; j < uiBindings; j++)
      {
        const ezVulkanDescriptorSetLayoutBinding& binding = set.bindings[j];
        stream.WriteString(binding.m_sName).AssertSuccess();
        stream << binding.m_uiBinding;
        stream << binding.m_uiVirtualBinding;
        stream << static_cast<ezUInt8>(binding.m_ezType);
        stream << static_cast<ezUInt8>(binding.m_Type);
        stream << binding.m_uiDescriptorType;
        stream << binding.m_uiDescriptorCount;
        stream << binding.m_uiWordOffset;
      }
    }

    const ezUInt8 uiVIA = vertexInputAttributes.GetCount();
    stream << uiVIA;
    for (ezUInt8 i = 0; i < uiVIA; i++)
    {
      const ezVulkanVertexInputAttribute& via = vertexInputAttributes[i];
      stream << static_cast<ezUInt8>(via.m_eSemantic);
      stream << via.m_uiLocation;
      stream << static_cast<ezUInt8>(via.m_eFormat);
    }
  }

  /// \brief Reads Vulkan shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  void Read(const ezArrayPtr<const ezUInt8> data, ezArrayPtr<const ezUInt8>& out_shaderCode, ezDynamicArray<ezVulkanDescriptorSetLayout>& out_sets, ezDynamicArray<ezVulkanVertexInputAttribute>& out_vertexInputAttributes)
  {
    ezRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    ezUInt32 uiMagicNumber;
    stream >> uiMagicNumber;
    EZ_ASSERT_DEV(uiMagicNumber == s_uiSpirvMetaDataMagicNumber, "Vulkan shader does not start with s_uiSpirvMetaDataMagicNumber");
    ezTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::Version3);

    ezUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = ezArrayPtr<const ezUInt8>(&data[(ezUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    ezUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (ezUInt8 i = 0; i < uiSets; i++)
    {
      ezVulkanDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;
      ezUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.bindings.Reserve(uiBindings);

      for (ezUInt8 j = 0; j < uiBindings; j++)
      {
        ezVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();

        ezUInt32 uiStringElements = 0;
        stream >> uiStringElements;
        binding.m_sName = ezStringView(reinterpret_cast<const char*>(&data[(ezUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);
        stream >> binding.m_uiBinding;
        if (uiVersion >= MetaDataVersion::Version2)
        {
          stream >> binding.m_uiVirtualBinding;
          stream >> reinterpret_cast<ezUInt8&>(binding.m_ezType);
        }
        else
        {
          binding.m_uiVirtualBinding = binding.m_uiBinding;
          binding.m_ezType = ezShaderResourceType::Texture2D;
        }
        stream >> reinterpret_cast<ezUInt8&>(binding.m_Type);
        stream >> binding.m_uiDescriptorType;
        stream >> binding.m_uiDescriptorCount;
        stream >> binding.m_uiWordOffset;
      }
    }

    if (uiVersion >= MetaDataVersion::Version3)
    {
      ezUInt8 uiVIA = 0;
      stream >> uiVIA;
      out_vertexInputAttributes.Reserve(uiVIA);
      for (ezUInt8 i = 0; i < uiVIA; i++)
      {
        ezVulkanVertexInputAttribute& via = out_vertexInputAttributes.ExpandAndGetRef();
        stream >> reinterpret_cast<ezUInt8&>(via.m_eSemantic);
        stream >> via.m_uiLocation;
        stream >> reinterpret_cast<ezUInt8&>(via.m_eFormat);
      }
    }
  }
} // namespace ezSpirvMetaData
