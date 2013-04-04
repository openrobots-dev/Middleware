# List of all the RTCAN files.
R2MWCPPSRC = ${R2MW}/BaseMessage.cpp \
	${R2MW}/MessageQueue.cpp \
	${R2MW}/BasePublisher.cpp \
	${R2MW}/BaseSubscriber.cpp \
	${R2MW}/LocalPublisher.cpp \
	${R2MW}/LocalSubscriber.cpp \
	${R2MW}/RemotePublisher.cpp \
	${R2MW}/RemoteSubscriber.cpp \
	${R2MW}/Node.cpp \
	${R2MW}/Middleware.cpp \
	${R2MW}/transport/LedTransport.cpp \
	${R2MW}/transport/RTcan.cpp
	
	
# Required include directories
R2MWINC = ${R2MW} ${R2MW}/msg
