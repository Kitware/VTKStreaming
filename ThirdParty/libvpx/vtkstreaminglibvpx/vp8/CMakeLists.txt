# Commented out lines that begin with `VP8_[COMMON,CX,DX]_SRCS-`
# correspond to features that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vp8_common_headers)
set(vp8_common_c_sources)

set(vp8_common_sse2_headers)
set(vp8_common_sse2_c_sources)
set(vp8_common_sse2_asm_sources)

set(vp8_common_ssse3_headers)
set(vp8_common_ssse3_c_sources)
set(vp8_common_ssse3_asm_sources)

set(vp8_common_sse4_1_headers)
set(vp8_common_sse4_1_c_sources)
set(vp8_common_sse4_1_asm_sources)

set(vp8_common_avx2_headers)
set(vp8_common_avx2_c_sources)
set(vp8_common_avx2_asm_sources)

set(vp8_common_avx512_headers)
set(vp8_common_avx512_c_sources)
set(vp8_common_avx512_asm_sources)

set(vp8_common_mmx_headers)
set(vp8_common_mmx_c_sources)
set(vp8_common_mmx_asm_sources)

set(vp8_common_neon_headers)
set(vp8_common_neon_c_sources)
set(vp8_common_neon_asm_sources)

if (CONFIG_VP8 OR CONFIG_VP8_ENCODER OR CONFIG_VP8_DECODER)
  list(APPEND vp8_common_headers
    "${CMAKE_CURRENT_LIST_DIR}/common/ppflags.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/onyx.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/onyxd.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/coefupdateprobs.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/default_coef_probs.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/filter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/alloccommon.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/blockd.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropy.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropymode.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropymv.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/extend.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/findnearmv.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/header.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/invtrans.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/loopfilter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/modecont.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/mv.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/onyxc_int.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/quant_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconinter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconintra.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconintra4x4.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/setupintrarecon.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/swapyv12buffer.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/systemdependent.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/threading.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/treecoder.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp8_entropymodedata.h"
  )
  # VP8_COMMON_SRCS-yes += "${CMAKE_CURRENT_LIST_DIR}/common/debugmodes.c"
  list(APPEND vp8_common_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/alloccommon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/blockd.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/dequantize.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropy.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropymode.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/entropymv.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/extend.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/filter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/findnearmv.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/generic/systemdependent.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/idct_blk.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/idctllm.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp8_loopfilter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/loopfilter_filters.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/mbpitch.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/modecont.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/quant_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconinter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconintra.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/reconintra4x4.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/setupintrarecon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/swapyv12buffer.c"
  )

  if (VPX_ARCH_ARM)
    file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_neon/vp8_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
  elseif (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
    file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_x86/vp8_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
  endif ()

  list(APPEND vp8_common_headers "${VPX_CONFIG_DIR}/vp8_rtcd.h")
  list(APPEND vp8_common_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/rtcd.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/treecoder.c"
  )

  if (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
    list(APPEND vp8_common_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp8_asm_stubs.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/x86/loopfilter_x86.c"
    )
  endif ()
  if (CONFIG_POSTPROC)
    list(APPEND vp8_common_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/common/mfqe.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/postproc.h"
      "${CMAKE_CURRENT_LIST_DIR}/common/postproc.c"
    )
  endif ()
  list(APPEND vp8_common_mmx_c_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/idct_blk_mmx.c")
  list(APPEND vp8_common_mmx_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/dequantize_mmx.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/idctllm_mmx.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/recon_mmx.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/subpixel_mmx.asm"
  )
  list(APPEND vp8_common_sse2_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/idct_blk_sse2.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/bilinear_filter_sse2.c"
  )
  list(APPEND vp8_common_sse2_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/idctllm_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/recon_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/subpixel_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/loopfilter_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/common/x86/iwalsh_sse2.asm"
  )
  list(APPEND vp8_common_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/subpixel_ssse3.asm")

  if (CONFIG_POSTPROC)
    list(APPEND vp8_common_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/mfqe_sse2.asm")
  endif ()

  if (VPX_ARCH_X86_64)
    list(APPEND vp8_common_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/loopfilter_block_sse2_x86_64.asm")
  endif ()

  # common (c)
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/idctllm_dspr2.c
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/filter_dspr2.c
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/vp8_loopfilter_filters_dspr2.c
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/reconinter_dspr2.c
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/idct_blk_dspr2.c
  # VP8_COMMON_SRCS-$(HAVE_DSPR2)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/dequantize_dspr2.c

  # common (c)
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/bilinear_filter_msa.c
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/copymem_msa.c
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/idct_msa.c
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/loopfilter_filters_msa.c
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/sixtap_filter_msa.c
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/vp8_macros_msa.h

  # common (c)
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/sixtap_filter_mmi.c
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/loopfilter_filters_mmi.c
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/idctllm_mmi.c
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/dequantize_mmi.c
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/copymem_mmi.c
  # VP8_COMMON_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/mmi/idct_blk_mmi.c

  # ifeq ($(CONFIG_POSTPROC),yes)
  # VP8_COMMON_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/mfqe_msa.c
  # endif

  # common (loongarch LSX intrinsics)
  # VP8_COMMON_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/common/loongarch/loopfilter_filters_lsx.c
  # VP8_COMMON_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/common/loongarch/sixtap_filter_lsx.c
  # VP8_COMMON_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/common/loongarch/idct_lsx.c

  # common (neon intrinsics)
  list(APPEND vp8_common_neon_headers "${CMAKE_CURRENT_LIST_DIR}/common/arm/loopfilter_arm.h")
  list(APPEND vp8_common_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/loopfilter_arm.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/bilinearpredict_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/copymem_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/dc_only_idct_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/dequant_idct_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/dequantizeb_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/idct_blk_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/iwalsh_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp8_loopfilter_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/loopfiltersimplehorizontaledge_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/loopfiltersimpleverticaledge_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/mbloopfilter_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/shortidct4x4llm_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/sixtappredict_neon.c"
  )
endif ()

# vp8 encoder
set(vp8_encoder_headers)
set(vp8_encoder_c_sources)

set(vp8_encoder_sse2_headers)
set(vp8_encoder_sse2_c_sources)
set(vp8_encoder_sse2_asm_sources)

set(vp8_encoder_ssse3_headers)
set(vp8_encoder_ssse3_c_sources)
set(vp8_encoder_ssse3_asm_sources)

set(vp8_encoder_sse4_1_headers)
set(vp8_encoder_sse4_1_c_sources)
set(vp8_encoder_sse4_1_asm_sources)

set(vp8_encoder_avx2_headers)
set(vp8_encoder_avx2_c_sources)
set(vp8_encoder_avx2_asm_sources)

set(vp8_encoder_avx512_headers)
set(vp8_encoder_avx512_c_sources)
set(vp8_encoder_avx512_asm_sources)

set(vp8_encoder_mmx_headers)
set(vp8_encoder_mmx_c_sources)
set(vp8_encoder_mmx_asm_sources)

set(vp8_encoder_neon_headers)
set(vp8_encoder_neon_c_sources)
set(vp8_encoder_neon_asm_sources)

if (CONFIG_VP8 AND CONFIG_VP8_ENCODER)
  list(APPEND vp8_encoder_headers
    "${CMAKE_CURRENT_LIST_DIR}/encoder/defaultcoefcounts.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodeframe.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/block.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/boolhuff.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/bitstream.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodeintra.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodemb.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodemv.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/firstpass.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/lookahead.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/mcomp.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/modecosts.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/onyx_int.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/pickinter.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/quantize.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/ratectrl.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/rdopt.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/tokenize.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/treewriter.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/picklpf.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/segmentation.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp8_skin_detection.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/dct_value_cost.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/dct_value_tokens.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/temporal_filter.h"
  )
  list(APPEND vp8_encoder_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/vp8_cx_iface.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/bitstream.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/boolhuff.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/copy_c.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/dct.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodeframe.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodeintra.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodemb.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/encodemv.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/lookahead.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/mcomp.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/modecosts.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/onyx_if.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/pickinter.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/picklpf.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp8_quantize.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/ratectrl.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/rdopt.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/segmentation.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp8_skin_detection.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/tokenize.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/temporal_filter.c"
  )
  if (CONFIG_MULTITHREAD)
    list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/ethreading.c")
    list(APPEND vp8_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/ethreading.h")
  endif ()
  list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/firstpass.c")
  if (CONFIG_TEMPORAL_DENOISING)
    list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/denoising.h")
    list(APPEND vp8_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/denoising.c")
  endif ()
  list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/treewriter.c")
  if (CONFIG_INTERNAL_STATS)
    list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/common/postproc.h")
    list(APPEND vp8_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/common/postproc.c")
  endif ()
  if (CONFIG_MULTI_RES_ENCODING)
    list(APPEND vp8_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/mr_dissim.c")
    list(APPEND vp8_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/mr_dissim.h")
  endif ()

  if (CONFIG_MULTI_RES_ENCODING)
    list(REMOVE_ITEM vp8_encoder_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/encoder/firstpass.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/temporal_filter.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/temporal_filter.h"
    )
  endif ()

  list(APPEND vp8_encoder_sse2_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/copy_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/copy_sse3.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/dct_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/fwalsh_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/block_error_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_apply_sse2.asm"
  )
  list(APPEND vp8_encoder_sse2_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp8_quantize_sse2.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp8_enc_stubs_sse2.c"
  )
  list(APPEND vp8_encoder_ssse3_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp8_quantize_ssse3.c")
  list(APPEND vp8_encoder_sse4_1_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/quantize_sse4.c")

  if (CONFIG_TEMPORAL_DENOISING)
    list(APPEND vp8_encoder_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/denoising_sse2.c")
  endif ()


  if (CONFIG_REALTIME_ONLY)
    list(REMOVE_ITEM vp8_encoder_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_apply_sse2.asm")
  endif ()

  list(APPEND vp8_encoder_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/denoising_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/fastquantizeb_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/shortfdct_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp8_shortwalsh4x4_neon.c"
  )
  # VP8_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/dct_msa.c
  # VP8_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/encodeopt_msa.c
  # VP8_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/quantize_msa.c
  # VP8_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/temporal_filter_msa.c

  # VP8_CX_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/mmi/vp8_quantize_mmi.c
  # VP8_CX_SRCS-$(HAVE_MMI) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/mmi/dct_mmi.c

  # ifeq ($(CONFIG_TEMPORAL_DENOISING),yes)
  # VP8_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/denoising_msa.c
  # endif

  # ifeq ($(CONFIG_REALTIME_ONLY),yes)
  # VP8_CX_SRCS_REMOVE-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/temporal_filter_msa.c
  # endif

  # common (loongarch LSX intrinsics)
  # VP8_CX_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/encoder/loongarch/quantize_lsx.c
  # VP8_CX_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/encoder/loongarch/dct_lsx.c
  # VP8_CX_SRCS-$(HAVE_LSX) += "${CMAKE_CURRENT_LIST_DIR}/encoder/loongarch/encodeopt_lsx.c
endif ()

# vp8 decoder
set(vp8_decoder_headers)
set(vp8_decoder_c_sources)

set(vp8_decoder_sse2_headers)
set(vp8_decoder_sse2_c_sources)
set(vp8_decoder_sse2_asm_sources)

set(vp8_decoder_ssse3_headers)
set(vp8_decoder_ssse3_c_sources)
set(vp8_decoder_ssse3_asm_sources)

set(vp8_decoder_sse4_1_headers)
set(vp8_decoder_sse4_1_c_sources)
set(vp8_decoder_sse4_1_asm_sources)

set(vp8_decoder_avx2_headers)
set(vp8_decoder_avx2_c_sources)
set(vp8_decoder_avx2_asm_sources)

set(vp8_decoder_avx512_headers)
set(vp8_decoder_avx512_c_sources)
set(vp8_decoder_avx512_asm_sources)

set(vp8_decoder_mmx_headers)
set(vp8_decoder_mmx_c_sources)
set(vp8_decoder_mmx_asm_sources)

set(vp8_decoder_neon_headers)
set(vp8_decoder_neon_c_sources)
set(vp8_decoder_neon_asm_sources)

if (CONFIG_VP8 AND CONFIG_VP8_DECODER)
  list(APPEND vp8_decoder_headers
    "${CMAKE_CURRENT_LIST_DIR}/decoder/dboolhuff.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/decodemv.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/decoderthreading.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/detokenize.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/onyxd_int.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/treereader.h"
  )
  list(APPEND vp8_decoder_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/vp8_dx_iface.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/dboolhuff.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/decodemv.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/decodeframe.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/detokenize.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/onyxd_if.c"
  )
  if (CONFIG_ERROR_CONCEALMENT)
    list(APPEND vp8_decoder_headers
      "${CMAKE_CURRENT_LIST_DIR}/decoder/ec_types.h"
      "${CMAKE_CURRENT_LIST_DIR}/decoder/error_concealment.h"
    )
    list(APPEND vp8_decoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/decoder/error_concealment.c")
  endif ()
  if (CONFIG_MULTITHREAD)
    list(APPEND vp8_decoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/decoder/threading.c")
  endif ()
endif ()

if (CONFIG_VP8 OR CONFIG_VP8_ENCODER OR CONFIG_VP8_DECODER)
  add_library(vp8_common OBJECT
    ${vp8_common_headers}
    ${vp8_common_c_sources}
    $<TARGET_OBJECTS:vpx_scale>
    $<TARGET_OBJECTS:vpx_dsp>
  )  
endif ()

if (CONFIG_VP8 AND CONFIG_VP8_ENCODER)
  add_library(vp8_encoder OBJECT
    ${vp8_encoder_headers}
    ${vp8_encoder_c_sources}
    $<TARGET_OBJECTS:vp8_common>
  )  
endif ()

if (CONFIG_VP8 AND CONFIG_VP8_DECODER)
  add_library(vp8_decoder OBJECT
    ${vp8_decoder_headers}
    ${vp8_decoder_c_sources}
    $<TARGET_OBJECTS:vp8_common>
  )  
endif ()

foreach(_base_target IN ITEMS "common" "encoder" "decoder")
  if (HAVE_SSE2)
    if (vp8_${_base_target}_sse2_c_sources)
      create_intrinsics_object_target("-msse2" "sse2" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_sse2_asm_sources)
      create_assembly_object_target("sse2" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_SSSE3)
    if (vp8_${_base_target}_ssse3_c_sources)
      create_intrinsics_object_target("-mssse3" "ssse3" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_ssse3_asm_sources)
      create_assembly_object_target("ssse3" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_SSE4_1)
    if (vp8_${_base_target}_sse4_1_c_sources)
      create_intrinsics_object_target("-msse4.1" "sse4_1" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_sse4_1_asm_sources)
      create_assembly_object_target("sse4_1" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_AVX2)
    if (vp8_${_base_target}_avx2_c_sources)
      create_intrinsics_object_target("-mavx2" "avx2" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_avx2_asm_sources)
      create_assembly_object_target("avx2" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_AVX512)
    if (vp8_${_base_target}_avx512_c_sources)
      create_intrinsics_object_target("-mavx512" "avx512" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_avx512_asm_sources)
      create_assembly_object_target("avx512" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_MMX)
    if (vp8_${_base_target}_mmx_c_sources)
      create_intrinsics_object_target("" "mmx" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_mmx_asm_sources)
      create_assembly_object_target("mmx" "vp8_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_NEON)
    if (vp8_${_base_target}_neon_c_sources)
      create_intrinsics_object_target("${VPX_NEON_INTRIN_FLAG}" "neon" "vp8_${_base_target}")      
    endif ()
    if (vp8_${_base_target}_neon_asm_sources AND HAVE_NEON_ASM)
      create_assembly_object_target("neon" "vp8_${_base_target}")      
    endif ()
  endif ()
endforeach()
