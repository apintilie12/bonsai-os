ARMGNU ?= aarch64-linux-gnu

COPS   = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -g
ASMOPS = -Iinclude -g

BUILD_DIR = build

all: kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

# --- Source files by module ---
ARCH_S   = $(wildcard src/arch/aarch64/*.S)
KERNEL_C = $(wildcard src/kernel/*.c)
MM_C     = $(wildcard src/mm/*.c)
DRIVER_C = $(wildcard src/drivers/serial/*.c) $(wildcard src/drivers/timer/*.c) $(wildcard src/drivers/sd/*.c)
LIB_C    = $(wildcard src/lib/*.c)
TEST_C   = $(wildcard src/tests/*.c)

ALL_C = $(KERNEL_C) $(MM_C) $(DRIVER_C) $(LIB_C) $(TEST_C)
ALL_S = $(ARCH_S)

# --- Object files ---
OBJ_FILES  = $(ALL_C:%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ALL_S:%.S=$(BUILD_DIR)/%_s.o)

# --- Dependency files ---
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

# --- Compilation rules ---
$(BUILD_DIR)/%_c.o: %.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: %.S
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

kernel8.img: src/arch/aarch64/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T src/arch/aarch64/linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img
