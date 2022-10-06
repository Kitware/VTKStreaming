/*=========================================================================

  Library:   VTKStreaming
  Module:    OpenGL2
  File:      vtkIYUVCaptureFS.glsl

  Copyright (c) 2022 Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * Description: Shader program that renders RGBA(1:1:1:1) into IYUV(4:2:0) suitable
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
 * 'x' indicates padding
 * 'y,u,v' indicates color
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
 *
 *  /|\  +000+001+002+003+004+005+006+007+008+009+010+011+012+013+014+015+
 *   |   | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 *   |   | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 *   |   | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 *   Y   | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 *   |   | y | y | y | y | y | y | y | y | y | y | x | x | x | x | x | x |
 *   |   | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 *   |   | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 *   |   | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
 *  \|/  ------------------------------end-luma---------------------------
 *  /|\  | u | u | u | u | u | x | x | x | u | u | u | u | u | x | x | x |
 *   U   | u | u | u | u | u | x | x | x | x | x | x | x | x | x | x | x |
 *  \|/  ---------------------------end-chroma-blue-----------------------
 *  /|\  | v | v | v | v | v | x | x | x | v | v | v | v | v | x | x | x |
 *   V   | v | v | v | v | v | x | x | x | x | x | x | x | x | x | x | x |
 *  \|/  ---------------------------end-chroma-red------------------------
 */

//VTK::System::Dec
//VTK::Output::Dec

// texture with 4-component tuples r,g,b,a
uniform sampler2D rgba32Texture;
// resolution of the RGBA32 texture
uniform int resolution[2];
// row sizes for the output IYUV texture
uniform int strides[3];
// number of rows in luma block. (Y)
uniform int lumaHeight;
// overall height of the U + V contiguous memory block.
uniform int chromaHeight;

/**
 * Given an x,y output index, does it map into a padded region?
 */
bool isPaddingIndex(ivec2 idx)
{
  int a = resolution[0] >> 1;

  return (idx.y >= resolution[1] && idx.y < lumaHeight)
   || (idx.y < lumaHeight && idx.x >= resolution[0])
   || (idx.y >= lumaHeight && idx.x >= a && idx.x < strides[1])
   || (idx.y >= lumaHeight && idx.x >= a + strides[1]);
}

/**
 * Given an x,y output index, does it map into the luma memory block?
 */
bool isLumaIndex(ivec2 idx)
{
  return idx.x < resolution[0] && idx.y < resolution[1];
}

/**
 * Given an x,y output index, does it map into the chroma blue memory block?
 */
bool isChromaBlueIndex(ivec2 idx)
{
  return (idx.y >= lumaHeight) && idx.y < (lumaHeight + (chromaHeight >> 1));
}

/**
 * Given an x,y output index, does it map into the chroma red memory block?
 */
bool isChromaRedIndex(ivec2 idx)
{
  return idx.y >= (lumaHeight + (chromaHeight >> 1));
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
vec3 getDownSampledRGB(ivec2 begin, ivec2 end)
{
  vec3 rgb = vec3(0, 0, 0);
  int k = 0;
  for (int i = begin.x; i < end.x; ++i)
  {
    for (int j = begin.y; j < end.y; ++j)
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
  ivec2 id_IYUV = ivec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);

  if (isPaddingIndex(id_IYUV))
  {
    gl_FragData[0] = vec4(0.5, 0, 0, 0);
  }
  else if (isLumaIndex(id_IYUV))
  {
    ivec2 id_RGBA = id_IYUV;
    //VTK::LumaFlipY::Impl
    vec3 rgb = getRGB(id_RGBA);
    float r = rgb.x;
    float g = rgb.y;
    float b = rgb.z;
    float luma = r * 0.2126 + g * 0.7152 + b * 0.0722;
    luma /= 255.0f;
    gl_FragData[0] = vec4(luma, 0.0, 0.0, 0.0);
  }
  else if (isChromaBlueIndex(id_IYUV))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 id_RGBA_start = id_IYUV;
    id_RGBA_start.x = (id_RGBA_start.x << 1) % (strides[0]);
    if (id_IYUV.x >= strides[1])
    {
      id_RGBA_start.y = ((id_IYUV.y - lumaHeight) << 2) + 2;
    }
    else
    {
      id_RGBA_start.y = ((id_IYUV.y - lumaHeight) << 2);
    }
    //VTK::CrFlipY::Impl
    ivec2 id_RGBA_end = id_RGBA_start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(id_RGBA_start, id_RGBA_end);
    vec3 cb_multiplier = vec3(-0.2126, -0.7152, 0.9278) * 224.0f / (219.0f * 1.8556);
    float cb = (dot(rgb, cb_multiplier) + 128.f) / 255.0f;

    gl_FragData[0] = vec4(cb, 0.0, 0.0, 0.0);
  }
  else if (isChromaRedIndex(id_IYUV))
  {
    // down sample r,g,b from 2x2 block.
    ivec2 id_RGBA_start = id_IYUV;
    id_RGBA_start.x = (id_RGBA_start.x << 1) % (strides[0]);
    if (id_IYUV.x >= strides[1])
    {
      id_RGBA_start.y = ((id_IYUV.y - lumaHeight - (chromaHeight >> 1)) << 2) + 2;
    }
    else
    {
      id_RGBA_start.y = (id_IYUV.y - lumaHeight - (chromaHeight >> 1)) << 2;
    }
    //VTK::CbFlipY::Impl

    ivec2 id_RGBA_end = id_RGBA_start + ivec2(1, 1);

    vec3 rgb = getDownSampledRGB(id_RGBA_start, id_RGBA_end);
    vec3 cr_multiplier = vec3(0.7874, -0.7152, -0.0722) * 224.0f / (219.0f * 1.5748);
    float cr = (dot(rgb, cr_multiplier) + 128.0f) / 255.0f;

    gl_FragData[0] = vec4(cr, 0.0, 0.0, 0.0);
  }
  else
  {
    // special value to indicate failure. for debugging.
    gl_FragData[0] = vec4(0.8, 0.0, 0.0, 0.0);
  }
}
