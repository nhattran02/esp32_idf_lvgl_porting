# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/nhat/esp/esp-idf/components/bootloader/subproject"
  "/home/nhat/esp/myGUI/build/bootloader"
  "/home/nhat/esp/myGUI/build/bootloader-prefix"
  "/home/nhat/esp/myGUI/build/bootloader-prefix/tmp"
  "/home/nhat/esp/myGUI/build/bootloader-prefix/src/bootloader-stamp"
  "/home/nhat/esp/myGUI/build/bootloader-prefix/src"
  "/home/nhat/esp/myGUI/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/nhat/esp/myGUI/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/nhat/esp/myGUI/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
