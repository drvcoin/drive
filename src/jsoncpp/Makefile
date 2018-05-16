ROOT = $(realpath ../..)

TARGETLIB = libjson
TARGETSO = libjson

include $(ROOT)/Config.mk

SRCS = \
  json_reader.cpp \
  json_value.cpp \
  json_writer.cpp

CCFLAGS = -Iinclude -fPIC

include $(ROOT)/Cpp.mk
