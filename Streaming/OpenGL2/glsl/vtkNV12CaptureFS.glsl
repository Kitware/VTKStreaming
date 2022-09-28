/*=========================================================================

  Program:   Visualization Toolkit
  Module:   vtkNV12CaptureFS.glsl

  Copyright (c) 2022 Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//VTK::System::Dec
//VTK::Output::Dec

in vec2 texCoord;

uniform sampler2D rgba32Texture;
uniform int resolution[2];

/**
 * Given an i,j index, does it map into the luma memory block?
 */
bool isLumaIndex(ivec2 index)
{
  return index.y < resolution[1];
}

/**
 * Given an i,j index, does it map into the chroma blue memory block?
 */
bool isChromaBlueIndex(ivec2 index)
{
  return index.y >= resolution[1] && ((index.x & 1) == 0);
}

/**
 * Given an i,j index, does it map into the chroma red memory block?
 */
bool isChromaRedIndex(ivec2 index)
{
  return index.y >= resolution[1] && ((index.x & 1) == 1);
}

/**
 * Convenient function to get RGB (0-255)
 */
vec3 getRGB(ivec2 index)
{
  vec3 rgb;
  rgb.x = texelFetch(rgba32Texture, index, 0).x;
  rgb.y = texelFetch(rgba32Texture, index, 0).y;
  rgb.z = texelFetch(rgba32Texture, index, 0).z;
  return rgb * 255.0f;
}

/**
 * Convenient function to average RGB (0-255) over a block of pixels.
 */
vec3 getDownSampledRGB(ivec2 startIndex, ivec2 endIndex)
{
  vec3 rgb;
  int k = 0;
  for (int i = startIndex.x; i < endIndex.x; ++i)
  {
    for (int j = startIndex.y; j < endIndex.y; ++j)
    {
      ivec2 index = ivec2(i, j);
      rgb += texelFetch(rgba32Texture, index, 0).xyz;
      ++k;
    }
  }
  return rgb * 255.0f / k;
}

void main()
{
  ivec2 dstIndex = ivec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);
  // ITU-R BT.709
  if (isLumaIndex(dstIndex))
  {
    ivec2 srcIndex = dstIndex;
    //VTK::LumaFlipY::Impl
    vec3 rgb = getRGB(srcIndex);
    float r = rgb.x;
    float g = rgb.y;
    float b = rgb.z;
    float luma = r * 0.2126 + g * 0.7152 + b * 0.0722;
    luma /= 255.0f;
    gl_FragData[0] = vec4(luma, 0.0, 0.0, 0.0);
  }
  else if (isChromaBlueIndex(dstIndex))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 start = dstIndex;
    start.y = (dstIndex.y - resolution[1]) << 1;
    //VTK::CrFlipY::Impl

    ivec2 end = start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(start, end);
    vec3 cb_multiplier = vec3(-0.2126, -0.7152, 0.9278) * 224.0f / (219.0f * 1.8556);
    float cb = (dot(rgb, cb_multiplier) + 128.f) / 255.0f;

    gl_FragData[0] = vec4(cb, 0.0, 0.0, 0.0);
  }
  else if (isChromaRedIndex(dstIndex))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 start = dstIndex;
    start.x = dstIndex.x - 1;
    start.y = (dstIndex.y - resolution[1]) << 1;
    //VTK::CbFlipY::Impl

    ivec2 end = start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(start, end);
    vec3 cr_multiplier = vec3(0.7874, -0.7152, -0.0722) * 224.0f / (219.0f * 1.5748);
    float cr = (dot(rgb, cr_multiplier) + 128.0f) / 255.0f;

    gl_FragData[0] = vec4(cr, 0.0, 0.0, 0.0);
  }
}
