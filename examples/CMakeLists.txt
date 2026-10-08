# Examples move the real mouse and keyboard: they are built, never run by CTest.
function(inpututil_add_example name)
    add_executable(inpututil_${name} ${name}.cpp ExampleSupport.h)
    target_compile_definitions(inpututil_${name} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    target_link_libraries(inpututil_${name} PRIVATE inpututil::inpututil inpututil_warnings)
endfunction()

inpututil_add_example(form_fill)
inpututil_add_example(gaming)
inpututil_add_example(autoclicker)
inpututil_add_example(macro_sequence)
inpututil_add_example(manual_check)
