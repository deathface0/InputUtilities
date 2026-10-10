# Examples move the real mouse and keyboard: they are built, never run by CTest.
function(inpututil_add_example name)
    add_executable(inpututil_${name} ${name}.cpp ExampleSupport.h)
    target_link_libraries(inpututil_${name} PRIVATE inpututil::inpututil inpututil_build_options)
endfunction()

inpututil_add_example(form_fill)
inpututil_add_example(gaming)
inpututil_add_example(autoclicker)
inpututil_add_example(macro_sequence)
inpututil_add_example(manual_check)
