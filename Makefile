default: help

define HELP_MAKE_STR
 Targets:

    test          test it
    bin           build it
    download      get tools
    bootstrap     setup repo
    ci            build for ci
    help          print this message

 Usage: make target [target2]... [options]
    make download bootstrap
    make bin
endef

help: detectCompiler
	@:$(info $(HELP_MAKE_STR))

download: | compiler
	if [ ! -e $(CROSS_CC_DIR) ]; then curl -LSf -o compiler/$(TOOL_FILE) $(TOOL_URL); fi
	if [ ! -e $(CROSS_CC_DIR) ]; then cd compiler; $(EXPAND_CMD) $(TOOL_FILE) && \
	mv $(EXPANDED_DIR) ../$(CROSS_CC_DIR); fi
	@echo Compiler installed: $(CROSS_CC_DIR) Common practice is to move and simlink, see readme.md

CROSS_CC_DIR=compiler/arm-gcc-13.2
detectCompiler: $(CROSS_CC_DIR)
# No fancy detection, just sees if prerequisite CROSS_CC exists
compiler: ; mkdir -p compiler
$(CROSS_CC_DIR):
	@echo "Welcome to a Fluke Electronics project."
	@echo "No tools detected. Run 'make download; make bootstrap' to get started."
# We don't force auto downloads of compiler

# Cross-OS compatibility
EXPANDED_DIR=$$(tar -tf $(TOOL_FILE) | awk -F'/' '{print $$1; exit 0;}')
EXPAND_CMD=tar -xf
ifeq ($(OS), Windows_NT) #Windows
  TOOL_URL=https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-mingw-w64-i686-arm-none-eabi.zip
  EXPANDED_DIR=$$(unzip -qql $(TOOL_FILE) | awk '{print $$4; exit 0;}')
  EXPAND_CMD=unzip -q
else ifeq ($(shell uname -s), Darwin) #MacOSX
  TOOL_URL=https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-darwin-x86_64-arm-none-eabi.tar.xz
else #Linux
  TOOL_URL=https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz
endif
TOOL_FILE=$(notdir $(TOOL_URL))
# Originally downloaded from: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# Backup saved to:
# Past compilers: arm-gcc-13.2

# Can run tools/bootstrap.sh OR just put commands below.
bootstrap:
	command -v python || echo "Python 3 not found. Install it"
	python -c "import sys; sys.version_info < (3,8) and sys.exit(1)" || echo "Warning: Python < 3.8"
	@# command -v gcc || sudo apt-get install -y gcc
	cmake --version | awk -F'[ .]' '{if ($$3 != 3) exit 3; if ($$4 < 20) exit 2; exit 0;}' \
	  || echo "Warning: Zephyr requires cmake 3.20 or greater, see docs.zephyrproject.org/latest/develop/getting_started/index.html"
	command -v west || pip install west
	test -e .west/ || west init -l manifest
	test -e zephyr/zephyr/VERSION || west update
	python -m pip install -r zephyr/zephyr/scripts/requirements-base.txt

list-targets:
	$(MAKE) -rnp | awk '/^[^# .\t][^# .]*:/'
test:
bin: build/CMakeCache.txt
	cmake --build build/ -- -sr -j8
build/CMakeCache.txt:
	ZEPHYR_BASE=zephyr/zephyr \
	ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb \
	GNUARMEMB_TOOLCHAIN_PATH=$(abspath $(CROSS_CC_DIR)) \
	cmake -B build/ -DBOARD=lpcxpresso55s28 -G"Unix Makefiles"
ci: bin

.PHONY: default help download detectCompiler bootstrap test bin ci
