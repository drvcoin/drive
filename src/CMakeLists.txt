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

project(drive_src)

set(ROOT ${PROJECT_SOURCE_DIR}/..)

add_subdirectory(jsoncpp)

if (EXISTS ${PROJECT_SOURCE_DIR}/cm256)
  add_subdirectory(cm256)
endif (EXISTS ${PROJECT_SOURCE_DIR}/cm256)

if (EXISTS ${PROJECT_SOURCE_DIR}/ubd)
  add_subdirectory(ubd)
endif (EXISTS ${PROJECT_SOURCE_DIR}/ubd)

if (EXISTS ${PROJECT_SOURCE_DIR}/kademlia)
  add_subdirectory(kademlia)
endif (EXISTS ${PROJECT_SOURCE_DIR}/kademlia)

add_subdirectory(mongoose)
add_subdirectory(bdcontract-lib)
add_subdirectory(bdcontract)
add_subdirectory(bdfs-lib)
add_subdirectory(bdfsclient-lib)
add_subdirectory(bdfs)
add_subdirectory(bdfsclient)
add_subdirectory(bdblob)

add_subdirectory(httpserver)
add_subdirectory(bdhost)
add_subdirectory(bdkad)
add_subdirectory(config)
add_subdirectory(service)

if (UNIX)
  if (NOT DEFINED NO_DEB)
    add_subdirectory(setup)
  endif (NOT DEFINED NO_DEB)
endif (UNIX)

