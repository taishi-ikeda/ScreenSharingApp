cmake_minimum_required(VERSION 3.16)

project(ScreenSharing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# The macOS screen capture backend is Objective-C++ (ScreenCaptureKit).
if(APPLE)
  enable_language(OBJCXX)
  set(CMAKE_OBJCXX_STANDARD 17)
  set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Default install destination is the build directory itself, unless the
# caller explicitly passes -DCMAKE_INSTALL_PREFIX=... to `cmake`.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Werror)
endif()

# Deployment targets Qt 5.15, but Qt 6 is accepted for local development so
# contributors on either toolchain can build the same tree. Subdirectories
# link against Qt${QT_VERSION_MAJOR}::<Component>.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
if(QT_VERSION_MAJOR EQUAL 5)
  find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test)
else()
  find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
