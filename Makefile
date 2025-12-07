# ======================================================
#  TOP-LEVEL MAKEFILE
# ======================================================

.PHONY: all driver app clean

all: driver app

driver:
	$(MAKE) -C driver

app:
	$(MAKE) -C app

clean:
	$(MAKE) -C driver clean
	$(MAKE) -C app clean
	# Clean any leftover build artifacts in top-level folder
	rm -f *.o *.ko *.mod *.mod.c *.symvers *.order *.cmd *yuyv
	rm -rf .tmp_versions

# ----------------------------------------------
# Module load / unload / reload exposed at top
# ----------------------------------------------

load:
	$(MAKE) -C driver load

unload:
	$(MAKE) -C driver unload
