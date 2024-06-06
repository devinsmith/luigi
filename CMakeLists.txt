# luigi library build system

# The minimum version is the version supported by the oldest Debian release
# Ubuntu 20.04LTS uses 3.16.3
cmake_minimum_required(VERSION 3.13.4)

#set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(LUIGI_MAJOR_VERSION 0)
set(LUIGI_MINOR_VERSION 0)
set(LUIGI_PATCH_LEVEL 1)
set(VERSION ${LUIGI_MAJOR_VERSION}.${LUIGI_MINOR_VERSION}.${LUIGI_PATCH_LEVEL})

project(luigi VERSION ${VERSION})

find_package(X11)
#find_package(Freetype) // Not yet

if (X11_FOUND)
  set(UI_LINUX ON)
else()
  set(UI_WINDOWS ON)
endif()

configure_file(config.h.in config.h)
configure_file(luigi.pc.in luigi.pc @ONLY)

add_subdirectory(src)

include(CMakePackageConfigHelpers)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/luigi.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#configure_package_config_file(luigi.cmake.in
#  ${CMAKE_CURRENT_BINARY_DIR}/luigi.cmake
#  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/luigi
#  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)

