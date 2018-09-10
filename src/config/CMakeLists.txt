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

project(src_config)

set(ROOT ${PROJECT_SOURCE_DIR}/../..)

include(${ROOT}/Config.cmake)

add_custom_target(config ALL)

file(MAKE_DIRECTORY ${CFGDIR})

add_custom_command(TARGET config PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/bdkad.conf ${CFGDIR}/
)

if (DEFINED OFFICIAL_BUILD)
  message("-- OFFICIAL_BULID is set to ${OFFICIAL_BUILD}")
  add_custom_command(TARGET config PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/default_contacts.json ${CFGDIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/bdhost.conf ${CFGDIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/bdfs.conf ${CFGDIR}/
  )
else ()
  add_custom_command(TARGET config PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/default_contacts.json.test ${CFGDIR}/default_contacts.json
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/bdhost.conf.test ${CFGDIR}/bdhost.conf
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/bdfs.conf.test ${CFGDIR}/bdfs.conf
  )
endif (DEFINED OFFICIAL_BUILD)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  ${CFGDIR}/bdkad.conf
  ${CFGDIR}/default_contacts.json
  ${CFGDIR}/bdhost.conf
  ${CFGDIR}/bdfs.conf
)
