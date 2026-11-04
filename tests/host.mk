P := runtime
D := deps
B := build/host
CPPFLAGS := -I$(P) -I$(D)/potator -I$(D)/vrEmu6502/src -include $(P)/sv_config.h
CFLAGS := -O2 -g -funsigned-char -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -MMD -MP
CXXFLAGS := $(CFLAGS) -std=gnu++17
LDFLAGS := -fsanitize=address,undefined -no-pie
SV := $(addprefix $(B)/,sv_runtime.o sv_pcfilter_core.o sv_fixed_boot.o controls.o memorymap.o gpu.o sound.o timer.o clock.o)
all: $(B)/sv
$(B)/sv: $(B)/host_test.o $(SV)
	$(CXX) $(LDFLAGS) $^ -lm -o $@
$(B)/host_test.o: tests/host_test.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(B)/clock.o: tests/clock.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(B)/%.o: $(P)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(B)/%.o: $(D)/potator/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
-include $(wildcard $(B)/*.d)
