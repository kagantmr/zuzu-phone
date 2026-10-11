BOARD ?= phone

all run smoke clean menuconfig defconfig:
	$(MAKE) -C zuzu EXT=$(CURDIR) BOARD=$(BOARD) $@

.PHONY: compile_commands.json
compile_commands.json:
	$(MAKE) -C zuzu EXT=$(CURDIR) BOARD=$(BOARD) compile_commands.json
	cp zuzu/compile_commands.json $@