/* Generated automatically by the program 'build/genpreds'
   from the machine description file '../../../source/gcc-14.2.0/gcc/config/sh/sh.md'.  */

#ifndef GCC_TM_PREDS_H
#define GCC_TM_PREDS_H

#ifdef HAVE_MACHINE_MODES
extern bool general_operand (rtx, machine_mode);
extern bool address_operand (rtx, machine_mode);
extern bool register_operand (rtx, machine_mode);
extern bool pmode_register_operand (rtx, machine_mode);
extern bool scratch_operand (rtx, machine_mode);
extern bool immediate_operand (rtx, machine_mode);
extern bool const_int_operand (rtx, machine_mode);
extern bool const_double_operand (rtx, machine_mode);
extern bool nonimmediate_operand (rtx, machine_mode);
extern bool nonmemory_operand (rtx, machine_mode);
extern bool push_operand (rtx, machine_mode);
extern bool pop_operand (rtx, machine_mode);
extern bool memory_operand (rtx, machine_mode);
extern bool indirect_operand (rtx, machine_mode);
extern bool ordered_comparison_operator (rtx, machine_mode);
extern bool comparison_operator (rtx, machine_mode);
extern bool aligned_register_operand (rtx, machine_mode);
extern bool arith_reg_operand (rtx, machine_mode);
extern bool arith_reg_dest (rtx, machine_mode);
extern bool arith_operand (rtx, machine_mode);
extern bool arith_or_int_operand (rtx, machine_mode);
extern bool arith_reg_or_0_operand (rtx, machine_mode);
extern bool arith_reg_or_0_or_1_operand (rtx, machine_mode);
extern bool clips_min_const_int (rtx, machine_mode);
extern bool clips_max_const_int (rtx, machine_mode);
extern bool clipu_max_const_int (rtx, machine_mode);
extern bool fp_arith_reg_operand (rtx, machine_mode);
extern bool fpscr_operand (rtx, machine_mode);
extern bool fpscr_movsrc_operand (rtx, machine_mode);
extern bool fpscr_movdst_operand (rtx, machine_mode);
extern bool fpul_operand (rtx, machine_mode);
extern bool fpul_fsca_operand (rtx, machine_mode);
extern bool fsca_scale_factor (rtx, machine_mode);
extern bool general_extend_operand (rtx, machine_mode);
extern bool simple_mem_operand (rtx, machine_mode);
extern bool displacement_mem_operand (rtx, machine_mode);
extern bool short_displacement_mem_operand (rtx, machine_mode);
extern bool long_displacement_mem_operand (rtx, machine_mode);
extern bool post_inc_mem (rtx, machine_mode);
extern bool pre_dec_mem (rtx, machine_mode);
extern bool zero_extend_movu_operand (rtx, machine_mode);
extern bool general_movsrc_operand (rtx, machine_mode);
extern bool movsrc_no_disp_mem_operand (rtx, machine_mode);
extern bool general_movdst_operand (rtx, machine_mode);
extern bool unaligned_load_operand (rtx, machine_mode);
extern bool mem_index_disp_operand (rtx, machine_mode);
extern bool logical_operand (rtx, machine_mode);
extern bool const_logical_operand (rtx, machine_mode);
extern bool logical_and_operand (rtx, machine_mode);
extern bool logical_operator (rtx, machine_mode);
extern bool sh_const_vec (rtx, machine_mode);
extern bool sh_1el_vec (rtx, machine_mode);
extern bool sh_rep_vec (rtx, machine_mode);
extern bool shift_count_operand (rtx, machine_mode);
extern bool p27_shift_count_operand (rtx, machine_mode);
extern bool not_p27_shift_count_operand (rtx, machine_mode);
extern bool p27_rshift_count_operand (rtx, machine_mode);
extern bool not_p27_rshift_count_operand (rtx, machine_mode);
extern bool symbol_ref_operand (rtx, machine_mode);
extern bool bitwise_memory_operand (rtx, machine_mode);
extern bool treg_set_expr (rtx, machine_mode);
extern bool treg_set_expr_not_const01 (rtx, machine_mode);
extern bool t_reg_operand (rtx, machine_mode);
extern bool negt_reg_operand (rtx, machine_mode);
extern bool cmpsi_operand (rtx, machine_mode);
extern bool cbranch_treg_value (rtx, machine_mode);
extern bool arith_reg_or_t_reg_operand (rtx, machine_mode);
extern bool arith_reg_or_treg_set_expr (rtx, machine_mode);
extern bool negt_reg_shl31_operand (rtx, machine_mode);
extern bool gbr_displacement (rtx, machine_mode);
extern bool gbr_address_mem (rtx, machine_mode);
extern bool atomic_arith_operand_0 (rtx, machine_mode);
extern bool atomic_mem_operand_0 (rtx, machine_mode);
extern bool atomic_arith_operand_1 (rtx, machine_mode);
extern bool atomic_logical_operand_1 (rtx, machine_mode);
extern bool atomic_mem_operand_1 (rtx, machine_mode);
#endif /* HAVE_MACHINE_MODES */

#ifdef GCC_HARD_REG_SET_H
struct target_constraints {
  HARD_REG_SET register_filters[1];
};

extern struct target_constraints default_target_constraints;
#if SWITCHABLE_TARGET
extern struct target_constraints *this_target_constraints;
#else
#define this_target_constraints (&default_target_constraints)
#endif

#define TEST_REGISTER_FILTER_BIT(ID, REGNO) \
  ((void) (ID), (void) (REGNO), false)

inline bool
test_register_filters (unsigned int, unsigned int)
{
  return true;
}
#endif

#define CONSTRAINT_NUM_DEFINED_P 1
enum constraint_num
{
  CONSTRAINT__UNKNOWN = 0,
  CONSTRAINT_r,
  CONSTRAINT_a,
  CONSTRAINT_c,
  CONSTRAINT_d,
  CONSTRAINT_e,
  CONSTRAINT_f,
  CONSTRAINT_k,
  CONSTRAINT_l,
  CONSTRAINT_t,
  CONSTRAINT_u,
  CONSTRAINT_w,
  CONSTRAINT_x,
  CONSTRAINT_y,
  CONSTRAINT_z,
  CONSTRAINT_I08,
  CONSTRAINT_I16,
  CONSTRAINT_I20,
  CONSTRAINT_I28,
  CONSTRAINT_Jmb,
  CONSTRAINT_Jmw,
  CONSTRAINT_Jhb,
  CONSTRAINT_K03,
  CONSTRAINT_K04,
  CONSTRAINT_K05,
  CONSTRAINT_K08,
  CONSTRAINT_K12,
  CONSTRAINT_K13,
  CONSTRAINT_P27,
  CONSTRAINT_M,
  CONSTRAINT_N,
  CONSTRAINT_Pso,
  CONSTRAINT_Psz,
  CONSTRAINT_m,
  CONSTRAINT_o,
  CONSTRAINT_Sua,
  CONSTRAINT_Sdd,
  CONSTRAINT_Snd,
  CONSTRAINT_Sid,
  CONSTRAINT_Ssd,
  CONSTRAINT_Sbv,
  CONSTRAINT_Sbw,
  CONSTRAINT_Sra,
  CONSTRAINT_Ara,
  CONSTRAINT_Add,
  CONSTRAINT_p,
  CONSTRAINT_G,
  CONSTRAINT_H,
  CONSTRAINT_Bsc,
  CONSTRAINT_Ccl,
  CONSTRAINT_W,
  CONSTRAINT_V,
  CONSTRAINT__l,
  CONSTRAINT__g,
  CONSTRAINT_Q,
  CONSTRAINT_i,
  CONSTRAINT_s,
  CONSTRAINT_n,
  CONSTRAINT_E,
  CONSTRAINT_F,
  CONSTRAINT_X,
  CONSTRAINT_Csy,
  CONSTRAINT_Z,
  CONSTRAINT_Cpg,
  CONSTRAINT__LIMIT
};

extern enum constraint_num lookup_constraint_1 (const char *);
extern const unsigned char lookup_constraint_array[];

/* Return the constraint at the beginning of P, or CONSTRAINT__UNKNOWN if it
   isn't recognized.  */

static inline enum constraint_num
lookup_constraint (const char *p)
{
  unsigned int index = lookup_constraint_array[(unsigned char) *p];
  return (index == UCHAR_MAX
          ? lookup_constraint_1 (p)
          : (enum constraint_num) index);
}

extern bool (*constraint_satisfied_p_array[]) (rtx);

/* Return true if X satisfies constraint C.  */

static inline bool
constraint_satisfied_p (rtx x, enum constraint_num c)
{
  int i = (int) c - (int) CONSTRAINT_I08;
  return i >= 0 && constraint_satisfied_p_array[i] (x);
}

static inline bool
insn_extra_register_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_r && c <= CONSTRAINT_z;
}

static inline bool
insn_extra_memory_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_m && c <= CONSTRAINT_Add;
}

static inline bool
insn_extra_special_memory_constraint (enum constraint_num)
{
  return false;
}

static inline bool
insn_extra_relaxed_memory_constraint (enum constraint_num)
{
  return false;
}

static inline bool
insn_extra_address_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_p && c <= CONSTRAINT_p;
}

static inline void
insn_extra_constraint_allows_reg_mem (enum constraint_num c,
				      bool *allows_reg, bool *allows_mem)
{
  if (c >= CONSTRAINT_G && c <= CONSTRAINT_W)
    return;
  if (c >= CONSTRAINT_V && c <= CONSTRAINT_Q)
    {
      *allows_mem = true;
      return;
    }
  (void) c;
  *allows_reg = true;
  *allows_mem = true;
}

static inline size_t
insn_constraint_len (char fc, const char *str ATTRIBUTE_UNUSED)
{
  switch (fc)
    {
    case 'A': return 3;
    case 'B': return 3;
    case 'C': return 3;
    case 'I': return 3;
    case 'J': return 3;
    case 'K': return 3;
    case 'P': return 3;
    case 'S': return 3;
    default: break;
    }
  return 1;
}

#define CONSTRAINT_LEN(c_,s_) insn_constraint_len (c_,s_)

extern enum reg_class reg_class_for_constraint_1 (enum constraint_num);

static inline enum reg_class
reg_class_for_constraint (enum constraint_num c)
{
  if (insn_extra_register_constraint (c))
    return reg_class_for_constraint_1 (c);
  return NO_REGS;
}

extern bool insn_const_int_ok_for_constraint (HOST_WIDE_INT, enum constraint_num);
#define CONST_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    insn_const_int_ok_for_constraint (v_, lookup_constraint (s_))

enum constraint_type
{
  CT_REGISTER,
  CT_CONST_INT,
  CT_MEMORY,
  CT_SPECIAL_MEMORY,
  CT_RELAXED_MEMORY,
  CT_ADDRESS,
  CT_FIXED_FORM
};

static inline enum constraint_type
get_constraint_type (enum constraint_num c)
{
  if (c >= CONSTRAINT_p)
    {
      if (c >= CONSTRAINT_G)
        return CT_FIXED_FORM;
      return CT_ADDRESS;
    }
  if (c >= CONSTRAINT_m)
    return CT_MEMORY;
  if (c >= CONSTRAINT_I08)
    return CT_CONST_INT;
  return CT_REGISTER;
}

#ifdef GCC_HARD_REG_SET_H
static inline const HARD_REG_SET *
get_register_filter (constraint_num)
{
  return nullptr;
}
#endif

static inline int
get_register_filter_id (constraint_num)
{
  return -1;
}
#endif /* tm-preds.h */
