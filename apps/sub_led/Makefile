##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -fomit-frame-pointer -fverbose-asm -O0 -ggdb
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti -fno-exceptions -fno-threadsafe-statics
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = no
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Enable this if you really want to use the STM FWLib.
ifeq ($(USE_FWLIB),)
  USE_FWLIB = no
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = sub_led

# Imported source files and paths
include ../common/paths.mk
include $(CHIBIOS)/os/hal/platforms/STM32F1xx/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/ports/GCC/ARMCMx/STM32F1xx/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk
include $(BOARD)/board.mk
include $(R2P)/r2p.mk
include $(R2P)/port/chibios/port.mk

# Define linker script file here
LDSCRIPT= $(BOARD)/STM32F103xB_bootloader.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = 

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = ../common/common.cpp \
         sub_led.cpp

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#	   option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#	   option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#	   option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#	   option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(PORTASM)

INCDIR = $(PORTINC) $(KERNINC) $(TESTINC) \
		 $(HALINC) $(PLATFORMINC) $(R2PINC) \
		 $(BOARD)/$(BOARDINC) \
		 $(CHIBIOS)/os/various \
		 ../common

SYS_ELF = $(BOARD)/build/ch.elf

# Application (R2P Node) name
ifeq ($(APP_NAME),)
	APP_NAME = $(PROJECT)
endif

# Module name
ifeq ($(BOOT_MODULE),)
    BOOT_MODULE = IMU_GW
endif

# Bootloader topic name
ifeq ($(BOOT_TOPIC),)
	BOOT_TOPIC = BOOT_$(BOOT_MODULE)
endif

# Application stack length
ifeq ($(APP_STACK_SIZE),)
	APP_STACK_SIZE = 1024
endif

# Parameter default (invalid) values
ifeq ($(PARAM_OFFSET),)
	PARAM_OFFSET = 0
endif

ifeq ($(PARAM_LENGTH),)
	PARAM_LENGTH = 0
endif

ifeq ($(PARAM_BYTES),)
	PARAM_BYTES = "" # Hex byte string
endif

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

MCU  = cortex-m3

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#	   runtime support makes code size explode.
LD   = $(TRGT)gcc -Wl,--just-symbols=$(SYS_ELF)
#LD   = $(TRGT)g++ -Wl,--just-symbols=$(SYS_ELF)
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
DDEFS = -DPORT_INT_REQUIRED_STACK=128 -DR2P_MODULE_NAME="$(BOOT_MODULE)"

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS = -lm

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS +=

# Define ASM defines here
UADEFS +=

# List all user directories here
UINCDIR +=

# List the user directory to look for the libraries here
ULIBDIR +=

# List all user libraries here
ULIBS +=

#
# End of user defines
##############################################################################

ifeq ($(USE_FWLIB),yes)
	include $(CHIBIOS)/ext/stm32lib/stm32lib.mk
	CSRC += $(STM32SRC)
	INCDIR += $(STM32INC)
	USE_OPT += -DUSE_STDPERIPH_DRIVER
endif

$(shell mkdir -p build/obj build/lst)

include $(CHIBIOS)/os/ports/GCC/ARMCMx/rules.mk

load: $(OBJS) $(LDSCRIPT) all
	@rm -f $(BUILDDIR)/$(PROJECT).hex $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).dmp
	
	@"$(R2PY)/load.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		--app-name            "$(APP_NAME)" \
		--app-stack-size      "$(APP_STACK_SIZE)" \
		--sys-elf             "$(SYS_ELF)" \
		--app-elf             "$(BUILDDIR)/$(PROJECT).elf" \
		--app-hex             "$(BUILDDIR)/$(PROJECT).hex" \
		--ld-cmd              "$(TRGT)ld" \
		--ld-script           "$(LDSCRIPT)" \
		--ld-map              "$(BUILDDIR)/$(PROJECT).map" \
		--ld-objects          $(OBJS) \
		#

getappinfo: $(OBJ) $(LDSCRIPT) all
	@"$(R2PY)/getappinfo.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		#

getparam: $(OBJ) $(LDSCRIPT) all
	@"$(R2PY)/getparam.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		--app-name            "$(APP_NAME)" \
		--param-offset        "$(PARAM_OFFSET)" \
		--param-length        "$(PARAM_LENGTH)" \
		#

setparam: $(OBJ) $(LDSCRIPT) all
	@"$(R2PY)/setparam.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		--app-name            "$(APP_NAME)" \
		--param-offset        "$(PARAM_OFFSET)" \
		--param-bytes         "$(PARAM_BYTES)" \
		#

remove_last:
	@"$(R2PY)/remove.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		--last \
		#

remove_all:
	@"$(R2PY)/remove.py" \
		$(VERBOSITY) \
		--boot-topic          "$(BOOT_TOPIC)" \
		--boot-module         "$(BOOT_MODULE)" \
		--all \
		#

reboot:
	@"$(R2PY)/reboot.py" \
		$(VERBOSITY) \
		--boot-module         "$(BOOT_MODULE)" \
		#

##############################################################################
