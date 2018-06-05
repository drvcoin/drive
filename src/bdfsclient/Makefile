# 
# Copyright (c) 2018 Drive Foundation
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

ROOT = $(realpath ../..)

TARGETEXE = bdfsclient

include $(ROOT)/Config.mk

HEADERS = \
  BitSet.h \
  Util.h \
  Partition.h \
  Volume.h \
  ClientManager.h \
  ActionHandler.h

SRCS = \
  Util.cpp \
  Main.cpp \
  Volume.cpp \
  VolumeCell.cpp \
  VolumeColumn.cpp \
  VolumeRow.cpp \
  BitSet.cpp \
  Partition.cpp \
  ClientManager.cpp \
  ActionHandler.cpp

VPATH = $(BINDIR)

LIBS = \
	libbdfs.a \
  libbdcontract.a \
	libjson.a \
  $(ROOT)/../ubd/out/lib/ubd.a \
  $(ROOT)/../cm256/out/lib/cm256.a


TARGET = bdfsclient
CCFLAGS = -std=c++17 -msse4.1 -I$(ROOT)/../ubd/out/inc -I$(ROOT)/../cm256/out/inc -I../jsoncpp/include -I../bdfs-lib -I../bdcontract-lib

SYSLIBS = -lcrypto -lcurl -ldl -pthread

include $(ROOT)/Cpp.mk
