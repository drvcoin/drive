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

TARGETEXE = drive

include $(ROOT)/Config.mk

SRCS = \
  Action.cpp \
  Main.cpp \
  Options.cpp 

VPATH = $(BINDIR) $(ROOT_UBD)/out/bin $(ROOT_CM256)/out/bin

LIBS = \
  libbdfsclient.a \
	libbdfs.a \
	libbdcontract.a \
	libjson.a \
  ubd.a \
  cm256.a

TARGET = drive
CCFLAGS = -std=c++17  -I$(ROOT_UBD)/out/inc -I$(ROOT_CM256)/src -I../jsoncpp/include -I../bdfs-lib -I../bdcontract-lib -I../bdfsclient-lib

SYSLIBS = -lcrypto -lcurl -ldl -pthread

include $(ROOT)/Cpp.mk
