# Commented out lines that begin with `VP9_[COMMON,CX,DX]_SRCS-`
# correspond to features that we don't cater to.
# If upstream already commented them, we use a double `##`

# vp9 common
set(vp9_common_headers)
set(vp9_common_c_sources)

set(vp9_common_sse2_headers)
set(vp9_common_sse2_c_sources)
set(vp9_common_sse2_asm_sources)

set(vp9_common_ssse3_headers)
set(vp9_common_ssse3_c_sources)
set(vp9_common_ssse3_asm_sources)

set(vp9_common_sse4_1_headers)
set(vp9_common_sse4_1_c_sources)
set(vp9_common_sse4_1_asm_sources)

set(vp9_common_avx2_headers)
set(vp9_common_avx2_c_sources)
set(vp9_common_avx2_asm_sources)

set(vp9_common_avx512_headers)
set(vp9_common_avx512_c_sources)
set(vp9_common_avx512_asm_sources)

set(vp9_common_mmx_headers)
set(vp9_common_mmx_c_sources)
set(vp9_common_mmx_asm_sources)

set(vp9_common_neon_headers)
set(vp9_common_neon_c_sources)
set(vp9_common_neon_asm_sources)

if (CONFIG_VP9 OR CONFIG_VP9_ENCODER OR CONFIG_VP9_DECODER)
  list(APPEND vp9_common_headers
    "${CMAKE_CURRENT_LIST_DIR}/vp9_iface_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_ppflags.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_frame_buffers.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_alloccommon.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_blockd.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropy.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropymode.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropymv.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_enums.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_filter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_idct.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_loopfilter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_thread_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_mv.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_onyxc_int.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_pred_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_quant_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_reconinter.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_reconintra.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_scale.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_seg_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_tile_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_mvref_common.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_common_data.h"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_scan.h"
  )
  ## VP9_COMMON_SRCS-yes += "${CMAKE_CURRENT_LIST_DIR}/common/vp9_debugmodes.c"
  list(APPEND vp9_common_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/vp9_iface_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_alloccommon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_blockd.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropy.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropymode.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_entropymv.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_frame_buffers.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_idct.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_filter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_pred_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_scale.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_seg_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_tile_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_loopfilter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_thread_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_mvref_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_quant_common.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_reconinter.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_reconintra.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_common_data.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/vp9_scan.c"
  )
  if (CONFIG_VP9_POSTPROC)
    list(APPEND vp9_common_headers
      "${CMAKE_CURRENT_LIST_DIR}/common/vp9_postproc.h"
      "${CMAKE_CURRENT_LIST_DIR}/common/vp9_mfqe.h"
    )
    list(APPEND vp9_common_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/common/vp9_postproc.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/vp9_mfqe.c"
    )
  endif ()
    
  if (CONFIG_VP9_HIGHBITDEPTH)
    # VP9_COMMON_SRCS-$(HAVE_MSA)   += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/vp9_idct4x4_msa.c"
    # VP9_COMMON_SRCS-$(HAVE_MSA)   += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/vp9_idct8x8_msa.c"
    # VP9_COMMON_SRCS-$(HAVE_MSA)   += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/vp9_idct16x16_msa.c"
  endif ()

  list(APPEND vp9_common_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp9_idct_intrin_sse2.c")
  # VP9_COMMON_SRCS-$(HAVE_VSX)   += "${CMAKE_CURRENT_LIST_DIR}/common/ppc/vp9_idct_vsx.c"
  list(APPEND vp9_common_neon_headers "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_iht_neon.h")
  list(APPEND vp9_common_neon_c_sources 
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_iht4x4_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_iht8x8_add_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_iht16x16_add_neon.c"
  )

  if (CONFIG_VP9_POSTPROC)
    # VP9_COMMON_SRCS-$(HAVE_MSA)  += "${CMAKE_CURRENT_LIST_DIR}/common/mips/msa/vp9_mfqe_msa.c"
    list(APPEND vp9_common_sse2_asm_sources "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp9_mfqe_sse2.asm")
  endif ()

  if (NOT CONFIG_VP9_HIGHBITDEPTH)
    # VP9_COMMON_SRCS-$(HAVE_DSPR2) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/vp9_itrans4_dspr2.c"
    # VP9_COMMON_SRCS-$(HAVE_DSPR2) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/vp9_itrans8_dspr2.c"
    # VP9_COMMON_SRCS-$(HAVE_DSPR2) += "${CMAKE_CURRENT_LIST_DIR}/common/mips/dspr2/vp9_itrans16_dspr2.c"
  else ()
    list(APPEND vp9_common_neon_c_sources 
      "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_highbd_iht4x4_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_highbd_iht8x8_add_neon.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/arm/neon/vp9_highbd_iht16x16_add_neon.c"
    )
    list(APPEND vp9_common_sse4_1_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp9_highbd_iht4x4_add_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp9_highbd_iht8x8_add_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/common/x86/vp9_highbd_iht16x16_add_sse4.c"
    )
  endif ()

  add_rtcd_build_step("${VPX_ROOT}/vp9/common/vp9_rtcd_defs.pl"
    "${VPX_CONFIG_DIR}/vp9_rtcd.h"
    "${VPX_ROOT}/vp9/common/vp9_rtcd.c"
    "vp9_rtcd"
  )

  list(APPEND vp9_common_headers "${VPX_CONFIG_DIR}/vp9_rtcd.h")
  list(APPEND vp9_common_c_sources "${CMAKE_CURRENT_LIST_DIR}/common/vp9_rtcd.c")
endif ()

# vp9 encoder
set(vp9_encoder_headers)
set(vp9_encoder_c_sources)

set(vp9_encoder_sse2_headers)
set(vp9_encoder_sse2_c_sources)
set(vp9_encoder_sse2_asm_sources)

set(vp9_encoder_ssse3_headers)
set(vp9_encoder_ssse3_c_sources)
set(vp9_encoder_ssse3_asm_sources)

set(vp9_encoder_sse4_1_headers)
set(vp9_encoder_sse4_1_c_sources)
set(vp9_encoder_sse4_1_asm_sources)

set(vp9_encoder_avx_headers)
set(vp9_encoder_avx_c_sources)
set(vp9_encoder_avx_asm_sources)

set(vp9_encoder_avx2_headers)
set(vp9_encoder_avx2_c_sources)
set(vp9_encoder_avx2_asm_sources)

set(vp9_encoder_avx512_headers)
set(vp9_encoder_avx512_c_sources)
set(vp9_encoder_avx512_asm_sources)

set(vp9_encoder_mmx_headers)
set(vp9_encoder_mmx_c_sources)
set(vp9_encoder_mmx_asm_sources)

set(vp9_encoder_neon_headers)
set(vp9_encoder_neon_c_sources)
set(vp9_encoder_neon_asm_sources)

if (CONFIG_VP9 AND CONFIG_VP9_ENCODER)
  list(APPEND vp9_encoder_headers
    "${CMAKE_CURRENT_LIST_DIR}/vp9_cx_iface.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_context_tree.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_cost.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodeframe.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ethread.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_block.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_bitstream.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodemb.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodemv.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_extend.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_firstpass.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_job_queue.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_lookahead.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_mcomp.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_multi_thread.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encoder.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_quantize.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ratectrl.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_rd.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_rdopt.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_pickmode.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_svc_layercontext.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_tokenize.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_treewriter.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_picklpf.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_partition_models.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_segmentation.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_speed_features.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_subexp.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_resize.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_variance.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_360.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_cyclicrefresh.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_complexity.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_alt_ref_aq.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_skin_detection.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_noise_estimate.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ext_ratectrl.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_temporal_filter.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_mbgraph.h"
  )
  list(APPEND vp9_encoder_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/vp9_cx_iface.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_bitstream.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_context_tree.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_cost.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_dct.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodeframe.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodemb.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encodemv.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ethread.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_extend.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_firstpass.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_frame_scale.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_lookahead.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_multi_thread.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_mcomp.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_encoder.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_picklpf.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_quantize.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ratectrl.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_rd.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_rdopt.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_pickmode.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_segmentation.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_speed_features.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_subexp.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_svc_layercontext.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_resize.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_tokenize.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_treewriter.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_variance.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_360.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_cyclicrefresh.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_complexity.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_alt_ref_aq.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_skin_detection.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_noise_estimate.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_ext_ratectrl.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_temporal_filter.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_mbgraph.c"
  )
  if (CONFIG_VP9_TEMPORAL_DENOISING)
    list(APPEND vp9_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_denoiser.h")
    list(APPEND vp9_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_denoiser.c")
  endif ()
  if (CONFIG_INTERNAL)
    list(APPEND vp9_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_blockiness.h")
    list(APPEND vp9_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_blockiness.c")
  endif ()
  if (CONFIG_NON_GREEDY_MV)
    list(APPEND vp9_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_non_greedy_mv.h")
    list(APPEND vp9_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_non_greedy_mv.c")
  endif ()
  if (CONFIG_VP9_POSTPROC AND CONFIG_INTERNAL_STATS)
    list(APPEND vp9_encoder_headers "${CMAKE_CURRENT_LIST_DIR}/common/vp9_postproc.h")
    list(APPEND vp9_encoder_c_sources "${CMAKE_CURRENT_LIST_DIR}/common/vp9_postproc.c")
  endif ()

  list(APPEND vp9_encoder_sse4_1_headers "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_constants.h")
  list(APPEND vp9_encoder_sse4_1_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_sse4.c")

  list(APPEND vp9_encoder_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_quantize_sse2.c")
  list(APPEND vp9_encoder_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_quantize_avx2.c")
  list(APPEND vp9_encoder_avx_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_diamond_search_sad_avx.c")
  if (CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vp9_encoder_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_highbd_block_error_intrin_sse2.c")
    list(APPEND vp9_encoder_sse4_1_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/highbd_temporal_filter_sse4.c")
  endif ()

  list(APPEND vp9_encoder_sse2_asm_sources
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_dct_sse2.asm"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_error_sse2.asm"
  )

  if (VPX_ARCH_X86_64)
    list(APPEND vp9_encoder_ssse3_asm_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_quantize_ssse3_x86_64.asm")
  endif ()

  list(APPEND vp9_encoder_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_dct_intrin_sse2.c")
  list(APPEND vp9_encoder_ssse3_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_frame_scale_ssse3.c")
  list(APPEND vp9_encoder_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp9_dct_neon.c")

  if (CONFIG_VP9_TEMPORAL_DENOISING)
    list(APPEND vp9_encoder_sse2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_denoiser_sse2.c")
    list(APPEND vp9_encoder_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp9_denoiser_neon.c")
  endif ()

  list(APPEND vp9_encoder_avx2_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/vp9_error_avx2.c")

  if (NOT CONFIG_VP9_HIGHBITDEPTH)
    list(APPEND vp9_encoder_neon_c_sources "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp9_error_neon.c")
  endif ()
  list(APPEND vp9_encoder_neon_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp9_frame_scale_neon.c"
    "${CMAKE_CURRENT_LIST_DIR}/encoder/arm/neon/vp9_quantize_neon.c"
  )

  # VP9_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/vp9_error_msa.c"

  if (NOT CONFIG_VP9_HIGHBITDEPTH)
    # VP9_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/vp9_fdct4x4_msa.c"
    # VP9_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/vp9_fdct8x8_msa.c"
    # VP9_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/vp9_fdct16x16_msa.c"
    # VP9_CX_SRCS-$(HAVE_MSA) += "${CMAKE_CURRENT_LIST_DIR}/encoder/mips/msa/vp9_fdct_msa.h"
  endif ()

  # VP9_CX_SRCS-$(HAVE_VSX) += "${CMAKE_CURRENT_LIST_DIR}/encoder/ppc/vp9_quantize_vsx.c"

  # Strip unnecessary files with CONFIG_REALTIME_ONLY
  if (CONFIG_REALTIME_ONLY)
    list(REMOVE_ITEM vp9_encoder_headers
      "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_constants.h"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_alt_ref_aq.h"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_variance.h"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_360.h"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_complexity.h"
    )
    list(REMOVE_ITEM vp9_encoder_c_sources
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_firstpass.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_mbgraph.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_temporal_filter.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/temporal_filter_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/x86/highbd_temporal_filter_sse4.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_alt_ref_aq.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_variance.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_360.c"
      "${CMAKE_CURRENT_LIST_DIR}/encoder/vp9_aq_complexity.c"
    )
  endif ()
endif ()

# vp9 decoder
set(vp9_decoder_headers)
set(vp9_decoder_c_sources)

set(vp9_decoder_sse2_headers)
set(vp9_decoder_sse2_c_sources)
set(vp9_decoder_sse2_asm_sources)

set(vp9_decoder_ssse3_headers)
set(vp9_decoder_ssse3_c_sources)
set(vp9_decoder_ssse3_asm_sources)

set(vp9_decoder_sse4_1_headers)
set(vp9_decoder_sse4_1_c_sources)
set(vp9_decoder_sse4_1_asm_sources)

set(vp9_decoder_avx2_headers)
set(vp9_decoder_avx2_c_sources)
set(vp9_decoder_avx2_asm_sources)

set(vp9_decoder_avx512_headers)
set(vp9_decoder_avx512_c_sources)
set(vp9_decoder_avx512_asm_sources)

set(vp9_decoder_mmx_headers)
set(vp9_decoder_mmx_c_sources)
set(vp9_decoder_mmx_asm_sources)

set(vp9_decoder_neon_headers)
set(vp9_decoder_neon_c_sources)
set(vp9_decoder_neon_asm_sources)

if (CONFIG_VP9 AND CONFIG_VP9_DECODER)
  list(APPEND vp9_decoder_headers
    "${CMAKE_CURRENT_LIST_DIR}/vp9_dx_iface.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decodeframe.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decodemv.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_detokenize.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decoder.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_dsubexp.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_job_queue.h"
  )
  list(APPEND vp9_decoder_c_sources
    "${CMAKE_CURRENT_LIST_DIR}/vp9_dx_iface.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decodemv.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decodeframe.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_detokenize.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_decoder.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_dsubexp.c"
    "${CMAKE_CURRENT_LIST_DIR}/decoder/vp9_job_queue.c"
  )
endif ()

if (CONFIG_VP9 OR CONFIG_VP9_ENCODER OR CONFIG_VP9_DECODER)
  add_library(vp9_common OBJECT
    ${vp9_common_headers}
    ${vp9_common_c_sources}
    $<TARGET_OBJECTS:vpx_scale>
    $<TARGET_OBJECTS:vpx_dsp>
  )  
endif ()

if (CONFIG_VP9 AND CONFIG_VP9_ENCODER)
  add_library(vp9_encoder OBJECT
    ${vp9_encoder_headers}
    ${vp9_encoder_c_sources}
    $<TARGET_OBJECTS:vp9_common>
  )  
endif ()

if (CONFIG_VP9 AND CONFIG_VP9_DECODER)
  add_library(vp9_decoder OBJECT
    ${vp9_decoder_headers}
    ${vp9_decoder_c_sources}
    $<TARGET_OBJECTS:vp9_common>
  )  
endif ()

foreach(_base_target IN ITEMS "common" "encoder" "decoder")
  if (HAVE_SSE2)
    if (vp9_${_base_target}_sse2_c_sources)
      create_intrinsics_object_target("-msse2" "sse2" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_sse2_asm_sources)
      create_assembly_object_target("sse2" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_SSSE3)
    if (vp9_${_base_target}_ssse3_c_sources)
      create_intrinsics_object_target("-mssse3" "ssse3" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_ssse3_asm_sources)
      create_assembly_object_target("ssse3" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_SSE4_1)
    if (vp9_${_base_target}_sse4_1_c_sources)
      create_intrinsics_object_target("-msse4.1" "sse4_1" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_sse4_1_asm_sources)
      create_assembly_object_target("sse4_1" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_AVX)
    if (vp9_${_base_target}_avx_c_sources)
      create_intrinsics_object_target("-mavx" "avx" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_avx_asm_sources)
      create_assembly_object_target("avx" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_AVX2)
    if (vp9_${_base_target}_avx2_c_sources)
      create_intrinsics_object_target("-mavx2" "avx2" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_avx2_asm_sources)
      create_assembly_object_target("avx2" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_AVX512)
    if (vp9_${_base_target}_avx512_c_sources)
      create_intrinsics_object_target("-mavx512" "avx512" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_avx512_asm_sources)
      create_assembly_object_target("avx512" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_MMX)
    if (vp9_${_base_target}_mmx_c_sources)
      create_intrinsics_object_target("" "mmx" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_mmx_asm_sources)
      create_assembly_object_target("mmx" "vp9_${_base_target}")      
    endif ()
  endif ()

  if (HAVE_NEON)
    if (vp9_${_base_target}_neon_c_sources)
      create_intrinsics_object_target("${VPX_NEON_INTRIN_FLAG}" "neon" "vp9_${_base_target}")      
    endif ()
    if (vp9_${_base_target}_neon_asm_sources AND HAVE_NEON_ASM)
      create_assembly_object_target("neon" "vp9_${_base_target}")      
    endif ()
  endif ()
endforeach()
