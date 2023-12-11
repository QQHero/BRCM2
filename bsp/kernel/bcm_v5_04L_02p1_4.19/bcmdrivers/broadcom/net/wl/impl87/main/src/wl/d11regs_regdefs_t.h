typedef struct d11regdefs_struct {
	const d11regs_regs_common_lt40_t *regs_common_lt40;
	const d11regs_regs_common_ge40_lt64_t *regs_common_ge40_lt64;
	const d11regs_regs_common_ge64_lt80_t *regs_common_ge64_lt80;
	const d11regs_regs_common_ge80_lt128_t *regs_common_ge80_lt128;
	const d11regs_regs_common_ge128_lt129_t *regs_common_ge128_lt129;
	const d11regs_regs_common_ge129_t *regs_common_ge129;
	const d11regs_regs_common_ge130_t *regs_common_ge130;
	const d11regs_regs_common_ge131_t *regs_common_ge131;
	const d11regs_regs_common_ge132_t *regs_common_ge132;
} d11regdefs_t;
