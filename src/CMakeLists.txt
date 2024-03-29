# Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.5.1)

# project information
project(sample_classification)

# Compile options
add_compile_options(-std=c++11)

# Specify target generation path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "../../outputs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "../../outputs")
set(CMAKE_INSTALL_PREFIX "../../../run")
set(CMAKE_OUTPUT_DIR "out")


set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O2 -Wall")

execute_process(COMMAND bash ../../../script/deploy.sh RESULT_VARIABLE ret)
IF(${ret})
   message(FATAL_ERROR "some error occured!")
ENDIF(${ret})


# Header path
include_directories(
.
$ENV{DDK_PATH}/include/inc/
$ENV{DDK_PATH}/include/third_party/protobuf/include
$ENV{DDK_PATH}/include/third_party/cereal/include
$ENV{DDK_PATH}/include/libc_sec/include
$ENV{HOME}/ascend_ddk/include
$ENV{DDK_PATH}/include/third_party/opencv/include
common general_image general_inference general_post
)

if(type STREQUAL host)
    if(target STREQUAL "RC")
        add_compile_options(-DIS_RC)
    endif()

    link_directories($ENV{NPU_HOST_LIB})

    add_executable(main Main.cpp)
    add_library(Host SHARED general_image/general_image.cpp general_post/general_post.cpp common/common.cpp)


    if(target STREQUAL "RC")
        target_link_libraries(main matrixdaemon mmpa pthread dl rt)
        target_link_libraries(Host matrixdaemon hiai_common media_mini)
        message(NOTICE "RC HOST")
    else()
        target_link_libraries(main matrix mmpa pthread dl rt)
        target_link_libraries(Host matrix hiai_common opencv_world)
        message(NOTICE "EP HOST")
    endif()

    install(TARGETS main Host DESTINATION ${CMAKE_OUTPUT_DIR})
else()
    add_compile_options(-DCPU_ONLY)

    link_directories($ENV{NPU_DEV_LIB} ../lib/device)
    set(CMAKE_INSTALL_RPATH "/home/HwHiAiUser/matrix/1676964745")

    add_library(Device SHARED general_inference/general_inference.cpp common/common.cpp)

    if(target STREQUAL "RC")
        target_link_libraries(Device Dvpp_api Dvpp_jpeg_decoder Dvpp_jpeg_encoder Dvpp_vpc idedaemon hiai_common)
        message(NOTICE "RC DEVICE")
    else()
        target_link_libraries(Device Dvpp_api Dvpp_jpeg_decoder Dvpp_jpeg_encoder Dvpp_vpc idedaemon hiai_common ascend_ezdvpp)
        message(NOTICE "EP DEVICE")
    endif()

    install(TARGETS Device DESTINATION ${CMAKE_OUTPUT_DIR})
endif()
