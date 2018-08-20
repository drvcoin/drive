#
# MIT License
#
# Copyright (c) 2018 drvcoin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# =============================================================================
#

###############################################################################
# Prerequisites
###############################################################################

BUILD_ROOT := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

-include $(BUILD_ROOT)/Local.mk

ifeq ($(ROOT),)
$(error "Please define ROOT variable to point to the repository's root folder")
endif

###############################################################################
# Depot Roots
###############################################################################

DEVROOT         ?= $(realpath $(BUILD_ROOT)/..)

###############################################################################
# Versioning
###############################################################################

BUILD_NUMBER := $(shell cat $(BUILD_ROOT)/BuildNumber)

DRIVE_VERSION_MAJOR        ?= $(shell cat $(BUILD_ROOT)/BuildMajorVersion)
DRIVE_VERSION_MINOR        ?= $(shell cat $(BUILD_ROOT)/BuildMinorVersion)
DRIVE_VERSION_BUILD        ?= $(BUILD_NUMBER)
ifeq ($(shell uname -r | grep -o el7), el7)
	DRIVE_VERSION_PLATFORM     ?= el7
endif
ifeq ($(shell uname -r | grep -o fc14), fc14)
	DRIVE_VERSION_PLATFORM     ?= fc14
endif
ifeq ($(shell uname -r | grep -o fc17), fc17)
	DRIVE_VERSION_PLATFORM     ?= fc17
endif
ifeq ($(shell uname -r | grep -o fc21), fc21)
	DRIVE_VERSION_PLATFORM     ?= fc21
endif
ifeq ($(shell uname -r | grep -o fc23), fc23)
	DRIVE_VERSION_PLATFORM     ?= fc23
endif
DRIVE_VERSION_MILESTONE    ?= $(shell cat $(BUILD_ROOT)/BuildMilestone)
DRIVE_VERSION_BRANCH       ?= $(shell git rev-parse --abbrev-ref HEAD)
DRIVE_VERSION_USER         ?= $(shell whoami)
DRIVE_VERSION_CODE         ?= $(DRIVE_VERSION_USER)_$(DRIVE_VERSION_BRANCH)_$(DRIVE_VERSION_MILESTONE)
DRIVE_VERSION               = $(DRIVE_VERSION_MAJOR).$(DRIVE_VERSION_MINOR)
DRIVE_VERSION_FULL          = $(DRIVE_VERSION).$(DRIVE_VERSION_BUILD).$(DRIVE_VERSION_PLATFORM)


###############################################################################
# Default Directories
###############################################################################

#BLD    = $(OSTYPE).$(BUILDARCH)/$(BUILDTYPE)
#THISBLD= $(OSTYPE).$(BUILDARCH)/$(THISBUILDTYPE)
#OUTDIR = $(ROOT)/out
#INCDIR = $(OUTDIR)/include
#BLDDIR = $(OUTDIR)/$(THISBLD)
#BINDIR = $(BLDDIR)/bin
#OBJDIR = $(BLDDIR)/obj
#LIBDIR = $(BLDDIR)/lib
#SYMDIR = $(BLDDIR)/sym

#DEPLOYROOT ?= $(HOME)/builds
#DEPLOYDIR ?= $(DEPLOYROOT)/$(DRIVE_VERSION_MAJOR)-$(DRIVE_VERSION_MINOR)-$(DRIVE_VERSION_BUILD).$(DRIVE_VERSION_CODE)
