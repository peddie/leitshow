### Standard Makefile template
### Copyright (C) Matthew Peddie <peddie@alum.mit.edu>
###
### This file is hereby placed in the public domain, or, if your legal
### system doesn't recognize this concept, you may consider it
### licensed under the WTFPL version 2.0 or any BSD license you
### choose.
###
### This file should be all you need to configure a basic project;
### obviously for more complex projects, you'll need to edit the other
### files as well.  It supports only one project at a time.  Type
### ``make help'' for usage help.

# What's the executable called?
PROJ = main

COMMON_DIR = ../../cosmogia/common
OPENCM3_DIR = $(COMMON_DIR)/libopencm3/
LDSCRIPT ?= stm32f4-discovery.ld
DEP_PKG = python-yaml autoconf libusb-1.0-0-dev doxygen doxygen-latex

# What C or C++ files must we compile to make the executable?
SRC ?= $(PROJ).c \
       adc.c \
       usart.c \
       timer.c \
       pwm.c

SIZECUT = --specs=nano.specs

QUIET ?=

# What additional headers are needed as dependencies?
HDR ?=

# What directories must we include?
INCLUDENAMES ?= $(OPENCM3_DIR)/include # e.g. ../mathlib; the makefile will add the -I

# With what libraries should we link?
LIBNAMES ?= opencm3_stm32f4 # e.g. m gsl atlas; the makefile will add the -l
LIBDIRS ?= $(OPENCM3_DIR)/lib $(OPENCM3_DIR)/lib/stm32/f4 # e.g. ../; the makefile will add the -L
OTHERLIB ?=

# You can add custom compiler and linker flags here.  USERFLAGS gets
# used by all compiler and linker calls, except when creating a static
# lib.  The others are specific to their stage.
#
# Add -DCSMG_DEBUG_ESP here to run the debug port (USART2) as a
# packetized IO port for use with supermux.
USERFLAGS ?= -Os -ffunction-sections -fdata-sections -fno-common -g
USERCFLAGS ?=
USERCXXFLAGS ?=
USERLDFLAGS ?= $(SIZECUT) $(RELOCATE) -lm
USERCLEAN ?=

# Unit tests?
# TESTS_SRC ?=

# What's the extension on C++ files?  .cc is the Google default, but
# lots of people use .cpp instead.
CXX_EXT = cc

MKFILE_DIR = $(COMMON_DIR)/Makefile-template/

NO_WERROR = 22

.PHONY: all deps toolchain clean-docs opencm3
all: $(PROJ:%=%.images) $(PROJ:%=%.size) $(CM4_PROJ)

DOC_DIR = docs
DEPS = $(OPENCM3_DIR)

# Use the Google C++ linter on C files, rather than the 'splint' command
C_LINT = $(COMMON_DIR)/../dev_tools/bin/cpplint.py

# Use the Google C++ linter for flymake targets.
EXTRA_CHECKS = lint

include $(MKFILE_DIR)common_head.mk
include $(MKFILE_DIR)cortex-m4.mk
include $(MKFILE_DIR)build.mk
include $(MKFILE_DIR)test.mk
include $(MKFILE_DIR)syntax.mk
include $(MKFILE_DIR)clean.mk
include $(MKFILE_DIR)docs.mk


TOOLCHAIN_NAME = gcc-arm-none-eabi-4_7-2012q4
TOOLCHAIN_TARBALL = https://launchpad.net/gcc-arm-embedded/4.7/4.7-2012-q4-major/+download/$(TOOLCHAIN_NAME)-20121208-linux.tar.bz2
TOOLCHAIN_STOW_DIR = /usr/local/stow
STOW = xstow
APT_GET = apt-get
SUDO = sudo

START_DIR = $(shell pwd)

toolchain: $(TOOLCHAIN_STOW_DIR)/$(TOOLCHAIN_NAME)/bin/$(CC)

$(TOOLCHAIN_STOW_DIR)/$(TOOLCHAIN_NAME)/bin/$(CC):
	@echo "Couldn't find $(TOOLCHAIN_STOW_DIR)/$(TOOLCHAIN_NAME)/bin/$(CC)"
	@echo
	@echo "============================================================================="
	@echo "Installing gcc-arm-embedded toolchain for Cortex M{0,3,4}."
	@echo "We're using version '$(TOOLCHAIN_NAME)'."
	@echo "This requires superuser privileges (using '$(SUDO)')."
	@echo "The package '$(STOW)' will be installed."
	@echo "The directory '$(TOOLCHAIN_STOW_DIR)' will be created."
	@echo "You can remove this toolchain by going to '$(TOOLCHAIN_STOW_DIR)' and running"
	@echo
	@echo "    sudo xstow -D $(TOOLCHAIN_NAME)"
	@echo
	@echo "Please make sure that '/usr/local/bin' is in your PATH (add it in ~/.bashrc)."
	@echo "============================================================================="
	@echo
	$(Q)$(SUDO) $(APT_GET) install $(STOW)
	$(Q)$(SUDO) mkdir -p $(TOOLCHAIN_STOW_DIR) && cd $(TOOLCHAIN_STOW_DIR); \
	            wget -O - "$(TOOLCHAIN_TARBALL)" | $(SUDO) tar -xj; \
	            $(SUDO) $(STOW) $(TOOLCHAIN_NAME); \
                    cd $(START_DIR)

deps: opencm3
	@echo DEPS: $(DEPS) $(DEP_PKG)
	$(Q)$(SUDO) $(APT_GET) install $(DEP_PKG)

opencm3:
	@echo "Running libopencm make"
	$(Q)$(MAKE) -C $(OPENCM3_DIR)


include $(MKFILE_DIR)common_tail.mk
