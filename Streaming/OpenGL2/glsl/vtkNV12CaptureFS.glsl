/*=========================================================================

  Library:   VTKStreaming
  Module:    OpenGL2
  File:      vtkNV12CaptureFS.glsl

  Copyright (c) 2022 Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * Description: Shader program that converts RGB(1:1:1) into NV12(4:2:0) suitable
 *  for video encoders. Implements https://www.itu.int/rec/R-REC-BT.709-6-201506-I/en
 *
 * Generates 4:2:0 chroma-subsamples.
 * - 1 rgba pixel contributes to 1 luma value.
 * - 2x2 block of rgba pixels contribute 1U, 1V value.
 *
 * Example: Display wxh = 10x5
 *          Storate wxh = 16x8
 *          Strides     = |16|8|8
 *
 * resolution   = {10, 5}
 * lumaHeight   = 8
 * chromaHeight = 4
 * strides      = {16, 8, 8}
 *
 * Input:
 *
 * + 00 + 01 + 02 + 03 + 04 + 05 + 06 + 07 + 08 + 09 + 10 + 11 + 12 + 13 + 14 + 15 +
 * 0|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 1|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 2|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 3|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 4|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|rgba|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 5|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 6|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 * 7|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|
 *
 * Output:
 * 'x' indicates padding
 * 'y,u,v' indicates color
 *
 * + 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 +10 +11 +12 +13 +14 +15 +
 * | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 * | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 * | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 * | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 * | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 * | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 * | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 * | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 * ------------------end-luma------------------
 * | u | v | u | v | u | v | u | v | u | v | x | x | x | x | x | x |
 * | u | v | u | v | u | v | u | v | u | v | x | x | x | x | x | x |
 * | u | v | u | v | u | v | u | v | u | v | x | x | x | x | x | x |
 * ------------------end-chroma-interleaved-------------------
 */

//VTK::System::Dec
//VTK::Output::Dec

in vec2 texCoord;

// texture with 4-component tuples r,g,b,a
uniform sampler2D rgba32Texture;
// resolution of the RGBA32 texture
uniform int resolution[2];
// number of rows in luma block. (Y)
uniform int lumaHeight;
// overall height of the U,V interleaved contiguous memory block.
uniform int chromaHeight;

/**
 * Given an x,y index, does it map into a padded region?
 */
bool isPaddingIndex(ivec2 idx)
{
  return (idx.y >= resolution[1] && idx.y < lumaHeight) || (idx.x >= resolution[0]);
}

/**
 * Given an x,y index, does it map into the luma memory block?
 */
bool isLumaIndex(ivec2 idx)
{
  return idx.y < resolution[1];
}

/**
 * Given an x,y index, does it map into the chroma blue memory block?
 */
bool isChromaBlueIndex(ivec2 idx)
{
  return idx.y >= lumaHeight && ((idx.x & 1) == 0);
}

/**
 * Given an x,y index, does it map into the chroma red memory block?
 */
bool isChromaRedIndex(ivec2 idx)
{
  return idx.y >= lumaHeight && ((idx.x & 1) == 1);
}

/**
 * Convenient function to get RGB (0-255)
 */
vec3 getRGB(ivec2 idx)
{
  vec3 rgb;
  rgb.x = texelFetch(rgba32Texture, idx, 0).x;
  rgb.y = texelFetch(rgba32Texture, idx, 0).y;
  rgb.z = texelFetch(rgba32Texture, idx, 0).z;
  return rgb * 255.0f;
}

/**
 * Convenient function to average RGB (0-255) over a block of pixels.
 */
vec3 getDownSampledRGB(ivec2 start, ivec2 end)
{
  vec3 rgb;
  int k = 0;
  for (int i = start.x; i < end.x; ++i)
  {
    for (int j = start.y; j < end.y; ++j)
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
  ivec2 id_RGBA = ivec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);

  if (isPaddingIndex(id_RGBA))
  {
    gl_FragData[0] = vec4(0.5, 0, 0, 0);
  }
  else if (isLumaIndex(id_RGBA))
  {
    ivec2 id_NV12 = id_RGBA;
    //VTK::LumaFlipY::Impl
    vec3 rgb = getRGB(id_NV12);
    float r = rgb.x;
    float g = rgb.y;
    float b = rgb.z;
    float luma = r * 0.2126 + g * 0.7152 + b * 0.0722;
    luma /= 255.0f;
    gl_FragData[0] = vec4(luma, 0.0, 0.0, 0.0);
  }
  else if (isChromaBlueIndex(id_RGBA))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 id_NV12_start = id_RGBA;
    id_NV12_start.y = (id_RGBA.y - lumaHeight) << 1;
    //VTK::CrFlipY::Impl

    ivec2 id_NV12_end = id_NV12_start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(id_NV12_start, id_NV12_end);
    vec3 cb_multiplier = vec3(-0.2126, -0.7152, 0.9278) * 224.0f / (219.0f * 1.8556);
    float cb = (dot(rgb, cb_multiplier) + 128.f) / 255.0f;

    gl_FragData[0] = vec4(cb, 0.0, 0.0, 0.0);
  }
  else if (isChromaRedIndex(id_RGBA))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 id_NV12_start = id_RGBA;
    id_NV12_start.x = id_RGBA.x - 1;
    id_NV12_start.y = (id_RGBA.y - lumaHeight) << 1;
    //VTK::CbFlipY::Impl

    ivec2 id_NV12_end = id_NV12_start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(id_NV12_start, id_NV12_end);
    vec3 cr_multiplier = vec3(0.7874, -0.7152, -0.0722) * 224.0f / (219.0f * 1.5748);
    float cr = (dot(rgb, cr_multiplier) + 128.0f) / 255.0f;

    gl_FragData[0] = vec4(cr, 0.0, 0.0, 0.0);
  }
}
