
/* regdefs selection function */
#ifndef DONGLEBUILD
int BCMRAMFN(d11regs_select_offsets_tbl)(const d11regdefs_t **regdefs, int d11rev)
{
	int ret = 0;
	switch(d11rev) {
	case 30:
		*regdefs = &d11regs_struct_d11rev_30;
		break;
	case 42:
		*regdefs = &d11regs_struct_d11rev_42;
		break;
	case 49:
		*regdefs = &d11regs_struct_d11rev_49;
		break;
	case 65:
		*regdefs = &d11regs_struct_d11rev_65;
		break;
	case 129:
		*regdefs = &d11regs_struct_d11rev_129;
		break;
	case 130:
		*regdefs = &d11regs_struct_d11rev_130;
		break;
	case 131:
		*regdefs = &d11regs_struct_d11rev_131;
		break;
	case 132:
		*regdefs = &d11regs_struct_d11rev_132;
		break;
		default:
		ret = -1;	}
	return ret;
}
#endif /* not-DONGLEBUILD */
