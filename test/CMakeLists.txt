
set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_BINARY_DIR})

set(SOURCES
  luigi_example.c
)

add_executable(luigi_test ${SOURCES})
target_link_libraries(luigi_test luigi)
target_include_directories(luigi_test PRIVATE ${INCLUDE_DIRS})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
endif()


