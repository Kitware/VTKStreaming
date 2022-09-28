/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNV12RenderFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//VTK::System::Dec
//VTK::Output::Dec
in vec2 texCoord;

uniform sampler2D nv12Texture;
uniform int resolution[2];
uniform int strides[3];

//VTK::NV12::Decl

void main()
{
  //VTK::FLIPY::Impl

  ivec2 pixelCoord = ivec2(gl_FragCoord.x - 0.5, yCoord);
  
  int a = (pixelCoord.x >> 1) << 1; // nearest even number lesser than l.h.s argument for '+'
  int b = ((resolution[1] + 1) >> 1) << 1; // nearest even number greater than l.h.s argument for '+'
  int c = pixelCoord.y >> 1;

  // luminance.
  ivec2 lumaOfst;
  lumaOfst.x = pixelCoord.x;
  lumaOfst.y = pixelCoord.y;
  float luma = texelFetch(nv12Texture, lumaOfst, 0).r;

  // chroma blue.
  ivec2 uOffset = ivec2(a, b + c);
  float u = texelFetch(nv12Texture, uOffset, 0).r;

  // chroma red.
  ivec2 vOffset = ivec2(a + 1, b + c);
  float v = texelFetch(nv12Texture, vOffset, 0).r;

  const mat3 YCbCrToRGBmatrix = mat3(
  1.1643835616, 0.0000000000, 1.7927410714,
  1.1643835616, -0.2132486143, -0.5329093286,
  1.1643835616, 2.1124017857, 0.0000000000
  );
  const vec3 YCbCrToRGBzero = vec3(-0.972945075, 0.301482665, -1.133402218);
  vec3 RGBFullRange = vec3(luma, u, v) * YCbCrToRGBmatrix + YCbCrToRGBzero;
  vec3 RGBFullRangeClamped = clamp(RGBFullRange, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0));
  gl_FragData[0] = vec4(RGBFullRangeClamped, 1);

};
