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

project(drive)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

add_executable(
  drive

  Action.cpp
  Main.cpp
  Options.cpp 
)

include_directories(${ROOT_UBD}/src)
include_directories(${ROOT_CM256}/src)
include_directories(${ROOT}/src/jsoncpp/include)
include_directories(${ROOT}/src/bdcontract-lib)
include_directories(${ROOT}/src/bdfs-lib)
include_directories(${ROOT}/src/bdfsclient-lib)

bd_lib(drive bdfsclient-static ${LIBDIR}/libbdfsclient.a)
bd_lib(drive bdfs-static ${LIBDIR}/libbdfs.a)
bd_lib(drive bdcontract-static ${LIBDIR}/libbdcontract.a)
bd_lib(drive jsoncpp ${LIBDIR}/libjsoncpp.a)
bd_lib(drive ubd ${ROOT_UBD}/out/lib/libubd.a)
bd_lib(drive cm256 ${ROOT_CM256}/out/lib/libcm256.a)

bd_sys_lib(drive crypto)
bd_sys_lib(drive curl)
bd_sys_lib(drive dl)

bd_use_pthread(drive)
