/*=========================================================================

  Library:   VTKStreaming
  Module:    OpenGL2
  File:      vtkRGBA32CaptureFS.glsl

  Copyright (c) 2022 Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * Description: Shader program that renders RGBA32(1:1:1:1) into an upright
 *              or inverted RGBA32(1:1:1:1) destination texture. The alpha
 *              channel may be optionally ignored.
 */

//VTK::System::Dec
//VTK::Output::Dec

// texture with 4-component tuples r,g,b,a
uniform sampler2D rgba32Texture;
// resolution of the RGBA32 texture
uniform int resolution[2];
// flag indicates that we need to ignore alpha channel.
uniform int ignoreAlpha;

/**
 * Given an x,y index, does it map into a padded region?
 */
bool isPaddingIndex(ivec2 idx)
{
  return (idx.x >= resolution[0] || idx.y >= resolution[1]);
}

void main()
{
  ivec2 id_RGBA = ivec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);

  //VTK::RGBA32FlipY::Impl
  if (isPaddingIndex(id_RGBA))
  {
    gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
  }
  else
  {
    float r = texelFetch(rgba32Texture, id_RGBA, 0).x;
    float g = texelFetch(rgba32Texture, id_RGBA, 0).y;
    float b = texelFetch(rgba32Texture, id_RGBA, 0).z;
    float a = texelFetch(rgba32Texture, id_RGBA, 0).w;
    if (ignoreAlpha == 1)
    {
      gl_FragData[0] = vec4(r, g, b, 1.0);
    }
    else
    {
      gl_FragData[0] = vec4(r, g, b, a);
    }
  }
}
