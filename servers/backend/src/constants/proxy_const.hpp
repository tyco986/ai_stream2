#pragma once

namespace deepstream {

inline const char* SRC_PREFIX = "/deepstream";
inline const char* DES_PREFIX = "/ai_stream2/deepstream";
inline const char* DES_BASE_URL = "http://ai_stream2_deepstream:8000";

}

namespace generator {

inline const char* SRC_PREFIX = "/generator";
inline const char* DES_PREFIX = "/ai_stream2/generator";
inline const char* DES_BASE_URL = "http://ai_stream2_generator:8091";

}

namespace ffmpeg {

inline const char* SRC_PREFIX = "/ffmpeg";
inline const char* DES_PREFIX = "";
inline const char* DES_BASE_URL = "http://ai_stream2_ffmpeg:8080";

}

namespace export_trt {

inline const char* SRC_PREFIX = "/export_trt";
inline const char* DES_PREFIX = "/ai_stream2/export_trt";
inline const char* DES_BASE_URL = "http://ai_stream2_export_trt:8092";
inline const double REQUEST_TIMEOUT_SECONDS = 180;

}

namespace export_onnx {

inline const char* SRC_PREFIX = "/export_onnx";
inline const char* DES_PREFIX = "/ai_stream2/export_onnx";
inline const char* DES_BASE_URL = "http://ai_stream2_export_onnx:8090";
inline const double REQUEST_TIMEOUT_SECONDS = 180;

}

namespace proxy {

inline const char* MSG_UPSTREAM_UNREACHABLE = "upstream unreachable";

}

namespace mediamtx {

inline const char* SRC_PREFIX = "/mediamtx";
inline const char* DES_PREFIX = "";
inline const char* DES_BASE_URL = "http://ai_stream2_mediamtx:9997";

}
