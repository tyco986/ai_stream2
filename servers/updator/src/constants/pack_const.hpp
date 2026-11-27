#pragma once

namespace pack {

inline constexpr const char* PACK_PATH = "/ai_stream2/updator/pack";
inline constexpr const char* UPDATE_PATH = "/ai_stream2/updator/update";
inline constexpr const char* DOCKER_HOST = "ai_stream2_docker_socket_proxy";
inline constexpr int DOCKER_PORT = 2375;
inline constexpr const char* REGISTRY_HOST = "ai_stream2_registry2";
inline constexpr int REGISTRY_PORT = 5000;
inline constexpr const char* PUSH_HOST = "127.0.0.1:5000";
inline constexpr int CONNECT_TIMEOUT_SECONDS = 5;
inline constexpr int READ_TIMEOUT_SECONDS = 3600;
inline constexpr const char* REGISTRY_AUTH = "e30=";
inline constexpr const char* MANIFEST_ACCEPT = "application/vnd.docker.distribution.manifest.list.v2+json, application/vnd.oci.image.index.v1+json, application/vnd.docker.distribution.manifest.v2+json, application/vnd.oci.image.manifest.v1+json";
inline constexpr const char* DEFAULT_MEDIA_TYPE = "application/vnd.oci.image.manifest.v1+json";
inline constexpr const char* FILE_NAME = "update.zip";
inline constexpr const char* OUTPUT_DIRECTORY = "/root/updator";
inline constexpr const char* WORK_DIRECTORY = "updator-pack";
inline constexpr const char* UPDATE_WORK_DIRECTORY = "updator-update";
inline constexpr const char* OCI_LAYOUT = "{\"imageLayoutVersion\":\"1.0.0\"}\n";
inline constexpr const char* KEY_FROM = "from";
inline constexpr const char* KEY_TO = "to";
inline constexpr const char* KEY_TAG = "tag";
inline constexpr const char* KEY_FILE_PATH = "file_path";
inline constexpr const char* ANNOTATION_REF = "org.opencontainers.image.ref.name";
inline constexpr const char* ANNOTATION_FROM = "ai_stream2.from";
inline constexpr const char* HEADER_DIGEST = "Docker-Content-Digest";
inline constexpr const char* HEADER_ACCEPT = "Accept";
inline constexpr const char* HEADER_AUTH = "X-Registry-Auth";

}
