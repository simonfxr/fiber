macro(add_test_exec tgt)
  add_executable(${tgt} ${ARGN})
  target_link_libraries(${tgt} fiber::fiber)
  target_compile_options(${tgt} PRIVATE ${cflags})
  if(CMU_COMP_MSVC)
    target_compile_definitions(${tgt} PRIVATE _CRT_SECURE_NO_WARNINGS=1)
  endif()
  if(FIBER_LTO)
    set_property(TARGET ${tgt} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endmacro()

macro(add_test_run tgt)
  add_test_exec("${tgt}_exec" ${ARGN})

  add_test(
    NAME "${tgt}_run"
    COMMAND "${tgt}_exec" "test_${tgt}.out"
  )

  add_test(
    "${tgt}_cmp"
    ${CMAKE_COMMAND} -E compare_files "test_${tgt}.out" "${CMAKE_CURRENT_SOURCE_DIR}/test_${tgt}.out"
  )

  add_test(
    "${tgt}_build"
    "${CMAKE_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --config $<CONFIG>
    --target "${tgt}_exec"
  )

  set_tests_properties("${tgt}_build" PROPERTIES FIXTURES_SETUP "${tgt}_build_fixture")
  set_tests_properties("${tgt}_run" PROPERTIES FIXTURES_REQUIRED "${tgt}_build_fixture")
  set_tests_properties("${tgt}_run" PROPERTIES FIXTURES_SETUP "${tgt}_run_fixture")
  set_tests_properties("${tgt}_cmp" PROPERTIES FIXTURES_REQUIRED "${tgt}_run_fixture")
endmacro()

add_test_exec(hello hello.c)

add_test_run(basic basic.c)
add_test_run(coop coop.c)
add_test_run(coop_shared coop_shared.c)
add_test_run(generators generators.c)
add_test_run(fp_stress fp_stress.c)
