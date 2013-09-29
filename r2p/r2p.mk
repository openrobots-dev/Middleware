R2PSRC = $(R2P)/src/impl/ArrayQueue_.cpp \
         $(R2P)/src/impl/SimplePool_.cpp \
         $(R2P)/src/impl/StaticList_.cpp \
         $(R2P)/src/impl/StaticQueue_.cpp \
         \
         $(R2P)/src/BasePublisher.cpp \
         $(R2P)/src/BaseSubscriber.cpp \
         $(R2P)/src/BaseSubscriberQueue.cpp \
		 $(R2P)/src/BootMsg.cpp \
		 $(R2P)/src/Checksummer.cpp \
         $(R2P)/src/LocalPublisher.cpp \
         $(R2P)/src/LocalSubscriber.cpp \
         $(R2P)/src/MessagePtrQueue.cpp \
		 $(R2P)/src/Message.cpp \
         $(R2P)/src/Middleware.cpp \
         $(R2P)/src/Node.cpp \
         $(R2P)/src/RemotePublisher.cpp \
         $(R2P)/src/RemoteSubscriber.cpp \
         $(R2P)/src/Time.cpp \
         $(R2P)/src/TimestampedMsgPtrQueue.cpp \
         $(R2P)/src/Topic.cpp \
         $(R2P)/src/Transport.cpp \
         $(R2P)/src/Utils.cpp \
#

ifeq ($(R2P_USE_BOOTLOADER),)
R2P_USE_BOOTLOADER = yes
endif
ifeq ($(R2P_USE_BOOTLOADER),yes)
R2PSRC += $(R2P)/src/Bootloader.cpp
endif

ifeq ($(R2P_USE_DEBUGTRANSPORT),yes)
R2PSRC += $(R2P)/src/transport/DebugTransport.cpp \
          $(R2P)/src/transport/DebugPublisher.cpp \
          $(R2P)/src/transport/DebugSubscriber.cpp
endif
ifeq ($(R2P_USE_RTCANTRANSPORT),yes)
R2PSRC += $(R2P)/src/transport/RTCANTransport.cpp \
          $(R2P)/src/transport/RTCANPublisher.cpp \
          $(R2P)/src/transport/RTCANSubscriber.cpp
endif

R2PINC = $(R2P)/include \
#

R2PNODES = $(R2P)/src/node
