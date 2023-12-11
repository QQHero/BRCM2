	.arch armv7-a
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 2	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 2	@ Tag_ABI_PCS_wchar_t
	.file	"bounds.c"
@ GNU C89 (Buildroot 2019.11.1) version 9.2.0 (arm-buildroot-linux-gnueabi)
@	compiled by GNU C version 4.8.1, GMP version 6.1.2, MPFR version 4.0.2, MPC version 1.1.0, isl version isl-0.18-GMP

@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I ./arch/arm/include
@ -I ./arch/arm/include/generated -I ./include -I ./bcmkernel/include
@ -I ./arch/arm/include/uapi -I ./arch/arm/include/generated/uapi
@ -I ./include/uapi -I ./include/generated/uapi
@ -I bcmdrivers/opensource/include/bcm963xx/
@ -I /HomeCoverage20231207/bsp/kernel/bcm_v5_04L_02p1_4.19/bcmkernel/include
@ -I /HomeCoverage20231207/bsp/kernel/bcm_v5_04L_02p1_4.19/shared/opensource/include/bcm963xx
@ -I /HomeCoverage20231207/bsp/kernel/bcm_v5_04L_02p1_4.19/bcmkernel/include/uapi
@ -I /HomeCoverage20231207/bsp/kernel/bcm_v5_04L_02p1_4.19/net/bridge
@ -iprefix /projects/hnd/tools/bcm/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/bin/../lib/gcc/arm-buildroot-linux-gnueabi/9.2.0/
@ -isysroot /projects/hnd/tools/bcm/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/arm-buildroot-linux-gnueabi/sysroot
@ -D __KERNEL__ -D __LINUX_ARM_ARCH__=7 -U arm -D BCA_HNDROUTER
@ -D BCA_CPEROUTER -D KBUILD_BASENAME="bounds" -D KBUILD_MODNAME="bounds"
@ -isystem /projects/hnd/tools/bcm/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/bin/../lib/gcc/arm-buildroot-linux-gnueabi/9.2.0/include
@ -include ./include/linux/kconfig.h
@ -include ./include/linux/compiler_types.h -MD kernel/.bounds.s.d
@ kernel/bounds.c -mlittle-endian -mabi=aapcs-linux -mfpu=vfp -marm
@ -mfloat-abi=soft -mtls-dialect=gnu -march=armv7-a
@ -auxbase-strip kernel/bounds.s -Os -O2 -Wall -Wundef -Wstrict-prototypes
@ -Wno-trigraphs -Werror=implicit-function-declaration -Werror=return-type
@ -Wno-format-security -Wno-frame-address -Wformat-truncation=0
@ -Wformat-overflow=0 -Wno-int-in-bool-context
@ -Wno-address-of-packed-member -Wframe-larger-than=2048
@ -Wno-unused-but-set-variable -Wunused-const-variable=0
@ -Wdeclaration-after-statement -Wno-pointer-sign -Wno-stringop-truncation
@ -Wstringop-overflow=0 -Wno-restrict -Werror=implicit-int
@ -Werror=strict-prototypes -Werror=date-time
@ -Werror=incompatible-pointer-types -Werror=designated-init
@ -Wno-packed-not-aligned -Wno-array-bounds -Wno-maybe-uninitialized
@ -std=gnu90 -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE
@ -fno-dwarf2-cfi-asm -fno-ipa-sra -funwind-tables
@ -fno-delete-null-pointer-checks -fstack-protector-strong
@ -fomit-frame-pointer -fno-var-tracking-assignments -fno-strict-overflow
@ -fno-merge-all-constants -fmerge-constants -fstack-check=no
@ -fconserve-stack -fmacro-prefix-map=./= -fverbose-asm
@ --param allow-store-data-races=0
@ options enabled:  -faggressive-loop-optimizations -falign-functions
@ -falign-jumps -falign-labels -falign-loops -fassume-phsa -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fcode-hoisting
@ -fcombine-stack-adjustments -fcompare-elim -fcprop-registers
@ -fcrossjumping -fcse-follow-jumps -fdefer-pop -fdevirtualize
@ -fdevirtualize-speculatively -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffp-int-builtin-inexact -ffunction-cse -fgcse
@ -fgcse-lm -fgnu-runtime -fgnu-unique -fguess-branch-probability
@ -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-atomics
@ -finline-functions-called-once -finline-small-functions -fipa-bit-cp
@ -fipa-cp -fipa-icf -fipa-icf-functions -fipa-icf-variables -fipa-profile
@ -fipa-pure-const -fipa-ra -fipa-reference -fipa-reference-addressable
@ -fipa-stack-alignment -fipa-vrp -fira-hoist-pressure
@ -fira-share-save-slots -fira-share-spill-slots
@ -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
@ -fleading-underscore -flifetime-dse -flra-remat -flto-odr-type-merging
@ -fmath-errno -fmerge-constants -fmerge-debug-strings
@ -fmove-loop-invariants -fomit-frame-pointer -foptimize-sibling-calls
@ -foptimize-strlen -fpartial-inlining -fpeephole -fpeephole2 -fplt
@ -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns
@ -fschedule-insns2 -fsection-anchors -fsemantic-interposition
@ -fshow-column -fshrink-wrap -fshrink-wrap-separate -fsigned-zeros
@ -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-backprop -fssa-phiopt
@ -fstack-protector-strong -fstdarg-opt -fstore-merging
@ -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
@ -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp -ftree-builtin-call-dce
@ -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop -ftree-cselim
@ -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
@ -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr
@ -ftree-sra -ftree-switch-conversion -ftree-tail-merge -ftree-ter
@ -ftree-vrp -funit-at-a-time -funwind-tables -fverbose-asm -fwrapv
@ -fwrapv-pointer -fzero-initialized-in-bss -marm -mbe32 -mglibc
@ -mlittle-endian -mpic-data-is-text-relative -msched-prolog
@ -munaligned-access -mvectorize-with-neon-quad

	.text
	.syntax divided
	.syntax unified
	.arm
	.syntax unified
	.section	.text.startup,"ax",%progbits
	.align	2
	.global	main
	.syntax unified
	.arm
	.fpu softvfp
	.type	main, %function
main:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
@ kernel/bounds.c:19: 	DEFINE(NR_PAGEFLAGS, __NR_PAGEFLAGS);
	.syntax divided
@ 19 "kernel/bounds.c" 1
	
.ascii "->NR_PAGEFLAGS #21 __NR_PAGEFLAGS"	@
@ 0 "" 2
@ kernel/bounds.c:20: 	DEFINE(MAX_NR_ZONES, __MAX_NR_ZONES);
@ 20 "kernel/bounds.c" 1
	
.ascii "->MAX_NR_ZONES #3 __MAX_NR_ZONES"	@
@ 0 "" 2
@ kernel/bounds.c:22: 	DEFINE(NR_CPUS_BITS, ilog2(CONFIG_NR_CPUS));
@ 22 "kernel/bounds.c" 1
	
.ascii "->NR_CPUS_BITS #2 ilog2(CONFIG_NR_CPUS)"	@
@ 0 "" 2
@ kernel/bounds.c:24: 	DEFINE(SPINLOCK_SIZE, sizeof(spinlock_t));
@ 24 "kernel/bounds.c" 1
	
.ascii "->SPINLOCK_SIZE #4 sizeof(spinlock_t)"	@
@ 0 "" 2
@ kernel/bounds.c:28: }
	.arm
	.syntax unified
	mov	r0, #0	@,
	bx	lr	@
	.fnend
	.size	main, .-main
	.ident	"GCC: (Buildroot 2019.11.1) 9.2.0"
	.section	.note.GNU-stack,"",%progbits
