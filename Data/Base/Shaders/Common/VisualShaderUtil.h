#pragma once

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

// ToSameType returns the first value packed into a vector of the same type as the second parameter
// Use this if you want to enforce use of the type of a certain variable
// Additional components are discarded, missing components are zero extended

// ToSameType float1, x

float ToSameType(float value, float type)
{
  return value;
}

float2 ToSameType(float value, float2 type)
{
  return float2(value, value);
}

float3 ToSameType(float value, float3 type)
{
  return float3(value, value, value);
}

float4 ToSameType(float value, float4 type)
{
  return float4(value, value, value, value);
}

// ToSameType float2, x

float ToSameType(float2 value, float type)
{
  return value.x;
}

float2 ToSameType(float2 value, float2 type)
{
  return value;
}

float3 ToSameType(float2 value, float3 type)
{
  return float3(value.x, value.y, 0);
}

float4 ToSameType(float2 value, float4 type)
{
  return float4(value.x, value.y, 0, 0);
}

// ToSameType float3, x

float ToSameType(float3 value, float type)
{
  return value.x;
}

float2 ToSameType(float3 value, float2 type)
{
  return value.xy;
}

float3 ToSameType(float3 value, float3 type)
{
  return value;
}

float4 ToSameType(float3 value, float4 type)
{
  return float4(value.x, value.y, value.z, 0);
}

// ToSameType float4, x

float ToSameType(float4 value, float type)
{
  return value.x;
}

float2 ToSameType(float4 value, float2 type)
{
  return value.xy;
}

float3 ToSameType(float4 value, float3 type)
{
  return value.xyz;
}

float4 ToSameType(float4 value, float4 type)
{
  return value;
}

