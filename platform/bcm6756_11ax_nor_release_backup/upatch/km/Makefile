
apps_module_dir_y = kernel
apps_module_dir_y += user
apps_module_dir_y += tools
KM_SVN_VERSION := $(shell svn info|sed -n '/^Revision/{p}'|awk '{print $$2}')
PWD := $(shell pwd)
$(shell echo $(KM_SVN_VERSION) > $(PWD)/km_version)

KM_SVN_PATH := $(shell svn info|sed -n '/^URL/{p}'|awk '{print $$2}')
$(shell echo $(KM_SVN_PATH) >> $(PWD)/km_version)


all:
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


