link_directories(${PROJECT_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a)

# Identify the target library and the source files
add_library(wifidiscovery SHARED wifidiscovery.cpp wifidiscovery_renderer.cpp shaderutils.cpp matrixutils.cpp textutils.cpp)

target_compile_options(wifidiscovery PUBLIC -std=c++11 -DGL_GLEXT_PROTOTYPES)

include_directories(.)

target_link_libraries(wifidiscovery log android EGL GLESv3 gvr)