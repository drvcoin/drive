
VPATH = ../../out/cm256/lib ../../out/ubd/lib

HEADERS = \
  Action.h \
  BitSet.h \
  Util.h \
  Options.h \
  Partition.h \
  Volume.h

SOURCES = \
  Action.cpp \
  Util.cpp \
  Main.cpp \
  Options.cpp \
  Volume.cpp \
  VolumeCell.cpp \
  VolumeColumn.cpp \
  VolumeRow.cpp \
  BitSet.cpp \
  Partition.cpp

LIBS = \
  ../../ubd/out/lib/ubd.a \
  ../../cm256/out/lib/cm256.a


TARGET = drive
CCFLAGS = -std=c++11 -msse4.1 -I../../ubd/out/inc -I../../cm256/out/inc
LDFLAGS = -lcrypto

include Cpp.mk
