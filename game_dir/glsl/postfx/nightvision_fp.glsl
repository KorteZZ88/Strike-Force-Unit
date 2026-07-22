/* Classic image-intensifier night vision. */
#include "const.h"

uniform sampler2D u_ScreenMap;
uniform sampler2D u_DepthMap;
uniform float u_RealTime;
uniform float u_zFar;

varying vec2 var_TexCoord;

float Hash(vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	vec3 source = texture(u_ScreenMap, var_TexCoord).rgb;
	float depth = texture(u_DepthMap, var_TexCoord).r;
	const float zNear = 4.0;
	float viewDistance = (zNear * u_zFar) /
		max(u_zFar - depth * (u_zFar - zNear), 0.001);
	const float range = 50.0 * 39.37;
	float inRange = 1.0 - smoothstep(range * 0.90, range, viewDistance);

	float luminance = dot(source, vec3(0.2126, 0.7152, 0.0722));
	// Lift black detail strongly while allowing bright lamps to burn out.
	float intensified = 1.0 - exp(-luminance * 4.5);
	intensified += smoothstep(0.72, 1.2, luminance) * 0.65;

	float line = sin((var_TexCoord.y * 900.0) + u_RealTime * 7.0) * 0.025;
	float interference = step(0.982, Hash(vec2(floor(var_TexCoord.y * 260.0), floor(u_RealTime * 18.0))));
	float grain = (Hash(var_TexCoord * vec2(1280.0, 720.0) + u_RealTime) - 0.5) * 0.055;
	float signal = intensified + line + grain + interference * 0.10;

	vec2 centered = var_TexCoord * 2.0 - 1.0;
	float vignette = 1.0 - smoothstep(0.48, 1.25, dot(centered, centered));
	vec3 nightColor = vec3(signal * 0.08, signal * 1.15, signal * 0.16) * vignette;

	gl_FragColor = vec4(mix(source, nightColor, inRange), 1.0);
}
