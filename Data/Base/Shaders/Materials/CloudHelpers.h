#pragma once

// https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf
// https://www.guerrilla-games.com/read/nubis-realtime-volumetric-cloudscapes-in-a-nutshell
// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
// http://www.diva-portal.org/smash/get/diva2:1223894/FULLTEXT01.pdf


#define SINGLE_CLOUD

#define PLANET_RADIUS 6371e3 /* radius of the planet */

#ifdef SINGLE_CLOUD,
#define CLOUD_START 10
#define CLOUD_END 20
#else
#define CLOUD_START 600
#define CLOUD_END 800
#endif


float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float HenyeyGreenstein(float cosAngle, float inG)
{
	return ((1.0 - inG * inG) / pow((1.0 + inG * inG - 2.0 * inG * cosAngle), 3.0 / 2.0)) / 4.0 * 3.1415;
}

// Blends a strong forward-scattering lobe with a weaker back-scattering lobe to approximate the
// multi-lobed Mie phase function of water droplets. The forward lobe produces the bright "silver
// lining" rim seen when looking towards the sun through a cloud edge; the back lobe brightens
// clouds when the sun is behind the viewer. This is the dual-lobe phase approximation described in
// the Horizon Zero Dawn cloud rendering talk.
float CloudPhase(float cosAngle, float eccentricityScale)
{
	float forward = HenyeyGreenstein(cosAngle, 0.8 * eccentricityScale);
	float back = HenyeyGreenstein(cosAngle, -0.2 * eccentricityScale);
	return lerp(back, forward, 0.7);
}

// Fraction of the way through the cloud layer, 0 at the bottom and 1 at the top.
float GetHeightFraction(float3 p)
{
	return saturate((p.z - CLOUD_START) / (CLOUD_END - CLOUD_START));
}

// Sunlight in-scatter fades out right at the base of the cloud layer, where the cloud mass above
// blocks most of the light from reaching down - this is why cloud undersides look dark even when
// lit from above. Modeled as a fast ramp from the bottom of the layer rather than tracking real
// occlusion.
float GetVerticalProbability(float heightFraction)
{
	return pow(saturate(remap(heightFraction, 0.07, 0.14, 0.1, 1.0)), 0.8);
}

// Approximates multiple scattering: real light bounces through the cloud many times before
// reaching the eye, which both attenuates less steeply than single-scattering Beer's law predicts
// and makes the phase function more isotropic. Each octave re-evaluates Beer's law and the phase
// function as if seen through a thinner, more isotropic version of the cloud, then adds it with a
// shrinking weight - approximating another bounce of scattered light without tracing it.
float GetMultipleScattering(float densityToSun, float cosAngle)
{
	const int NUM_OCTAVES = 4;
	const float ATTENUATION = 0.5;       // less optical thickness "seen" by each octave
	const float CONTRIBUTION = 0.4;      // less weight given to each octave
	const float PHASE_ATTENUATION = 0.5; // more isotropic phase for each octave

	float a = 1.0;
	float b = 1.0;
	float c = 1.0;
	float energy = 0.0;

	[unroll]
	for (int i = 0; i < NUM_OCTAVES; i++)
	{
		float beersLaw = exp(-densityToSun * a);
		energy += b * beersLaw * CloudPhase(cosAngle, c);

		a *= ATTENUATION;
		b *= CONTRIBUTION;
		c *= PHASE_ATTENUATION;
	}

	return energy;
}

// Combines multiple-scattering, the "powder sugar" edge darkening and the height-based vertical
// probability into the scalar amount of sunlight in-scattered towards the eye at a sample point.
// Beer's law (inside GetMultipleScattering) is already what makes a thin, clear-to-the-sun edge
// bright and a deeply shadowed core dark; the powder term only adds a secondary, bounded dip on
// top of that for visual richness, so it must never pull the result down to zero by itself - if it
// did, every thinly-shadowed edge (where densityToSun is close to zero) would go dark instead of
// staying bright and white. It is also faded out towards the sun direction so it doesn't eat into
// the silver lining it would otherwise darken.
float GetLightEnergy(float densityToSun, float cosAngle, float heightFraction)
{
	float powderSugarEffect = 1.0 - exp(-densityToSun * 2.0);
	powderSugarEffect = lerp(1.0, powderSugarEffect, 0.5);
	powderSugarEffect = lerp(powderSugarEffect, 1.0, saturate(remap(cosAngle, 0.7, 1.0, 0.0, 1.0)));

	float energy = GetMultipleScattering(densityToSun, cosAngle) * powderSugarEffect;

	return energy * GetVerticalProbability(heightFraction);
}

// Isotropic sky/ground light entering the cloud outside of direct sunlight: darker and cooler near
// the base, where the cloud mass above blocks most of the sky, and brighter near the top, which
// faces the open sky. This is the height-based ambient gradient used in the Horizon Zero Dawn
// cloud rendering talk in place of a single flat ambient term. BaseAmbientColor/TopAmbientColor are
// material parameters so they can be tuned per asset or driven by the VolumetricClouds component.
float3 GetAmbientLight(float heightFraction)
{
	return lerp(GetMaterialData(BaseAmbientColor).rgb, GetMaterialData(TopAmbientColor).rgb, heightFraction);
}

// Noise generation functions (by iq)
float noise1D( float n )
{
    return frac(sin(n)*43758.5453);
}

float GetWeatherData(float2 xy)
{
    #if CLOUDS_SOURCE == CLOUDS_SOURCE_TEXTURE
    /*if(xy.x < 0.0f || xy.x > 1.0f)
        return 0.0f;
    if(xy.y < 0.0f || xy.y > 1.0f)
        return 0.0f;*/
    return 1;

    #elif defined(SINGLE_CLOUD)
    float grad = length(xy);
    grad = 1.0f - saturate(grad / 10.0f);
    grad = saturate(grad * 1.5f);
    //return grad * 0.4;
	return 0.2;
    #else
    
	float4 noise = NoiseMap.Sample(NoiseMap_AutoSampler, float3(xy.x, xy.y, 0.0f));
	/*float wfbm = noise.x * .625 +
				 noise.y * .125 +
			     noise.z * .25; 
				 
	// cloud shape modeled after the GPU Pro 7 chapter
    float cloud = remap(noise.w, wfbm - 1., 1., 0., 1.);
    cloud = remap(cloud, 1.0f - coverage, 1., 0., 1.); // fake cloud coverage*/
	
	return remap(noise.x, coverage, 1.0, 0.0, 1.0);
    #endif
}

float HeightProfile(float3 p, float start_height, float end_height, float hardness)
{
    float height = p.z - start_height;
    float grad = saturate(height / (end_height - start_height));
    grad = saturate(1.0f - abs(grad - 0.5f) * 2.0f);
    grad = saturate(grad * hardness);
    
    return grad;
}

float SampleCloudDensity(float3 p, float weatherData)
{
    #if CLOUDS_SOURCE == CLOUDS_SOURCE_TEXTURE
    // The texture stores a normalized [0, 1] density fraction, not a physically calibrated
    // extinction coefficient - used as-is it barely attenuates light over the size of the cloud,
    // which is why the cloud looked grey and see-through instead of properly opaque.
    //
    // SampleLevel with an explicit mip is used instead of Sample: this call sits inside a loop
    // whose per-pixel iteration count varies (the march exits early once it leaves the cloud), so
    // the screen-space derivatives an implicit Sample() would use to pick a mip level are not
    // well-defined here - the shader compiler flags exactly this ("gradient instruction used in a
    // loop with varying iteration") and falls back to an inconsistent mip choice. That showed up as
    // strong per-pixel salt-and-pepper noise, which got much worse at high DensityMultiplier because
    // Beer's law turns small per-pixel density differences into large transmittance differences once
    // density is scaled up. Sampling a slightly coarser, fixed mip also pre-filters the volume a
    // little, which helps since the march's step size is close to a single voxel and can't otherwise
    // resolve the texture's full resolution without aliasing.
    return VolumeMap.SampleLevel(VolumeMap_AutoSampler, (p.xyz)*float3(0.05, 0.05, -0.05), 1).r * HeightProfile(p, CLOUD_START, CLOUD_END, 10.f) * GetMaterialData(DensityMultiplier);
    //return 1.0f;
    #else
    
    //return HeightProfile(p, CLOUD_START, CLOUD_END, 8.0f);

    float4 noise = NoiseMap.Sample(NoiseMap_AutoSampler, p * 0.09);
    
    float low_freq_fbm = (noise.g * 0.625) + (noise.b * 0.25) + (noise.a * 0.125);
    
    float base_cloud = remap(noise.r, low_freq_fbm - 1.0, 1.0, 0.0, 1.0);
    
    base_cloud *= HeightProfile(p, CLOUD_START, CLOUD_END, 1.2f);
    
    float base_cloud_with_coverage = remap(base_cloud, 1.0 - weatherData, 1.0, 0.0, 1.0);
    //base_cloud_with_coverage *= 1.0 - weatherData;
	//float base_cloud_with_coverage = base_cloud * weatherData;
    
    float3 detailNoise = DetailNoiseMap.Sample(DetailNoiseMap_AutoSampler, p * 0.009).rgb;
    
    float detailFbm = (detailNoise.r * 0.625) + (detailNoise.g * 0.25) + (detailNoise.b * 0.125);
    
    //float highFreqMultiplier = lerp(detailFbm, 1.0 - detailFbm, saturate(p.z - CLOUD_START) / (CLOUD_END - CLOUD_START));
    float highFreqMultiplier = 1.0 - detailFbm;
    
    float final_cloud = remap(base_cloud_with_coverage, highFreqMultiplier * 0.2, 1.0, 0.0, 1.0);
    
    return max(base_cloud_with_coverage, 0.0);
    
    //return p.z > CLOUD_START ? 1.0f : 0.0f;
    //return p.z > 1500.0f ? 1.0f : 0.0f;
    #endif
}

float2 ray_sphere_intersect(
    float3 start, // starting position of the ray
    float3 dir, // the direction of the ray
    float radius // and the sphere radius
) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (radius * radius);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return float2(1e5,-1e5);
    return float2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}
