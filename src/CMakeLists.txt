
set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_BINARY_DIR})

if (X11_FOUND)
  list(APPEND INCLUDE_DIRS ${X11_INCLUDE_DIR})
  list(APPEND LIBS ${X11_LIBRARIES})
endif()

set(HEADERS
  ../include/luigi.h
)

set(SOURCES
  clipboard.c
  debug.c
  font.c
  luigi.c
  luigi_private.h
  rect.c
  string.c
  utils.c
  themes.c

  widget/button.c
  widget/code.c
  widget/colorpicker.c
  widget/dialog.c
  widget/expandpane.c
  widget/gauge.c
  widget/imagedisplay.c
  widget/label.c
  widget/mdi.c
  widget/panel.c
  widget/scrollbar.c
  widget/slider.c
  widget/spacer.c
  widget/splitpane.c
  widget/table.c
  widget/tabpane.c
  widget/textbox.c
  widget/wrappanel.c
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${VERSION} SOVERSION ${LUIGI_PATCH_LEVEL})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME luigi-${LUIGI_MAJOR_VERSION}.${LUIGI_MINOR_VERSION})

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION include/luigi-${LUIGI_MAJOR_VERSION}.${LUIGI_MINOR_VERSION})
install(FILES ${HEADERS} DESTINATION include/luigi-${LUIGI_MAJOR_VERSION}.${LUIGI_MINOR_VERSION})

