/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#include "RegisterInfoPOSIX_ascend910B.h"
#include "lldb/Utility/LLDBLog.h"

using namespace lldb_private;
using namespace lldb;
using namespace llvm;
using namespace std;

constexpr uint8_t ADDR_OFFSET = 48U;
constexpr uint8_t AIC_MASK = 1U << static_cast<int>(CoreType::AIC);
constexpr uint8_t AIV_MASK = 1U << static_cast<int>(CoreType::AIV);
constexpr uint8_t MIX_MASK = AIC_MASK | AIV_MASK;
constexpr uint8_t UINT32_BIT_NUM = 32U;

namespace {
enum LLDB_ASCEND_RENUM {
  k_first_gpr_ascend,
  lldb_x0_ascend = k_first_gpr_ascend,
  lldb_x1_ascend,
  lldb_x2_ascend,
  lldb_x3_ascend,
  lldb_x4_ascend,
  lldb_x5_ascend,
  lldb_x6_ascend,
  lldb_x7_ascend,
  lldb_x8_ascend,
  lldb_x9_ascend,
  lldb_x10_ascend,
  lldb_x11_ascend,
  lldb_x12_ascend,
  lldb_x13_ascend,
  lldb_x14_ascend,
  lldb_x15_ascend,
  lldb_x16_ascend,
  lldb_x17_ascend,
  lldb_x18_ascend,
  lldb_x19_ascend,
  lldb_x20_ascend,
  lldb_x21_ascend,
  lldb_x22_ascend,
  lldb_x23_ascend,
  lldb_x24_ascend,
  lldb_x25_ascend,
  lldb_x26_ascend,
  lldb_x27_ascend,
  lldb_x28_ascend,
  lldb_x29_ascend,
  lldb_x30_ascend,
  lldb_x31_ascend,
  k_last_gpr_ascend = lldb_x31_ascend,
  lldb_pc_ascend,
  lldb_status_ascend,
  lldb_ctrl_ascend,
  lldb_lpcnt_ascend,
  lldb_condition_flag_ascend,
  lldb_cond_ascend,
  lldb_sys_cnt_ascend,
  lldb_safety_crc_en_ascend,
  lldb_call_depth_cnt_ascend,
  lldb_icache_prl_st_ascend,
  lldb_st_atomic_cfg_ascend,
  lldb_ffts_base_addr_ascend,
  lldb_vec_event_table_ascend,
  lldb_cube_event_table_ascend,
  lldb_fixp_event_table_ascend,
  lldb_scalar_event_table_ascend,
  lldb_mte1_event_table_ascend,
  lldb_mte2_event_table_ascend,
  lldb_mte3_event_table_ascend,
  lldb_mask_ascend,
  lldb_cmpmask_ascend,
  lldb_deqscale_ascend,
  lldb_rpn_cor_ir_ascend,
  lldb_vms4_sr_ascend,
  lldb_data_exp0_ascend,
  lldb_data_exp1_ascend,
  lldb_data_exp2_ascend,
  lldb_data_exp3_ascend,
  lldb_rpn_offset_ascend,
  lldb_rsvd_cnt_ascend,
  lldb_pnt_coe_ascend,
  lldb_lrelu_alpha_ascend,
  lldb_maxmin_cnt_ascend,
  lldb_acc_val_ascend,
  lldb_varf0_ascend,
  lldb_varf1_ascend,
  lldb_varf2_ascend,
  lldb_varf3_ascend,
  lldb_varf4_ascend,
  lldb_varf5_ascend,
  lldb_varf6_ascend,
  lldb_varf7_ascend,
  k_num_dbg_registers_ascend,
  // Above registers is used for both coredump and onboard.
  // If you add a new shared register, please insert it before this line.
  lldb_aic_error_0 = k_num_dbg_registers_ascend, 
  lldb_aic_error_1,
  lldb_aic_error_2, 
  lldb_aic_error_3, 
  lldb_aic_error_4, 
  lldb_aic_error_5, 
  lldb_biu_err_info_0,
  lldb_biu_err_info_1, 
  lldb_ccu_err_info_0, 
  lldb_ccu_err_info_1, 
  lldb_cube_err_info, 
  lldb_biu_err_info_2, 
  lldb_ifu_err_info_0, 
  lldb_ifu_err_info_1, 
  lldb_mte_err_info_0, 
  lldb_mte_err_info_1, 
  lldb_vec_err_info_0, 
  lldb_vec_err_info_1, 
  lldb_fixp_err_info_0, 
  lldb_su_dc_ecc_1bit_err_info, 
  lldb_fixp_err_info_1, 
  k_num_registers_ascend
};

enum {
  k_num_gpr_registers = k_last_gpr_ascend - k_first_gpr_ascend + 1,
  k_num_aic_err_registers = lldb_aic_error_5 - lldb_aic_error_0 + 1,
  k_num_err_info_registers = lldb_fixp_err_info_1 - lldb_biu_err_info_0 + 1,
  k_num_register_sets_default = 1,
  k_num_register_sets = 3
};

static const uint32_t g_dbg_regnums[] = {
  lldb_x0_ascend,  lldb_x1_ascend,  lldb_x2_ascend, lldb_x3_ascend,
  lldb_x4_ascend,  lldb_x5_ascend, lldb_x6_ascend,  lldb_x7_ascend,
  lldb_x8_ascend, lldb_x9_ascend,  lldb_x10_ascend, lldb_x11_ascend,
  lldb_x12_ascend, lldb_x13_ascend, lldb_x14_ascend, lldb_x15_ascend,
  lldb_x16_ascend, lldb_x17_ascend, lldb_x18_ascend, lldb_x19_ascend,
  lldb_x20_ascend, lldb_x21_ascend, lldb_x22_ascend, lldb_x23_ascend,
  lldb_x24_ascend, lldb_x25_ascend, lldb_x26_ascend, lldb_x27_ascend,
  lldb_x28_ascend, lldb_x29_ascend, lldb_x30_ascend, lldb_x31_ascend,
  lldb_pc_ascend, lldb_status_ascend, lldb_ctrl_ascend, lldb_lpcnt_ascend,
  lldb_condition_flag_ascend, lldb_cond_ascend, lldb_sys_cnt_ascend,
  lldb_safety_crc_en_ascend, lldb_call_depth_cnt_ascend,
  lldb_icache_prl_st_ascend, lldb_st_atomic_cfg_ascend,
  lldb_ffts_base_addr_ascend, lldb_vec_event_table_ascend,
  lldb_cube_event_table_ascend, lldb_fixp_event_table_ascend,
  lldb_scalar_event_table_ascend, lldb_mte1_event_table_ascend,
  lldb_mte2_event_table_ascend, lldb_mte3_event_table_ascend,
  lldb_mask_ascend, lldb_cmpmask_ascend, lldb_deqscale_ascend,
  lldb_rpn_cor_ir_ascend, lldb_vms4_sr_ascend, lldb_data_exp0_ascend,
  lldb_data_exp1_ascend, lldb_data_exp2_ascend, lldb_data_exp3_ascend,
  lldb_rpn_offset_ascend, lldb_rsvd_cnt_ascend, lldb_pnt_coe_ascend,
  lldb_lrelu_alpha_ascend, lldb_maxmin_cnt_ascend, lldb_acc_val_ascend,
  lldb_varf0_ascend, lldb_varf1_ascend, lldb_varf2_ascend, lldb_varf3_ascend,
  lldb_varf4_ascend, lldb_varf5_ascend, lldb_varf6_ascend, lldb_varf7_ascend,
  LLDB_INVALID_REGNUM};

static const uint32_t g_aic_err_regnums[] = {
  lldb_aic_error_0, lldb_aic_error_1, lldb_aic_error_2, 
  lldb_aic_error_3, lldb_aic_error_4, lldb_aic_error_5,
  LLDB_INVALID_REGNUM};

static const uint32_t g_err_info_regnums[] = {
  lldb_biu_err_info_0, lldb_biu_err_info_1, lldb_ccu_err_info_0, 
  lldb_ccu_err_info_1, lldb_cube_err_info, lldb_biu_err_info_2, 
  lldb_ifu_err_info_0, lldb_ifu_err_info_1, lldb_mte_err_info_0, 
  lldb_mte_err_info_1, lldb_vec_err_info_0, lldb_vec_err_info_1, 
  lldb_fixp_err_info_0, lldb_su_dc_ecc_1bit_err_info, lldb_fixp_err_info_1, 
  LLDB_INVALID_REGNUM};

static const RegisterSet g_reg_sets_ascend910b[k_num_register_sets_default] = {
  {"Registers", "register", k_num_dbg_registers_ascend,
   g_dbg_regnums}};

// The array should be POD type.
static const DeviceRegisterInfo REGISTER_910B_INFO[] = {
    {ASCEND_GPR(GPR0, x0, LLDB_INVALID_REGNUM), 0, MIX_MASK},
    {ASCEND_GPR(GPR1, x1, LLDB_INVALID_REGNUM), 1, MIX_MASK},
    {ASCEND_GPR(GPR2, x2, LLDB_INVALID_REGNUM), 2, MIX_MASK},
    {ASCEND_GPR(GPR3, x3, LLDB_INVALID_REGNUM), 3, MIX_MASK},
    {ASCEND_GPR(GPR4, x4, LLDB_INVALID_REGNUM), 4, MIX_MASK},
    {ASCEND_GPR(GPR5, x5, LLDB_INVALID_REGNUM), 5, MIX_MASK},
    {ASCEND_GPR(GPR6, x6, LLDB_INVALID_REGNUM), 6, MIX_MASK},
    {ASCEND_GPR(GPR7, x7, LLDB_INVALID_REGNUM), 7, MIX_MASK},
    {ASCEND_GPR(GPR8, x8, LLDB_INVALID_REGNUM), 8, MIX_MASK},
    {ASCEND_GPR(GPR9, x9, LLDB_INVALID_REGNUM), 9, MIX_MASK},
    {ASCEND_GPR(GPR10, x10, LLDB_INVALID_REGNUM), 10, MIX_MASK},
    {ASCEND_GPR(GPR11, x11, LLDB_INVALID_REGNUM), 11, MIX_MASK},
    {ASCEND_GPR(GPR12, x12, LLDB_INVALID_REGNUM), 12, MIX_MASK},
    {ASCEND_GPR(GPR13, x13, LLDB_INVALID_REGNUM), 13, MIX_MASK},
    {ASCEND_GPR(GPR14, x14, LLDB_INVALID_REGNUM), 14, MIX_MASK},
    {ASCEND_GPR(GPR15, x15, LLDB_INVALID_REGNUM), 15, MIX_MASK},
    {ASCEND_GPR(GPR16, x16, LLDB_INVALID_REGNUM), 16, MIX_MASK},
    {ASCEND_GPR(GPR17, x17, LLDB_INVALID_REGNUM), 17, MIX_MASK},
    {ASCEND_GPR(GPR18, x18, LLDB_INVALID_REGNUM), 18, MIX_MASK},
    {ASCEND_GPR(GPR19, x19, LLDB_INVALID_REGNUM), 19, MIX_MASK},
    {ASCEND_GPR(GPR20, x20, LLDB_INVALID_REGNUM), 20, MIX_MASK},
    {ASCEND_GPR(GPR21, x21, LLDB_INVALID_REGNUM), 21, MIX_MASK},
    {ASCEND_GPR(GPR22, x22, LLDB_INVALID_REGNUM), 22, MIX_MASK},
    {ASCEND_GPR(GPR23, x23, LLDB_INVALID_REGNUM), 23, MIX_MASK},
    {ASCEND_GPR(GPR24, x24, LLDB_INVALID_REGNUM), 24, MIX_MASK},
    {ASCEND_GPR(GPR25, x25, LLDB_INVALID_REGNUM), 25, MIX_MASK},
    {ASCEND_GPR(GPR26, x26, LLDB_INVALID_REGNUM), 26, MIX_MASK},
    {ASCEND_GPR(GPR27, x27, LLDB_INVALID_REGNUM), 27, MIX_MASK},
    {ASCEND_GPR(GPR28, x28, LLDB_INVALID_REGNUM), 28, MIX_MASK},
    {ASCEND_GPR_ALT(GPR29, x29, fp, LLDB_REGNUM_GENERIC_FP), 29, MIX_MASK},
    {ASCEND_GPR_ALT(GPR30, x30, sp, LLDB_REGNUM_GENERIC_SP), 30, MIX_MASK},
    {ASCEND_GPR_ALT(GPR31, x31, lr, LLDB_REGNUM_GENERIC_RA), 31, MIX_MASK},
    {ASCEND_GPR(PC, pc, LLDB_REGNUM_GENERIC_PC), 64, MIX_MASK},
    {ASCEND_REG(STATUS, lldb_status_ascend), 66, MIX_MASK},
    {ASCEND_REG(CTRL, lldb_ctrl_ascend), 67, MIX_MASK},
    {ASCEND_REG(LPCNT, lldb_lpcnt_ascend), 68, MIX_MASK},
    {ASCEND_REG(CONDITION_FLAG, lldb_condition_flag_ascend), 69, MIX_MASK},
    {ASCEND_REG(COND, lldb_cond_ascend), 70, MIX_MASK},
    {ASCEND_REG(SYS_CNT, lldb_sys_cnt_ascend), 72, MIX_MASK},
    {ASCEND_REG(SAFETY_CRC_EN, lldb_safety_crc_en_ascend), 73, MIX_MASK},
    {ASCEND_REG(CALL_DEPTH_CNT, lldb_call_depth_cnt_ascend), 75, MIX_MASK},
    {ASCEND_REG(ICACHE_PRL_ST, lldb_icache_prl_st_ascend), 76, MIX_MASK},
    {ASCEND_REG(ST_ATOMIC_CFG, lldb_st_atomic_cfg_ascend), 77, MIX_MASK},
    {ASCEND_REG(FFTS_BASE_ADDR, lldb_ffts_base_addr_ascend), 78, MIX_MASK},
    {ASCEND_REG(VEC_EVENT_TABLE, lldb_vec_event_table_ascend), 131, AIV_MASK},
    {ASCEND_REG(CUBE_EVENT_TABLE, lldb_cube_event_table_ascend), 132, AIC_MASK},
    {ASCEND_REG(FIXP_EVENT_TABLE, lldb_fixp_event_table_ascend), 133, AIC_MASK},
    {ASCEND_REG(SCALAR_EVENT_TABLE, lldb_scalar_event_table_ascend), 134, MIX_MASK},
    {ASCEND_REG(MTE1_EVENT_TABLE, lldb_mte1_event_table_ascend), 135, AIC_MASK},
    {ASCEND_REG(MTE2_EVENT_TABLE, lldb_mte2_event_table_ascend), 136, MIX_MASK},
    {ASCEND_REG(MTE3_EVENT_TABLE, lldb_mte3_event_table_ascend), 137, AIV_MASK},
    {ASCEND_REG(MASK, lldb_mask_ascend), 256ULL << ADDR_OFFSET | 0, AIV_MASK},
    {ASCEND_REG(CMPMASK, lldb_cmpmask_ascend), 256ULL << ADDR_OFFSET | 1, AIV_MASK},
    {ASCEND_REG(DEQSCALE, lldb_deqscale_ascend), 256ULL << ADDR_OFFSET | 2, AIV_MASK},
    {ASCEND_REG(RPN_COR_IR, lldb_rpn_cor_ir_ascend), 256UL << ADDR_OFFSET | 3, AIV_MASK},
    {ASCEND_REG(VMS4_SR, lldb_vms4_sr_ascend), 256ULL << ADDR_OFFSET | 4, AIV_MASK},
    {ASCEND_REG(DATA_EXP0, lldb_data_exp0_ascend), 256ULL << ADDR_OFFSET | 5, AIV_MASK},
    {ASCEND_REG(DATA_EXP1, lldb_data_exp1_ascend), 256ULL << ADDR_OFFSET | 6, AIV_MASK},
    {ASCEND_REG(DATA_EXP2, lldb_data_exp2_ascend), 256ULL << ADDR_OFFSET | 7, AIV_MASK},
    {ASCEND_REG(DATA_EXP3, lldb_data_exp3_ascend), 256ULL << ADDR_OFFSET | 8, AIV_MASK},
    {ASCEND_REG(RPN_OFFSET, lldb_rpn_offset_ascend), 256ULL << ADDR_OFFSET | 9, AIV_MASK},
    {ASCEND_REG(RSVD_CNT, lldb_rsvd_cnt_ascend), 256ULL << ADDR_OFFSET | 11, AIV_MASK},
    {ASCEND_REG(PNT_COE, lldb_pnt_coe_ascend), 256ULL << ADDR_OFFSET | 12, AIV_MASK},
    {ASCEND_REG(LRELU_ALPHA, lldb_lrelu_alpha_ascend), 256ULL << ADDR_OFFSET | 13, AIV_MASK},
    {ASCEND_REG(MAXMIN_CNT, lldb_maxmin_cnt_ascend), 256ULL << ADDR_OFFSET | 14, AIV_MASK},
    {ASCEND_REG(ACC_VAL, lldb_acc_val_ascend), 256ULL << ADDR_OFFSET | 15, AIV_MASK},
    {ASCEND_REG(VARF0, lldb_varf0_ascend), 257ULL << ADDR_OFFSET | 0, AIV_MASK},
    {ASCEND_REG(VARF1, lldb_varf1_ascend), 257ULL << ADDR_OFFSET | 1, AIV_MASK},
    {ASCEND_REG(VARF2, lldb_varf2_ascend), 257ULL << ADDR_OFFSET | 2, AIV_MASK},
    {ASCEND_REG(VARF3, lldb_varf3_ascend), 257ULL << ADDR_OFFSET | 3, AIV_MASK},
    {ASCEND_REG(VARF4, lldb_varf4_ascend), 257ULL << ADDR_OFFSET | 4, AIV_MASK},
    {ASCEND_REG(VARF5, lldb_varf5_ascend), 257ULL << ADDR_OFFSET | 5, AIV_MASK},
    {ASCEND_REG(VARF6, lldb_varf6_ascend), 257ULL << ADDR_OFFSET | 6, AIV_MASK},
    {ASCEND_REG(VARF7, lldb_varf7_ascend), 257ULL << ADDR_OFFSET | 7, AIV_MASK},
  // for core file only. if you add new both reg, please insert before
    {ASCEND_REG_4B(AIC_ERROR_0, lldb_aic_error_0), 0x700, MIX_MASK},
    {ASCEND_REG_4B(AIC_ERROR_1, lldb_aic_error_1), 0x704, MIX_MASK},
    {ASCEND_REG_4B(AIC_ERROR_2, lldb_aic_error_2), 0x760, MIX_MASK},
    {ASCEND_REG_4B(AIC_ERROR_3, lldb_aic_error_3), 0x764, MIX_MASK},
    {ASCEND_REG_4B(AIC_ERROR_4, lldb_aic_error_4), 0x780, MIX_MASK},
    {ASCEND_REG_4B(AIC_ERROR_5, lldb_aic_error_5), 0x790, MIX_MASK},
    {ASCEND_REG_4B(BIU_ERR_INFO_0, lldb_biu_err_info_0), 0x710, MIX_MASK},
    {ASCEND_REG_4B(BIU_ERR_INFO_1, lldb_biu_err_info_1), 0x714, MIX_MASK},
    {ASCEND_REG_4B(CCU_ERR_INFO_0, lldb_ccu_err_info_0), 0x718, MIX_MASK},
    {ASCEND_REG_4B(CCU_ERR_INFO_1, lldb_ccu_err_info_1), 0x71C, MIX_MASK},
    {ASCEND_REG_4B(CUBE_ERR_INFO, lldb_cube_err_info), 0x720, MIX_MASK},
    {ASCEND_REG_4B(BIU_ERR_INFO_2, lldb_biu_err_info_2), 0x724, MIX_MASK},
    {ASCEND_REG_4B(IFU_ERR_INFO_0, lldb_ifu_err_info_0), 0x728, MIX_MASK},
    {ASCEND_REG_4B(IFU_ERR_INFO_1, lldb_ifu_err_info_1), 0x72C, MIX_MASK},
    {ASCEND_REG_4B(MTE_ERR_INFO_0, lldb_mte_err_info_0), 0x730, MIX_MASK},
    {ASCEND_REG_4B(MTE_ERR_INFO_1, lldb_mte_err_info_1), 0x734, MIX_MASK},
    {ASCEND_REG_4B(VEC_ERR_INFO_0, lldb_vec_err_info_0), 0x738, MIX_MASK},
    {ASCEND_REG_4B(VEC_ERR_INFO_1, lldb_vec_err_info_1), 0x73C, MIX_MASK},
    {ASCEND_REG_4B(FIXP_ERR_INFO_0, lldb_fixp_err_info_0), 0x78C, MIX_MASK},
    {ASCEND_REG_4B(SU_DC_ECC_1BIT_ERR_INFO, lldb_su_dc_ecc_1bit_err_info), 0x7C4, MIX_MASK},
    {ASCEND_REG_4B(FIXP_ERR_INFO_1, lldb_fixp_err_info_1), 0x7C8, MIX_MASK}
};

static_assert(sizeof(REGISTER_910B_INFO) / sizeof(DeviceRegisterInfo) == k_num_registers_ascend,
              "REGISTER_910B_INFO size is invalid");

const char *AIC_ERROR_0_BIT_NAMES[UINT32_BIT_NUM] = {
    "biu_l2_read_oob", "biu_l2_write_oob", "ccu_call_depth_ovrflw", "ccu_div0",
    "ccu_illegal_instr", "ccu_loop_cnt_err", "ccu_loop_err", "ccu_neg_sqrt",
    "ccu_ub_ecc", "cube_invld_input", "cube_l0a_ecc", "cube_l0a_rdwr_cflt",
    "cube_l0a_wrap_around", "cube_l0b_ecc", "cube_l0b_rdwr_cflt", "cube_l0b_wrap_around",
    "cube_l0c_ecc", "cube_l0c_rdwr_cflt", "cube_l0c_self_rdwr_cflt", "cube_l0c_wrap_around",
    "ifu_bus_err", "mte_aipp_illegal_param", "mte_bas_raddr_obound", "mte_biu_rdwr_resp",
    "mte_cidx_overflow", "mte_decomp", "mte_f1wpos_larger_fsize", "mte_fmap_less_kernel",
    "mte_fmapwh_larger_l1size", "mte_fpos_larger_fsize", "mte_gdma_illegal_burst_len", "mte_gdma_illegal_burst_num",
};

const char *AIC_ERROR_1_BIT_NAMES[UINT32_BIT_NUM] = {
    "mte_gdma_read_overflow", "mte_gdma_write_overflow", "mte_comp", "mte_illegal_fm_size",
    "mte_illegal_l1_3d_size", "mte_illegal_stride", "mte_l0a_rdwr_cflt", "mte_l0b_rdwr_cflt",
    "mte_l1_ecc", "mte_padding_cfg", "mte_read_overflow", "mte_rob_ecc",
    "mte_tlu_ecc", "mte_ub_ecc", "mte_unzip", "mte_write_3d_overflow",
    "mte_write_overflow", "vec_data_excp_ccu", "vec_data_excp_mte", "vec_data_excp_vec",
    "vec_div0", "vec_illegal_mask", "vec_inf_nan", "vec_l0c_ecc",
    "vec_l0c_rdwr_cflt", "vec_neg_ln", "vec_neg_sqrt", "vec_same_blk_addr",
    "vec_ub_ecc", "vec_ub_self_rdwr_cflt", "vec_ub_wrap_around", "biu_dfx_err",
};

const char *AIC_ERROR_2_BIT_NAMES[UINT32_BIT_NUM] = {
    "ccu_sbuf_ecc", "vec_col2img_rd_fm_addr_ovflow", "vec_col2img_rd_dfm_addr_ovfflow",
    "vec_col2img_illegal_fm_size", "vec_col2img_illegal_stride", "vec_col2img_illegal_1st_win_pos",
    "vec_col2img_illegal_fetch_pos", "vec_col2img_illegal_k_size", "ccu_inf_nan",
    "mte_illegal_schn_cfg", "mte_atm_addr_misalg", "vec_instr_addr_misalign",
    "vec_instr_illegal_cfg", "vec_instr_undef", "ccu_addr_err",
    "ccu_bus_err", "mte_err_addr_misalign", "mte_err_dw_pad_conf_err",
    "mte_err_dw_fmap_h_illegal", "mte_err_wino_l0b_write_overflow", "mte_err_wino_l0b_read_overflow",
    "mte_err_illegal_v_cov_pad_ctl", "mte_err_illegal_h_cov_pad_ctl", "mte_err_illegal_w_size",
    "mte_err_illegal_h_size", "mte_err_illegal_chn_size", "mte_err_illegal_k_m_ext_step",
    "mte_err_illegal_k_m_start_pos", "mte_err_illegal_smallk_cfg", "mte_err_read_3d_overflow",
    "ccu_veciq_ecc", "ccu_dc_data_ecc"
};

const char *AIC_ERROR_3_BIT_NAMES[UINT32_BIT_NUM] = {
    "mte_gdma_read_overflow", "mte_gdma_write_overflow", "mte_comp", "mte_illegal_fm_size",
    "mte_illegal_l1_3d_size", "mte_illegal_stride", "mte_l0a_rdwr_cflt", "mte_l0b_rdwr_cflt",
    "mte_l1_ecc", "mte_padding_cfg", "mte_read_overflow", "mte_rob_ecc",
    "mte_tlu_ecc", "mte_ub_ecc", "mte_unzip", "mte_write_3d_overflow",
    "mte_write_overflow", "vec_data_excp_ccu", "vec_data_excp_mte", "vec_data_excp_vec",
    "vec_div0", "vec_illegal_mask", "vec_inf_nan", "vec_l0c_ecc",
    "vec_l0c_rdwr_cflt", "vec_neg_ln", "vec_neg_sqrt", "vec_same_blk_addr",
    "vec_ub_ecc", "vec_ub_self_rdwr_cflt", "vec_ub_wrap_around", "biu_dfx_err",
};

const char *AIC_ERROR_4_BIT_NAMES[UINT32_BIT_NUM] = {
    "mte_timeout", "mte_ub_rd_cflt", "mte_ub_wr_cflt", "mte_ktable_wr_addr_overflow",
    "mte_ktable_rd_addr_overflow", "ccu_ub_rd_cflt", "ccu_ub_wr_cflt", "ccu_ub_overflow_err",
    "biu_unsplit_err", "mte_stb_ecc_err", "mte_aipp_ecc_err", "ccu_lsu_atomic_err",
    "ccu_cross_core_set_ovfl_err", "fixp_err_out_write_overflow", "cube_err_pbuf_wrap_around", "fixp_l0c_ecc",
    "mte_err_l0c_rdwr_cflt", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved"
};

const char *AIC_ERROR_5_BIT_NAMES[UINT32_BIT_NUM] = {
    "vec_data_excpt_mte", "vec_data_excpt_su", "vec_data_excpt_vec", "vec_instr_timeout",
    "vec_instrs_undef", "vec_instr_ill_cfg", "vec_instr_misalign", "vec_instr_ill_mask",
    "vec_instr_ill_sqzn", "vec_ub_addr_wrap_around", "vec_ub_ecc_mberr", "vec_idata_inf_nan",
    "vec_div_by_zero", "vec_valu_neg_ln", "vec_valu_neg_sqrt", "vec_vci_idata_out_range",
    "vec_ill_vloop_op", "vec_ill_vloop_sreg", "vec_ld_num_mismatch", "vec_st_num_mismatch",
    "vec_ex_num_mismatch", "vec_ld_num_exceed_limit", "vec_st_num_exceed_limit", "vec_ill_instr_padding",
    "vec_ill_vga_vpd_order", "vec_ic_ecc_err", "vec_biu_resp_err", "vec_pb_ecc_mberr",
    "vec_pb_read_no_resp", "vec_valu_ill_issue", "vec_err_parity_err", "reserved"
};

const char **AIC_ERROR_BIT_NAMES[k_num_aic_err_registers] = {
    AIC_ERROR_0_BIT_NAMES,
    AIC_ERROR_1_BIT_NAMES,
    AIC_ERROR_2_BIT_NAMES,
    AIC_ERROR_3_BIT_NAMES,
    AIC_ERROR_4_BIT_NAMES,
    AIC_ERROR_5_BIT_NAMES,
};

enum ErrorInfoId {
    cube_info_id,
    ccu_info_id,
    ifu_info_id,
    mte_info_id,
    vec_info_id,
    fixp_info_id,
    error_info_id_num
};

// Returns uint32_t with bits [start, end] = 1
inline constexpr uint32_t GenMask(uint32_t start, uint32_t end) {
  return static_cast<uint32_t>(((1UL << (end - start + 1)) - 1) << start);
}

inline void AddCubeMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_cube_err_info,
      {
        {GenMask(0, 7), GenMask(2, 9)},
        {GenMask(24, 31), GenMask(10, 17)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

inline void AddCcuMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_ccu_err_info_0,
      {
        {GenMask(0, 7), GenMask(2, 9)},
        {GenMask(23, 30), GenMask(10, 17)}
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

inline void AddMteMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_mte_err_info_0,
      {
        {GenMask(0, 7), GenMask(2, 9)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  pc_map = {
      lldb_mte_err_info_1,
      {
        {GenMask(0, 7), GenMask(10, 17)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

inline void AddVecMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_vec_err_info_0,
      {
        {GenMask(0, 7), GenMask(2, 9)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  pc_map = {
      lldb_vec_err_info_1,
      {
        {GenMask(0, 7), GenMask(10, 17)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

inline void AddFixpMask(uint32_t mask, vector<ErrRegMask> &err_infos) {
  ErrRegMask info_map = { mask };
  ErrInfoReg pc_map = {
      lldb_fixp_err_info_0,
      {
          {GenMask(0, 7), GenMask(2, 9)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  pc_map = {
      lldb_fixp_err_info_1,
      {
        {GenMask(0, 7), GenMask(10, 17)},
      }
  };
  info_map.err_info_regs.emplace_back(pc_map);
  err_infos.emplace_back(info_map);
}

} // namespace

const vector<vector<ErrRegMask>> *RegisterInfoPOSIXCore_ascend910B::GetAicErrorRegTable() {
  static vector<vector<ErrRegMask>> table(k_num_aic_err_registers);
  for (int i = 0; i < k_num_aic_err_registers; i++) {
    if (!table[i].empty()) {
        return &table;
    }
    vector<uint32_t> masks(error_info_id_num);
    for (int b = 0; b < 32; b++) {
        auto bit_name = StringRef(AIC_ERROR_BIT_NAMES[i][b]);
        if (bit_name.starts_with("cube_")) {
            masks[cube_info_id] |= (1U << b);
        } else if (bit_name.starts_with("ccu_")) {
            masks[ccu_info_id] |= (1U << b);
        } else if (bit_name.starts_with("mte_")) {
            masks[mte_info_id] |= (1U << b);
        } else if (bit_name.starts_with("vec_")) {
            masks[vec_info_id] |= (1U << b);
        } else if (bit_name.starts_with("fixp_")) {
            masks[fixp_info_id] |= (1U << b);
        }
    }
    AddCubeMask(masks[cube_info_id], table[i]);
    AddCcuMask(masks[ccu_info_id], table[i]);
    AddMteMask(masks[mte_info_id], table[i]);
    AddVecMask(masks[vec_info_id], table[i]);
    AddFixpMask(masks[fixp_info_id], table[i]);
  }
  return &table;
}

const RegExtractor &RegisterInfoPOSIX_ascend910B::GetRegExtractor() {
  static RegExtractor instance(REGISTER_910B_INFO, k_num_dbg_registers_ascend);
  static std::once_flag flag{};
  std::call_once(flag, []() {
    LLDB_LOGF(GetLog(LLDBLog::Process), "raw_register_infos_size=%lu",
              instance.raw_register_infos.size());
  });
  return instance;
}

RegisterInfoPOSIX_ascend910B::RegisterInfoPOSIX_ascend910B(const ArchSpec &target_arch)
  : RegisterInfoPOSIX_ascend(target_arch, k_num_dbg_registers_ascend,
    k_num_gpr_registers,
    GetRegExtractor().raw_register_infos.data(),
    GetRegExtractor().register_map) {
}

RegisterInfoPOSIX_ascend910B::RegisterInfoPOSIX_ascend910B(const ArchSpec &target_arch, uint32_t register_info_count,
  uint32_t register_gpr_count, const RegisterInfo *register_info_p,
  const std::map<std::string, DeviceRegisterInfo>& register_map)
  : RegisterInfoPOSIX_ascend(target_arch, register_info_count,
    register_gpr_count, register_info_p, register_map) {
}

const RegisterSet* RegisterInfoPOSIX_ascend910B::GetRegisterSet(size_t set_index) const {
  if (set_index < GetRegisterSetCount())
    return &g_reg_sets_ascend910b[set_index];
  LLDB_LOGF(GetLog(LLDBLog::Thread), "get A2/A3 register set failed");
  return nullptr;
}

Status RegisterInfoPOSIX_ascend910B::GetRegisterAddr(const llvm::StringRef reg_name,
  CoreType core_type, uint64_t &addr) {
  Status error;
  auto reg_info = m_register_map.find(reg_name.str());
  if (reg_info == m_register_map.end()) {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register name: {0}", reg_name);
	return error;
  }
  if ((1U << static_cast<int>(core_type)) & reg_info->second.core_type_support_mask) {
    addr = reg_info->second.addr;
  } else {
    error.SetErrorStringWithFormatv(
        "Can not get addr, register {0} is not support", reg_name);
  }
  return error;
}
// ============== RegisterInfoPOSIXCore_ascend910B =================

const RegExtractor &RegisterInfoPOSIXCore_ascend910B::GetRegExtractor() {
  static RegExtractor instance(REGISTER_910B_INFO, k_num_registers_ascend);
  static std::once_flag flag{};
  std::call_once(flag, []() {
    LLDB_LOGF(GetLog(LLDBLog::Process), "raw_register_infos_size=%lu",
              instance.raw_register_infos.size());
  });
  return instance;
}

RegisterInfoPOSIXCore_ascend910B::RegisterInfoPOSIXCore_ascend910B(const ArchSpec &target_arch)
  : RegisterInfoPOSIX_ascend910B(target_arch, k_num_registers_ascend,
    k_num_gpr_registers,
    GetRegExtractor().raw_register_infos.data(),
    GetRegExtractor().register_map) {
}

static const RegisterSet g_core_reg_sets[k_num_register_sets] = {
    {"DebugRegisters", "dbg_reg", k_num_dbg_registers_ascend, g_dbg_regnums},
    {"AicErrorRegisters", "aic_err_reg", k_num_aic_err_registers, g_aic_err_regnums},
    {"ErrorInfoRegisters", "err_info_reg", k_num_err_info_registers, g_err_info_regnums},
};

const RegisterSet* RegisterInfoPOSIXCore_ascend910B::GetRegisterSet(size_t set_index) const {
  if (set_index < GetRegisterSetCount())
    return &g_core_reg_sets[set_index];
  LLDB_LOGF(GetLog(LLDBLog::Thread), "get A2/A3 core register set failed");
  return nullptr;
}

size_t RegisterInfoPOSIXCore_ascend910B::GetRegisterSetCount() const {
  return k_num_register_sets;
}

#endif
