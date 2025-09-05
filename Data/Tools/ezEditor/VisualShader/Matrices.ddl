Node %FromCameraSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }
  string %Docs { "Transforms a coordinate from camera-space to another space." }

  InputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(CameraToScreenMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(CameraToWorldMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromScreenSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }
  string %Docs { "Transforms a coordinate from screen-space to another space." }

  InputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in screen-space." }
  }

  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(ScreenToCameraMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(ScreenToWorldMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }
}

Node %FromWorldSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }
  string %Docs { "Transforms a coordinate from world-space to another space." }

  InputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position in world-space." }
  }

  OutputPin %ObjectSpacePos
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(ToFloat4Position($in0), TransformToMatrix(GetInstanceData().ObjectToWorld))" }
    string %Tooltip { "Transformed position in object-space." }
  }  

  OutputPin %ObjectSpaceDir
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "mul(ToFloat3($in0), TransformToRotation(GetInstanceData().ObjectToWorld))" }
    string %Tooltip { "Transformed direction vector in object-space." }
  }
  OutputPin %CameraSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(WorldToCameraMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in camera-space." }
  }

  OutputPin %ScreenSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(WorldToScreenMatrix, ToFloat3($in0))" }
    string %Tooltip { "Transformed position in screen-space." }
  }
}

Node %FromObjectSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }
  string %Docs { "Transforms a coordinate from object-space to another space." }

  InputPin %ObjectSpace
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Tooltip { "Position or direction vector in object-space." }
  }

  OutputPin %WorldPosition
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "mul(TransformToMatrix(GetInstanceData().ObjectToWorld), ToFloat4Position($in0))" }
    string %Tooltip { "Transformed position in world-space." }
  }

  OutputPin %WorldDirection
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "mul(TransformToRotation(GetInstanceData().ObjectToWorld), ToFloat3($in0))" }
    string %Tooltip { "Transformed direction vector in world-space." }
  }
}

Node %TangentToWorldSpace
{
  string %Category { "Transformations" }
  string %Color { "Indigo" }
  string %Docs { "Transforms a normal from trangent-space to world-space." }

  InputPin %TangentSpace
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Tooltip { "Normal in tangent space." }
  }

  OutputPin %WorldSpace
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "TangentToWorldSpace(ToFloat3($in0))" }
    string %Tooltip { "Transformed normal in world-space." }
  }
}


