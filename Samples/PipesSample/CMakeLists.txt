add_executable(PipesSample main.cpp)

un_configure_target(PipesSample)

set_target_properties(PipesSample PROPERTIES FOLDER "Samples")
target_link_libraries(PipesSample UnAsync)
