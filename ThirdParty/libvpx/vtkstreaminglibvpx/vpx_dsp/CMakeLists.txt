# Commented out lines that begin with `DSP_SRCS-`
# correspond to architecture/asm extension that we don't cater to.
# When one of those lines is also commented upstream, we use a double `##`

set(vpx_dsp_headers)
set(vpx_dsp_c_sources)

set(vpx_dsp_sse2_headers)
set(vpx_dsp_sse2_c_sources)
set(vpx_dsp_sse2_asm_sources)

set(vpx_dsp_ssse3_headers)
set(vpx_dsp_ssse3_c_sources)
set(vpx_dsp_ssse3_asm_sources)

set(vpx_dsp_sse4_1_headers)
set(vpx_dsp_sse4_1_c_sources)
set(vpx_dsp_sse4_1_asm_sources)

set(vpx_dsp_avx2_headers)
set(vpx_dsp_avx2_c_sources)
set(vpx_dsp_avx2_asm_sources)

set(vpx_dsp_avx512_headers)
set(vpx_dsp_avx512_c_sources)
set(vpx_dsp_avx512_asm_sources)

set(vpx_dsp_neon_headers)
set(vpx_dsp_neon_c_sources)
set(vpx_dsp_neon_asm_sources)

# common
list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_dsp_common.h")

# DSP_SRCS-$(HAVE_MSA)    += mips/macros_msa.h

list(APPEND vpx_dsp_avx2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/bitdepth_conversion_avx2.h")
list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/bitdepth_conversion_sse2.h")
## This file is included in libs.mk. Including it here would cause it to be
## compiled into an object. Even as an empty file, this would create an
## executable section on the stack.
## DSP_SRCS-$(HAVE_SSE2)   += x86/bitdepth_conversion_sse2$(ASM)

# bit reader, bit writer
list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/prob.h")
list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/prob.c")

if (CONFIG_ENCODERS)
  list(APPEND vpx_dsp_headers
    "${CMAKE_CURRENT_LIST_DIR}/bitwriter.h"
    "${CMAKE_CURRENT_LIST_DIR}/bitwriter_buffer.h"
    "${CMAKE_CURRENT_LIST_DIR}/psnr.h"
  )
  list(APPEND vpx_dsp_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/bitwriter.c"
    "${CMAKE_CURRENT_LIST_DIR}/bitwriter_buffer.c"
    "${CMAKE_CURRENT_LIST_DIR}/psnr.c"
  )
  if (CONFIG_INTERNAL_STATS)
    list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/ssim.h")
    list(APPEND vpx_dsp_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/ssim.c"
      "${CMAKE_CURRENT_LIST_DIR}/psnrhvs.c"
      "${CMAKE_CURRENT_LIST_DIR}/fastssim.c"
    )
  endif ()
endif ()

if (CONFIG_DECODERS)
  list(APPEND vpx_dsp_headers
    "${CMAKE_CURRENT_LIST_DIR}/bitreader.h"
    "${CMAKE_CURRENT_LIST_DIR}/bitreader_buffer.h"
  )
  list(APPEND vpx_dsp_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/bitreader.c"
    "${CMAKE_CURRENT_LIST_DIR}/bitreader_buffer.c"
  )
endif ()

# intra predictions
list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/intrapred.c")
list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/intrapred_sse2.asm")
list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/intrapred_ssse3.asm")
# DSP_SRCS-$(HAVE_VSX) += ppc/intrapred_vsx.c

if (CONFIG_VP9_HIGHBITDEPTH)
  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_intrapred_intrin_sse2.asm")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_intrapred_intrin_sse2.c")
  list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_intrapred_intrin_ssse3.c")
  list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_intrapred_neon.c")
endif ()

if (CONFIG_POSTPROC OR CONFIG_VP9_POSTPROC)
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/postproc.h")
  list(APPEND vpx_dsp_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/add_noise.c"
    "${CMAKE_CURRENT_LIST_DIR}/deblock.c"
  )
  # DSP_SRCS-$(HAVE_MSA) += mips/add_noise_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/deblock_msa.c
  list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/deblock_neon.c")
  list(APPEND vpx_dsp_sse2_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/x86/add_noise_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/x86/deblock_sse2.asm"
  )
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/post_proc_sse2.c")
  # DSP_SRCS-$(HAVE_VSX) += ppc/deblock_vsx.c
endif ()

list(APPEND vpx_dsp_neon_asm_sources "${CMAKE_CURRENT_LIST_DIR}/arm/intrapred_neon_asm.asm")
list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/intrapred_neon.c")
# DSP_SRCS-$(HAVE_MSA) += mips/intrapred_msa.c
# DSP_SRCS-$(HAVE_LSX) += loongarch/intrapred_lsx.c
# DSP_SRCS-$(HAVE_DSPR2)  += mips/intrapred4_dspr2.c
# DSP_SRCS-$(HAVE_DSPR2)  += mips/intrapred8_dspr2.c
# DSP_SRCS-$(HAVE_DSPR2)  += mips/intrapred16_dspr2.c

# DSP_SRCS-$(HAVE_DSPR2)  += mips/common_dspr2.h
# DSP_SRCS-$(HAVE_DSPR2)  += mips/common_dspr2.c

list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_filter.h")

if (CONFIG_VP9 OR CONFIG_VP9_ENCODER OR CONFIG_VP9_DECODER)
  #interpolation filters
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_convolve.h")
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/vpx_convolve.c")
  if (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
    list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/x86/convolve.h")
  endif ()
  list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/convolve_sse2.h")
  list(APPEND vpx_dsp_ssse3_headers "${CMAKE_CURRENT_LIST_DIR}/x86/convolve_ssse3.h")
  list(APPEND vpx_dsp_avx2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/convolve_avx2.h")
  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_8t_sse2.asm")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_4t_intrin_sse2.c")
  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_bilinear_sse2.asm")
  list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_8t_ssse3.asm")
  list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_bilinear_ssse3.asm")
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_8t_intrin_avx2.c")
  list(APPEND vpx_dsp_ssse3_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_subpixel_8t_intrin_ssse3.c")
  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_sse2_asm_sources 
      "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_high_subpixel_8t_sse2.asm"
      "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_high_subpixel_bilinear_sse2.asm"
    )
    list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_convolve_avx2.c")
    list(APPEND vpx_dsp_neon_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_vpx_convolve_copy_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_vpx_convlve_avg_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_vpx_convolve8_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_vpx_convolve_neon.c"
    )
  endif ()
  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/vpx_convolve_copy_sse2.asm")
  list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_scaled_convolve8_neon.c")
  if (HAVE_NEON AND NOT HAVE_NEON_ASM)
    list(APPEND vpx_dsp_neon_c_sources 
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_copy_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_avg_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_neon.c"
    )
  endif ()
  list(APPEND vpx_dsp_neon_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_copy_neon_asm.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_horiz_filter_type2_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_vert_filter_type2_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_horiz_filter_type1_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_vert_filter_type1_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_avg_horiz_filter_type2_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_avg_vert_filter_type2_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_avg_horiz_filter_type1_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_avg_vert_filter_type1_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_avg_neon_asm.asm"
  )
  if (HAVE_NEON AND HAVE_NEON_ASM)
    list(APPEND vpx_dsp_neon_headers "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_neon_asm.h")
    list(APPEND vpx_dsp_neon_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_neon_asm.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve_neon.c"
    )
  endif ()

  # # common (msa)
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_avg_horiz_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_avg_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_avg_vert_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_horiz_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve8_vert_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve_avg_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve_copy_msa.c
  # DSP_SRCS-$(HAVE_MSA) += mips/vpx_convolve_msa.h
  # DSP_SRCS-$(HAVE_MMI) += mips/vpx_convolve8_mmi.c

  # # common (dspr2)
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve_common_dspr2.h
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve2_avg_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve2_avg_horiz_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve2_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve2_horiz_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve2_vert_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve8_avg_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve8_avg_horiz_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve8_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve8_horiz_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/convolve8_vert_dspr2.c

  # DSP_SRCS-$(HAVE_VSX)  += ppc/vpx_convolve_vsx.c

  # # common (lsx)
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_avg_horiz_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_avg_vert_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_horiz_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_vert_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve8_avg_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve_avg_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve_copy_lsx.c
  # DSP_SRCS-$(HAVE_LSX) += loongarch/vpx_convolve_lsx.h

  # loop filter
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/loopfilter.c")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/loopfilter_sse2.c")
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/loopfilter_avx2.c")
  list(APPEND vpx_dsp_neon_asm_sources 
    "${CMAKE_CURRENT_LIST_DIR}/arm/loopfilter_16_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/loopfilter_8_neon.asm"
    "${CMAKE_CURRENT_LIST_DIR}/arm/loopfilter_4_neon.asm"
  )
  if (HAVE_NEON AND NOT HAVE_NEON_ASM)
    list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/loopfilter_neon.c")
  endif ()

  # DSP_SRCS-$(HAVE_MSA)    += mips/loopfilter_msa.h
  # DSP_SRCS-$(HAVE_MSA)    += mips/loopfilter_16_msa.c
  # DSP_SRCS-$(HAVE_MSA)    += mips/loopfilter_8_msa.c
  # DSP_SRCS-$(HAVE_MSA)    += mips/loopfilter_4_msa.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_filters_dspr2.h
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_filters_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_macros_dspr2.h
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_masks_dspr2.h
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_mb_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_mb_horiz_dspr2.c
  # DSP_SRCS-$(HAVE_DSPR2)  += mips/loopfilter_mb_vert_dspr2.c

  # DSP_SRCS-$(HAVE_LSX)    += loongarch/loopfilter_lsx.h
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/loopfilter_16_lsx.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/loopfilter_8_lsx.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/loopfilter_4_lsx.c

  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_loopfilter_neon.c")
    list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_loopfilter_sse2.c")
  endif ()
endif ()

list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/txfm_common.h")
list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/txfm_common_sse2.h")
# DSP_SRCS-$(HAVE_MSA)    += mips/txfm_macros_msa.h
# DSP_SRCS-$(HAVE_LSX)    += loongarch/txfm_macros_lsx.h

# forward transform
if (CONFIG_VP9_ENCODER)
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/fwd_txfm.h")
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/fwd_txfm.c")
  list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_txfm_sse2.h")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_txfm_sse2.c")
  list(APPEND vpx_dsp_sse2_headers
    "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_txfm_impl_sse2.h"
    "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_dct32x32_impl_sse2.h"
  )
  if (VPX_ARCH_X86_64)
    list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_txfm_ssse3_x86_64.asm")
  endif ()
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_txfm_avx2.c")
  list(APPEND vpx_dsp_avx2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/fwd_dct32x32_impl_avx2.h")
  list(APPEND vpx_dsp_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/fdct_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/fdct16x16_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/fdct32x32_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/fdct_partial_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/fwd_txfm_neon.c"
  )
  # DSP_SRCS-$(HAVE_MSA)    += mips/fwd_txfm_msa.h
  # DSP_SRCS-$(HAVE_MSA)    += mips/fwd_txfm_msa.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/fwd_txfm_lsx.h
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/fwd_txfm_lsx.c
  if (NOT CONFIG_VP9_HIGHBITDEPTH)
  #   DSP_SRCS-$(HAVE_MSA)    += mips/fwd_dct32x32_msa.c
  #   DSP_SRCS-$(HAVE_LSX)    += loongarch/fwd_dct32x32_lsx.c
  endif ()

  # DSP_SRCS-$(HAVE_VSX)    += ppc/fdct32x32_vsx.c
endif ()

# inverse transform
if (CONFIG_VP9 OR CONFIG_VP9_ENCODER OR CONFIG_VP9_DECODER)
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/inv_txfm.h")
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/inv_txfm.c")
  list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/inv_txfm_sse2.h")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/inv_txfm_sse2.c")
  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/inv_wht_sse2.asm")
  list(APPEND vpx_dsp_ssse3_headers "${CMAKE_CURRENT_LIST_DIR}/x86/inv_txfm_ssse3.h")
  list(APPEND vpx_dsp_ssse3_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/inv_txfm_ssse3.c")
  list(APPEND vpx_dsp_neon_asm_sources "${CMAKE_CURRENT_LIST_DIR}/arm/save_reg_neon.asm")
  # DSP_SRCS-$(HAVE_VSX) += ppc/inv_txfm_vsx.c

  if (NOT CONFIG_VP9_HIGHBITDEPTH)
    # DSP_SRCS-$(HAVE_MSA)   += mips/inv_txfm_msa.h
    # DSP_SRCS-$(HAVE_MSA)   += mips/idct4x4_msa.c
    # DSP_SRCS-$(HAVE_MSA)   += mips/idct8x8_msa.c
    # DSP_SRCS-$(HAVE_MSA)   += mips/idct16x16_msa.c
    # DSP_SRCS-$(HAVE_MSA)   += mips/idct32x32_msa.c

    # DSP_SRCS-$(HAVE_DSPR2) += mips/inv_txfm_dspr2.h
    # DSP_SRCS-$(HAVE_DSPR2) += mips/itrans4_dspr2.c
    # DSP_SRCS-$(HAVE_DSPR2) += mips/itrans8_dspr2.c
    # DSP_SRCS-$(HAVE_DSPR2) += mips/itrans16_dspr2.c
    # DSP_SRCS-$(HAVE_DSPR2) += mips/itrans32_dspr2.c
    # DSP_SRCS-$(HAVE_DSPR2) += mips/itrans32_cols_dspr2.c

    # DSP_SRCS-$(HAVE_LSX)   += loongarch/idct32x32_lsx.c
  elseif (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_neon_headers "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct_neon.h")
    list(APPEND vpx_dsp_neon_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct4x4_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct8x8_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct16x16_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct32x32_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct32x32_34_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct32x32_135_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/highbd_idct32x32_1024_add_neon.c"
    )
    list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_inv_txfm_sse2.h")
    list(APPEND vpx_dsp_sse2_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct4x4_add_sse2.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct8x8_add_sse2.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct16x16_add_sse2.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct32x32_add_sse2.c"
    )
    list(APPEND vpx_dsp_sse4_1_c_headers "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_inv_txfm_sse4.h")
    list(APPEND vpx_dsp_sse4_1_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct4x4_add_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct8x8_add_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct16x16_add_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_idct32x32_add_sse4.c"
    )
  endif ()

  if (HAVE_NEON AND HAVE_NEON_ASM)
    list(APPEND vpx_dsp_neon_asm_sources
      "${CMAKE_CURRENT_LIST_DIR}/arm/idct_neon.asm"
      "${CMAKE_CURRENT_LIST_DIR}/arm/idct4x4_1_add_neon.asm"
      "${CMAKE_CURRENT_LIST_DIR}/arm/idct4x4_add_neon.asm"
    )
  elseif (HAVE_NEON)
    list(APPEND vpx_dsp_neon_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/arm/idct4x4_1_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/arm/idct4x4_add_neon.c"
    )
  endif ()
  list(APPEND vpx_dsp_neon_headers "${CMAKE_CURRENT_LIST_DIR}/arm/idct_neon.h")
  list(APPEND vpx_dsp_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct8x8_1_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct8x8_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct16x16_1_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct16x16_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct32x32_1_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct32x32_34_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct32x32_135_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/idct32x32_add_neon.c"
  )
endif ()

# quantization
if (CONFIG_VP9_ENCODER)
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/quantize.h")
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/quantize.c")

  list(APPEND vpx_dsp_sse2_headers "${CMAKE_CURRENT_LIST_DIR}/x86/quantize_sse2.c")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/quantize_sse2.h")
  list(APPEND vpx_dsp_ssse3_headers "${CMAKE_CURRENT_LIST_DIR}/x86/quantize_ssse3.h")
  list(APPEND vpx_dsp_ssse3_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/quantize_ssse3.c")
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/quantize_avx.c")
  list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/quantize_neon.c")
  # DSP_SRCS-$(HAVE_VSX)    += ppc/quantize_vsx.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/quantize_lsx.c
  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_quantize_intrin_sse2.c")
  endif ()

  # avg
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/avg.c")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/avg_intrin_sse2.c")
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/avg_intrin_avx2.c")
  list(APPEND vpx_dsp_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/avg_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/hadamard_neon.c"
  )
  # DSP_SRCS-$(HAVE_MSA)   += mips/avg_msa.c
  # DSP_SRCS-$(HAVE_LSX)   += loongarch/avg_lsx.c
  if (VPX_ARCH_X86_64)
    list(APPEND vpx_dsp_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/avg_ssse3_x86_64.asm")
  endif ()
  # DSP_SRCS-$(HAVE_VSX)   += ppc/hadamard_vsx.c
endif ()

# skin detection
list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/skin_detection.h")
list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/skin_detection.c")

if (CONFIG_ENCODERS)
  list(APPEND vpx_dsp_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/sad.c"
    "${CMAKE_CURRENT_LIST_DIR}/subtract.c"
    "${CMAKE_CURRENT_LIST_DIR}/sum_squares.c"
  )
  list(APPEND vpx_dsp_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm/sum_squares_neon.c")
  list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/sum_squares_sse2.c")
  # DSP_SRCS-$(HAVE_MSA)    += mips/sum_squares_msa.c

  list(APPEND vpx_dsp_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/sad4d_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/sad_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/subtract_neon.c"
  )

  # DSP_SRCS-$(HAVE_MSA)    += mips/sad_msa.c
  # DSP_SRCS-$(HAVE_MSA)    += mips/subtract_msa.c
  
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/sad_lsx.c
  
  # DSP_SRCS-$(HAVE_MMI)    += mips/sad_mmi.c
  # DSP_SRCS-$(HAVE_MMI)    += mips/subtract_mmi.c
  
  list(APPEND vpx_dsp_avx2_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/x86/sad4d_avx2.c"
    "${CMAKE_CURRENT_LIST_DIR}/x86/sad_avx2.c"
  )
  list(APPEND vpx_dsp_avx512_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/sad4d_avx512.c")

  list(APPEND vpx_dsp_sse2_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/x86/sad4d_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/x86/sad_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/x86/subtract_sse2.asm"
  )

  # DSP_SRCS-$(HAVE_VSX) += ppc/sad_vsx.c
  # DSP_SRCS-$(HAVE_VSX) += ppc/subtract_vsx.c
  
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/subtract_lsx.c
  
  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_sse2_asm_sources
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_sad4d_sse2.asm"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_sad_sse2.asm"
    )
  endif ()

endif ()

if (CONFIG_ENCODERS OR CONFIG_POSTPROC OR CONFIG_VP9_POSTPROC)
  list(APPEND vpx_dsp_headers "${CMAKE_CURRENT_LIST_DIR}/variance.h")
  list(APPEND vpx_dsp_c_sources "${CMAKE_CURRENT_LIST_DIR}/variance.c")

  list(APPEND vpx_dsp_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/arm/avg_pred_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/subpel_variance_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/arm/variance_neon.c"
  )

  # DSP_SRCS-$(HAVE_MSA)    += mips/variance_msa.c
  # DSP_SRCS-$(HAVE_MSA)    += mips/sub_pixel_variance_msa.c
  
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/variance_lsx.h
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/variance_lsx.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/sub_pixel_variance_lsx.c
  # DSP_SRCS-$(HAVE_LSX)    += loongarch/avg_pred_lsx.c
  
  # DSP_SRCS-$(HAVE_MMI)    += mips/variance_mmi.c

  list(APPEND vpx_dsp_sse2_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/x86/avg_pred_sse2.c"
    "${CMAKE_CURRENT_LIST_DIR}/x86/variance_sse2.c" # Contains SSE2 and SSSE3.
  )
  list(APPEND vpx_dsp_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/variance_avx2.c")
  # DSP_SRCS-$(HAVE_VSX)    += ppc/variance_vsx.c

  if (VPX_ARCH_X86_64)
    list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/ssim_opt_x86_64.asm")
  endif ()

  list(APPEND vpx_dsp_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/x86/subpel_variance_sse2.asm") # Contains SSE2 and SSSE3.

  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vpx_dsp_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_variance_sse2.c")
    list(APPEND vpx_dsp_sse2_asm_sources
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_variance_impl_sse2.asm"
      "${CMAKE_CURRENT_LIST_DIR}/x86/highbd_subpel_variance_impl_sse2.asm"
    )
  endif ()
endif ()

# Neon utilities
list(APPEND vpx_dsp_neon_headers
  "${CMAKE_CURRENT_LIST_DIR}/arm/mem_neon.h"
  "${CMAKE_CURRENT_LIST_DIR}/arm/sum_neon.h"
  "${CMAKE_CURRENT_LIST_DIR}/arm/transpose_neon.h"
  "${CMAKE_CURRENT_LIST_DIR}/arm/vpx_convolve8_neon.h"
)

# # PPC VSX utilities
# DSP_SRCS-$(HAVE_VSX)  += ppc/types_vsx.h
# DSP_SRCS-$(HAVE_VSX)  += ppc/txfm_common_vsx.h
# DSP_SRCS-$(HAVE_VSX)  += ppc/transpose_vsx.h
# DSP_SRCS-$(HAVE_VSX)  += ppc/bitdepth_conversion_vsx.h

# X86 utilities
list(APPEND vpx_dsp_sse2_headers
  "${CMAKE_CURRENT_LIST_DIR}/x86/mem_sse2.h"
  "${CMAKE_CURRENT_LIST_DIR}/x86/transpose_sse2.h"
)

# # LSX utilities
# DSP_SRCS-$(HAVE_LSX)  += loongarch/bitdepth_conversion_lsx.h

if (VPX_ARCH_ARM)
  file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_neon/vpx_dsp_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
elseif (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
  file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_x86/vpx_dsp_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
endif ()
list(APPEND vpx_dsp_headers "${VPX_CONFIG_DIR}/vpx_dsp_rtcd.h")
list(APPEND vpx_dsp_c_sources "${VPX_ROOT}/vpx_dsp/vpx_dsp_rtcd.c")

add_library(vpx_dsp OBJECT
  ${vpx_dsp_headers}
  ${vpx_dsp_c_sources}
)

if (HAVE_SSE2)
  if (vpx_dsp_sse2_c_sources)
    create_intrinsics_object_target("-msse2" "sse2" "vpx_dsp")    
  endif ()
  if (vpx_dsp_sse2_asm_sources)
    create_assembly_object_target("sse2" "vpx_dsp")    
  endif ()
endif ()

if (HAVE_SSSE3)
  if (vpx_dsp_ssse3_c_sources)
    create_intrinsics_object_target("-mssse3" "ssse3" "vpx_dsp")    
  endif ()
  if (vpx_dsp_ssse3_asm_sources)
    create_assembly_object_target("ssse3" "vpx_dsp")    
  endif ()
endif ()

if (HAVE_SSE4_1)
  if (vpx_dsp_sse4_1_c_sources)
    create_intrinsics_object_target("-msse4.1" "sse4_1" "vpx_dsp")    
  endif ()
  if (vpx_dsp_sse4_1_asm_sources)
    create_assembly_object_target("sse4_1" "vpx_dsp")    
  endif ()
endif ()

if (HAVE_AVX2)
  if (vpx_dsp_avx2_c_sources)
    create_intrinsics_object_target("-mavx2" "avx2" "vpx_dsp")    
  endif ()
  if (vpx_dsp_avx2_asm_sources)
    create_assembly_object_target("avx2" "vpx_dsp")    
  endif ()
endif ()

if (HAVE_AVX512)
  if (vpx_dsp_avx512_c_sources)
    create_intrinsics_object_target("-mavx512" "avx512" "vpx_dsp")    
  endif ()
  if (vpx_dsp_avx512_asm_sources)
    create_assembly_object_target("avx512" "vpx_dsp")    
  endif ()
endif ()

if (HAVE_NEON)
  if (vpx_dsp_neon_c_sources)
    create_intrinsics_object_target("${VPX_NEON_INTRIN_FLAG}" "neon" "vpx_dsp")    
  endif ()
  if (vpx_dsp_neon_asm_sources AND HAVE_NEON_ASM)
    create_assembly_object_target("neon" "vpx_dsp")    
  endif ()
endif ()
