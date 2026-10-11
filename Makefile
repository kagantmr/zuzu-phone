BOARD ?= phone
all run smoke clean menuconfig defconfig compile_commands.json:
	$(MAKE) -C zuzu EXT=$(CURDIR) BOARD=$(BOARD) $@