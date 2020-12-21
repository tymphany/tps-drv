AUX_SOURCE_DIRECTORY(. src_files)

ADD_EXECUTABLE(tps65987-drv-r1 ${src_files})

find_package(Threads)

target_link_libraries(tps65987-drv-r1 ${CMAKE_THREAD_LIBS_INIT})

INSTALL(TARGETS tps65987-drv-r1
	RUNTIME DESTINATION bin
)


