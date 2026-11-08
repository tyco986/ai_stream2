cmake_minimum_required(VERSION 3.16)
project(generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(libzip REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)
find_path(OPENCV_INCLUDE_DIR NAMES opencv2/core.hpp PATH_SUFFIXES opencv4 REQUIRED)
find_library(OPENCV_CORE NAMES opencv_core REQUIRED)
find_library(OPENCV_IMGPROC NAMES opencv_imgproc REQUIRED)
find_library(OPENCV_IMGCODECS NAMES opencv_imgcodecs REQUIRED)
find_library(OPENCV_VIDEOIO NAMES opencv_videoio REQUIRED)

add_executable(generator
    main.cpp
    src/routes/health_route.cpp
    src/routes/types_route.cpp
    src/routes/generate_route.cpp
    src/routes/history_route.cpp
    src/routes/logs_route.cpp
    src/logger/request_logger.cpp
    src/services/history_service.cpp
    src/services/logs_service.cpp
    src/manager/generator_manager.cpp
    src/response/response.cpp
    src/subelement_generator/pipeline.cpp
    src/subelement_generator/pgie.cpp
    src/subelement_generator/utils/pgie_parser.cpp
    src/subelement_generator/sgie.cpp
    src/subelement_generator/utils/sgie_parser.cpp
    src/subelement_generator/kafka.cpp
    src/subelement_generator/nvdsanalytics.cpp
    src/subelement_generator/nvmsgconv.cpp
    src/subelement_generator/nvsahipreprocess.cpp
    src/subelement_generator/utils/sahi.cpp
    src/subelement_generator/nvtracker.cpp
    src/subelement_generator/utils/nvtracker_parser.cpp
    src/subelement_generator/rtmpose_preprocess.cpp
    src/subelement_generator/stgcnpp.cpp
    src/subelement_generator/utils/stgcnpp_parser.cpp
    src/subelement_generator/stgcnpp_preprocess.cpp
    src/subelement_generator/utils/default_gie/yolo_det.cpp
    src/subelement_generator/utils/default_gie/yolo_pose.cpp
    src/subelement_generator/utils/default_gie/yolo_seg.cpp
    src/subelement_generator/utils/default_gie/rtmpose.cpp
    src/subelement_generator/utils/default_gie/stgcnpp.cpp
    src/base_generator/base_rtsp.cpp
    src/base_generator/base_event.cpp
    src/base_generator/event_image.cpp
    src/base_generator/event_video.cpp
    src/base_generator/event_sahi_video.cpp
    src/base_generator/vis_event_video.cpp
    src/base_generator/vis_event_sahi_video.cpp
    src/base_generator/event_rtsp.cpp
    src/base_generator/vis_event_rtsp.cpp
    src/base_generator/event_sahi_rtsp.cpp
    src/base_generator/vis_event_sahi_rtsp.cpp
    src/base_generator/base_video.cpp
    src/base_generator/vis_video.cpp
    src/yolo_generator/det_video.cpp
    src/yolo_generator/det_vis_video.cpp
    src/yolo_generator/det_sahi_video.cpp
    src/yolo_generator/det_sahi_vis_video.cpp
    src/yolo_generator/seg_video.cpp
    src/yolo_generator/seg_vis_video.cpp
    src/yolo_generator/seg_sahi_video.cpp
    src/yolo_generator/seg_sahi_vis_video.cpp
    src/yolo_generator/pose_video.cpp
    src/yolo_generator/pose_vis_video.cpp
    src/yolo_generator/pose_sahi_video.cpp
    src/yolo_generator/pose_sahi_vis_video.cpp
    src/yolo_generator/det_rtsp.cpp
    src/yolo_generator/det_vis_rtsp.cpp
    src/yolo_generator/det_sahi_rtsp.cpp
    src/yolo_generator/det_sahi_vis_rtsp.cpp
    src/yolo_generator/seg_rtsp.cpp
    src/yolo_generator/seg_vis_rtsp.cpp
    src/yolo_generator/seg_sahi_rtsp.cpp
    src/yolo_generator/seg_sahi_vis_rtsp.cpp
    src/yolo_generator/pose_rtsp.cpp
    src/yolo_generator/pose_vis_rtsp.cpp
    src/yolo_generator/pose_sahi_rtsp.cpp
    src/yolo_generator/pose_sahi_vis_rtsp.cpp
    src/yolo_generator/det_image.cpp
    src/yolo_generator/det_sahi_image.cpp
    src/yolo_generator/seg_image.cpp
    src/yolo_generator/seg_sahi_image.cpp
    src/yolo_generator/pose_image.cpp
    src/yolo_generator/pose_sahi_image.cpp
    src/event_generator/det_image_presence.cpp
    src/event_generator/det_video_presence.cpp
    src/event_generator/det_vis_video_presence.cpp
    src/event_generator/det_sahi_vis_video_presence.cpp
    src/event_generator/det_rtsp_presence.cpp
    src/event_generator/det_vis_rtsp_presence.cpp
    src/event_generator/det_sahi_rtsp_presence.cpp
    src/event_generator/det_sahi_vis_rtsp_presence.cpp
    src/base_generator/vis_rtsp.cpp
    src/base_generator/sahi_video.cpp
    src/base_generator/vis_sahi_video.cpp
    src/base_generator/sahi_pose_video.cpp
    src/base_generator/vis_sahi_pose_video.cpp
    src/base_generator/sahi_rtsp.cpp
    src/base_generator/vis_sahi_rtsp.cpp
    src/base_generator/sahi_pose_rtsp.cpp
    src/base_generator/vis_sahi_pose_rtsp.cpp
    src/base_generator/base_image.cpp
    src/base_generator/sahi_image.cpp
    src/base_generator/sahi_pose_image.cpp
    src/topdown_pose_generator/topdown_pose.cpp
    src/topdown_pose_generator/topdown_pose_image.cpp
    src/topdown_pose_generator/topdown_pose_sahi_image.cpp
    src/topdown_pose_generator/topdown_pose_video.cpp
    src/topdown_pose_generator/topdown_pose_vis_video.cpp
    src/topdown_pose_generator/topdown_pose_sahi_video.cpp
    src/topdown_pose_generator/topdown_pose_sahi_vis_video.cpp
    src/topdown_pose_generator/topdown_pose_rtsp.cpp
    src/topdown_pose_generator/topdown_pose_vis_rtsp.cpp
    src/topdown_pose_generator/topdown_pose_sahi_rtsp.cpp
    src/topdown_pose_generator/topdown_pose_sahi_vis_rtsp.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_video.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_vis_video.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_video.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_vis_video.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_rtsp.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_vis_rtsp.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_rtsp.cpp
    src/stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_vis_rtsp.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_video.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_vis_video.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_video.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_vis_video.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_rtsp.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_vis_rtsp.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_rtsp.cpp
    src/stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose_sahi_vis_rtsp.cpp
)

target_include_directories(generator PRIVATE src ${OPENCV_INCLUDE_DIR})
target_precompile_headers(generator PRIVATE libs/httplib.h)
target_link_libraries(generator PRIVATE nlohmann_json::nlohmann_json yaml-cpp libzip::zip spdlog::spdlog Threads::Threads ${OPENCV_CORE} ${OPENCV_IMGPROC} ${OPENCV_IMGCODECS} ${OPENCV_VIDEOIO})
