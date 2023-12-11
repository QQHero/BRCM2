PWD := $(shell pwd)

subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS = $(KM_PATH)/kernel/bm/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

GCC_VERSION = $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.` \< 5.5 | sed -e 's/\./*100+/g' | bc )

ifeq ($(GCC_VERSION), 1)
GCC_OPTIMIZATIONS = $(shell $(CC) --help=optimizers | grep faggressive-loop-optimizations)
endif

ifneq ($(GCC_OPTIMIZATIONS), )
EXTRA_CFLAGS += -fno-aggressive-loop-optimizations
endif

SUB_DIR := regexp_posix bm_algorithm dict_tree http_analyze ac_algorithm multi_tree
REG_DIR := regexp_posix
BM_DIR  := bm_algorithm
DICT_DIR := dict_tree
HTTP_DIR := http_analyze
ACA_DIR := ac_algorithm
MULTI_DIR := multi_tree

DEPEND_OBJ += 	ai_main.o \
		ai_feat_load.o \
		ai_feat_match.o \
		ai_dpi.o \
		k_sscanf.o \
		$(REG_DIR)/regcomp.o \
		$(REG_DIR)/regex_internal.o \
		$(REG_DIR)/regexec.o \
		$(BM_DIR)/ai_bm_search.o \
		$(ACA_DIR)/ai_acsmx2_search.o \
		$(HTTP_DIR)/ai_http_parse.o \
		$(DICT_DIR)/ft_dic_tree.o \
		$(DICT_DIR)/binary_tree.o \
		$(MULTI_DIR)/ai_multi_tree_search.o \

ai-objs := $(DEPEND_OBJ)
obj-m += ai.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 
all:

	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) TENDA_EXTRA_CFLAGS='$(EXTRA_CFLAGS)' M=$(PWD) modules

	$(CROSS_STRIP) -S *.ko

clean:
	for i in $(SUB_DIR) ;\
	do rm -rf $$i/*.cmd $$i/*.o.cmd $$i/.*.cmd $$i/.*.o.cmd;\
	done
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
