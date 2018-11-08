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

project(libbdfs)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

add_library(
  bdfs-static STATIC

	AsyncResult.cpp
	Base64Encoder.cpp
	BdKademlia.cpp
	BdObject.cpp
	BdPartition.cpp
	BdPartitionFolder.cpp
	BdSession.cpp
	BdTypes.cpp
	Buffer.cpp
	Contract.cpp
	ContractTerm.cpp
	Digest.cpp
  Event.cpp
	HexEncoder.cpp
	HostInfo.cpp
	HttpCookies.cpp
	HttpRequest.cpp
	KeyBase.cpp
	KeyManager.cpp
	ScopeGuard.cpp
  UnixDomainSocket.cpp
)

set_target_properties(bdfs-static PROPERTIES OUTPUT_NAME bdfs)

include_directories(${ROOT}/include)
include_directories(${ROOT}/src/jsoncpp/include)
include_directories(${ROOT}/src/mongoose)

if (DEFINED DEBUG_HTTP)
  add_definitions(-DDEBUG_HTTP)
endif (DEFINED DEBUG_HTTP)

if (DEFINED DEBUG_API)
  add_definitions(-DDEBUG_API)
endif (DEFINED DEBUG_API)

if (DEFINED DEBUG_HTTP_RELAY)
  add_definitions(-DDEBUG_HTTP_RELAY)
endif (DEFINED DEBUG_HTTP_RELAY)

add_definitions(-DOPENSSL_API_COMPAT=0x10100000L)

