/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "RegisterInfoPOSIX_ascend950.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/StreamString.h"
using namespace lldb_private;
using namespace lldb;
using namespace std;

namespace {

// bitmap:0|1|2|3
//        aic|aiv|simd_vf|simt_vf
// don't care pos_type
constexpr uint8_t AIC_MASK = 1U << static_cast<int>(CoreType::AIC);
constexpr uint8_t AIV_MASK = 1U << static_cast<int>(CoreType::AIV);
constexpr uint8_t MIX_MASK = AIC_MASK | AIV_MASK;

// only show on SIMD/SIMT
constexpr uint8_t SIMD_MASK_OFFSET = 2;
constexpr uint8_t SIMT_MASK_OFFSET = 3;
constexpr uint8_t SIMD_VF_MASK = 1U << SIMD_MASK_OFFSET;
constexpr uint8_t SIMT_VF_MASK = 1U << SIMT_MASK_OFFSET;

bool IsRegisterSupport(CoreType core_type, InterruptPosType pos_type,
                       uint8_t mask) {
  // if mask not belong to simd/simt, only consider core_type
  if ((mask & AIC_MASK) || (mask & AIV_MASK)) {
    return mask & (1U << static_cast<int>(core_type));
  }
  if (pos_type == InterruptPosType::VEC_INTERRUPT_SIMD) {
    return mask & SIMD_VF_MASK;
  }
  if (pos_type == InterruptPosType::VEC_INTERRUPT_SIMT) {
    return mask & SIMT_VF_MASK;
  }
  // su but register is vf
  return false;
}

enum LLDB_ASCEND_RENUM {
  k_first_gpr_ascend,
  lldb_x0_ascend = k_first_gpr_ascend, lldb_x1_ascend, lldb_x2_ascend, lldb_x3_ascend,
  lldb_x4_ascend, lldb_x5_ascend, lldb_x6_ascend, lldb_x7_ascend,
  lldb_x8_ascend, lldb_x9_ascend, lldb_x10_ascend, lldb_x11_ascend,
  lldb_x12_ascend, lldb_x13_ascend, lldb_x14_ascend, lldb_x15_ascend,
  lldb_x16_ascend, lldb_x17_ascend, lldb_x18_ascend, lldb_x19_ascend,
  lldb_x20_ascend, lldb_x21_ascend, lldb_x22_ascend, lldb_x23_ascend,
  lldb_x24_ascend, lldb_x25_ascend, lldb_x26_ascend, lldb_x27_ascend,
  lldb_x28_ascend, lldb_x29_ascend, lldb_x30_ascend, lldb_x31_ascend,
  k_last_gpr_ascend = lldb_x31_ascend,
  lldb_pc_ascend, lldb_status_ascend, lldb_ctrl_su_ascend, lldb_condition_flag_ascend,
  lldb_cond_ascend, lldb_sys_cnt_ascend, lldb_safety_crc_excpt_ascend, lldb_call_depth_cnt_ascend,
  lldb_icache_prl_st_ascend, lldb_ffts_base_addr_ascend, lldb_kick_start_pc_ascend,
  lldb_su_warn_status_0_ascend, lldb_su_warn_status_1_ascend,
  lldb_pb_slot_offset_ascend, lldb_pb_slot_id_ascend, lldb_pb_slot_occupy_bitmap_ascend, lldb_vec_event_table_ascend,
  lldb_cube_event_table_ascend, lldb_fixp_event_table_ascend, lldb_scalar_event_table_ascend,
  lldb_mte1_event_table_ascend, lldb_mte2_event_table_ascend, lldb_mte3_event_table_ascend,
  lldb_glb_get_cnt_ascend, lldb_glb_rel_cnt_ascend, lldb_set_cross_core_cnt_ascend,
  lldb_set_intra_blk_cnt_ascend,
  k_last_su_reg_ascend = lldb_set_intra_blk_cnt_ascend,
  lldb_mte_warn_ascend, lldb_l0_set_value_mte_ascend,
  lldb_padding_mte_ascend, lldb_pad_val_outtoub_ascend, lldb_pad_val_outtol1_ascend,
  lldb_loop_size_outtol1_ascend, lldb_loop1_stride_outtol1_ascend, lldb_loop2_stride_outtol1_ascend,
  lldb_loop_size_outtoub_ascend, lldb_loop1_stride_outtoub_ascend, lldb_loop2_stride_outtoub_ascend,
  lldb_loop_size_ubtoout_ascend, lldb_loop1_stride_ubtoout_ascend, lldb_loop2_stride_ubtoout_ascend,
  lldb_mte2_nz_para_ascend, lldb_mte2_src_para_ascend, lldb_pad_cnt_nddma_ascend,
  lldb_pad_val_nddma_ascend, lldb_loop0_stride_nddma_ascend, lldb_loop1_stride_nddma_ascend,
  lldb_loop2_stride_nddma_ascend, lldb_loop3_stride_nddma_ascend, lldb_loop4_stride_nddma_ascend,
  lldb_pcie_rd_ctrl_ascend, lldb_pcie_wr_ctrl_ascend,
  k_last_mte_reg_ascend = lldb_pcie_wr_ctrl_ascend,
  lldb_v0_ascend, lldb_v1_ascend, lldb_v2_ascend,
  lldb_v3_ascend, lldb_v4_ascend, lldb_v5_ascend, lldb_v6_ascend,
  lldb_v7_ascend, lldb_v8_ascend, lldb_v9_ascend, lldb_v10_ascend,
  lldb_v11_ascend, lldb_v12_ascend, lldb_v13_ascend, lldb_v14_ascend,
  lldb_v15_ascend, lldb_v16_ascend, lldb_v17_ascend, lldb_v18_ascend,
  lldb_v19_ascend, lldb_v20_ascend, lldb_v21_ascend, lldb_v22_ascend,
  lldb_v23_ascend, lldb_v24_ascend, lldb_v25_ascend, lldb_v26_ascend,
  lldb_v27_ascend, lldb_v28_ascend, lldb_v29_ascend, lldb_v30_ascend,
  lldb_v31_ascend, lldb_p0_ascend, lldb_p1_ascend, lldb_p2_ascend,
  lldb_p3_ascend, lldb_p4_ascend, lldb_p5_ascend, lldb_p6_ascend,
  lldb_p7_ascend, lldb_uld0_ascend, lldb_uld1_ascend, lldb_uld2_ascend,
  lldb_uld3_ascend, lldb_ust0_ascend, lldb_ust1_ascend, lldb_ust2_ascend,
  lldb_ust3_ascend, lldb_ust_flag_0_ascend, lldb_ust_flag_1_ascend, lldb_ust_flag_2_ascend,
  lldb_ust_flag_3_ascend,
  // 32 bit
  lldb_sreg_first,
  lldb_tpes0_ascend = lldb_sreg_first, lldb_tpes1_ascend, lldb_tpes2_ascend, lldb_tpes3_ascend,
  lldb_tpes4_ascend, lldb_tpes5_ascend, lldb_tpes6_ascend, lldb_tpes7_ascend,
  lldb_tpes8_ascend, lldb_tpes9_ascend, lldb_tpes10_ascend, lldb_tpes11_ascend,
  lldb_tpes12_ascend, lldb_tpes13_ascend, lldb_tpes14_ascend, lldb_tpes15_ascend,
  lldb_tpes16_ascend, lldb_tpes17_ascend, lldb_tpes18_ascend, lldb_tpes19_ascend,
  lldb_tpes20_ascend, lldb_tpes21_ascend, lldb_tpes22_ascend, lldb_tpes23_ascend,
  lldb_tpes24_ascend, lldb_tpes25_ascend, lldb_tpes26_ascend, lldb_tpes27_ascend,
  lldb_tpes28_ascend, lldb_tpes29_ascend, lldb_tpes30_ascend, lldb_tpes31_ascend,
  // 32bit
  lldb_s64_ascend, lldb_s65_ascend, lldb_s66_ascend, lldb_s67_ascend,
  lldb_s68_ascend, lldb_s69_ascend, lldb_s70_ascend, lldb_s71_ascend,
  lldb_s72_ascend, lldb_s73_ascend, lldb_s74_ascend, lldb_s75_ascend,
  lldb_s76_ascend, lldb_s77_ascend, lldb_s78_ascend, lldb_s79_ascend,
  lldb_s80_ascend, lldb_s81_ascend, lldb_s82_ascend, lldb_s83_ascend,
  lldb_s84_ascend, lldb_s85_ascend, lldb_s86_ascend, lldb_s87_ascend,
  lldb_s88_ascend, lldb_s89_ascend, lldb_s90_ascend, lldb_s91_ascend,
  lldb_s92_ascend, lldb_s93_ascend, lldb_s94_ascend, lldb_s95_ascend,
  // 16bit, equal tpess0~tpess63
  lldb_s0_ascend, lldb_s1_ascend, lldb_s2_ascend, lldb_s3_ascend,
  lldb_s4_ascend, lldb_s5_ascend, lldb_s6_ascend, lldb_s7_ascend,
  lldb_s8_ascend, lldb_s9_ascend, lldb_s10_ascend, lldb_s11_ascend,
  lldb_s12_ascend, lldb_s13_ascend, lldb_s14_ascend, lldb_s15_ascend,
  lldb_s16_ascend, lldb_s17_ascend, lldb_s18_ascend, lldb_s19_ascend,
  lldb_s20_ascend, lldb_s21_ascend, lldb_s22_ascend, lldb_s23_ascend,
  lldb_s24_ascend, lldb_s25_ascend, lldb_s26_ascend, lldb_s27_ascend,
  lldb_s28_ascend, lldb_s29_ascend, lldb_s30_ascend, lldb_s31_ascend,
  lldb_s32_ascend, lldb_s33_ascend, lldb_s34_ascend, lldb_s35_ascend,
  lldb_s36_ascend, lldb_s37_ascend, lldb_s38_ascend, lldb_s39_ascend,
  lldb_s40_ascend, lldb_s41_ascend, lldb_s42_ascend, lldb_s43_ascend,
  lldb_s44_ascend, lldb_s45_ascend, lldb_s46_ascend, lldb_s47_ascend,
  lldb_s48_ascend, lldb_s49_ascend, lldb_s50_ascend, lldb_s51_ascend,
  lldb_s52_ascend, lldb_s53_ascend, lldb_s54_ascend, lldb_s55_ascend,
  lldb_s56_ascend, lldb_s57_ascend, lldb_s58_ascend, lldb_s59_ascend,
  lldb_s60_ascend, lldb_s61_ascend, lldb_s62_ascend, lldb_s63_ascend,
  // 64bit
  lldb_tpesl0_ascend, lldb_tpesl1_ascend, lldb_tpesl2_ascend, lldb_tpesl3_ascend,
  lldb_tpesl4_ascend, lldb_tpesl5_ascend, lldb_tpesl6_ascend, lldb_tpesl7_ascend,
  lldb_tpesl8_ascend, lldb_tpesl9_ascend, lldb_tpesl10_ascend, lldb_tpesl11_ascend,
  lldb_tpesl12_ascend, lldb_tpesl13_ascend, lldb_tpesl14_ascend, lldb_tpesl15_ascend,
  // 128bit
  lldb_tpesll0_ascend, lldb_tpesll1_ascend, lldb_tpesll2_ascend, lldb_tpesll3_ascend,
  lldb_tpesll4_ascend, lldb_tpesll5_ascend, lldb_tpesll6_ascend, lldb_tpesll7_ascend,
  lldb_sreg_last = lldb_tpesll7_ascend,

  lldb_va0_ascend, lldb_va1_ascend, lldb_va2_ascend,
  lldb_va3_ascend, lldb_va4_ascend, lldb_va5_ascend, lldb_va6_ascend,
  lldb_va7_ascend, lldb_a0_ascend, lldb_a1_ascend, lldb_a2_ascend,
  lldb_a3_ascend, lldb_a4_ascend, lldb_a5_ascend, lldb_a6_ascend,
  lldb_a7_ascend, lldb_mask_ascend, lldb_ar_ascend, lldb_vms4_sr_ascend,
  lldb_sqzn_ascend, lldb_vpc_ascend, lldb_vl_ascend, lldb_agu_am_const_0_ascend,
  lldb_agu_am_const_1_ascend, lldb_agu_am_const_2_ascend, lldb_agu_am_const_3_ascend, lldb_agu_am_const_4_ascend,
  lldb_agu_am_const_5_ascend, lldb_agu_am_const_6_ascend, lldb_agu_am_const_7_ascend, lldb_agu_am_const_8_ascend,
  lldb_agu_am_const_9_ascend, lldb_agu_am_const_10_ascend, lldb_agu_am_const_11_ascend, lldb_agu_am_const_12_ascend,
  lldb_agu_am_const_13_ascend, lldb_agu_am_const_14_ascend, lldb_agu_am_const_15_ascend, lldb_agu_am_const_16_ascend,
  lldb_agu_am_const_17_ascend, lldb_agu_am_const_18_ascend, lldb_agu_am_const_19_ascend, lldb_agu_am_const_20_ascend,
  lldb_agu_am_const_21_ascend, lldb_agu_am_const_22_ascend, lldb_agu_am_const_23_ascend, lldb_agu_am_const_24_ascend,
  lldb_agu_am_const_25_ascend, lldb_agu_am_const_26_ascend, lldb_agu_am_const_27_ascend, lldb_agu_am_const_28_ascend,
  lldb_agu_am_const_29_ascend, lldb_agu_am_const_30_ascend, lldb_agu_am_const_31_ascend, lldb_agu_am_const_32_ascend,
  lldb_agu_am_const_33_ascend, lldb_agu_am_const_34_ascend, lldb_agu_am_const_35_ascend, lldb_agu_am_const_36_ascend,
  lldb_agu_am_const_37_ascend, lldb_agu_am_const_38_ascend, lldb_agu_am_const_39_ascend, lldb_agu_am_const_40_ascend,
  lldb_agu_am_const_41_ascend, lldb_agu_am_const_42_ascend, lldb_agu_am_const_43_ascend, lldb_agu_am_const_44_ascend,
  lldb_agu_am_const_45_ascend, lldb_agu_am_const_46_ascend, lldb_agu_am_const_47_ascend, lldb_agu_am_const_48_ascend,
  lldb_agu_am_const_49_ascend, lldb_agu_am_const_50_ascend, lldb_agu_am_const_51_ascend, lldb_agu_am_const_52_ascend,
  lldb_agu_am_const_53_ascend, lldb_agu_am_const_54_ascend, lldb_agu_am_const_55_ascend, lldb_agu_am_const_56_ascend,
  lldb_agu_am_const_57_ascend, lldb_agu_am_const_58_ascend, lldb_agu_am_const_59_ascend, lldb_agu_am_const_60_ascend,
  lldb_agu_am_const_61_ascend, lldb_agu_am_const_62_ascend, lldb_agu_am_const_63_ascend, lldb_ust_ele_ofs_0_ascend,
  lldb_ust_ele_ofs_1_ascend, lldb_ust_ele_ofs_2_ascend, lldb_ust_ele_ofs_3_ascend, lldb_core_id_ascend,
  lldb_vec_core_id_ascend, lldb_vthreaddim_x_ascend, lldb_vthreaddim_y_ascend, lldb_vthreaddim_z_ascend,
  lldb_simt_pc_ascend, lldb_su_pc_ascend, lldb_p_register_ascend, lldb_execmask_ascend,
  lldb_r0_ascend, lldb_r1_ascend, lldb_r2_ascend, lldb_r3_ascend,
  lldb_r4_ascend, lldb_r5_ascend, lldb_r6_ascend, lldb_r7_ascend,
  lldb_r8_ascend, lldb_r9_ascend, lldb_r10_ascend, lldb_r11_ascend,
  lldb_r12_ascend, lldb_r13_ascend, lldb_r14_ascend, lldb_r15_ascend,
  lldb_r16_ascend, lldb_r17_ascend, lldb_r18_ascend, lldb_r19_ascend,
  lldb_r20_ascend, lldb_r21_ascend, lldb_r22_ascend, lldb_r23_ascend,
  lldb_r24_ascend, lldb_r25_ascend, lldb_r26_ascend, lldb_r27_ascend,
  lldb_r28_ascend, lldb_r29_ascend, lldb_r30_ascend, lldb_r31_ascend,
  lldb_r32_ascend, lldb_r33_ascend, lldb_r34_ascend, lldb_r35_ascend,
  lldb_r36_ascend, lldb_r37_ascend, lldb_r38_ascend, lldb_r39_ascend,
  lldb_r40_ascend, lldb_r41_ascend, lldb_r42_ascend, lldb_r43_ascend,
  lldb_r44_ascend, lldb_r45_ascend, lldb_r46_ascend, lldb_r47_ascend,
  lldb_r48_ascend, lldb_r49_ascend, lldb_r50_ascend, lldb_r51_ascend,
  lldb_r52_ascend, lldb_r53_ascend, lldb_r54_ascend, lldb_r55_ascend,
  lldb_r56_ascend, lldb_r57_ascend, lldb_r58_ascend, lldb_r59_ascend,
  lldb_r60_ascend, lldb_r61_ascend, lldb_r62_ascend, lldb_r63_ascend,
  lldb_r64_ascend, lldb_r65_ascend, lldb_r66_ascend, lldb_r67_ascend,
  lldb_r68_ascend, lldb_r69_ascend, lldb_r70_ascend, lldb_r71_ascend,
  lldb_r72_ascend, lldb_r73_ascend, lldb_r74_ascend, lldb_r75_ascend,
  lldb_r76_ascend, lldb_r77_ascend, lldb_r78_ascend, lldb_r79_ascend,
  lldb_r80_ascend, lldb_r81_ascend, lldb_r82_ascend, lldb_r83_ascend,
  lldb_r84_ascend, lldb_r85_ascend, lldb_r86_ascend, lldb_r87_ascend,
  lldb_r88_ascend, lldb_r89_ascend, lldb_r90_ascend, lldb_r91_ascend,
  lldb_r92_ascend, lldb_r93_ascend, lldb_r94_ascend, lldb_r95_ascend,
  lldb_r96_ascend, lldb_r97_ascend, lldb_r98_ascend, lldb_r99_ascend,
  lldb_r100_ascend, lldb_r101_ascend, lldb_r102_ascend, lldb_r103_ascend,
  lldb_r104_ascend, lldb_r105_ascend, lldb_r106_ascend, lldb_r107_ascend,
  lldb_r108_ascend, lldb_r109_ascend, lldb_r110_ascend, lldb_r111_ascend,
  lldb_r112_ascend, lldb_r113_ascend, lldb_r114_ascend, lldb_r115_ascend,
  lldb_r116_ascend, lldb_r117_ascend, lldb_r118_ascend, lldb_r119_ascend,
  lldb_r120_ascend, lldb_r121_ascend, lldb_r122_ascend, lldb_r123_ascend,
  lldb_r124_ascend, lldb_r125_ascend, lldb_r126_ascend,
  k_last_vecrb_reg_ascend = lldb_r126_ascend,
  lldb_fmatrix_ascend, lldb_padding_l1_ascend, lldb_l0_set_value_l1_ascend, lldb_l1_3d_size_ascend,
  lldb_l3d_rpt_ascend, lldb_fmatrix_dual_0_ascend, lldb_fmatrix_dual_1_ascend, lldb_fmatrix_b_ascend,
  lldb_l3d_rpt_b_ascend, lldb_padding_b_ascend, lldb_relu_alpha_ascend, lldb_quant_post_ascend,
  lldb_fpc_ascend, lldb_quant_pre_ascend, lldb_elt_src_para_ascend, lldb_loop3_para_ascend,
  lldb_fix_clip_relu_ascend, lldb_loop4_para_ascend, lldb_channel_para_ascend, lldb_elt_antiq_para_ascend,
  lldb_l1_warn_ascend,
  k_last_l1_reg_ascend = lldb_l1_warn_ascend,
  k_num_dbg_registers_ascend,
  // Above registers is used for both coredump and onboard.
  // If you add a new shared register, please insert it before this line.
  lldb_sc_error_t0_0 = k_num_dbg_registers_ascend,
  lldb_su_error_t0_0,
  lldb_mte_error_t0_0,
  lldb_mte_error_t1_0,
  lldb_vec_error_t0_0,
  lldb_vec_error_t0_2,
  lldb_cube_error_t0_0,
  lldb_cube_error_t0_1,
  lldb_l1_error_t0_0,
  lldb_l1_error_t0_1,

  lldb_sc_err_info_t0_0,
  lldb_sc_err_info_t0_1,

  lldb_su_err_info_t0_0,
  lldb_su_err_info_t0_1,
  lldb_su_err_info_t0_2,
  lldb_su_err_info_t0_3,

  lldb_mte_err_info_t0_0,
  lldb_mte_err_info_t0_1,
  lldb_mte_err_info_t0_2,
  lldb_mte_err_info_t1_0,
  lldb_mte_err_info_t1_1,
  lldb_mte_err_info_t1_2,

  lldb_vec_err_info_t0_0,
  lldb_vec_err_info_t0_1,
  lldb_vec_err_info_t0_2,
  lldb_vec_err_info_t0_3,
  lldb_vec_err_info_t0_4,
  lldb_vec_err_info_t0_5,

  lldb_cube_err_info_t0_0,
  lldb_cube_err_info_t0_1,

  lldb_l1_err_info_t0_0,
  lldb_l1_err_info_t0_1,
  k_num_registers_ascend
};

enum {
  k_num_gpr_registers = k_last_gpr_ascend - k_first_gpr_ascend + 1,
  k_num_su_registers = k_last_su_reg_ascend + 1,
  k_num_mte_registers = k_last_mte_reg_ascend - k_last_su_reg_ascend,
  k_num_vecrb_registers = k_last_vecrb_reg_ascend - k_last_mte_reg_ascend,
  k_num_l1_registers = k_last_l1_reg_ascend - k_last_vecrb_reg_ascend,
  k_num_aic_error_registers = lldb_l1_error_t0_1 - lldb_sc_error_t0_0 + 1,
  k_num_err_info_registers = lldb_l1_err_info_t0_1 - lldb_sc_err_info_t0_0 + 1,
  k_num_register_sets_default = 4,
  k_num_register_sets = 6
};

static const uint32_t g_su_regnums_ascend950[] = {
  lldb_x0_ascend, lldb_x1_ascend, lldb_x2_ascend, lldb_x3_ascend,
  lldb_x4_ascend, lldb_x5_ascend, lldb_x6_ascend, lldb_x7_ascend,
  lldb_x8_ascend, lldb_x9_ascend, lldb_x10_ascend, lldb_x11_ascend,
  lldb_x12_ascend, lldb_x13_ascend, lldb_x14_ascend, lldb_x15_ascend,
  lldb_x16_ascend, lldb_x17_ascend, lldb_x18_ascend, lldb_x19_ascend,
  lldb_x20_ascend, lldb_x21_ascend, lldb_x22_ascend, lldb_x23_ascend,
  lldb_x24_ascend, lldb_x25_ascend, lldb_x26_ascend, lldb_x27_ascend,
  lldb_x28_ascend, lldb_x29_ascend, lldb_x30_ascend, lldb_x31_ascend,
  lldb_pc_ascend, lldb_status_ascend, lldb_ctrl_su_ascend, lldb_condition_flag_ascend,
  lldb_cond_ascend, lldb_sys_cnt_ascend, lldb_safety_crc_excpt_ascend, lldb_call_depth_cnt_ascend,
  lldb_icache_prl_st_ascend, lldb_ffts_base_addr_ascend, lldb_kick_start_pc_ascend,
  lldb_su_warn_status_0_ascend, lldb_su_warn_status_1_ascend,
  lldb_pb_slot_offset_ascend, lldb_pb_slot_id_ascend, lldb_pb_slot_occupy_bitmap_ascend, lldb_vec_event_table_ascend,
  lldb_cube_event_table_ascend, lldb_fixp_event_table_ascend, lldb_scalar_event_table_ascend,
  lldb_mte1_event_table_ascend, lldb_mte2_event_table_ascend, lldb_mte3_event_table_ascend,
  lldb_glb_get_cnt_ascend, lldb_glb_rel_cnt_ascend, lldb_set_cross_core_cnt_ascend,
  lldb_set_intra_blk_cnt_ascend, LLDB_INVALID_REGNUM
};

static_assert(sizeof(g_su_regnums_ascend950) / sizeof(uint32_t) - 1 == k_num_su_registers,
              "Invalid size of g_su_regnums_ascend950");

static const uint32_t g_mte_regnums_ascend950[] = {
  lldb_mte_warn_ascend, lldb_l0_set_value_mte_ascend,
  lldb_padding_mte_ascend, lldb_pad_val_outtoub_ascend, lldb_pad_val_outtol1_ascend,
  lldb_loop_size_outtol1_ascend, lldb_loop1_stride_outtol1_ascend, lldb_loop2_stride_outtol1_ascend,
  lldb_loop_size_outtoub_ascend, lldb_loop1_stride_outtoub_ascend, lldb_loop2_stride_outtoub_ascend,
  lldb_loop_size_ubtoout_ascend, lldb_loop1_stride_ubtoout_ascend, lldb_loop2_stride_ubtoout_ascend,
  lldb_mte2_nz_para_ascend, lldb_mte2_src_para_ascend, lldb_pad_cnt_nddma_ascend,
  lldb_pad_val_nddma_ascend, lldb_loop0_stride_nddma_ascend, lldb_loop1_stride_nddma_ascend,
  lldb_loop2_stride_nddma_ascend, lldb_loop3_stride_nddma_ascend, lldb_loop4_stride_nddma_ascend,
  lldb_pcie_rd_ctrl_ascend, lldb_pcie_wr_ctrl_ascend, LLDB_INVALID_REGNUM
};

static_assert(sizeof(g_mte_regnums_ascend950) / sizeof(uint32_t) - 1 == k_num_mte_registers,
              "Invalid size of g_mte_regnums_ascend950");

static const uint32_t g_vecrb_regnums_ascend950[] = {
  lldb_v0_ascend, lldb_v1_ascend, lldb_v2_ascend,
  lldb_v3_ascend, lldb_v4_ascend, lldb_v5_ascend, lldb_v6_ascend,
  lldb_v7_ascend, lldb_v8_ascend, lldb_v9_ascend, lldb_v10_ascend,
  lldb_v11_ascend, lldb_v12_ascend, lldb_v13_ascend, lldb_v14_ascend,
  lldb_v15_ascend, lldb_v16_ascend, lldb_v17_ascend, lldb_v18_ascend,
  lldb_v19_ascend, lldb_v20_ascend, lldb_v21_ascend, lldb_v22_ascend,
  lldb_v23_ascend, lldb_v24_ascend, lldb_v25_ascend, lldb_v26_ascend,
  lldb_v27_ascend, lldb_v28_ascend, lldb_v29_ascend, lldb_v30_ascend,
  lldb_v31_ascend, lldb_p0_ascend, lldb_p1_ascend, lldb_p2_ascend,
  lldb_p3_ascend, lldb_p4_ascend, lldb_p5_ascend, lldb_p6_ascend,
  lldb_p7_ascend, lldb_uld0_ascend, lldb_uld1_ascend, lldb_uld2_ascend,
  lldb_uld3_ascend, lldb_ust0_ascend, lldb_ust1_ascend, lldb_ust2_ascend,
  lldb_ust3_ascend, lldb_ust_flag_0_ascend, lldb_ust_flag_1_ascend, lldb_ust_flag_2_ascend,
  lldb_ust_flag_3_ascend,
  // 32bt
  lldb_tpes0_ascend, lldb_tpes1_ascend, lldb_tpes2_ascend, lldb_tpes3_ascend,
  lldb_tpes4_ascend, lldb_tpes5_ascend, lldb_tpes6_ascend, lldb_tpes7_ascend,
  lldb_tpes8_ascend, lldb_tpes9_ascend, lldb_tpes10_ascend, lldb_tpes11_ascend,
  lldb_tpes12_ascend, lldb_tpes13_ascend, lldb_tpes14_ascend, lldb_tpes15_ascend,
  lldb_tpes16_ascend, lldb_tpes17_ascend, lldb_tpes18_ascend, lldb_tpes19_ascend,
  lldb_tpes20_ascend, lldb_tpes21_ascend, lldb_tpes22_ascend, lldb_tpes23_ascend,
  lldb_tpes24_ascend, lldb_tpes25_ascend, lldb_tpes26_ascend, lldb_tpes27_ascend,
  lldb_tpes28_ascend, lldb_tpes29_ascend, lldb_tpes30_ascend, lldb_tpes31_ascend,
  // 32bit
  lldb_s64_ascend, lldb_s65_ascend, lldb_s66_ascend, lldb_s67_ascend,
  lldb_s68_ascend, lldb_s69_ascend, lldb_s70_ascend, lldb_s71_ascend,
  lldb_s72_ascend, lldb_s73_ascend, lldb_s74_ascend, lldb_s75_ascend,
  lldb_s76_ascend, lldb_s77_ascend, lldb_s78_ascend, lldb_s79_ascend,
  lldb_s80_ascend, lldb_s81_ascend, lldb_s82_ascend, lldb_s83_ascend,
  lldb_s84_ascend, lldb_s85_ascend, lldb_s86_ascend, lldb_s87_ascend,
  lldb_s88_ascend, lldb_s89_ascend, lldb_s90_ascend, lldb_s91_ascend,
  lldb_s92_ascend, lldb_s93_ascend, lldb_s94_ascend, lldb_s95_ascend,
  // 16bit, equal tpess0~tpess63
  lldb_s0_ascend, lldb_s1_ascend, lldb_s2_ascend, lldb_s3_ascend,
  lldb_s4_ascend, lldb_s5_ascend, lldb_s6_ascend, lldb_s7_ascend,
  lldb_s8_ascend, lldb_s9_ascend, lldb_s10_ascend, lldb_s11_ascend,
  lldb_s12_ascend, lldb_s13_ascend, lldb_s14_ascend, lldb_s15_ascend,
  lldb_s16_ascend, lldb_s17_ascend, lldb_s18_ascend, lldb_s19_ascend,
  lldb_s20_ascend, lldb_s21_ascend, lldb_s22_ascend, lldb_s23_ascend,
  lldb_s24_ascend, lldb_s25_ascend, lldb_s26_ascend, lldb_s27_ascend,
  lldb_s28_ascend, lldb_s29_ascend, lldb_s30_ascend, lldb_s31_ascend,
  lldb_s32_ascend, lldb_s33_ascend, lldb_s34_ascend, lldb_s35_ascend,
  lldb_s36_ascend, lldb_s37_ascend, lldb_s38_ascend, lldb_s39_ascend,
  lldb_s40_ascend, lldb_s41_ascend, lldb_s42_ascend, lldb_s43_ascend,
  lldb_s44_ascend, lldb_s45_ascend, lldb_s46_ascend, lldb_s47_ascend,
  lldb_s48_ascend, lldb_s49_ascend, lldb_s50_ascend, lldb_s51_ascend,
  lldb_s52_ascend, lldb_s53_ascend, lldb_s54_ascend, lldb_s55_ascend,
  lldb_s56_ascend, lldb_s57_ascend, lldb_s58_ascend, lldb_s59_ascend,
  lldb_s60_ascend, lldb_s61_ascend, lldb_s62_ascend, lldb_s63_ascend,
  // 64bit
  lldb_tpesl0_ascend, lldb_tpesl1_ascend, lldb_tpesl2_ascend, lldb_tpesl3_ascend,
  lldb_tpesl4_ascend, lldb_tpesl5_ascend, lldb_tpesl6_ascend, lldb_tpesl7_ascend,
  lldb_tpesl8_ascend, lldb_tpesl9_ascend, lldb_tpesl10_ascend, lldb_tpesl11_ascend,
  lldb_tpesl12_ascend, lldb_tpesl13_ascend, lldb_tpesl14_ascend, lldb_tpesl15_ascend,
  // 128bit
  lldb_tpesll0_ascend, lldb_tpesll1_ascend, lldb_tpesll2_ascend, lldb_tpesll3_ascend,
  lldb_tpesll4_ascend, lldb_tpesll5_ascend, lldb_tpesll6_ascend, lldb_tpesll7_ascend,

  lldb_va0_ascend, lldb_va1_ascend, lldb_va2_ascend,
  lldb_va3_ascend, lldb_va4_ascend, lldb_va5_ascend, lldb_va6_ascend,
  lldb_va7_ascend, lldb_a0_ascend, lldb_a1_ascend, lldb_a2_ascend,
  lldb_a3_ascend, lldb_a4_ascend, lldb_a5_ascend, lldb_a6_ascend,
  lldb_a7_ascend, lldb_mask_ascend, lldb_ar_ascend, lldb_vms4_sr_ascend,
  lldb_sqzn_ascend, lldb_vpc_ascend, lldb_vl_ascend, lldb_agu_am_const_0_ascend,
  lldb_agu_am_const_1_ascend, lldb_agu_am_const_2_ascend, lldb_agu_am_const_3_ascend, lldb_agu_am_const_4_ascend,
  lldb_agu_am_const_5_ascend, lldb_agu_am_const_6_ascend, lldb_agu_am_const_7_ascend, lldb_agu_am_const_8_ascend,
  lldb_agu_am_const_9_ascend, lldb_agu_am_const_10_ascend, lldb_agu_am_const_11_ascend, lldb_agu_am_const_12_ascend,
  lldb_agu_am_const_13_ascend, lldb_agu_am_const_14_ascend, lldb_agu_am_const_15_ascend, lldb_agu_am_const_16_ascend,
  lldb_agu_am_const_17_ascend, lldb_agu_am_const_18_ascend, lldb_agu_am_const_19_ascend, lldb_agu_am_const_20_ascend,
  lldb_agu_am_const_21_ascend, lldb_agu_am_const_22_ascend, lldb_agu_am_const_23_ascend, lldb_agu_am_const_24_ascend,
  lldb_agu_am_const_25_ascend, lldb_agu_am_const_26_ascend, lldb_agu_am_const_27_ascend, lldb_agu_am_const_28_ascend,
  lldb_agu_am_const_29_ascend, lldb_agu_am_const_30_ascend, lldb_agu_am_const_31_ascend, lldb_agu_am_const_32_ascend,
  lldb_agu_am_const_33_ascend, lldb_agu_am_const_34_ascend, lldb_agu_am_const_35_ascend, lldb_agu_am_const_36_ascend,
  lldb_agu_am_const_37_ascend, lldb_agu_am_const_38_ascend, lldb_agu_am_const_39_ascend, lldb_agu_am_const_40_ascend,
  lldb_agu_am_const_41_ascend, lldb_agu_am_const_42_ascend, lldb_agu_am_const_43_ascend, lldb_agu_am_const_44_ascend,
  lldb_agu_am_const_45_ascend, lldb_agu_am_const_46_ascend, lldb_agu_am_const_47_ascend, lldb_agu_am_const_48_ascend,
  lldb_agu_am_const_49_ascend, lldb_agu_am_const_50_ascend, lldb_agu_am_const_51_ascend, lldb_agu_am_const_52_ascend,
  lldb_agu_am_const_53_ascend, lldb_agu_am_const_54_ascend, lldb_agu_am_const_55_ascend, lldb_agu_am_const_56_ascend,
  lldb_agu_am_const_57_ascend, lldb_agu_am_const_58_ascend, lldb_agu_am_const_59_ascend, lldb_agu_am_const_60_ascend,
  lldb_agu_am_const_61_ascend, lldb_agu_am_const_62_ascend, lldb_agu_am_const_63_ascend, lldb_ust_ele_ofs_0_ascend,
  lldb_ust_ele_ofs_1_ascend, lldb_ust_ele_ofs_2_ascend, lldb_ust_ele_ofs_3_ascend, lldb_core_id_ascend,
  lldb_vec_core_id_ascend, lldb_vthreaddim_x_ascend, lldb_vthreaddim_y_ascend, lldb_vthreaddim_z_ascend,
  lldb_simt_pc_ascend, lldb_su_pc_ascend, lldb_p_register_ascend, lldb_execmask_ascend,
  lldb_r0_ascend, lldb_r1_ascend, lldb_r2_ascend, lldb_r3_ascend,
  lldb_r4_ascend, lldb_r5_ascend, lldb_r6_ascend, lldb_r7_ascend,
  lldb_r8_ascend, lldb_r9_ascend, lldb_r10_ascend, lldb_r11_ascend,
  lldb_r12_ascend, lldb_r13_ascend, lldb_r14_ascend, lldb_r15_ascend,
  lldb_r16_ascend, lldb_r17_ascend, lldb_r18_ascend, lldb_r19_ascend,
  lldb_r20_ascend, lldb_r21_ascend, lldb_r22_ascend, lldb_r23_ascend,
  lldb_r24_ascend, lldb_r25_ascend, lldb_r26_ascend, lldb_r27_ascend,
  lldb_r28_ascend, lldb_r29_ascend, lldb_r30_ascend, lldb_r31_ascend,
  lldb_r32_ascend, lldb_r33_ascend, lldb_r34_ascend, lldb_r35_ascend,
  lldb_r36_ascend, lldb_r37_ascend, lldb_r38_ascend, lldb_r39_ascend,
  lldb_r40_ascend, lldb_r41_ascend, lldb_r42_ascend, lldb_r43_ascend,
  lldb_r44_ascend, lldb_r45_ascend, lldb_r46_ascend, lldb_r47_ascend,
  lldb_r48_ascend, lldb_r49_ascend, lldb_r50_ascend, lldb_r51_ascend,
  lldb_r52_ascend, lldb_r53_ascend, lldb_r54_ascend, lldb_r55_ascend,
  lldb_r56_ascend, lldb_r57_ascend, lldb_r58_ascend, lldb_r59_ascend,
  lldb_r60_ascend, lldb_r61_ascend, lldb_r62_ascend, lldb_r63_ascend,
  lldb_r64_ascend, lldb_r65_ascend, lldb_r66_ascend, lldb_r67_ascend,
  lldb_r68_ascend, lldb_r69_ascend, lldb_r70_ascend, lldb_r71_ascend,
  lldb_r72_ascend, lldb_r73_ascend, lldb_r74_ascend, lldb_r75_ascend,
  lldb_r76_ascend, lldb_r77_ascend, lldb_r78_ascend, lldb_r79_ascend,
  lldb_r80_ascend, lldb_r81_ascend, lldb_r82_ascend, lldb_r83_ascend,
  lldb_r84_ascend, lldb_r85_ascend, lldb_r86_ascend, lldb_r87_ascend,
  lldb_r88_ascend, lldb_r89_ascend, lldb_r90_ascend, lldb_r91_ascend,
  lldb_r92_ascend, lldb_r93_ascend, lldb_r94_ascend, lldb_r95_ascend,
  lldb_r96_ascend, lldb_r97_ascend, lldb_r98_ascend, lldb_r99_ascend,
  lldb_r100_ascend, lldb_r101_ascend, lldb_r102_ascend, lldb_r103_ascend,
  lldb_r104_ascend, lldb_r105_ascend, lldb_r106_ascend, lldb_r107_ascend,
  lldb_r108_ascend, lldb_r109_ascend, lldb_r110_ascend, lldb_r111_ascend,
  lldb_r112_ascend, lldb_r113_ascend, lldb_r114_ascend, lldb_r115_ascend,
  lldb_r116_ascend, lldb_r117_ascend, lldb_r118_ascend, lldb_r119_ascend,
  lldb_r120_ascend, lldb_r121_ascend, lldb_r122_ascend, lldb_r123_ascend,
  lldb_r124_ascend, lldb_r125_ascend, lldb_r126_ascend,
  LLDB_INVALID_REGNUM
};

static_assert(sizeof(g_vecrb_regnums_ascend950) / sizeof(uint32_t) - 1 == k_num_vecrb_registers,
              "Invalid size of g_vecrb_regnums_ascend950");

static const uint32_t g_l1_regnums_ascend950[] = {
  lldb_fmatrix_ascend, lldb_padding_l1_ascend, lldb_l0_set_value_l1_ascend, lldb_l1_3d_size_ascend,
  lldb_l3d_rpt_ascend, lldb_fmatrix_dual_0_ascend, lldb_fmatrix_dual_1_ascend, lldb_fmatrix_b_ascend,
  lldb_l3d_rpt_b_ascend, lldb_padding_b_ascend, lldb_relu_alpha_ascend, lldb_quant_post_ascend,
  lldb_fpc_ascend, lldb_quant_pre_ascend, lldb_elt_src_para_ascend, lldb_loop3_para_ascend,
  lldb_fix_clip_relu_ascend, lldb_loop4_para_ascend, lldb_channel_para_ascend, lldb_elt_antiq_para_ascend,
  lldb_l1_warn_ascend, LLDB_INVALID_REGNUM
};

static const uint32_t g_aic_error_regnums[] = {
  lldb_sc_error_t0_0,
  lldb_su_error_t0_0, lldb_mte_error_t0_0, lldb_mte_error_t1_0,
  lldb_vec_error_t0_0, lldb_vec_error_t0_2, lldb_cube_error_t0_0,
  lldb_cube_error_t0_1, lldb_l1_error_t0_0, lldb_l1_error_t0_1,
};

static_assert(sizeof(g_aic_error_regnums) / sizeof(uint32_t) == k_num_aic_error_registers,
              "Invalid size of g_aic_error_regnums");

static const uint32_t g_err_info_regnums[] = {
  lldb_sc_err_info_t0_0, lldb_sc_err_info_t0_1,

  lldb_su_err_info_t0_0, lldb_su_err_info_t0_1, lldb_su_err_info_t0_2, lldb_su_err_info_t0_3,

  lldb_mte_err_info_t0_0, lldb_mte_err_info_t0_1, lldb_mte_err_info_t0_2,
  lldb_mte_err_info_t1_0, lldb_mte_err_info_t1_1, lldb_mte_err_info_t1_2,

  lldb_vec_err_info_t0_0, lldb_vec_err_info_t0_1, lldb_vec_err_info_t0_2,
  lldb_vec_err_info_t0_3, lldb_vec_err_info_t0_4, lldb_vec_err_info_t0_5,

  lldb_cube_err_info_t0_0, lldb_cube_err_info_t0_1,

  lldb_l1_err_info_t0_0, lldb_l1_err_info_t0_1,
};

static_assert(sizeof(g_err_info_regnums) / sizeof(uint32_t) == k_num_err_info_registers,
              "Invalid size of g_err_info_regnums");


static const RegisterSet g_reg_sets_ascend950[k_num_register_sets_default] = {
  {"SURegisters", "su_dbg_reg", k_num_su_registers, g_su_regnums_ascend950},
  {"MTERegisters", "mte_dbg_reg", k_num_mte_registers, g_mte_regnums_ascend950},
  {"VECRegisters", "vec_dbg_reg", k_num_vecrb_registers, g_vecrb_regnums_ascend950},
  {"L1Registers", "l1_dbg_reg", k_num_l1_registers, g_l1_regnums_ascend950}
};

static constexpr uint64_t SU_ID = 1UL;
static constexpr uint64_t MTE_ID = 2UL;
static constexpr uint64_t VEC_ID = 3UL;
static constexpr uint64_t L1_ID = 6UL;
static constexpr uint64_t ID_OFFSET = 52UL;

// The array should be POD type.
static const DeviceRegisterInfo REGISTER_950_INFO[] = {
  {ASCEND_GPR(GPR0, x0, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 0, MIX_MASK},
  {ASCEND_GPR(GPR1, x1, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 1, MIX_MASK},
  {ASCEND_GPR(GPR2, x2, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 2, MIX_MASK},
  {ASCEND_GPR(GPR3, x3, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 3, MIX_MASK},
  {ASCEND_GPR(GPR4, x4, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 4, MIX_MASK},
  {ASCEND_GPR(GPR5, x5, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 5, MIX_MASK},
  {ASCEND_GPR(GPR6, x6, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 6, MIX_MASK},
  {ASCEND_GPR(GPR7, x7, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 7, MIX_MASK},
  {ASCEND_GPR(GPR8, x8, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 8, MIX_MASK},
  {ASCEND_GPR(GPR9, x9, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 9, MIX_MASK},
  {ASCEND_GPR(GPR10, x10, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 10, MIX_MASK},
  {ASCEND_GPR(GPR11, x11, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 11, MIX_MASK},
  {ASCEND_GPR(GPR12, x12, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 12, MIX_MASK},
  {ASCEND_GPR(GPR13, x13, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 13, MIX_MASK},
  {ASCEND_GPR(GPR14, x14, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 14, MIX_MASK},
  {ASCEND_GPR(GPR15, x15, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 15, MIX_MASK},
  {ASCEND_GPR(GPR16, x16, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 16, MIX_MASK},
  {ASCEND_GPR(GPR17, x17, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 17, MIX_MASK},
  {ASCEND_GPR(GPR18, x18, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 18, MIX_MASK},
  {ASCEND_GPR(GPR19, x19, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 19, MIX_MASK},
  {ASCEND_GPR(GPR20, x20, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 20, MIX_MASK},
  {ASCEND_GPR(GPR21, x21, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 21, MIX_MASK},
  {ASCEND_GPR(GPR22, x22, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 22, MIX_MASK},
  {ASCEND_GPR(GPR23, x23, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 23, MIX_MASK},
  {ASCEND_GPR(GPR24, x24, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 24, MIX_MASK},
  {ASCEND_GPR(GPR25, x25, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 25, MIX_MASK},
  {ASCEND_GPR(GPR26, x26, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 26, MIX_MASK},
  {ASCEND_GPR(GPR27, x27, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 27, MIX_MASK},
  {ASCEND_GPR(GPR28, x28, LLDB_INVALID_REGNUM), SU_ID << ID_OFFSET | 28, MIX_MASK},
  {ASCEND_GPR_ALT(GPR29, x29, fp, LLDB_REGNUM_GENERIC_FP), SU_ID << ID_OFFSET | 29, MIX_MASK},
  {ASCEND_GPR_ALT(GPR30, x30, sp, LLDB_REGNUM_GENERIC_SP), SU_ID << ID_OFFSET | 30, MIX_MASK},
  {ASCEND_GPR_ALT(GPR31, x31, lr, LLDB_REGNUM_GENERIC_RA), SU_ID << ID_OFFSET | 31, MIX_MASK},
  {ASCEND_GPR(PC, pc, LLDB_REGNUM_GENERIC_PC), SU_ID << ID_OFFSET | 64, MIX_MASK},
  {ASCEND_REG(STATUS, lldb_status_ascend), SU_ID << ID_OFFSET | 65, MIX_MASK},
  {ASCEND_REG(CTRL_SU, lldb_ctrl_su_ascend), SU_ID << ID_OFFSET | 66, MIX_MASK},
  {ASCEND_REG(CONDITION_FLAG, lldb_condition_flag_ascend), SU_ID << ID_OFFSET | 67, MIX_MASK},
  {ASCEND_REG(COND, lldb_cond_ascend), SU_ID << ID_OFFSET | 68, MIX_MASK},
  {ASCEND_REG(SYS_CNT, lldb_sys_cnt_ascend), SU_ID << ID_OFFSET | 69, MIX_MASK},
  {ASCEND_REG(SAFETY_CRC_EXCPT, lldb_safety_crc_excpt_ascend), SU_ID << ID_OFFSET | 71, MIX_MASK},
  {ASCEND_REG(CALL_DEPTH_CNT, lldb_call_depth_cnt_ascend), SU_ID << ID_OFFSET | 72, MIX_MASK},
  {ASCEND_REG(ICACHE_PRL_ST, lldb_icache_prl_st_ascend), SU_ID << ID_OFFSET | 73, MIX_MASK},
  {ASCEND_REG(FFTS_BASE_ADDR, lldb_ffts_base_addr_ascend), SU_ID << ID_OFFSET | 75, MIX_MASK},
  {ASCEND_REG(KICK_START_PC, lldb_kick_start_pc_ascend), SU_ID << ID_OFFSET | 76, MIX_MASK},
  {ASCEND_REG(SU_WARN_STATUS_0, lldb_su_warn_status_0_ascend), SU_ID << ID_OFFSET | 77, MIX_MASK},
  {ASCEND_REG(SU_WARN_STATUS_1, lldb_su_warn_status_1_ascend), SU_ID << ID_OFFSET | 78, MIX_MASK},
  {ASCEND_REG(PB_SLOT_OFFSET, lldb_pb_slot_offset_ascend), SU_ID << ID_OFFSET | 128, MIX_MASK},
  {ASCEND_REG(PB_SLOT_ID, lldb_pb_slot_id_ascend), SU_ID << ID_OFFSET | 129, MIX_MASK},
  {ASCEND_REG(PB_SLOT_OCCUPY_BITMAP, lldb_pb_slot_occupy_bitmap_ascend), SU_ID << ID_OFFSET | 130, MIX_MASK},
  {ASCEND_REG(VEC_EVENT_TABLE, lldb_vec_event_table_ascend), SU_ID << ID_OFFSET | 131, MIX_MASK},
  {ASCEND_REG(CUBE_EVENT_TABLE, lldb_cube_event_table_ascend), SU_ID << ID_OFFSET | 132, MIX_MASK},
  {ASCEND_REG(FIXP_EVENT_TABLE, lldb_fixp_event_table_ascend), SU_ID << ID_OFFSET | 133, MIX_MASK},
  {ASCEND_REG(SCALAR_EVENT_TABLE, lldb_scalar_event_table_ascend), SU_ID << ID_OFFSET | 134, MIX_MASK},
  {ASCEND_REG(MTE1_EVENT_TABLE, lldb_mte1_event_table_ascend), SU_ID << ID_OFFSET | 135, MIX_MASK},
  {ASCEND_REG(MTE2_EVENT_TABLE, lldb_mte2_event_table_ascend), SU_ID << ID_OFFSET | 136, MIX_MASK},
  {ASCEND_REG(MTE3_EVENT_TABLE, lldb_mte3_event_table_ascend), SU_ID << ID_OFFSET | 137, MIX_MASK},
  {ASCEND_REG(GLB_GET_CNT, lldb_glb_get_cnt_ascend), SU_ID << ID_OFFSET | 5UL << 48, MIX_MASK},
  {ASCEND_REG(GLB_REL_CNT, lldb_glb_rel_cnt_ascend), SU_ID << ID_OFFSET | 5UL << 48 | 1UL << 47, MIX_MASK},
  {ASCEND_REG(SET_CROSS_CORE_CNT, lldb_set_cross_core_cnt_ascend), SU_ID << ID_OFFSET | 6UL << 48, MIX_MASK},
  {ASCEND_REG(SET_INTRA_BLK_CNT, lldb_set_intra_blk_cnt_ascend), SU_ID << ID_OFFSET | 6UL << 48 | 1UL << 47, MIX_MASK},

  {ASCEND_REG(MTE_WARN, lldb_mte_warn_ascend), MTE_ID << ID_OFFSET | 1UL << 48, MIX_MASK},
  {ASCEND_REG(L0_SET_VALUE_MTE, lldb_l0_set_value_mte_ascend), MTE_ID << ID_OFFSET | 2UL << 48, AIC_MASK},
  {ASCEND_REG(PADDING_MTE, lldb_padding_mte_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 1, AIC_MASK},
  {ASCEND_REG(PAD_VAL_OUTTOUB, lldb_pad_val_outtoub_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 2, AIV_MASK},
  {ASCEND_REG(PAD_VAL_OUTTOL1, lldb_pad_val_outtol1_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 3, AIC_MASK},
  {ASCEND_REG(LOOP_SIZE_OUTTOL1, lldb_loop_size_outtol1_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 4, AIC_MASK},
  {ASCEND_REG(LOOP1_STRIDE_OUTTOL1, lldb_loop1_stride_outtol1_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 5, AIC_MASK},
  {ASCEND_REG(LOOP2_STRIDE_OUTTOL1, lldb_loop2_stride_outtol1_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 6, AIC_MASK},
  {ASCEND_REG(LOOP_SIZE_OUTTOUB, lldb_loop_size_outtoub_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 7, AIV_MASK},
  {ASCEND_REG(LOOP1_STRIDE_OUTTOUB, lldb_loop1_stride_outtoub_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 8, AIV_MASK},
  {ASCEND_REG(LOOP2_STRIDE_OUTTOUB, lldb_loop2_stride_outtoub_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 9, AIV_MASK},
  {ASCEND_REG(LOOP_SIZE_UBTOOUT, lldb_loop_size_ubtoout_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 13, AIV_MASK},
  {ASCEND_REG(LOOP1_STRIDE_UBTOOUT, lldb_loop1_stride_ubtoout_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 14, AIV_MASK},
  {ASCEND_REG(LOOP2_STRIDE_UBTOOUT, lldb_loop2_stride_ubtoout_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 15, AIV_MASK},
  {ASCEND_REG(MTE2_NZ_PARA, lldb_mte2_nz_para_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 18, AIC_MASK},
  {ASCEND_REG(MTE2_SRC_PARA, lldb_mte2_src_para_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 20, AIC_MASK},
  {ASCEND_REG(PAD_CNT_NDDMA, lldb_pad_cnt_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 22, AIV_MASK},
  {ASCEND_REG(PAD_VAL_NDDMA, lldb_pad_val_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 23, AIV_MASK},
  {ASCEND_REG(LOOP0_STRIDE_NDDMA, lldb_loop0_stride_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 24, AIV_MASK},
  {ASCEND_REG(LOOP1_STRIDE_NDDMA, lldb_loop1_stride_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 25, AIV_MASK},
  {ASCEND_REG(LOOP2_STRIDE_NDDMA, lldb_loop2_stride_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 26, AIV_MASK},
  {ASCEND_REG(LOOP3_STRIDE_NDDMA, lldb_loop3_stride_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 27, AIV_MASK},
  {ASCEND_REG(LOOP4_STRIDE_NDDMA, lldb_loop4_stride_nddma_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 28, AIV_MASK},
  {ASCEND_REG(PCIE_RD_CTRL, lldb_pcie_rd_ctrl_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 29, AIV_MASK},
  {ASCEND_REG(PCIE_WR_CTRL, lldb_pcie_wr_ctrl_ascend), MTE_ID << ID_OFFSET | 2UL << 48 | 30, AIV_MASK},

  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V0, v0, 256), VEC_ID << ID_OFFSET | 2UL << 48, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V1, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 16, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V2, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 32, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V3, v3, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 48, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V4, v4, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 64, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V5, v5, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 80, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V6, v6, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 96, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V7, v7, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 112, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V8, v8, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 128, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V9, v9, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 144, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V10, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 160, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V11, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 176, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V12, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 192, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V13, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 208, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V14, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 224, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V15, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 240, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V16, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 256, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V17, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 272, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V18, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 288, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V19, v1, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 304, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V20, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 320, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V21, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 336, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V22, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 352, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V23, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 368, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V24, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 384, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V25, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 400, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V26, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 416, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V27, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 432, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V28, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 448, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V29, v2, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 464, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V30, v3, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 480, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC_FORMAT_UINT32(V31, v3, 256), VEC_ID << ID_OFFSET | 2UL << 48 | 496, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P0, p0, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 512, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P1, p1, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 514, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P2, p2, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 516, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P3, p3, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 518, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P4, p4, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 520, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P5, p5, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 522, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P6, p6, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 524, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(P7, p7, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 526, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(ULD0, uld0, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 1536, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(ULD1, uld1, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 1537, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(ULD2, uld2, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 1538, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(ULD3, uld3, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 1539, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(UST0, ust0, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 2048, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(UST1, ust1, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 2049, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(UST2, ust2, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 2050, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(UST3, ust3, 32), VEC_ID << ID_OFFSET | 2UL << 48 | 2051, AIV_MASK},
  {ASCEND_REG(UST_FLAG_0, lldb_ust_flag_0_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 2052, AIV_MASK},
  {ASCEND_REG(UST_FLAG_1, lldb_ust_flag_1_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 2053, AIV_MASK},
  {ASCEND_REG(UST_FLAG_2, lldb_ust_flag_2_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 2054, AIV_MASK},
  {ASCEND_REG(UST_FLAG_3, lldb_ust_flag_3_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 2055, AIV_MASK},
  // 32bit
  {ASCEND_GPR_NBYTES(TPES0, tpes0, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2560, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES1, tpes1, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2561, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES2, tpes2, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2562, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES3, tpes3, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2563, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES4, tpes4, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2564, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES5, tpes5, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2565, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES6, tpes6, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2566, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES7, tpes7, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2567, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES8, tpes8, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2568, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES9, tpes9, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2569, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES10, tpes10, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2570, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES11, tpes11, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2571, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES12, tpes12, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2572, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES13, tpes13, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2573, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES14, tpes14, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2574, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES15, tpes15, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2575, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES16, tpes16, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2576, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES17, tpes17, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2577, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES18, tpes18, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2578, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES19, tpes19, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2579, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES20, tpes20, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2580, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES21, tpes21, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2581, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES22, tpes22, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2582, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES23, tpes23, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2583, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES24, tpes24, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2584, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES25, tpes25, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2585, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES26, tpes26, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2586, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES27, tpes27, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2587, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES28, tpes28, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2588, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES29, tpes29, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2589, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES30, tpes30, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2590, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPES31, tpes31, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2591, AIV_MASK},
  {ASCEND_GPR_NBYTES(S64, s64, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2592, AIV_MASK},
  {ASCEND_GPR_NBYTES(S65, s65, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2593, AIV_MASK},
  {ASCEND_GPR_NBYTES(S66, s66, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2594, AIV_MASK},
  {ASCEND_GPR_NBYTES(S67, s67, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2595, AIV_MASK},
  {ASCEND_GPR_NBYTES(S68, s68, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2596, AIV_MASK},
  {ASCEND_GPR_NBYTES(S69, s69, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2597, AIV_MASK},
  {ASCEND_GPR_NBYTES(S70, s70, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2598, AIV_MASK},
  {ASCEND_GPR_NBYTES(S71, s71, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2599, AIV_MASK},
  {ASCEND_GPR_NBYTES(S72, s72, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2600, AIV_MASK},
  {ASCEND_GPR_NBYTES(S73, s73, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2601, AIV_MASK},
  {ASCEND_GPR_NBYTES(S74, s74, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2602, AIV_MASK},
  {ASCEND_GPR_NBYTES(S75, s75, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2603, AIV_MASK},
  {ASCEND_GPR_NBYTES(S76, s76, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2604, AIV_MASK},
  {ASCEND_GPR_NBYTES(S77, s77, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2605, AIV_MASK},
  {ASCEND_GPR_NBYTES(S78, s78, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2606, AIV_MASK},
  {ASCEND_GPR_NBYTES(S79, s79, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2607, AIV_MASK},
  {ASCEND_GPR_NBYTES(S80, s80, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2608, AIV_MASK},
  {ASCEND_GPR_NBYTES(S81, s81, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2609, AIV_MASK},
  {ASCEND_GPR_NBYTES(S82, s82, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2610, AIV_MASK},
  {ASCEND_GPR_NBYTES(S83, s83, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2611, AIV_MASK},
  {ASCEND_GPR_NBYTES(S84, s84, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2612, AIV_MASK},
  {ASCEND_GPR_NBYTES(S85, s85, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2613, AIV_MASK},
  {ASCEND_GPR_NBYTES(S86, s86, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2614, AIV_MASK},
  {ASCEND_GPR_NBYTES(S87, s87, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2615, AIV_MASK},
  {ASCEND_GPR_NBYTES(S88, s88, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2616, AIV_MASK},
  {ASCEND_GPR_NBYTES(S89, s89, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2617, AIV_MASK},
  {ASCEND_GPR_NBYTES(S90, s90, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2618, AIV_MASK},
  {ASCEND_GPR_NBYTES(S91, s91, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2619, AIV_MASK},
  {ASCEND_GPR_NBYTES(S92, s92, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2620, AIV_MASK},
  {ASCEND_GPR_NBYTES(S93, s93, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2621, AIV_MASK},
  {ASCEND_GPR_NBYTES(S94, s94, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2622, AIV_MASK},
  {ASCEND_GPR_NBYTES(S95, s95, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 2623, AIV_MASK},
  // 16bit, 这个寄存器可能一般不会用到？
  {ASCEND_GPR_NBYTES(S0, s0, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S1, s1, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S2, s2, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S3, s3, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S4, s4, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S5, s5, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S6, s6, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S7, s7, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S8, s8, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S9, s9, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S10, s10, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S11, s11, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S12, s12, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S13, s13, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S14, s14, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S15, s15, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S16, s16, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S17, s17, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S18, s18, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S19, s19, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S20, s20, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S21, s21, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S22, s22, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S23, s23, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S24, s24, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S25, s25, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S26, s26, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S27, s27, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S28, s28, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S29, s29, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S30, s30, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S31, s31, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S32, s32, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S33, s33, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S34, s34, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S35, s35, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S36, s36, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S37, s37, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S38, s38, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S39, s39, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S40, s40, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S41, s41, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S42, s42, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S43, s43, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S44, s44, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S45, s45, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S46, s46, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S47, s47, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S48, s48, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S49, s49, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S50, s50, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S51, s51, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S52, s52, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S53, s53, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S54, s54, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S55, s55, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S56, s56, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S57, s57, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S58, s58, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S59, s59, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S60, s60, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S61, s61, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S62, s62, 2), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(S63, s63, 2), 0, AIV_MASK},
  // 64bit
  {ASCEND_GPR_NBYTES(TPESL0, tpesl0, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL1, tpesl1, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL2, tpesl2, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL3, tpesl3, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL4, tpesl4, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL5, tpesl5, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL6, tpesl6, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL7, tpesl7, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL8, tpesl8, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL9, tpesl9, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL10, tpesl10, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL11, tpesl11, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL12, tpesl12, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL13, tpesl13, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL14, tpesl14, 8), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES(TPESL15, tpesl15, 8), 0, AIV_MASK},
  // 128bit
  {ASCEND_GPR_NBYTES_VEC(TPESLL0, tpesll0, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL1, tpesll1, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL2, tpesll2, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL3, tpesll3, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL4, tpesll4, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL5, tpesll5, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL6, tpesll6, 16), 0, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(TPESLL7, tpesll7, 16), 0, AIV_MASK},

  {ASCEND_GPR_NBYTES_VEC(VA0, va0, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3072, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA1, va1, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3073, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA2, va2, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3074, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA3, va3, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3075, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA4, va4, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3076, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA5, va5, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3077, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA6, va6, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3078, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(VA7, va7, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 3079, AIV_MASK},
  {ASCEND_GPR_NBYTES(A0, a0, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3584, AIV_MASK},
  {ASCEND_GPR_NBYTES(A1, a1, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3585, AIV_MASK},
  {ASCEND_GPR_NBYTES(A2, a2, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3586, AIV_MASK},
  {ASCEND_GPR_NBYTES(A3, a3, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3587, AIV_MASK},
  {ASCEND_GPR_NBYTES(A4, a4, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3588, AIV_MASK},
  {ASCEND_GPR_NBYTES(A5, a5, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3589, AIV_MASK},
  {ASCEND_GPR_NBYTES(A6, a6, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3590, AIV_MASK},
  {ASCEND_GPR_NBYTES(A7, a7, 4), VEC_ID << ID_OFFSET | 2UL << 48 | 3591, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(MASK, mask, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 4608, AIV_MASK},
  {ASCEND_REG(AR, lldb_ar_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 5632, AIV_MASK},
  {ASCEND_REG(VMS4_SR, lldb_vms4_sr_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 6144, AIV_MASK},
  {ASCEND_GPR_NBYTES_VEC(SQZN, sqzn, 16), VEC_ID << ID_OFFSET | 2UL << 48 | 6656, AIV_MASK},
  {ASCEND_REG(VPC, lldb_vpc_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 7168, AIV_MASK},
  {ASCEND_GPR_NBYTES(VL, vl, 2), VEC_ID << ID_OFFSET | 2UL << 48 | 7680, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_0, lldb_agu_am_const_0_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8192, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_1, lldb_agu_am_const_1_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8193, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_2, lldb_agu_am_const_2_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8194, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_3, lldb_agu_am_const_3_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8195, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_4, lldb_agu_am_const_4_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8196, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_5, lldb_agu_am_const_5_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8197, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_6, lldb_agu_am_const_6_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8198, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_7, lldb_agu_am_const_7_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8199, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_8, lldb_agu_am_const_8_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8200, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_9, lldb_agu_am_const_9_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8201, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_10, lldb_agu_am_const_10_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8202, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_11, lldb_agu_am_const_11_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8203, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_12, lldb_agu_am_const_12_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8204, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_13, lldb_agu_am_const_13_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8205, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_14, lldb_agu_am_const_14_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8206, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_15, lldb_agu_am_const_15_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8207, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_16, lldb_agu_am_const_16_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8208, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_17, lldb_agu_am_const_17_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8209, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_18, lldb_agu_am_const_18_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8210, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_19, lldb_agu_am_const_19_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8211, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_20, lldb_agu_am_const_20_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8212, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_21, lldb_agu_am_const_21_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8213, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_22, lldb_agu_am_const_22_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8214, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_23, lldb_agu_am_const_23_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8215, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_24, lldb_agu_am_const_24_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8216, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_25, lldb_agu_am_const_25_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8217, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_26, lldb_agu_am_const_26_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8218, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_27, lldb_agu_am_const_27_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8219, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_28, lldb_agu_am_const_28_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8220, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_29, lldb_agu_am_const_29_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8221, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_30, lldb_agu_am_const_30_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8222, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_31, lldb_agu_am_const_31_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8223, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_32, lldb_agu_am_const_32_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8224, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_33, lldb_agu_am_const_33_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8225, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_34, lldb_agu_am_const_34_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8226, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_35, lldb_agu_am_const_35_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8227, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_36, lldb_agu_am_const_36_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8228, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_37, lldb_agu_am_const_37_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8229, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_38, lldb_agu_am_const_38_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8230, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_39, lldb_agu_am_const_39_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8231, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_40, lldb_agu_am_const_40_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8232, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_41, lldb_agu_am_const_41_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8233, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_42, lldb_agu_am_const_42_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8234, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_43, lldb_agu_am_const_43_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8235, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_44, lldb_agu_am_const_44_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8236, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_45, lldb_agu_am_const_45_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8237, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_46, lldb_agu_am_const_46_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8238, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_47, lldb_agu_am_const_47_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8239, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_48, lldb_agu_am_const_48_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8240, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_49, lldb_agu_am_const_49_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8241, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_50, lldb_agu_am_const_50_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8242, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_51, lldb_agu_am_const_51_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8243, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_52, lldb_agu_am_const_52_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8244, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_53, lldb_agu_am_const_53_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8245, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_54, lldb_agu_am_const_54_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8246, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_55, lldb_agu_am_const_55_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8247, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_56, lldb_agu_am_const_56_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8248, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_57, lldb_agu_am_const_57_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8249, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_58, lldb_agu_am_const_58_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8250, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_59, lldb_agu_am_const_59_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8251, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_60, lldb_agu_am_const_60_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8252, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_61, lldb_agu_am_const_61_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8253, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_62, lldb_agu_am_const_62_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8254, AIV_MASK},
  {ASCEND_REG(AGU_AM_CONST_63, lldb_agu_am_const_63_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8255, AIV_MASK},
  {ASCEND_REG(UST_ELE_OFS_0, lldb_ust_ele_ofs_0_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8704, AIV_MASK},
  {ASCEND_REG(UST_ELE_OFS_1, lldb_ust_ele_ofs_1_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8705, AIV_MASK},
  {ASCEND_REG(UST_ELE_OFS_2, lldb_ust_ele_ofs_2_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8706, AIV_MASK},
  {ASCEND_REG(UST_ELE_OFS_3, lldb_ust_ele_ofs_3_ascend), VEC_ID << ID_OFFSET | 2UL << 48 | 8707, AIV_MASK},
  {ASCEND_REG(CORE_ID, lldb_core_id_ascend), VEC_ID << ID_OFFSET | 9UL << 48, AIV_MASK},
  {ASCEND_REG(VEC_CORE_ID, lldb_vec_core_id_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 1, AIV_MASK},
  {ASCEND_REG_2B(VTHREADDIM_X, lldb_vthreaddim_x_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 2, AIV_MASK},
  {ASCEND_REG_2B(VTHREADDIM_Y, lldb_vthreaddim_y_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 3, AIV_MASK},
  {ASCEND_REG_2B(VTHREADDIM_Z, lldb_vthreaddim_z_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 4, AIV_MASK},
  {ASCEND_REG(SIMT_PC, lldb_simt_pc_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 16384, AIV_MASK},
  {ASCEND_REG(SU_PC, lldb_su_pc_ascend), SU_ID << ID_OFFSET | 64, MIX_MASK},
  {ASCEND_REG(P_REGISTER, lldb_p_register_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 98304, AIV_MASK},
  {ASCEND_REG_4B(EXECMASK, lldb_execmask_ascend), VEC_ID << ID_OFFSET | 9UL << 48 | 0x1c000, AIV_MASK},
  {ASCEND_GPR_NBYTES(R0, r0, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R1, r1, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R2, r2, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R3, r3, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R4, r4, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R5, r5, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R6, r6, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R7, r7, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R8, r8, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R9, r9, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R10, r10, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R11, r11, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R12, r12, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R13, r13, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R14, r14, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R15, r15, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R16, r16, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R17, r17, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R18, r18, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R19, r19, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R20, r20, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R21, r21, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R22, r22, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R23, r23, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R24, r24, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R25, r25, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R26, r26, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R27, r27, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R28, r28, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R29, r29, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R30, r30, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R31, r31, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R32, r32, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R33, r33, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R34, r34, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R35, r35, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R36, r36, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R37, r37, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R38, r38, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R39, r39, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R40, r40, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R41, r41, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R42, r42, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R43, r43, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R44, r44, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R45, r45, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R46, r46, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R47, r47, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R48, r48, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R49, r49, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R50, r50, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R51, r51, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R52, r52, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R53, r53, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R54, r54, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R55, r55, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R56, r56, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R57, r57, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R58, r58, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R59, r59, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R60, r60, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R61, r61, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R62, r62, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R63, r63, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R64, r64, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R65, r65, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R66, r66, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R67, r67, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R68, r68, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R69, r69, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R70, r70, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R71, r71, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R72, r72, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R73, r73, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R74, r74, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R75, r75, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R76, r76, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R77, r77, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R78, r78, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R79, r79, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R80, r80, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R81, r81, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R82, r82, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R83, r83, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R84, r84, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R85, r85, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R86, r86, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R87, r87, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R88, r88, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R89, r89, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R90, r90, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R91, r91, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R92, r92, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R93, r93, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R94, r94, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R95, r95, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R96, r96, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R97, r97, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R98, r98, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R99, r99, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R100, r100, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R101, r101, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R102, r102, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R103, r103, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R104, r104, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R105, r105, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R106, r106, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R107, r107, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R108, r108, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R109, r109, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R110, r110, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R111, r111, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R112, r112, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R113, r113, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R114, r114, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R115, r115, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R116, r116, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R117, r117, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R118, r118, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R119, r119, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R120, r120, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R121, r121, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R122, r122, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R123, r123, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R124, r124, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R125, r125, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},
  {ASCEND_GPR_NBYTES(R126, r126, 4), VEC_ID << ID_OFFSET | 9UL << 48 | 81920, AIV_MASK},

  {ASCEND_REG(FMATRIX, lldb_fmatrix_ascend), L1_ID << ID_OFFSET | 0, AIC_MASK},
  {ASCEND_REG(PADDING_L1, lldb_padding_l1_ascend), L1_ID << ID_OFFSET | 1, AIC_MASK},
  {ASCEND_REG(L0_SET_VALUE_L1, lldb_l0_set_value_l1_ascend), L1_ID << ID_OFFSET | 2, AIC_MASK},
  {ASCEND_REG(L1_3D_SIZE, lldb_l1_3d_size_ascend), L1_ID << ID_OFFSET | 3, AIC_MASK},
  {ASCEND_REG(L3D_RPT, lldb_l3d_rpt_ascend), L1_ID << ID_OFFSET | 4, AIC_MASK},
  {ASCEND_REG(FMATRIX_DUAL_0, lldb_fmatrix_dual_0_ascend), L1_ID << ID_OFFSET | 5, AIC_MASK},
  {ASCEND_REG(FMATRIX_DUAL_1, lldb_fmatrix_dual_1_ascend), L1_ID << ID_OFFSET | 6, AIC_MASK},
  {ASCEND_REG(FMATRIX_B, lldb_fmatrix_b_ascend), L1_ID << ID_OFFSET | 7, AIC_MASK},
  {ASCEND_REG(L3D_RPT_B, lldb_l3d_rpt_b_ascend), L1_ID << ID_OFFSET | 8, AIC_MASK},
  {ASCEND_REG(PADDING_B, lldb_padding_b_ascend), L1_ID << ID_OFFSET | 9, AIC_MASK},
  {ASCEND_REG(RELU_ALPHA, lldb_relu_alpha_ascend), L1_ID << ID_OFFSET | 1UL << 48, AIC_MASK},
  {ASCEND_REG(QUANT_POST, lldb_quant_post_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 1, AIC_MASK},
  {ASCEND_REG(FPC, lldb_fpc_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 2, AIC_MASK},
  {ASCEND_REG(QUANT_PRE, lldb_quant_pre_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 3, AIC_MASK},
  {ASCEND_REG(ELT_SRC_PARA, lldb_elt_src_para_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 4, AIC_MASK},
  {ASCEND_REG(LOOP3_PARA, lldb_loop3_para_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 5, AIC_MASK},
  {ASCEND_REG(FIX_CLIP_RELU, lldb_fix_clip_relu_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 6, AIC_MASK},
  {ASCEND_REG(LOOP4_PARA, lldb_loop4_para_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 7, AIC_MASK},
  {ASCEND_REG(CHANNEL_PARA, lldb_channel_para_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 8, AIC_MASK},
  {ASCEND_REG(ELT_ANTIQ_PARA, lldb_elt_antiq_para_ascend), L1_ID << ID_OFFSET | 1UL << 48 | 9, AIC_MASK},
  {ASCEND_REG(L1_WARN, lldb_l1_warn_ascend), L1_ID << ID_OFFSET | 7UL << 48, AIC_MASK},
  // for core file only. if you add new both reg, please insert before
  {ASCEND_REG_4B(SC_ERROR_T0_0, lldb_sc_error_t0_0), 0x4700, MIX_MASK},
  {ASCEND_REG_4B(SU_ERROR_T0_0, lldb_su_error_t0_0), 0x5700, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERROR_T0_0, lldb_mte_error_t0_0), 0x6700, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERROR_T1_0, lldb_mte_error_t1_0), 0x6708, MIX_MASK},
  {ASCEND_REG_4B(VEC_ERROR_T0_0, lldb_vec_error_t0_0), 0x7700, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERROR_T0_2, lldb_vec_error_t0_2), 0x7708, AIV_MASK},
  {ASCEND_REG_4B(CUBE_ERROR_T0_0, lldb_cube_error_t0_0), 0x8700, AIC_MASK},
  {ASCEND_REG_4B(CUBE_ERROR_T0_1, lldb_cube_error_t0_1), 0x8704, AIC_MASK},
  {ASCEND_REG_4B(L1_ERROR_T0_0, lldb_l1_error_t0_0), 0xa700, MIX_MASK},
  {ASCEND_REG_4B(L1_ERROR_T0_1, lldb_l1_error_t0_0), 0xa704, MIX_MASK},

  {ASCEND_REG_4B(SC_ERR_INFO_T0_0, lldb_sc_err_info_t0_0), 0x4730, MIX_MASK},
  {ASCEND_REG_4B(SC_ERR_INFO_T0_1, lldb_sc_err_info_t0_1), 0x4734, MIX_MASK},

  {ASCEND_REG_4B(SU_ERR_INFO_T0_0, lldb_su_err_info_t0_0), 0x5730, MIX_MASK},
  {ASCEND_REG_4B(SU_ERR_INFO_T0_1, lldb_su_err_info_t0_1), 0x5734, MIX_MASK},
  {ASCEND_REG_4B(SU_ERR_INFO_T0_2, lldb_su_err_info_t0_2), 0x5738, MIX_MASK},
  {ASCEND_REG_4B(SU_ERR_INFO_T0_3, lldb_su_err_info_t0_3), 0x573C, MIX_MASK},

  {ASCEND_REG_4B(MTE_ERR_INFO_T0_0, lldb_mte_err_info_t0_0), 0x6718, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERR_INFO_T0_1, lldb_mte_err_info_t0_0), 0x671c, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERR_INFO_T0_2, lldb_mte_err_info_t0_0), 0x6720, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERR_INFO_T1_0, lldb_mte_err_info_t0_0), 0x6724, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERR_INFO_T1_1, lldb_mte_err_info_t0_0), 0x6728, MIX_MASK},
  {ASCEND_REG_4B(MTE_ERR_INFO_T1_2, lldb_mte_err_info_t0_0), 0x673c, MIX_MASK}, // 地址是否有问题？

  {ASCEND_REG_4B(VEC_ERR_INFO_T0_0, lldb_vec_err_info_t0_0), 0x7730, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERR_INFO_T0_1, lldb_vec_err_info_t0_0), 0x7734, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERR_INFO_T0_2, lldb_vec_err_info_t0_0), 0x7738, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERR_INFO_T0_3, lldb_vec_err_info_t0_0), 0x773c, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERR_INFO_T0_4, lldb_vec_err_info_t0_0), 0x7740, AIV_MASK},
  {ASCEND_REG_4B(VEC_ERR_INFO_T0_5, lldb_vec_err_info_t0_0), 0x7744, AIV_MASK},

  {ASCEND_REG_4B(CUBE_ERR_INFO_T0_1, lldb_cube_err_info_t0_0), 0x8730, AIC_MASK},
  {ASCEND_REG_4B(CUBE_ERR_INFO_T0_2, lldb_cube_err_info_t0_0), 0x8734, AIC_MASK},

  {ASCEND_REG_4B(L1_ERR_INFO_T0_0, lldb_l1_err_info_t0_0), 0xa718, MIX_MASK},
  {ASCEND_REG_4B(L1_ERR_INFO_T0_1, lldb_l1_err_info_t0_0), 0xa71c, MIX_MASK},
};

// Returns uint32_t with bits [start, end] = 1
inline constexpr uint64_t GenMask(uint32_t start, uint32_t end) {
  return static_cast<uint64_t>(((1UL << (end - start + 1)) - 1) << start);
}

inline void AddMaskCommon(uint32_t mask, uint32_t err_info_reg_num, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      err_info_reg_num,
      {
        {GenMask(0, 15), GenMask(2, 17)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

inline void AddVecMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_vec_err_info_t0_1,
      {
        {GenMask(0, 31), GenMask(0, 31)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  pc_map = {
      lldb_vec_err_info_t0_2,
      {
        {GenMask(0, 16), GenMask(32, 48)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

static_assert(sizeof(REGISTER_950_INFO) / sizeof(DeviceRegisterInfo) == k_num_registers_ascend,
              "REGISTER_950_INFO size is invalid");
} // namespace

const vector<vector<ErrRegMask>> *RegisterInfoPOSIXCore_ascend950::GetAicErrorRegTable() {
  static vector<vector<ErrRegMask>> table;
  if (!table.empty()) {
    return &table;
  }

  auto addMask = [&](const auto& err_info, uint32_t mask) {
      vector<ErrRegMask> err_infos;
      AddMaskCommon(err_info, mask, err_infos);
      table.push_back(err_infos);
  };

  auto addVecMask = [&](uint32_t mask) {
      vector<ErrRegMask> err_infos;
      AddVecMask(mask, err_infos);
      table.push_back(err_infos);
  };
  // sc_error_t0_0
  table.push_back({});
  // su_error_t0_0
  addMask(lldb_su_err_info_t0_0, GenMask(0, 31));
  // mte_error_t0_0
  addMask(lldb_mte_err_info_t0_0, GenMask(0, 31));
  // mte_error_t1_0
  addMask(lldb_mte_err_info_t0_0, GenMask(0, 31));
  // vec_error_t0_0
  addVecMask(GenMask(0, 25));
  // vec_error_t0_2
  addVecMask(GenMask(0, 1));
  // cube_error_t0_9
  addMask(lldb_cube_err_info_t0_1, GenMask(0, 15));
  // cube_error_t0_1
  addMask(lldb_cube_err_info_t0_1, GenMask(0, 9));
  // l1_error_t0_0
  addMask(lldb_l1_err_info_t0_1, GenMask(0, 30));
  // l1_error_t0_1
  addMask(lldb_l1_err_info_t0_1, GenMask(0, 21));
  return &table;
}

const RegExtractor &RegisterInfoPOSIX_ascend950::GetRegExtractor() {
  static RegExtractor instance(REGISTER_950_INFO, k_num_dbg_registers_ascend);
  static std::once_flag flag{};
  std::call_once(flag, []() {
    LLDB_LOGF(GetLog(LLDBLog::Process), "raw_register_infos_size=%lu",
              instance.raw_register_infos.size());
  });
  return instance;
}

const RegisterInfo *RegisterInfoPOSIX_ascend950::GetRegisterInfoAt(uint32_t reg_num) {
  if (reg_num < GetRegExtractor().raw_register_infos.size()) {
    return GetRegExtractor().raw_register_infos.data() + reg_num;
  }
  return nullptr;
}

RegisterInfoPOSIX_ascend950::RegisterInfoPOSIX_ascend950(const ArchSpec &target_arch)
  : RegisterInfoPOSIX_ascend(target_arch, k_num_dbg_registers_ascend,
    k_last_gpr_ascend - k_first_gpr_ascend + 1,
    GetRegExtractor().raw_register_infos.data(),
    GetRegExtractor().register_map) {
}

size_t RegisterInfoPOSIX_ascend950::GetRegisterSetCount() const {
  return k_num_register_sets_default;
}

const RegisterSet* RegisterInfoPOSIX_ascend950::GetRegisterSet(size_t set_index) const {
  if (set_index < GetRegisterSetCount())
    return &g_reg_sets_ascend950[set_index];
  return nullptr;
}

Status RegisterInfoPOSIX_ascend950::GetRegisterAddr(
    const llvm::StringRef reg_name, CoreType core_type,
    InterruptPosType pos_type, uint64_t &addr) {
  Status error;
  auto reg_info = m_register_map.find(reg_name.str());
  if (reg_info == m_register_map.end()) {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register name: {0}", reg_name);
  }
  if (IsRegisterSupport(core_type, pos_type,
                        reg_info->second.core_type_support_mask)) {
    addr = reg_info->second.addr;
  } else {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register {0} is not support", reg_name);
  }
  return error;
}

bool RegisterInfoPOSIX_ascend950::IsSimtPC(const RegisterInfo *reg_info) {
  return reg_info->kinds[eRegisterKindLLDB] == lldb_simt_pc_ascend;
}

bool RegisterInfoPOSIX_ascend950::IsSReg(const RegisterInfo *reg_info) {
  return reg_info->kinds[eRegisterKindLLDB] >= lldb_sreg_first && reg_info->kinds[eRegisterKindLLDB] <= lldb_sreg_last;
}

bool RegisterInfoPOSIX_ascend950::IsVReg(const RegisterInfo *reg_info) {
  return reg_info->kinds[eRegisterKindLLDB] >= lldb_v0_ascend && reg_info->kinds[eRegisterKindLLDB] <= lldb_v31_ascend;
}

RegisterInfoPOSIX_ascend950::BaseRegGroup
RegisterInfoPOSIX_ascend950::GetSRegGroup(const RegisterInfo *reg_info) {
  constexpr uint32_t once_bytes = 4;
  uint8_t read_count = (reg_info->byte_size + once_bytes - 1) / once_bytes;
  uint32_t cur_reg_num = reg_info->kinds[eRegisterKindLLDB];
  // 16bit
  if (cur_reg_num >= lldb_s0_ascend && cur_reg_num <= lldb_s63_ascend) {
    return {(cur_reg_num - lldb_s0_ascend) / 2 + lldb_tpes0_ascend, read_count,
            static_cast<uint8_t>((cur_reg_num - lldb_s0_ascend) % 2 + 1)};
  }
  // 32bit
  if ((cur_reg_num >= lldb_tpes0_ascend && cur_reg_num <= lldb_tpes31_ascend)
      || (cur_reg_num >= lldb_s64_ascend && cur_reg_num <= lldb_s95_ascend)) {
    return {cur_reg_num, read_count, 0};
  }
  // 64bit
  if (cur_reg_num >= lldb_tpesl0_ascend && cur_reg_num <= lldb_tpesl15_ascend) {
    return {(cur_reg_num - lldb_tpesl0_ascend) * 2 + lldb_tpes0_ascend, read_count, 0};
  }
  // 128bit
  if (cur_reg_num >= lldb_tpesll0_ascend && cur_reg_num <= lldb_tpesll7_ascend) {
    return {(cur_reg_num - lldb_tpesll0_ascend) * 4 + lldb_tpes0_ascend, read_count, 0};
  }
  return {cur_reg_num, read_count, 0};
}

// ============== RegisterInfoPOSIXCore_ascend950 =================
const RegExtractor &RegisterInfoPOSIXCore_ascend950::GetRegExtractor() {
  static RegExtractor instance(REGISTER_950_INFO, k_num_registers_ascend);
  static std::once_flag flag{};
  std::call_once(flag, []() {
    LLDB_LOGF(GetLog(LLDBLog::Process), "core raw_register_infos_size=%lu",
              instance.raw_register_infos.size());
  });
  return instance;
}

RegisterInfoPOSIXCore_ascend950::RegisterInfoPOSIXCore_ascend950(const ArchSpec &target_arch)
  : RegisterInfoPOSIX_ascend950(target_arch, k_num_registers_ascend,
    k_num_gpr_registers,
    GetRegExtractor().raw_register_infos.data(),
    GetRegExtractor().register_map) {
}

RegisterInfoPOSIX_ascend950::RegisterInfoPOSIX_ascend950(const ArchSpec &target_arch, uint32_t register_info_count,
  uint32_t register_gpr_count, const RegisterInfo *register_info_p,
  const std::map<std::string, DeviceRegisterInfo>& register_map)
  : RegisterInfoPOSIX_ascend(target_arch, register_info_count,
    register_gpr_count, register_info_p, register_map) {
}

Status RegisterInfoPOSIX_ascend950::ReadVXReg(
        const RegisterInfo *reg_info, const InterruptPosInfo &pos,
        const RegisterDataInterface *reg_data_reader,
        RegisterValue &value) const {
  Log *log = GetLog(LLDBLog::Process);
  // simd is 256bit
  constexpr uint16_t vl_size = 256;
  constexpr uint8_t read_count = vl_size / 32;
  DataBufferHeap buffer;
  Status error;
  if (m_register_map.find(reg_info->name) == m_register_map.end()) {
    error.SetErrorStringWithFormatv("Internl error: find addr for register {0} failed.", reg_info->name);
    return error;
  }
  const DeviceRegisterInfo &device_reg_info = m_register_map.at(reg_info->name);
  for (uint8_t i = 0; i < read_count; i++) {
    RegisterValue tmp_value;
    RegisterInfo one_reg_info = *reg_info;
    one_reg_info.byte_size = 32;
    error = reg_data_reader->ReadRegister(device_reg_info.addr | i,
                                          &one_reg_info, pos.core_id, pos.core_type, tmp_value);
    if (error.Fail()) {
      return error;
    }
    const uint8_t *value_data = static_cast<const uint8_t *>(tmp_value.GetBytes());
    // little order
    buffer.AppendData(value_data, one_reg_info.byte_size);
  }
  value.SetBytes(buffer.GetBytes(), reg_info->byte_size, lldb::eByteOrderLittle);
  if (log) {
    StreamString ss;
    ss << "0x";
    uint8_t *cur_data = buffer.GetBytes();
    for (size_t i = 0; i < 32; i++) {
      ss.Printf("%02x", cur_data[i]);
    }
    LLDB_LOG(log, "first 32 bytes value: {1}", ss.GetString());
  }
  return error;
}

Status RegisterInfoPOSIX_ascend950::ReadSXReg(
        const RegisterInfo *reg_info, const InterruptPosInfo &pos,
        const RegisterDataInterface *reg_data_reader,
        RegisterValue &value) const {
  Log *log = GetLog(LLDBLog::Process);

  auto reg_group = GetSRegGroup(reg_info);
  Status error;
  DataBufferHeap buffer;
  LLDB_LOG(log, "Read register {0} with start_reg={1}, group_num={2}, half_type={3}",
           reg_info->name, reg_group.start_reg_num,
           reg_group.num, reg_group.half_type);
  if (reg_group.num == 0) {
    error.SetErrorStringWithFormatv("Get register {0} groups info failed", reg_info->name);
    return error;
  }
  for (uint8_t i = 0; i < reg_group.num; i++) {
    const RegisterInfo *base_reg_info = RegisterInfoPOSIX_ascend950::GetRegisterInfoAt(i + reg_group.start_reg_num);
    RegisterValue tmp_value;
    if (base_reg_info && m_register_map.find(base_reg_info->name) != m_register_map.end()) {
      const DeviceRegisterInfo &device_reg_info = m_register_map.at(base_reg_info->name);
      error = reg_data_reader->ReadRegister(device_reg_info.addr,
                                            base_reg_info, pos.core_id, pos.core_type, tmp_value);
      if (error.Fail()) {
        return error;
      }
      const uint8_t *value_data = static_cast<const uint8_t *>(tmp_value.GetBytes());
      uint8_t value_bytes = 4;
      if (reg_group.half_type == 1) {
        value_bytes = 2;
      } else if (reg_group.half_type == 2) {
        value_data += 2;
        value_bytes = 2;
      }
      // little order
      buffer.AppendData(value_data, value_bytes);
    }
  }
  value.SetBytes(buffer.GetBytes(), reg_info->byte_size, lldb::eByteOrderLittle);
  if (log) {
    StreamString ss;
    ss << "0x";
    uint8_t *cur_data = buffer.GetBytes();
    for (size_t i = 0; i < std::min(reg_info->byte_size, 8U); i++) {
      ss.Printf("%02x", cur_data[i]);
    }
    LLDB_LOG(log, "first {0} byte value: {1}",
             std::min(reg_info->byte_size, 8U), ss.GetString());
  }
  return error;
}

Status RegisterInfoPOSIX_ascend950::ReadRXReg(
        const RegisterInfo *reg_info, uint64_t base_addr,
        const InterruptPosInfo &pos,
        const RegisterDataInterface *reg_data_reader,
        RegisterValue &value) const {
  Log *log = GetLog(LLDBLog::Process);
  uint64_t rx_addr = base_addr;
  uint8_t r_idx = reg_info->kinds[eRegisterKindDWARF] - dwarf_r0_ascend;
  uint16_t thread_id = pos.thread_info.thread_id;
  uint16_t warp_id = pos.GetWarpId();
  rx_addr |= (warp_id % 4) << 10; // bit 11-10
  uint32_t wid_div = warp_id >> 2;
  rx_addr |= (wid_div & 0x1) << 9; // bit 9

  rx_addr |= (((wid_div >> 1) & 0x1) | ((r_idx >> 6) & 0x1)) << 8; // bit 8
  rx_addr |= (((wid_div >> 2) & 0x1) | ((r_idx >> 5) & 0x1)) << 7; // bit 7
  rx_addr |= (((wid_div >> 3) & 0x1) | ((r_idx >> 4) & 0x1)) << 6; // bit 6
  rx_addr |= (r_idx & 0xF) << 2; // bit 5-2
  rx_addr |= thread_id % 32 / 8;
  RegisterInfo fix_byte_reg_info = *reg_info;
  fix_byte_reg_info.byte_size = 32;
  fix_byte_reg_info.encoding = eEncodingVector;
  RegisterValue batch_thread_value;
  auto error = reg_data_reader->ReadRegister(rx_addr, &fix_byte_reg_info, pos.core_id, pos.core_type, batch_thread_value);
  if (error.Fail()) {
    return error;
  }
  const uint8_t *data = static_cast<const uint8_t*>(batch_thread_value.GetBytes());
  value.SetBytes(data + (thread_id % 8 * 4), 4, batch_thread_value.GetByteOrder());
  LLDB_LOG(log, "got R[{0}]={1:x}, warp_id={2}, thread_id={3}", r_idx, value.GetAsUInt64(), warp_id, thread_id);
  if (log) {
    StreamString ss;
    ss << "0x";
    const uint8_t *cur_data = static_cast<const uint8_t*>(batch_thread_value.GetBytes());
    for (size_t i = 0; i < fix_byte_reg_info.byte_size; i += sizeof(uint32_t)) {
      ss.Printf("%u ", *reinterpret_cast<const uint32_t*>(cur_data + i));
    }
    LLDB_LOG(log, "Read batch Rx, batch value: {0}", ss.GetString());
  }
  return error;
}

Status RegisterInfoPOSIX_ascend950::ReadSimtPC(
        const RegisterInfo *reg_info, uint64_t base_addr,
        const InterruptPosInfo &pos, const RegisterDataInterface *reg_data_reader,
        RegisterValue &value) const {
  Log *log = GetLog(LLDBLog::Process);
  uint16_t warp_id = pos.GetWarpId();
  Status error =  reg_data_reader->ReadRegister(base_addr | warp_id, reg_info, pos.core_id, pos.core_type, value);
  LLDB_LOG(log, "got SimtPC={0:x}, warp_id={1}", value.GetAsUInt64(), warp_id);
  return error;
}

Status RegisterInfoPOSIX_ascend950::ReadExecMask(
        const RegisterInfo *reg_info, uint64_t base_addr,
        const InterruptPosInfo &pos, const RegisterDataInterface *reg_data_reader,
        RegisterValue &value) const {
  Log *log = GetLog(LLDBLog::Process);
  uint16_t warp_id = pos.GetWarpId();
  Status error =  reg_data_reader->ReadRegister(base_addr | warp_id, reg_info, pos.core_id, pos.core_type, value);
  LLDB_LOG(log, "got exec_mask={0:x}, warp_id={1}", value.GetAsUInt32(), warp_id);
  return error;
}

Status RegisterInfoPOSIX_ascend950::ReadRegister(
        const RegisterInfo *reg_info, const InterruptPosInfo &pos_info,
        const RegisterDataInterface *reg_data_reader, RegisterValue &value) const {
  Status error;
  if (reg_info && reg_info->kinds[eRegisterKindLLDB] < m_register_map.size() &&
      m_register_map.find(reg_info->name) != m_register_map.end()) {
    const DeviceRegisterInfo &device_reg_info = m_register_map.at(reg_info->name);
    uint64_t addr = device_reg_info.addr;

    // Read R0~R129
    if (reg_info->kinds[eRegisterKindDWARF] >= dwarf_r0_ascend && reg_info->kinds[eRegisterKindDWARF] <= dwarf_rx_max_id) {
      return ReadRXReg(reg_info, addr, pos_info, reg_data_reader, value);
    }
    if (IsSimtPC(reg_info)) {
      return ReadSimtPC(reg_info, addr, pos_info, reg_data_reader, value);
    }
    if (reg_info->kinds[eRegisterKindLLDB] == lldb_execmask_ascend) {
      return ReadExecMask(reg_info, addr, pos_info, reg_data_reader, value);
    }
    // Read S0~S60
    if (IsSReg(reg_info)) {
      return ReadSXReg(reg_info, pos_info, reg_data_reader, value);
    }

    if (IsVReg(reg_info)) {
      return ReadVXReg(reg_info, pos_info, reg_data_reader, value);
    }

    error = reg_data_reader->ReadRegister(addr, reg_info, pos_info.core_id, pos_info.core_type, value);
    if (error.Fail()) {
      return error;
    }
  } else {
    error.SetErrorString("reg_info is null or reg_num in reg_info is invalid");
  }
  return error;
}

static const RegisterSet g_core_reg_sets[k_num_register_sets] = {
  {"SURegisters", "su_dbg_reg", k_num_su_registers, g_su_regnums_ascend950},
  {"MTERegisters", "mte_dbg_reg", k_num_mte_registers, g_mte_regnums_ascend950},
  {"VECRegisters", "vec_dbg_reg", k_num_vecrb_registers, g_vecrb_regnums_ascend950},
  {"L1Registers", "l1_dbg_reg", k_num_l1_registers, g_l1_regnums_ascend950},
  {"AicErrorRegisters", "aic_err_reg", k_num_aic_error_registers, g_aic_error_regnums},
  {"ErrorInfoRegisters", "err_info_reg", k_num_err_info_registers, g_err_info_regnums},
};

const RegisterSet* RegisterInfoPOSIXCore_ascend950::GetRegisterSet(size_t set_index) const {
  if (set_index < GetRegisterSetCount())
    return &g_core_reg_sets[set_index];
  LLDB_LOGF(GetLog(LLDBLog::Thread), "get A2/A3 core register set failed");
  return nullptr;
}

size_t RegisterInfoPOSIXCore_ascend950::GetRegisterSetCount() const {
  return k_num_register_sets;
}

#endif
