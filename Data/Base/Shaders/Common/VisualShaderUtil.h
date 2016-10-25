/// ToFloat1

float ToFloat1(float val)
{
  return val;
}

float ToFloat1(float2 val)
{
  return val.x;
}

float ToFloat1(float3 val)
{
  return val.x;
}

float ToFloat1(float4 val)
{
  return val.x;
}


/// ToFloat2

float2 ToFloat2(float val)
{
  return float2(val, 0);
}

float2 ToFloat2(float2 val)
{
  return val;
}

float2 ToFloat2(float3 val)
{
  return val.xy;
}

float2 ToFloat2(float4 val)
{
  return val.xy;
}

/// ToFloat3

float3 ToFloat3(float val)
{
  return float3(val, 0, 0);
}

float3 ToFloat3(float2 val)
{
  return float3(val.x, val.y, 0);
}

float3 ToFloat3(float3 val)
{
  return val;
}

float3 ToFloat3(float4 val)
{
  return val.xyz;
}

/// ToFloat4

float4 ToFloat4(float val)
{
  return float4(val, 0, 0, 0);
}

float4 ToFloat4(float2 val)
{
  return float4(val.x, val.y, 0, 0);
}

float4 ToFloat4(float3 val)
{
  return float4(val.x, val.y, val.z, 0);
}

float4 ToFloat4(float4 val)
{
  return val;
}


/// ToColor3

float3 ToColor3(float val)
{
  return float3(val, val, val);
}

float3 ToColor3(float2 val)
{
  return float3(val.x, val.y, 0);
}

float3 ToColor3(float3 val)
{
  return val;
}

float3 ToColor3(float4 val)
{
  return val.xyz;
}


/// ToColor4

float4 ToColor4(float val)
{
  return float4(val, val, val, 1);
}

float4 ToColor4(float2 val)
{
  return float4(val.x, val.y, 0, 1);
}

float4 ToColor4(float3 val)
{
  return float4(val.x, val.y, val.z, 1);
}

float4 ToColor4(float4 val)
{
  return val;
}
