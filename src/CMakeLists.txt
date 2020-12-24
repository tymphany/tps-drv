AUX_SOURCE_DIRECTORY(. src_files)

ADD_EXECUTABLE(tps65987-drv-r1 ${src_files})

find_package(Threads)

target_link_libraries(tps65987-drv-r1 ${CMAKE_THREAD_LIBS_INIT})

#SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -g")

SET(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -g")

INSTALL(TARGETS tps65987-drv-r1
	RUNTIME DESTINATION bin
)


