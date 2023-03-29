#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompilerDXC/SpirvMetaData.h>
#include <spirv_reflect.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxc/dxcapi.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerDXC, 1, ezRTTIDefaultAllocator<ezShaderCompilerDXC>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
struct ezComPtr
{
public:
  ezComPtr() {}
  ~ezComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  ezComPtr(const ezComPtr& other)
    : m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T* operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** put()
  {
    EZ_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty ezComPtr");
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

ezComPtr<IDxcUtils> s_pDxcUtils;
ezComPtr<IDxcCompiler3> s_pDxcCompiler;

static ezResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode);

static const char* GetProfileName(const char* szPlatform, ezGALShaderStage::Enum Stage)
{
  if (ezStringUtils::IsEqual(szPlatform, "VULKAN"))
  {
    switch (Stage)
    {
      case ezGALShaderStage::VertexShader:
        return "vs_6_0";
      case ezGALShaderStage::HullShader:
        return "hs_6_0";
      case ezGALShaderStage::DomainShader:
        return "ds_6_0";
      case ezGALShaderStage::GeometryShader:
        return "gs_6_0";
      case ezGALShaderStage::PixelShader:
        return "ps_6_0";
      case ezGALShaderStage::ComputeShader:
        return "cs_6_0";
      default:
        break;
    }
  }

  EZ_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", szPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = ezGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"] = ezGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"] = ezGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = ezGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = ezGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = ezGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = ezGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = ezGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = ezGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = ezGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = ezGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = ezGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = ezGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = ezGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = ezGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = ezGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = ezGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = ezGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = ezGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = ezGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = ezGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"] = ezGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = ezGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = ezGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = ezGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = ezGALVertexAttributeSemantic::BoneWeights1;
  }

  if (s_pDxcUtils != nullptr)
    return EZ_SUCCESS;

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));

  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  EZ_SUCCEED_OR_RETURN(Initialize());

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (!inout_Data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      ezLog::Debug("Shader for stage '{}' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    const char* szShaderSource = inout_Data.m_szShaderSource[stage];
    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && ezStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      if (CompileVulkanShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(ezShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (ezGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        EZ_SUCCEED_OR_RETURN(ReflectShaderStage(inout_Data, (ezGALShaderStage::Enum)stage));
      }
      else
      {
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  const char* szCompileSource = szSource;
  ezStringBuilder sDebugSource;

  ezDynamicArray<ezStringWChar> args;
  args.PushBack(ezStringWChar(szFile));
  args.PushBack(L"-E");
  args.PushBack(ezStringWChar(szEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(ezStringWChar(szProfile));
  args.PushBack(L"-spirv");
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(L"-fspv-target-env=vulkan1.1");

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;

    //ezLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  ezComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(szCompileSource, (UINT32)strlen(szCompileSource), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  ezHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (ezUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  ezComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  ezComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    ezLog::Error("Vulkan shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Error("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return EZ_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Warning("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  ezComPtr<IDxcBlob> pShader;
  ezComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    ezLog::Error("No Vulkan bytecode was generated.");
    return EZ_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<ezUInt32>(pShader->GetBufferSize()));

  ezMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<ezUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::FillResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_Type = ezShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info);

    return EZ_SUCCESS;
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_Type = ezShaderResourceType::Sampler;

    // TODO: not sure how this will map to Vulkan
    if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
    {
      ezStringBuilder sb = binding.m_sName.GetString();
      sb.TrimWordEnd("_AutoSampler");
      binding.m_sName.Assign(sb);
    }

    return EZ_SUCCESS;
  }

  ezLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return EZ_FAILURE;
}

ezResult ezShaderCompilerDXC::FillSRVResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct)
    {
      binding.m_Type = ezShaderResourceType::GenericBuffer;
      return EZ_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    switch (info.image.dim)
    {
      case SpvDim::SpvDim1D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = ezShaderResourceType::Texture1DArray;
            return EZ_SUCCESS;
          }
          else
          {
            binding.m_Type = ezShaderResourceType::Texture1D;
            return EZ_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDim2D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = ezShaderResourceType::Texture2DArray;
            return EZ_SUCCESS;
          }
          else
          {
            binding.m_Type = ezShaderResourceType::Texture2D;
            return EZ_SUCCESS;
          }
        }
        else
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = ezShaderResourceType::Texture2DMSArray;
            return EZ_SUCCESS;
          }
          else
          {
            binding.m_Type = ezShaderResourceType::Texture2DMS;
            return EZ_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDim3D:
      {
        if (info.image.ms == 0 && info.image.arrayed == 0)
        {
          binding.m_Type = ezShaderResourceType::Texture3D;
          return EZ_SUCCESS;
        }

        break;
      }

      case SpvDim::SpvDimCube:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed == 0)
          {
            binding.m_Type = ezShaderResourceType::TextureCube;
            return EZ_SUCCESS;
          }
          else
          {
            binding.m_Type = ezShaderResourceType::TextureCubeArray;
            return EZ_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDimBuffer:
        binding.m_Type = ezShaderResourceType::GenericBuffer;
        return EZ_SUCCESS;

      case SpvDim::SpvDimRect:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return EZ_FAILURE;

      case SpvDim::SpvDimSubpassData:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return EZ_FAILURE;

      case SpvDim::SpvDimMax:
        EZ_ASSERT_DEV(false, "Invalid enum value");
        break;
    }

    if (info.image.ms > 0)
    {
      ezLog::Error("Resource '{}': Multi-sampled textures of this type are not supported.", info.name);
      return EZ_FAILURE;
    }

    if (info.image.arrayed > 0)
    {
      ezLog::Error("Resource '{}': Array-textures of this type are not supported.", info.name);
      return EZ_FAILURE;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = ezShaderResourceType::GenericBuffer;
      return EZ_SUCCESS;
    }

    ezLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return EZ_FAILURE;
  }

  ezLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return EZ_FAILURE;
}

ezResult ezShaderCompilerDXC::FillUAVResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_Type = ezShaderResourceType::UAV;
    return EZ_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = ezShaderResourceType::UAV;
      return EZ_SUCCESS;
    }

    ezLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return EZ_FAILURE;
  }

  ezLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return EZ_FAILURE;
}

ezGALResourceFormat::Enum GetEZFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return ezGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return ezGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return ezGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return ezGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return ezGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return ezGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return ezGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return ezGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return ezGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return ezGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return ezGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return ezGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return ezGALResourceFormat::Invalid;
  }
}

ezResult ezShaderCompilerDXC::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  EZ_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& bytecode = inout_Data.m_StageBinary[Stage].GetByteCode();

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    ezLog::Error("Extracting shader reflection information failed.");
    return EZ_FAILURE;
  }

  EZ_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  ezHybridArray<ezVulkanVertexInputAttribute, 8> vertexInputAttributes;
  if (Stage == ezGALShaderStage::VertexShader)
  {
    ezUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve number of input variables.");
      return EZ_FAILURE;
    }
    ezDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve input variables.");
      return EZ_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (ezUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        ezVulkanVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation = static_cast<ezUInt8>(pVar->location);

        ezGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        EZ_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat = GetEZFormat(pVar->format);
        EZ_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }


  // descriptor bindings
  {
    ezUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve number of descriptor bindings.");
      return EZ_FAILURE;
    }

    ezDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve descriptor bindings.");
      return EZ_FAILURE;
    }

    ezMap<ezUInt32, ezUInt32> descriptorToEzBinding;
    ezUInt32 uiVirtualResourceView = 0;
    ezUInt32 uiVirtualSampler = 0;
    for (ezUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      ezLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      ezShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type = ezShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = info.binding;
      shaderResourceBinding.m_sName.Assign(info.name);

      if (FillResourceBinding(inout_Data.m_StageBinary[Stage], shaderResourceBinding, info).Failed())
        continue;

      // We pretend SRVs and Samplers are mapped per stage and nicely packed so we fit into the DX11-based high level render interface.
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
      {
        shaderResourceBinding.m_iSlot = uiVirtualResourceView;
        uiVirtualResourceView++;
      }
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
      {
        shaderResourceBinding.m_iSlot = uiVirtualSampler;
        uiVirtualSampler++;
      }


      EZ_ASSERT_DEV(shaderResourceBinding.m_Type != ezShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      descriptorToEzBinding[i] = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings().GetCount();
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }

    {
      ezArrayPtr<const ezShaderResourceBinding> ezBindings = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings();
      // Modify meta data
      ezDefaultMemoryStreamStorage storage;
      ezMemoryStreamWriter stream(&storage);

      const ezUInt32 uiCount = vars.GetCount();

      //#TODO_VULKAN Currently hard coded to a single DescriptorSetLayout.
      ezHybridArray<ezVulkanDescriptorSetLayout, 3> sets;
      ezVulkanDescriptorSetLayout& set = sets.ExpandAndGetRef();

      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info = *vars[i];
        EZ_ASSERT_DEV(info.set == 0, "Only a single descriptor set is currently supported.");
        ezVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();
        binding.m_sName = info.name;
        binding.m_uiBinding = static_cast<ezUInt8>(info.binding);
        binding.m_uiVirtualBinding = ezBindings[descriptorToEzBinding[i]].m_iSlot;
        binding.m_ezType = ezBindings[descriptorToEzBinding[i]].m_Type;
        switch (info.resource_type)
        {
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
            binding.m_Type = ezVulkanDescriptorSetLayoutBinding::ResourceType::Sampler;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV:
            binding.m_Type = ezVulkanDescriptorSetLayoutBinding::ResourceType::ConstantBuffer;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV:
            binding.m_Type = ezVulkanDescriptorSetLayoutBinding::ResourceType::ResourceView;
            break;
          default:
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV:
            binding.m_Type = ezVulkanDescriptorSetLayoutBinding::ResourceType::UAV;
            break;
        }
        binding.m_uiDescriptorType = static_cast<ezUInt32>(info.descriptor_type);
        binding.m_uiDescriptorCount = 1;
        for (ezUInt32 uiDim = 0; uiDim < info.array.dims_count; ++uiDim)
        {
          binding.m_uiDescriptorCount *= info.array.dims[uiDim];
        }
        binding.m_uiWordOffset = info.word_offset.binding;
      }
      set.bindings.Sort([](const ezVulkanDescriptorSetLayoutBinding& lhs, const ezVulkanDescriptorSetLayoutBinding& rhs) { return lhs.m_uiBinding < rhs.m_uiBinding; });

      ezSpirvMetaData::Write(stream, bytecode, sets, vertexInputAttributes);

      // Replaced compiled Spirv code with custom ezSpirvMetaData format.
      ezUInt64 uiBytesLeft = storage.GetStorageSize64();
      ezUInt64 uiReadPosition = 0;
      bytecode.Clear();
      bytecode.Reserve((ezUInt32)uiBytesLeft);
      while (uiBytesLeft > 0)
      {
        ezArrayPtr<const ezUInt8> data = storage.GetContiguousMemoryRange(uiReadPosition);
        bytecode.PushBackRange(data);
        uiReadPosition += data.GetCount();
        uiBytesLeft -= data.GetCount();
      }
    }
  }
  return EZ_SUCCESS;
}

ezShaderConstantBufferLayout* ezShaderCompilerDXC::ReflectConstantBufferLayout(ezShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& constantBufferReflection)
{
  const auto& block = constantBufferReflection.block;

  EZ_LOG_BLOCK("Constant Buffer Layout", constantBufferReflection.name);
  ezLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);

  ezShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = block.padded_size;

  for (ezUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    ezShaderConstantBufferLayout::Constant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    ezUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        ezLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    ezUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      EZ_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Bool;
          break;

        default:
          ezLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      // TODO: there doesn't seem to be a way to detect 'unsigned' types

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Int1;
          break;
        case 2:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Int2;
          break;
        case 3:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Int3;
          break;
        case 4:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Int4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Float1;
          break;
        case 2:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Float2;
          break;
        case 3:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Float3;
          break;
        case 4:
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Default;

      const ezUInt32 rows = svd.type_description->traits.numeric.matrix.row_count;
      const ezUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        ezLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        ezLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Struct;
    }

    if (uiFlags != 0)
    {
      ezLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == ezShaderConstantBufferLayout::Constant::Type::Default)
    {
      ezLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      ezLog::Info("{1} {3}[{2}] {0}", constant.m_sName, ezArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      ezLog::Info("{1} {3} {0}", constant.m_sName, ezArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}
