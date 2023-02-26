#pragma once

// https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf
// https://www.guerrilla-games.com/read/nubis-realtime-volumetric-cloudscapes-in-a-nutshell
// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
// http://www.diva-portal.org/smash/get/diva2:1223894/FULLTEXT01.pdf

#define PLANET_RADIUS 6371e3 /* radius of the planet */
#define CLOUD_START 1500
#define CLOUD_END 2000

float remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

float cloudCoverage(float2 xy, float coverage)
{
	float4 noise = NoiseMap.Sample(NoiseMap_AutoSampler, float3(xy.x, xy.y, 0.0f));
	float wfbm = noise.x * .625 +
				 noise.y * .125 +
			     noise.z * .25; 
				 
	// cloud shape modeled after the GPU Pro 7 chapter
    float cloud = remap(noise.w, wfbm - 1., 1., 0., 1.);
    cloud = remap(cloud, 1.0f - coverage, 1., 0., 1.); // fake cloud coverage
	
	return cloud;
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
