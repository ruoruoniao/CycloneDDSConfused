#
# Copyright(c) 2020 to 2022 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
cmake_minimum_required(VERSION 3.16)
project(helloworld LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

idlcxx_generate(TARGET helloworlddata FILES HelloWorldData.idl WARNINGS no-implicit-extensibility)
idlcxx_generate(TARGET timestepdata FILES Time.idl WARNINGS no-implicit-extensibility)

add_executable(PingPong PingPong.cpp)
add_executable(PingPongOneTopic PingPongOneTopic.cpp)
add_executable(TimeStep TimeStep.cpp)

# Link both executables to idl data type library and ddscxx.
target_link_libraries(PingPong CycloneDDS-CXX::ddscxx helloworlddata)
target_link_libraries(PingPongOneTopic CycloneDDS-CXX::ddscxx helloworlddata)
target_link_libraries(TimeStep CycloneDDS-CXX::ddscxx timestepdata)
