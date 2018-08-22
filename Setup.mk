# Copyright 2016 FileGear - All Rights Reserved

ifeq ($(ROOT),)
$(error "Please define ROOT")
endif

include $(ROOT)/Config.mk

include $(BUILD_ROOT)/Setup.mk
