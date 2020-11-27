#
# Copyright (c) 2017, 2020 ADLINK Technology Inc.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ADLINK zenoh team, <zenoh@adlink-labs.tech>
#
.PHONY: test clean

# Build type. This set the CMAKE_BUILD_TYPE variable.
# Accepted values: Release, Debug, GCov
BUILD_TYPE?=Release

# zenoh-pico/ directory
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Build directory
BUILD_DIR=build

# Crossbuild directory
CROSSBUILD_TARGETS=linux-armv5 linux-armv6 linux-armv7 linux-armv7a linux-arm64 linux-mips linux-x86 linux-x64
CROSSBUILD_DIR=crossbuilds
CROSSIMG_PREFIX=zenoh-pico_
# NOTES:
# - ARM:   old versions of dockcross/dockcross were creating some issues since they used an old GCC (4.8.3) which lacks <stdatomic.h> (even using -std=gnu11)

ifneq ($(ZENOH_DEBUG),)
	ZENOH_DEBUG_OPT := -DZENOH_DEBUG=$(ZENOH_DEBUG)
endif

CMAKE_OPT=$(ZENOH_DEBUG_OPT) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -H.

# ZENOH_JAVA: when building zenoh-pico for zenoh-java:
ifneq ($(ZENOH_JAVA),)
	CMAKE_OPT += -DSWIG_JAVA=ON -DTESTS=OFF -DEXAMPLES=OFF
endif
ifneq ($(JNI_INCLUDE_HOME),)
	CMAKE_OPT += -DJNI_INCLUDE_HOME=$(JNI_INCLUDE_HOME)
endif

all: make

$(BUILD_DIR)/Makefile: CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_OPT) -B$(BUILD_DIR)

make: $(BUILD_DIR)/Makefile
	make -C$(BUILD_DIR)

install: $(BUILD_DIR)/Makefile
	make -C$(BUILD_DIR) install

test: $(BUILD_DIR)/Makefile
	make -C$(BUILD_DIR) test

crossbuilds: $(CROSSBUILD_TARGETS)

DOCKER_OK := $(shell docker version 2> /dev/null)
check-docker:
ifndef DOCKER_OK
	$(error "Docker is not available. Please install Docker")
endif

crossbuild: check-docker
	@echo "FROM dockcross/$(CROSSIMG)\nRUN apt-get update && apt-get -y install rpm" | docker build -t $(CROSSIMG_PREFIX)$(CROSSIMG) -
	docker run --rm -v $(ROOT_DIR):/workdir -w /workdir $(CROSSIMG_PREFIX)$(CROSSIMG) bash -c "\
		cmake $(CMAKE_OPT) -DPACKAGING=DEB,RPM -DDEBARCH=$(DEBARCH) -DRPMARCH=$(RPMARCH) -B$(CROSSBUILD_DIR)/$(CROSSIMG) && \
		make VERBOSE=1 -C$(CROSSBUILD_DIR)/$(CROSSIMG) all package"
	docker rmi $(CROSSIMG_PREFIX)$(CROSSIMG)
	docker rmi dockcross/$(CROSSIMG)

linux-armv5: 
	CROSSIMG=$@ DEBARCH=arm RPMARCH=arm make crossbuild

linux-armv6: 
	CROSSIMG=$@ DEBARCH=arm RPMARCH=arm make crossbuild

linux-armv7: 
	CROSSIMG=$@ DEBARCH=armhf RPMARCH=armhf make crossbuild

linux-armv7a: 
	CROSSIMG=$@ DEBARCH=armhf RPMARCH=armhf make crossbuild

linux-arm64: 
	CROSSIMG=$@ DEBARCH=arm64 RPMARCH=aarch64 make crossbuild

linux-mips: 
	CROSSIMG=$@ DEBARCH=mips RPMARCH=mips make crossbuild

linux-x86:
	CROSSIMG=$@ DEBARCH=i386 RPMARCH=x86 make crossbuild

linux-x64: 
	CROSSIMG=$@ DEBARCH=amd64 RPMARCH=x86_64 make crossbuild


clean:
	rm -fr $(BUILD_DIR)
	rm -rf $(CROSSBUILD_DIR)
