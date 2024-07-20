
set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_BINARY_DIR})

set(LUIGI_TESTS luigi_example todo_list_example unit_converter_example)

foreach (LUIGI_TEST IN LISTS LUIGI_TESTS)
  add_executable(${LUIGI_TEST} "${LUIGI_TEST}.c")
  target_link_libraries(${LUIGI_TEST} luigi)
  target_include_directories(${LUIGI_TEST} PRIVATE ${INCLUDE_DIRS})
  if(MSVC)
    target_compile_options(${LUIGI_TEST} PRIVATE /W3)
  else()
    target_compile_options(${LUIGI_TEST} PRIVATE -Wall -Werror)
  endif()
endforeach()



