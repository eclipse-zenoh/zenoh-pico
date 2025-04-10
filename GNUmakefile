#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#
.PHONY: test clean

# Build type. This set the CMAKE_BUILD_TYPE variable.
# Accepted values: Release, Debug, GCov
BUILD_TYPE?=Release

# Build examples. This sets the BUILD_EXAMPLES variable.
# Accepted values: ON, OFF
BUILD_EXAMPLES?=ON

# Build testing. This sets the BUILD_TESTING variable.
# Accepted values: ON, OFF
BUILD_TESTING?=ON

# Build multicast tests. This sets the BUILD_MULTICAST variable.
# Accepted values: ON, OFF
BUILD_MULTICAST?=OFF

# Build integration tests. This sets the BUILD_INTEGRATION variable.
# Accepted values: ON, OFF
BUILD_INTEGRATION?=OFF

# Build integration tests. This sets the BUILD_TOOLS variable.
# Accepted values: ON, OFF
BUILD_TOOLS?=OFF

# Force the use of c99 standard.
# Accepted values: ON, OFF
FORCE_C99?=OFF

# Enable AddressSanitizer.
# Accepted values: ON, OFF
ASAN?=OFF

# Logging level. This sets the ZENOH_LOG variable.
# Accepted values (empty string means no log):
#  ERROR/error
#  WARN/warn
#  INFO/info
#  DEBUG/debug
#  TRACE/trace
ZENOH_LOG?=""

# Feature config toggle
# Accepted values: 0, 1
Z_FEATURE_UNSTABLE_API?=0
Z_FEATURE_MULTI_THREAD?=1
Z_FEATURE_PUBLICATION?=1
Z_FEATURE_SUBSCRIPTION?=1
Z_FEATURE_QUERY?=1
Z_FEATURE_QUERYABLE?=1
Z_FEATURE_INTEREST?=1
Z_FEATURE_LIVELINESS?=1
Z_FEATURE_RAWETH_TRANSPORT?=0
Z_FEATURE_LOCAL_SUBSCRIBER?=0
Z_FEATURE_UNICAST_PEER?=1

# Buffer sizes
FRAG_MAX_SIZE?=300000
BATCH_UNICAST_SIZE?=65535
BATCH_MULTICAST_SIZE?=8192

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

CMAKE_OPT=-DZENOH_DEBUG=$(ZENOH_DEBUG) -DZENOH_LOG=$(ZENOH_LOG) -DZENOH_LOG_PRINT=$(ZENOH_LOG_PRINT) -DBUILD_EXAMPLES=$(BUILD_EXAMPLES) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=$(BUILD_TESTING) -DBUILD_MULTICAST=$(BUILD_MULTICAST)\
 -DZ_FEATURE_MULTI_THREAD=$(Z_FEATURE_MULTI_THREAD) -DZ_FEATURE_INTEREST=$(Z_FEATURE_INTEREST) -DZ_FEATURE_UNSTABLE_API=$(Z_FEATURE_UNSTABLE_API)\
 -DZ_FEATURE_PUBLICATION=$(Z_FEATURE_PUBLICATION) -DZ_FEATURE_SUBSCRIPTION=$(Z_FEATURE_SUBSCRIPTION) -DZ_FEATURE_QUERY=$(Z_FEATURE_QUERY) -DZ_FEATURE_QUERYABLE=$(Z_FEATURE_QUERYABLE)\
 -DZ_FEATURE_RAWETH_TRANSPORT=$(Z_FEATURE_RAWETH_TRANSPORT) -DZ_FEATURE_LOCAL_SUBSCRIBER=$(Z_FEATURE_LOCAL_SUBSCRIBER) -DFRAG_MAX_SIZE=$(FRAG_MAX_SIZE) -DBATCH_UNICAST_SIZE=$(BATCH_UNICAST_SIZE)\
 -DBATCH_MULTICAST_SIZE=$(BATCH_MULTICAST_SIZE) -DZ_FEATURE_UNICAST_PEER=$(Z_FEATURE_UNICAST_PEER) -DASAN=$(ASAN) -DBUILD_INTEGRATION=$(BUILD_INTEGRATION) -DBUILD_TOOLS=$(BUILD_TOOLS) -DBUILD_SHARED_LIBS=$(BUILD_SHARED_LIBS) -H.

ifeq ($(FORCE_C99), ON)
	CMAKE_OPT += -DCMAKE_C_STANDARD=99
endif

all: make

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	echo $(CMAKE_OPT)
	cmake $(CMAKE_OPT) -B $(BUILD_DIR)

make: $(BUILD_DIR)/Makefile
	cmake --build $(BUILD_DIR)

install: $(BUILD_DIR)/Makefile
	cmake --install $(BUILD_DIR)

test: make
	ctest --verbose --test-dir build

crossbuilds: $(CROSSBUILD_TARGETS)

DOCKER_OK := $(shell docker version 2> /dev/null)
check-docker:
ifndef DOCKER_OK
	$(error "Docker is not available. Please install Docker")
endif

crossbuild: check-docker
	@echo "FROM dockcross/$(CROSSIMG)\nRUN apt-get update && apt-get -y install rpm" | docker build -t $(CROSSIMG_PREFIX)$(CROSSIMG) -
	docker run --rm -v $(ROOT_DIR):/workdir -w /workdir $(CROSSIMG_PREFIX)$(CROSSIMG) bash -c "\
		cmake $(CMAKE_OPT) -DCPACK_PACKAGE_NAME=$(PACKAGE_NAME) -DPACKAGING=DEB,RPM -DDEBARCH=$(DEBARCH) -DRPMARCH=$(RPMARCH) -B$(CROSSBUILD_DIR)/$(CROSSIMG) && \
		make VERBOSE=1 -C$(CROSSBUILD_DIR)/$(CROSSIMG) all package"
	docker rmi $(CROSSIMG_PREFIX)$(CROSSIMG)

linux-armv5:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=arm RPMARCH=arm make crossbuild

linux-armv6:
	PACKAGE_NAME="zenohpico-armv6" CROSSIMG=$@ DEBARCH=arm RPMARCH=arm make crossbuild

linux-armv7:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=armhf RPMARCH=armhf make crossbuild

linux-armv7a:
	PACKAGE_NAME="zenohpico-armv7a" CROSSIMG=$@ DEBARCH=armhf RPMARCH=armhf make crossbuild

linux-arm64:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=arm64 RPMARCH=aarch64 make crossbuild

linux-mips:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=mips RPMARCH=mips make crossbuild

linux-x86:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=i386 RPMARCH=x86 make crossbuild

linux-x64:
	PACKAGE_NAME="zenohpico" CROSSIMG=$@ DEBARCH=amd64 RPMARCH=x86_64 make crossbuild

clean:
	rm -fr $(BUILD_DIR)
	rm -rf $(CROSSBUILD_DIR)
