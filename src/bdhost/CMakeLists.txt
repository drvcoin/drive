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

cmake_minimum_required(VERSION 3.1)

project(bdhost)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

add_executable(
  bdhost

	BitSet.cpp
	ConfigHandler.cpp
	Main.cpp
	Options.cpp
	Partition.cpp
	PartitionHandler.cpp
	RelayManager.cpp
	Util.cpp
)

include_directories(${ROOT}/src/jsoncpp/include)
include_directories(${ROOT}/src/mongoose)
include_directories(${ROOT}/src/bdcontract-lib)
include_directories(${ROOT}/src/httpserver)
include_directories(${ROOT}/src/bdfs-lib)

bd_lib(bdhost bdcontract-static ${LIBDIR}/libbdcontract.a)
bd_lib(bdhost bdhttp ${LIBDIR}/libbdhttp.a)
bd_lib(bdhost mongoose ${LIBDIR}/libmongoose.a)
bd_lib(bdhost jsoncpp ${LIBDIR}/libjsoncpp.a)
bd_lib(bdhost bdfs-static ${LIBDIR}/libbdfs.a)

bd_sys_lib(bdhost curl)
bd_sys_lib(bdhost uuid)
bd_sys_lib(bdhost dl)

bd_use_pthread(bdhost)
