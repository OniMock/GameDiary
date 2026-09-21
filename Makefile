# GameDiary Root Dispatcher Makefile

.PHONY: all plugin app debug clean

# Sub-makes can compile in parallel. Do not parallelize plugin and app
# together: plugin clean deletes EBOOT.PBP / PARAM.SFO.
.NOTPARALLEL:

JOBS ?= $(shell nproc 2>/dev/null || echo 4)

all: plugin app

plugin:
	$(MAKE) -f Makefile_Plugin clean
	$(MAKE) -j$(JOBS) -f Makefile_Plugin

app:
	$(MAKE) -f Makefile_App clean
	$(MAKE) -j$(JOBS) -f Makefile_App

debug:
	$(MAKE) -f Makefile_Plugin clean
	$(MAKE) -j$(JOBS) -f Makefile_Plugin DEBUG=1
	$(MAKE) -f Makefile_App clean
	$(MAKE) -j$(JOBS) -f Makefile_App DEBUG=1

clean:
	$(MAKE) -f Makefile_Plugin clean
	$(MAKE) -f Makefile_App clean
	rm -rf obj
	rm -f build_log.txt
