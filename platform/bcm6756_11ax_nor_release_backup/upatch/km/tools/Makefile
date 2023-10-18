

apps_module_dir_y = debug_km

all: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


