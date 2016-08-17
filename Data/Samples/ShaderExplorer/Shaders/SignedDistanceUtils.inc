#pragma once

#ifndef SCENE_FUNC
#error please define which function defines the scene
#endif

#ifndef SCENE_FUNC_RETTYPE
#error please define the scene function return value type
#endif

#ifndef SCENE_INVALID_VALUE
#error please define the invalid scene value
#endif

SCENE_FUNC_RETTYPE SCENE_FUNC(float3 p);

// Basic geometry

float sdPlaneXZ( float3 p )
{
	return p.y;
}

float sdSphere( float3 p, float radius )
{
    return length(p)-radius;
}

float sdBox( float3 p, float3 extents )
{
  float3 d = abs(p) - extents;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float udRoundBox( float3 p, float3 extends, float camferRadius )
{
  return length(max(abs(p)-extends,0.0))-camferRadius;
}

float sdCapsule( float3 p, float3 start, float3 end, float radius )
{
	float3 pa = p-start, ba = end-start;
	float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
	return length( pa - ba*h ) - radius;
}

float sdCylinder( float3 p, float2 size )
{
  float2 d = abs(float2(length(p.xz),p.y)) - size;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

// Operations on signed distance fields

float opSubtract( float d1, float d2 )
{
    return max(-d2,d1);
}

float opUnion( float d1, float d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

// note y channel is used for additional scene data
float2 opUnion( float2 d1, float2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

// note yz channel is used for additional scene data
float3 opUnion( float3 d1, float3 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

// note yzw channel is used for additional scene data
float4 opUnion( float4 d1, float4 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

float opIntersection( float d1, float d2 )
{
    return max(d1,d2);
}

float2 opIntersection( float2 d1, float2 d2 )
{
    return (d1.x > d2.x) ? d1 : d2;
}

float3 opIntersection( float3 d1, float3 d2 )
{
    return (d1.x > d2.x) ? d1 : d2;
}

float4 opIntersection( float4 d1, float4 d2 )
{
    return (d1.x > d2.x) ? d1 : d2;
}

// General tracing functions
// Returns the hit distance in x and the remaining scene data in yzw
SCENE_FUNC_RETTYPE castRay( float3 ro, float3 rd )
{
    float tmin = 0.2;
    float tmax = 20.0;
    
	float precis = 0.002;
    float t = tmin;
    SCENE_FUNC_RETTYPE m = SCENE_INVALID_VALUE;
    for( int i=0; i<50; i++ )
    {
	    SCENE_FUNC_RETTYPE res = SCENE_FUNC( ro+rd*t );
        if( res.x<precis || t>tmax ) 
			break;
		
		t += max(res.x, 0.001);
	    m = res;
    }

    if( t>tmax ) 
		m = SCENE_INVALID_VALUE;
	m.x = t;
    return m;
}

// Computes the scene normal at a given point
float3 calcNormal( float3 pos )
{
	float3 eps = float3( 0.001, 0, 0 );
	float3 nor = float3(
	    SCENE_FUNC(pos+eps.xyy).x - SCENE_FUNC(pos-eps.xyy).x,
	    SCENE_FUNC(pos+eps.yxy).x - SCENE_FUNC(pos-eps.yxy).x,
	    SCENE_FUNC(pos+eps.yyx).x - SCENE_FUNC(pos-eps.yyx).x );
	return normalize(nor);
}