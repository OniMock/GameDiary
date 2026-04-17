# GameDiary Root Dispatcher Makefile

.PHONY: all plugin app clean

all: plugin app

plugin:
	$(MAKE) -f Makefile_Plugin clean
	$(MAKE) -f Makefile_Plugin

app:
	$(MAKE) -f Makefile_App clean
	$(MAKE) -f Makefile_App

clean:
	$(MAKE) -f Makefile_Plugin clean
	$(MAKE) -f Makefile_App clean
	rm -rf obj
	rm -f build_log.txt
