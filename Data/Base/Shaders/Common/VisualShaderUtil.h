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
  return float2(val, val);
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
  return float3(val, val, val);
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
  return float4(val, val, val, val);
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


/// ToColor3 (identical to ToFloat3)

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


/// ToColor4 (ensures alpha is 1, when not available)

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

// ToBiggerType float1, x

float ToBiggerType(float a, float b)
{
  return a;
}

float2 ToBiggerType(float a, float2 b)
{
  return float2(a, a);
}

float3 ToBiggerType(float a, float3 b)
{
  return float3(a, a, a);
}

float4 ToBiggerType(float a, float4 b)
{
  return float4(a, a, a, a);
}

// ToBiggerType float2, x

float2 ToBiggerType(float2 a, float b)
{
  return a;
}

float2 ToBiggerType(float2 a, float2 b)
{
  return a;
}

float3 ToBiggerType(float2 a, float3 b)
{
  return float3(a.x, a.y, 0);
}

float4 ToBiggerType(float2 a, float4 b)
{
  return float4(a.x, a.y, 0, 0);
}

// ToBiggerType float3, x

float3 ToBiggerType(float3 a, float b)
{
  return a;
}

float3 ToBiggerType(float3 a, float2 b)
{
  return a;
}

float3 ToBiggerType(float3 a, float3 b)
{
  return a;
}

float4 ToBiggerType(float3 a, float4 b)
{
  return float4(a.x, a.y, a.z, 0);
}

// ToBiggerType float4, x

float4 ToBiggerType(float4 a, float b)
{
  return a;
}

float4 ToBiggerType(float4 a, float2 b)
{
  return a;
}

float4 ToBiggerType(float4 a, float3 b)
{
  return a;
}

float4 ToBiggerType(float4 a, float4 b)
{
  return a;
}

