noinst_LIBRARIES += \
	libgwater/libgwater-wayland.a

libgwater_libgwater_wayland_a_SOURCES = \
	libgwater/wayland/libgwater-wayland.c \
	libgwater/wayland/libgwater-wayland.h

libgwater_libgwater_wayland_a_CFLAGS = \
	$(AM_CFLAGS) \
	$(GW_WAYLAND_INTERNAL_CFLAGS)


GW_WAYLAND_CFLAGS = \
	-I$(srcdir)/libgwater/wayland \
	$(GW_WAYLAND_INTERNAL_CFLAGS)

GW_WAYLAND_LIBS = \
	libgwater/libgwater-wayland.a \
	$(GW_WAYLAND_INTERNAL_LIBS)

