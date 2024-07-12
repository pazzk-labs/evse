PROJECT  := EVSE
BASEDIR  := $(shell pwd)
PLATFORM ?= esp32s3
BUILDIR  := build/$(PLATFORM)
CMAKE    ?= cmake

VERBOSE ?= 0
V ?= $(VERBOSE)
ifeq ($(V), 0)
	Q = @
else
	Q =
endif
export PROJECT
export BASEDIR
export BUILDIR
export Q

include projects/version.mk

all: build $(BUILDIR)/$(PROJECT).img

$(BUILDIR)/$(PROJECT).img: $(BUILDIR)/$(PROJECT).hdr $(BUILDIR)/$(PROJECT).bin
	$(Q)cat $^ > $@

$(BUILDIR)/$(PROJECT).hdr: $(BUILDIR)/$(PROJECT).bin $(BUILDIR)/$(PROJECT).sig
	$(Q)python external/libmcu/modules/dfu/generate_dfu_header.py \
		--input $< --output $@ --type app \
		--signature $(shell cat $(BUILDIR)/$(PROJECT).sig)

$(BUILDIR)/$(PROJECT).sig: $(BUILDIR)/$(PROJECT).bin
	$(Q)openssl dgst -sha256 -binary -out $@.bin $<
	$(Q)cat $@.bin | xxd -p -c 64 > $@

$(BUILDIR)/$(PROJECT).bin: build

$(BUILDIR):
	$(CMAKE) -S . -B $(BUILDIR) -DTARGET_PLATFORM=$(PLATFORM)

.PHONY: build
build: $(BUILDIR)
	$(CMAKE) --build $(BUILDIR)

.PHONY: coverage
coverage:
	$(Q)$(MAKE) -C tests $@

.PHONY: confirm
confirm:
	@echo 'Are you sure? [y/N] ' && read ans && [ $${ans:-N} = y ]

## help: print this help message
.PHONY: help
help:
	@echo 'Usage:'
	@sed -n 's/^##//p' ${MAKEFILE_LIST} | column -t -s ':' |  sed -e 's/^/ /'

## version: print firmware version
.PHONY: version
version:
	$(info $(VERSION_TAG), $(VERSION))

## test: run unit testing
.PHONY: test
test:
	$(Q)$(MAKE) -C tests

## clean
.PHONY: clean
clean:
	$(Q)rm -fr $(BUILDIR)
	$(Q)$(MAKE) -C tests $@

FORCE:
