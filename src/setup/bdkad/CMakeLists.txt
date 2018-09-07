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

project(setup_bdkad)

set(ROOT ${PROJECT_SOURCE_DIR}/../../..)

include(${ROOT}/Config.cmake)

set(BDPKG_NAME drive-bdkad)
set(BDPKG_CFG_DIR "/etc/drive")

set(BDPKG_CONTROL control)
set(BDPKG_EXTRA preinst postinst prerm postrm conffiles)
set(BDPKG_BIN
  ${BINDIR}/bdkad
  ${BINDIR}/bdkadd
)
set(BDPKG_CFG
  ${CFGDIR}/default_contacts.json
  ${CFGDIR}/bdkad.conf
)
set(BDPKG_SVC
  ${BINDIR}/bdkad.service
)

set(BDPKG_TARGET_DEPENDS bdkad config service-bdkad)

include(${ROOT}/Setup.cmake)
