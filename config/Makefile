
TARGETS_DIR = $(TOP_PATH)/targets/$(TARGETS_FILE)
HOSTCC="/usr/bin/gcc"
CC="/usr/bin/gcc"
all:
	make -C kconfig -f Makefile.br mconf
#	make -C kconfig -f Makefile.br conf
	export UGW_CONFIG=$(TARGETS_DIR)/config.mk;\
	kconfig/mconf config.in;
#	kconfig/conf --silentoldconfig config.in
	md5sum $(TARGETS_DIR)/config.mk > $(TARGETS_DIR)/config.mk.md5 
	@echo TARGETS_DIR=$(TARGETS_DIR)
clean:
	make -C kconfig -f Makefile.br clean
distclean:
	make -C kconfig -f Makefile.br distclean
	
export TARGETS_DIR