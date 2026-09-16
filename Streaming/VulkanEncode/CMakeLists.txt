set(classes
  vtkVulkanEncoder)
set(private_classes
  vtkVulkanEncoderInternals
  vtkVulkanFunctions
  vtkVulkanGLInterop
  vtkVulkanVideoDevice)
# Vendored from Vulkan-Headers 1.4.357; vulkan_core.h pulls in every codec header.
set(private_headers
  vulkan/vulkan_core.h
  vulkan/vk_platform.h
  vk_video/vulkan_video_codecs_common.h
  vk_video/vulkan_video_codec_h264std.h
  vk_video/vulkan_video_codec_h264std_decode.h
  vk_video/vulkan_video_codec_h264std_encode.h
  vk_video/vulkan_video_codec_h265std.h
  vk_video/vulkan_video_codec_h265std_decode.h
  vk_video/vulkan_video_codec_h265std_encode.h
  vk_video/vulkan_video_codec_av1std.h
  vk_video/vulkan_video_codec_av1std_decode.h
  vk_video/vulkan_video_codec_av1std_encode.h
  vk_video/vulkan_video_codec_vp9std.h
  vk_video/vulkan_video_codec_vp9std_decode.h
  vtkVulkanDynamicLoader.h
  # SPIR-V generated from shaders/*.comp by shaders/compile.sh.
  vtkVulkanRGBAToNV12CS.h)

vtk_module_add_module(VTKStreaming::VulkanEncode
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})