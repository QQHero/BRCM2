
EXEC = km_cmd
OBJS = cmd.o cJSON.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 
all: $(EXEC) 

$(EXEC): $(OBJS)
	$(CC)  -o $@ $(OBJS) -ldl -lm
	$(CROSS_STRIP) $(EXEC)
	
clean:
ifneq (0,$(words $(file)))
	-rm -f $(EXEC) *.elf *.gdb 
else
	-rm -f $(EXEC) *.elf *.gdb
endif
 
