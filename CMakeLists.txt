cmake_minimum_required(VERSION 3.21)
project(MediaWorkbench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
	# 源码里有中文字符串，强制按 UTF-8 编译，避免运行时乱码
	add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets Test)

add_library(contracts STATIC
	contracts/ToolModule.cpp
	contracts/ExportJob.cpp
	contracts/FFmpegHelper.cpp
)
target_include_directories(contracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(contracts PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

add_library(video_module STATIC
	modules/video/VideoController.cpp
	modules/video/VideoExportJob.cpp
	modules/video/SegmentSlider.cpp
	modules/video/VideoModuleWidget.cpp
)
target_include_directories(video_module PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/modules/video
)
target_link_libraries(video_module PUBLIC contracts Qt6::Multimedia Qt6::MultimediaWidgets)

add_library(audio_module STATIC
	modules/audio/AudioController.cpp
	modules/audio/AudioExportJob.cpp
	modules/audio/WaveformAnalyzer.cpp
	modules/audio/WaveformWidget.cpp
	modules/audio/AudioModuleWidget.cpp
	modules/audio/AudioModuleFactory.cpp
)
target_link_libraries(audio_module PUBLIC contracts Qt6::Multimedia)

add_library(frame_module STATIC
	modules/frame/FrameController.cpp
	modules/frame/ImageExportJob.cpp
	modules/frame/FrameModuleWidget.cpp
	modules/frame/FrameModuleFactory.cpp
	modules/frame/filters/GrayFilter.cpp
	modules/frame/filters/InvertFilter.cpp
	modules/frame/filters/BrightnessFilter.cpp
	modules/frame/filters/ContrastFilter.cpp
	modules/frame/filters/BlurFilter.cpp
)
target_link_libraries(frame_module PUBLIC contracts Qt6::Multimedia)

add_executable(media_workbench app/main.cpp app/MainWindow.cpp)
target_link_libraries(media_workbench PRIVATE video_module audio_module frame_module)

add_executable(video_demo standalone/video_main.cpp)
target_link_libraries(video_demo PRIVATE video_module)

add_executable(audio_demo standalone/audio_main.cpp)
target_link_libraries(audio_demo PRIVATE audio_module)

add_executable(frame_demo standalone/frame_main.cpp)
target_link_libraries(frame_demo PRIVATE frame_module)

# ---- 音频模块单元测试（B）----
enable_testing()
add_executable(audio_tests modules/audio/tests/tst_audio.cpp)
target_include_directories(audio_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules/audio)
target_link_libraries(audio_tests PRIVATE audio_module Qt6::Test)
add_test(NAME audio_tests COMMAND audio_tests)

# ---- 视频模块单元测试（A）----
add_executable(video_tests modules/video/tests/tst_video.cpp)
target_include_directories(video_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules/video)
target_link_libraries(video_tests PRIVATE video_module Qt6::Test)
add_test(NAME video_tests COMMAND video_tests)

# 统一外观：四个可执行程序都拷贝同一份 style.qss
foreach(tgt media_workbench video_demo audio_demo frame_demo)
	add_custom_command(TARGET ${tgt} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_SOURCE_DIR}/app/style.qss
			$<TARGET_FILE_DIR:${tgt}>/style.qss
	)
endforeach()

if(WIN32)
	find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
	if(WINDEPLOYQT_EXECUTABLE)
		foreach(tgt media_workbench video_demo audio_demo frame_demo audio_tests video_tests)
			add_custom_command(TARGET ${tgt} POST_BUILD
				COMMAND "${WINDEPLOYQT_EXECUTABLE}" --multimedia "$<TARGET_FILE:${tgt}>"
				COMMENT "windeployqt ${tgt}"
			)
		endforeach()
	endif()
endif()
