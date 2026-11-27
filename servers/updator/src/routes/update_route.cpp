#include "routes/update_route.hpp"

#include "constants/pack_const.hpp"
#include "response/response.hpp"

#include <cctype>
#include <cstring>
#include <format>
#include <fstream>
#include <set>
#include <stdexcept>
#include <vector>

void UpdateRoute::bind(httplib::Server& server) {
    server.Post(pack::UPDATE_PATH, update);
}

void UpdateRoute::update(const httplib::Request& request, httplib::Response& response) {
    const nlohmann::json body = request.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(request.body);
    const bool named = body.contains(pack::KEY_FILE_PATH) && body[pack::KEY_FILE_PATH].is_string() && !body[pack::KEY_FILE_PATH].get<std::string>().empty();
    const std::string filePath = named ? body[pack::KEY_FILE_PATH].get<std::string>() : (std::filesystem::path(pack::OUTPUT_DIRECTORY) / pack::FILE_NAME).string();
    Update updater;
    const std::vector<UpdateImage> images = updater.load(filePath);
    nlohmann::json data = nlohmann::json::object();
    data[pack::KEY_FILE_PATH] = filePath;
    for (const UpdateImage& image : images) {
        apply(image);
        data[image.name] = {
            {pack::KEY_TAG, image.tag},
            {pack::KEY_FROM, image.fromDigest.empty() ? nlohmann::json(nullptr) : nlohmann::json(image.fromDigest)},
            {pack::KEY_TO, image.toDigest},
        };
    }
    updater.cleanup();
    Response::ok(response, data);
}

void UpdateRoute::apply(const UpdateImage& image) {
    httplib::Headers unused;
    const httplib::Headers accept = {{pack::HEADER_ACCEPT, pack::MANIFEST_ACCEPT}};
    if (!image.fromDigest.empty()) {
        std::vector<nlohmann::json> documents = {nlohmann::json::parse(request(pack::REGISTRY_HOST, pack::REGISTRY_PORT, "GET", std::format("/v2/{}/manifests/{}", image.name, image.fromDigest), accept, unused, 200, 299))};
        for (std::size_t index = 0; index < documents.size(); ++index) {
            const nlohmann::json& document = documents[index];
            if (document.contains("manifests")) {
                for (const nlohmann::json& child : document["manifests"]) {
                    documents.push_back(nlohmann::json::parse(request(pack::REGISTRY_HOST, pack::REGISTRY_PORT, "GET", std::format("/v2/{}/manifests/{}", image.name, child["digest"].get<std::string>()), accept, unused, 200, 299)));
                }
            }
        }
        std::set<std::string> blobDigests;
        std::set<std::string> manifestDigests;
        for (const nlohmann::json& document : documents) {
            if (document.contains("config")) {
                blobDigests.insert(document["config"]["digest"].get<std::string>());
            }
            if (document.contains("layers")) {
                for (const nlohmann::json& layer : document["layers"]) {
                    blobDigests.insert(layer["digest"].get<std::string>());
                }
            }
            if (document.contains("manifests")) {
                for (const nlohmann::json& child : document["manifests"]) {
                    manifestDigests.insert(child["digest"].get<std::string>());
                }
            }
        }
        for (const std::string& digest : blobDigests) {
            request(pack::REGISTRY_HOST, pack::REGISTRY_PORT, "HEAD", std::format("/v2/{}/blobs/{}", image.name, digest), accept, unused, 200, 200);
        }
        for (const std::string& digest : manifestDigests) {
            request(pack::REGISTRY_HOST, pack::REGISTRY_PORT, "HEAD", std::format("/v2/{}/manifests/{}", image.name, digest), accept, unused, 200, 200);
        }
    }
    for (const UpdateBlob& blob : image.blobs) {
        if (blob.kind == "blobs") {
            save(image.name, blob, blob.digest);
        }
    }
    for (const UpdateBlob& blob : image.blobs) {
        if (blob.kind == "manifests" && blob.digest != image.toDigest) {
            save(image.name, blob, blob.digest);
        }
    }
    for (const UpdateBlob& blob : image.blobs) {
        if (blob.digest == image.toDigest) {
            save(image.name, blob, image.tag);
        }
    }
    const httplib::Headers auth = {{pack::HEADER_AUTH, pack::REGISTRY_AUTH}};
    request(pack::DOCKER_HOST, pack::DOCKER_PORT, "POST", std::format("/images/create?fromImage={}&tag={}", encode(std::string(pack::PUSH_HOST) + "/" + image.name), encode(image.tag)), auth, unused, 200, 299);
}

void UpdateRoute::save(const std::string& name, const UpdateBlob& blob, const std::string& reference) {
    httplib::Client client(pack::REGISTRY_HOST, pack::REGISTRY_PORT);
    client.set_connection_timeout(pack::CONNECT_TIMEOUT_SECONDS);
    client.set_read_timeout(pack::READ_TIMEOUT_SECONDS);
    const httplib::Headers accept = {{pack::HEADER_ACCEPT, pack::MANIFEST_ACCEPT}};
    const bool manifest = blob.kind == "manifests";
    const std::string headPath = std::format("/v2/{}/{}/{}", name, blob.kind, blob.digest);
    const httplib::Result existed = client.Head(headPath, accept);
    const bool skip = !manifest && existed && existed->status == 200;
    if (manifest) {
        const std::uintmax_t size = std::filesystem::file_size(blob.file);
        std::ifstream input(blob.file, std::ios::binary);
        const httplib::Result put = client.Put(
            std::format("/v2/{}/manifests/{}", name, reference),
            {{"Content-Type", blob.mediaType}},
            size,
            [&input](size_t offset, size_t length, httplib::DataSink& sink) {
                input.seekg(static_cast<std::streamoff>(offset));
                std::vector<char> buffer(length);
                input.read(buffer.data(), static_cast<std::streamsize>(length));
                sink.write(buffer.data(), static_cast<size_t>(input.gcount()));
                return true;
            },
            blob.mediaType);
        const bool ok = static_cast<bool>(put) && put->status >= 200 && put->status < 300;
        if (!ok) {
            throw std::runtime_error(std::format("PUT manifest {} {} -> {}", name, reference, put ? put->status : 0));
        }
    }
    if (!manifest && !skip) {
        httplib::Client upload(pack::REGISTRY_HOST, pack::REGISTRY_PORT);
        upload.set_connection_timeout(pack::CONNECT_TIMEOUT_SECONDS);
        upload.set_read_timeout(pack::READ_TIMEOUT_SECONDS);
        const httplib::Result started = upload.Post(std::format("/v2/{}/blobs/uploads/", name), "", "application/octet-stream");
        const bool accepted = static_cast<bool>(started) && started->status == 202;
        if (!accepted) {
            throw std::runtime_error(std::format("POST uploads {} -> {}", name, started ? started->status : 0));
        }
        std::string location = started->get_header_value("Location");
        const std::size_t v2 = location.find("/v2/");
        location = v2 == std::string::npos ? location : location.substr(v2);
        const std::string separator = location.find('?') == std::string::npos ? "?" : "&";
        const std::uintmax_t size = std::filesystem::file_size(blob.file);
        std::ifstream input(blob.file, std::ios::binary);
        const httplib::Result put = upload.Put(
            location + separator + "digest=" + blob.digest,
            size,
            [&input](size_t offset, size_t length, httplib::DataSink& sink) {
                input.seekg(static_cast<std::streamoff>(offset));
                std::vector<char> buffer(length);
                input.read(buffer.data(), static_cast<std::streamsize>(length));
                sink.write(buffer.data(), static_cast<size_t>(input.gcount()));
                return true;
            },
            "application/octet-stream");
        const bool ok = static_cast<bool>(put) && put->status >= 200 && put->status < 300;
        if (!ok) {
            throw std::runtime_error(std::format("PUT blob {} {} -> {}", name, blob.digest, put ? put->status : 0));
        }
    }
}

std::string UpdateRoute::request(const std::string& host, int port, const std::string& method, const std::string& path, const httplib::Headers& headers, httplib::Headers& responseHeaders, int okFrom, int okTo) {
    httplib::Client client(host, port);
    client.set_connection_timeout(pack::CONNECT_TIMEOUT_SECONDS);
    client.set_read_timeout(pack::READ_TIMEOUT_SECONDS);
    httplib::Result result;
    if (method == "GET") {
        result = client.Get(path, headers);
    }
    if (method == "POST") {
        result = client.Post(path, headers, "", "application/octet-stream");
    }
    if (method == "HEAD") {
        result = client.Head(path, headers);
    }
    const bool ok = static_cast<bool>(result) && result->status >= okFrom && result->status <= okTo;
    if (!ok) {
        throw std::runtime_error(std::format("{} {} -> {}", method, path, result ? result->status : 0));
    }
    responseHeaders = result->headers;
    return result->body;
}

std::string UpdateRoute::encode(const std::string& value) {
    std::string encoded;
    for (const unsigned char character : value) {
        const bool plain = std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~';
        encoded += plain ? std::string(1, static_cast<char>(character)) : std::format("%{:02X}", static_cast<int>(character));
    }
    return encoded;
}
