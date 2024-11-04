add_library(tiny-json STATIC)

target_sources(tiny-json PUBLIC
	${CMAKE_SOURCE_DIR}/lib/tiny-json/tiny-json.c
)

target_include_directories(tiny-json PUBLIC 
	${CMAKE_SOURCE_DIR}/lib/tiny-json
)
