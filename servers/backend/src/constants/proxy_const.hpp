#pragma once

namespace deepstream {

inline const char* PREFIX = "/puskas/deepstream";
inline const char* BASE_URL = "http://puskas_deepstream:8000";

}

namespace generator {

inline const char* PREFIX = "/puskas/generator";
inline const char* BASE_URL = "http://puskas_generator:8091";

}

namespace ffmpeg {

inline const char* PREFIX = "/puskas/ffmpeg";
inline const char* BASE_URL = "http://puskas_ffmpeg:8080";

}

namespace export_trt {

inline const char* PREFIX = "/puskas/export_trt";
inline const char* BASE_URL = "http://puskas_export_trt:9000";

}

namespace export_onnx {

inline const char* PREFIX = "/puskas/export_onnx";
inline const char* BASE_URL = "http://puskas_export_onnx:8090";

}

namespace mediamtx {

inline const char* PREFIX = "/puskas/mediamtx";
inline const char* BASE_URL = "http://puskas_mediamtx:9997";

}
