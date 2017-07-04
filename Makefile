export BUILDDIR=$(CURDIR)

all:
	$(MAKE) -C src

check:
	$(MAKE) -C tests
