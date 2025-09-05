Node %PointSampler
{
  string %Category { "Texturing/Samplers" }
  string %Color { "Cyan" }
  string %Docs { "Nearest-neighbor sampler for texture sampling." }

  string %CodePixelSamplers { "" }

  OutputPin %Sampler
  {
    string %Type { "sampler" }
    string %Color { "Cyan" }
    string %Inline { "PointSampler" }
  }
}

Node %LinearSampler
{
  string %Category { "Texturing/Samplers" }
  string %Color { "Cyan" }
  string %Docs { "Bilinear sampler for texture sampling." }

  string %CodePixelSamplers { "" }

  OutputPin %Sampler
  {
    string %Type { "sampler" }
    string %Color { "Cyan" }
    string %Inline { "LinearSampler" }
  }
}

Node %PointClampSampler
{
  string %Category { "Texturing/Samplers" }
  string %Color { "Cyan" }
  string %Docs { "Nearest-neighbor sampler with UV clamping for texture sampling." }

  string %CodePixelSamplers { "" }

  OutputPin %Sampler
  {
    string %Type { "sampler" }
    string %Color { "Cyan" }
    string %Inline { "PointClampSampler" }
  }
}

Node %LinearClampSampler
{
  string %Category { "Texturing/Samplers" }
  string %Color { "Cyan" }
  string %Docs { "Bilinear sampler with UV clamping for texture sampling." }

  string %CodePixelSamplers { "" }

  OutputPin %Sampler
  {
    string %Type { "sampler" }
    string %Color { "Cyan" }
    string %Inline { "LinearClampSampler" }
  }
}
