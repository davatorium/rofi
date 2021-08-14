noinst_LIBRARIES += \
	libgwater-xcb.a

libgwater_xcb_a_SOURCES = \
	subprojects/libgwater/xcb/libgwater-xcb.c \
	subprojects/libgwater/xcb/libgwater-xcb.h

libgwater_xcb_a_CFLAGS = \
	$(AM_CFLAGS) \
	$(GW_XCB_INTERNAL_CFLAGS)

GW_XCB_CFLAGS = \
	-I$(srcdir)/subprojects/libgwater/xcb \
	$(GW_XCB_INTERNAL_CFLAGS)

GW_XCB_LIBS = \
	libgwater-xcb.a \
	$(GW_XCB_INTERNAL_LIBS)
