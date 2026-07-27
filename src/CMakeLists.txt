set(SCREENSHARING_CORE_SOURCES
    log_dialog.cpp
    presenter_panel.cpp
    viewer_panel.cpp
    main_window.cpp
    session_paths.cpp
    session_writer.cpp
    session_reader.cpp
    invite_code.cpp
    xor_cipher.cpp
    frame_image.cpp
    logger.cpp
    presence.cpp
)

# Screen capture is platform-specific: macOS uses ScreenCaptureKit
# (Objective-C++), Linux uses X11. See SPEC.md section 7.
if(APPLE)
  list(APPEND SCREENSHARING_CORE_SOURCES screen_capturer_mac.mm)
elseif(UNIX)
  list(APPEND SCREENSHARING_CORE_SOURCES screen_capturer_x11.cpp)
endif()

# Q_OBJECT headers are listed explicitly (not just #included from the .cpp
# files above) so AUTOMOC is guaranteed to run moc on them.
set(SCREENSHARING_CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/screensharing/log_dialog.h
    ${CMAKE_SOURCE_DIR}/include/screensharing/presenter_panel.h
    ${CMAKE_SOURCE_DIR}/include/screensharing/viewer_panel.h
    ${CMAKE_SOURCE_DIR}/include/screensharing/main_window.h
    ${CMAKE_SOURCE_DIR}/include/screensharing/logger.h
    ${CMAKE_SOURCE_DIR}/include/screensharing/presence.h
)

add_library(screensharing_core STATIC ${SCREENSHARING_CORE_SOURCES} ${SCREENSHARING_CORE_HEADERS})
target_include_directories(screensharing_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(screensharing_core PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

if(APPLE)
  find_library(SCREENCAPTUREKIT_FRAMEWORK ScreenCaptureKit REQUIRED)
  find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
  find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
  find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
  find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
  target_link_libraries(screensharing_core PUBLIC
      ${SCREENCAPTUREKIT_FRAMEWORK}
      ${COREMEDIA_FRAMEWORK}
      ${COREVIDEO_FRAMEWORK}
      ${COREGRAPHICS_FRAMEWORK}
      ${APPKIT_FRAMEWORK}
  )
elseif(UNIX)
  find_package(X11 REQUIRED)
  target_include_directories(screensharing_core PUBLIC ${X11_INCLUDE_DIR})
  target_link_libraries(screensharing_core PUBLIC ${X11_LIBRARIES})
  if(NOT X11_Xrandr_FOUND)
    message(FATAL_ERROR "Xrandr development headers not found (required for multi-display capture)")
  endif()
  target_include_directories(screensharing_core PUBLIC ${X11_Xrandr_INCLUDE_PATH})
  target_link_libraries(screensharing_core PUBLIC ${X11_Xrandr_LIB})
endif()

add_executable(ScreenSharing main.cpp)
target_link_libraries(ScreenSharing PRIVATE screensharing_core)

install(TARGETS ScreenSharing RUNTIME DESTINATION bin BUNDLE DESTINATION .)
