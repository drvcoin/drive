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

TARGETLIB = libbdfs

include $(ROOT)/Config.mk

SRCS = \
	AsyncResult.cpp \
	Base64Encoder.cpp \
	BdObject.cpp \
	BdPartition.cpp \
	BdPartitionFolder.cpp \
	BdSession.cpp \
	BdTypes.cpp \
	Buffer.cpp \
	HttpCookies.cpp \
	HttpRequest.cpp \
  UnixDomainSocket.cpp \
  Event.cpp

CCFLAGS = -I../jsoncpp/include -I../mongoose -fPIC -std=c++17

ifneq ($(DEBUG_HTTP),)
CCFLAGS += -DDEBUG_HTTP
endif

include $(ROOT)/Cpp.mk
