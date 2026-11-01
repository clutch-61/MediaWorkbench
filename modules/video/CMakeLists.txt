# A：视频工作台模块�?add_library(video_module STATIC
    VideoModuleWidget.h
    VideoModuleWidget.cpp
    
    VideoController.h
    VideoController.cpp
    VideoExportJob.h
    VideoExportJob.cpp
    SegmentSlider.h
    SegmentSlider.cpp
)

target_include_directories(video_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(video_module PUBLIC
    contracts
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::MultimediaWidgets
)

# 模块单元测试：只测纯逻辑（参数校验、FFmpeg 参数生成、时间格式化），不依赖真实视�?if(BUILD_TESTING)
    add_executable(video_tests tests/tst_video.cpp)
    target_link_libraries(video_tests PRIVATE video_module Qt5::Test)
    add_test(NAME video_tests COMMAND video_tests)
endif()
