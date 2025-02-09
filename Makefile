ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
TOPS = -Wall -Iinclude -g
ASMOPS = -Iinclude 

BUILD_DIR = build
BUILD_NATIVE_DIR = build-native
SRC_DIR = src
TEST_SRC_DIR = test-src

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR) *.img 
	rm -rf $(BUILD_NATIVE_DIR)
	rm test.exe

$(BUILD_NATIVE_DIR)/%_c.o : $(TEST_SRC_DIR)/%.c
	mkdir -p $(@D)
	gcc $(TOPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
TEST_C_FILES = $(wildcard $(TEST_SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

NATIVE_OBJ_FILES = $(TEST_C_FILES:$(TEST_SRC_DIR)/%.c=$(BUILD_NATIVE_DIR)/%_c.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)


NATIVE_DEP_FILES = $(NATIVE_OBJ_FILES:%.o=%.d)
-include $(NATIVE_DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

test : $(NATIVE_OBJ_FILES)
	gcc $(NATIVE_OBJ_FILES) -o test.exe