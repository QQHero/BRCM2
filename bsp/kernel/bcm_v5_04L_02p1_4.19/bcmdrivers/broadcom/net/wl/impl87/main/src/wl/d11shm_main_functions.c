
/* ucode_std shmdefs selection function */
void BCMRAMFN(d11shm_select_ucode_std)(const shmdefs_t **shmdefs, int d11rev)
{
	switch(d11rev) {
	case 30:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_30;
		break;
	case 42:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_42;
		break;
	case 49:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_49;
		break;
	case 65:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_65;
		break;
	case 129:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_129;
		break;
	case 130:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_130;
		break;
	case 131:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_131;
		break;
	case 132:
		*shmdefs = &d11shm_struct_ucode_std_d11rev_132;
		break;
	}
}
