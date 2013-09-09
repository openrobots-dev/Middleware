ifeq ($(R2PSRC),)
  $(error r2p.mk must be inclused before port.mk)
endif

R2PSRC += $(R2P)/port/chibios/src/impl/Bootloader_.cpp \
          $(R2P)/port/chibios/src/impl/Flasher_.cpp \
          $(R2P)/port/chibios/src/impl/Middleware_.cpp \
          $(R2P)/port/chibios/src/impl/Time_.cpp


R2PINC += $(R2P)/port/chibios/include
