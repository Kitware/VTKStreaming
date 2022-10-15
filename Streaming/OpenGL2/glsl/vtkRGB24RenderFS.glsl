/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRGB24RenderFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * Description: Shader program that renders RGB24 texture.
 */

//VTK::System::Dec
//VTK::Output::Dec

uniform sampler2D rgb24Texture;
// resolution of the RGB24 texture
uniform int resolution[2];

//VTK::RGB24::Decl

void main()
{
  //VTK::FLIPY::Impl

  ivec2 pixelCoord = ivec2(gl_FragCoord.x - 0.5, yCoord);
  vec3 rgb = texelFetch(rgb24Texture, pixelCoord, 0).xyz;
  gl_FragData[0] = vec4(rgb.xyz,1.0f);
};
