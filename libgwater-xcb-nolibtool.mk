noinst_LIBRARIES += \
	libgwater/libgwater-xcb.a

libgwater_libgwater_xcb_a_SOURCES = \
	libgwater/xcb/libgwater-xcb.c \
	libgwater/xcb/libgwater-xcb.h

libgwater_libgwater_xcb_a_CFLAGS = \
	$(AM_CFLAGS) \
	$(GW_XCB_INTERNAL_CFLAGS)


GW_XCB_CFLAGS = \
	-I$(srcdir)/libgwater/xcb \
	$(GW_XCB_INTERNAL_CFLAGS)

GW_XCB_LIBS = \
	libgwater/libgwater-xcb.a \
	$(GW_XCB_INTERNAL_LIBS)

