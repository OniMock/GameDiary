# GameDiary Root Dispatcher Makefile

.PHONY: all plugin app clean

all: plugin app

plugin:
	$(MAKE) -f Makefile.plugin clean
	$(MAKE) -f Makefile.plugin

app:
	$(MAKE) -f Makefile.app clean
	$(MAKE) -f Makefile.app

clean:
	$(MAKE) -f Makefile.plugin clean
	$(MAKE) -f Makefile.app clean
	rm -rf obj
	rm -f build_log.txt
