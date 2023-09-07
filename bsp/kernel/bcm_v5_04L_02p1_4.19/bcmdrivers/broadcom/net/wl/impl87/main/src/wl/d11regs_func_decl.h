extern void d11regs_select_regdefs(int d11rev);
#ifdef DONGLEBUILD
#define d11regs_select_offsets_tbl(a, b) (b & 0)
#else /* not-DONGLEBUILD */
extern int d11regs_select_offsets_tbl(const d11regdefs_t **regdefs, int d11rev);
#endif /* not-DONGLEBUILD */
