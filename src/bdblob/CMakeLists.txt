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

project(bdblob)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

add_executable(
  bdblob

  commands/GetCommand.cpp
  commands/ListCommand.cpp
  commands/MkdirCommand.cpp
  commands/PutCommand.cpp
  commands/RmCommand.cpp
  ArgumentParser.cpp
  BlobApi.cpp
  BlobConfig.cpp
  BlobError.cpp
  BlobMap.cpp
  BlobMetadata.cpp
  Buffer.cpp
  BufferedInputStream.cpp
  BufferedOutputStream.cpp
  Command.cpp
  File.cpp
  FileBlob.cpp
  FileBlobProvider.cpp
  Folder.cpp
  IInputStream.cpp
  IOutputStream.cpp
  Main.cpp
)

include_directories(".")

if (NOT APPLE)
bd_sys_lib(bdblob uuid)
endif (NOT APPLE)
