AUX_SOURCE_DIRECTORY(. src_files)

ADD_EXECUTABLE(tps65987-drv ${src_files})

find_package(Threads)

target_link_libraries(tps65987-drv ${CMAKE_THREAD_LIBS_INIT})

INSTALL(TARGETS tps65987-drv
	RUNTIME DESTINATION bin
)


