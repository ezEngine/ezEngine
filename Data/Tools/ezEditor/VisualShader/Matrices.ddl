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

  OutputPin %ObjectSpacePos
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "mul(ToFloat4Position($in0), TransformToMatrix(GetInstanceData().ObjectToWorld))" }
    string %Tooltip { "Transformed position in object-space." }
  }  

  OutputPin %ObjectSpaceDir
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %Inline { "mul(ToFloat3($in0), TransformToRotation(GetInstanceData().ObjectToWorld))" }
    string %Tooltip { "Transformed direction vector in object-space." }
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

Node %FromObjectSpace
{
  string %Category { "Transformations" }
  unsigned_int8 %Color { 38, 105, 0 }

  InputPin %ObjectSpace
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Tooltip { "Position or direction vector in object-space." }
  }

  OutputPin %WorldPosition
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 38, 105, 0 }
    string %Inline { "mul(TransformToMatrix(GetInstanceData().ObjectToWorld), ToFloat4Position($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }

  OutputPin %WorldDirection
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %Inline { "mul(TransformToRotation(GetInstanceData().ObjectToWorld), ToFloat3($in0))" }
    string %Tooltip { "Transformed direction vector in world-space." }
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
    string %Inline { "TangentToWorldSpace(ToFloat3($in0))" }
    string %Tooltip { "Transformed normal in world-space." }
  }
}


