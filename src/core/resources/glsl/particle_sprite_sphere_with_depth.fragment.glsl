///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

/***********************************************************************
 * This OpenGL fragment shader renders a shaded particle 
 * on a textured imposter.
 ***********************************************************************/

uniform sampler2D tex;			// The imposter texture.

// Input from vertex shader:
varying float depth_radius;		// The particle radius.
varying float ze0;				// The particle's Z coordinate in eye coordinates.

void main() 
{
	vec2 shifted_coords = gl_TexCoord[0].xy - vec2(0.5, 0.5);
	float rsq = dot(shifted_coords, shifted_coords);
	if(rsq >= 0.25) discard;
	vec4 texValue = texture2D(tex, gl_TexCoord[0].xy);
	
	// Specular highlights are stored in the alpha channel of the texture. 
	// Modulate diffuse color with brightness value stored in the texture.
	gl_FragColor = vec4(texValue.rgb * gl_Color.rgb + texValue.a, 1);

	// Vary the depth value across the imposter to obtain proper intersections between particles.	
	float dz = sqrt(1.0 - 4.0 * rsq) * depth_radius;
	float ze = ze0 + dz;
	float zn = (gl_ProjectionMatrix[2][2] * ze + gl_ProjectionMatrix[3][2]) / (gl_ProjectionMatrix[2][3] * ze + gl_ProjectionMatrix[3][3]);
	gl_FragDepth = 0.5 * (zn * gl_DepthRange.diff + (gl_DepthRange.far + gl_DepthRange.near));
}
