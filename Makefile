# to make everything

all:
	cd "compute shader" && $(MAKE)
	cd "cubemap" && $(MAKE)
	cd "environment mapping" && $(MAKE)
	cd "hello wrapper" && $(MAKE)
	cd "planar mirror" && $(MAKE)