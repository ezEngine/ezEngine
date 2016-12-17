Node %FromCameraSpace
{
  string %Category { "Transformations" }
  unsigned_int8 %Color { 38, 105, 0 }

  InputPin %CameraSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(CameraToScreenMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(CameraToWorldMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromScreenSpace
{
  string %Category { "Transformations" }
  unsigned_int8 %Color { 38, 105, 0 }

  InputPin %ScreenSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Position in screen-space." }
  }

  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(ScreenToCameraMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(ScreenToWorldMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromWorldSpace
{
  string %Category { "Transformations" }
  unsigned_int8 %Color { 38, 105, 0 }

  InputPin %WorldSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Position in world-space." }
  }

  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(WorldToCameraMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(WorldToScreenMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }
}

Node %TangentToWorldSpace
{
  string %Category { "Transformations" }
  unsigned_int8 %Color { 38, 105, 0 }

  InputPin %TangentSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %Tooltip { "Normal in tangent space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %Inline { "TangentToWorldSpace(ToFloat3($in0), Input)" }
    string %Tooltip { "Transformed normal in world-space." }
  }
}


