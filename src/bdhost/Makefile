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

TARGETEXE = bdhost

include $(ROOT)/Config.mk

SRCS = \
	Base64Encoder.cpp \
	BitSet.cpp \
	Global.cpp \
	HttpModule.cpp \
	HttpServer.cpp \
	Main.cpp \
	MGHttpContext.cpp \
	Options.cpp \
	Partition.cpp \
	PartitionHandler.cpp \
	Util.cpp \

VPATH = $(BINDIR)

LIBS = \
	libbdcontract.a \
	libmongoose.a \
  libjson.a \

SYSLIBS = -lcurl -luuid -ldl -pthread

CCFLAGS = -I../jsoncpp/include -I../mongoose -I../bdcontract-lib -fPIC -std=c++17

include $(ROOT)/Cpp.mk
