#include "routes/pack_route.hpp"

#include "constants/pack_const.hpp"
#include "response/response.hpp"
#include "services/pack_service.hpp"

#include <cctype>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>

void PackRoute::bind(httplib::Server& server) {
    server.Post(pack::PACK_PATH, pack);
}

void PackRoute::pack(const httplib::Request& request, httplib::Response& response) {
    const nlohmann::json body = nlohmann::json::parse(request.body);
    nlohmann::json data = nlohmann::json::object();
    Pack writer;
    std::vector<std::string> names;
    for (nlohmann::json::const_iterator item = body.begin(); item != body.end(); ++item) {
        names.push_back(item.key());
    }
    writer.createDirectoryInTmp(names);
    for (nlohmann::json::const_iterator item = body.begin(); item != body.end(); ++item) {
        const std::string name = item.key();
        const nlohmann::json spec = item.value();
        const bool hasFrom = spec.contains(pack::KEY_FROM) && spec[pack::KEY_FROM].is_string() && !spec[pack::KEY_FROM].get<std::string>().empty();
        const std::string from = hasFrom ? spec[pack::KEY_FROM].get<std::string>() : "";
        const std::string to = spec[pack::KEY_TO].get<std::string>();
        std::string fromDigest;
        if (hasFrom) {
            manifest(name, from, fromDigest);
        }
        push(name, to);
        std::string toDigest;
        const nlohmann::json toManifest = manifest(name, to, toDigest);
        const std::vector<nlohmann::json> fromManifests = hasFrom ? tree(name, fromDigest) : std::vector<nlohmann::json>{};
        const std::vector<nlohmann::json> toManifests = tree(name, toDigest);
        const std::vector<PackBlob> blobs = writer.missing(fromManifests, toManifests);
        for (const PackBlob& blob : blobs) {
            save(name, blob.kind, blob.digest, writer.blobFile(name, blob.digest));
        }
        save(name, "manifests", toDigest, writer.blobFile(name, toDigest));
        PackImage image;
        image.name = name;
        image.tag = to;
        image.fromDigest = fromDigest;
        image.toDigest = toDigest;
        image.toManifest = toManifest;
        writer.layout(image);
        data[name] = {
            {pack::KEY_TAG, to},
            {pack::KEY_FROM, fromDigest.empty() ? nlohmann::json(nullptr) : nlohmann::json(fromDigest)},
            {pack::KEY_TO, toDigest},
        };
    }
    data[pack::KEY_FILE_PATH] = writer.archive();
    Response::ok(response, data);
}

void PackRoute::push(const std::string& name, const std::string& tag) {
    httplib::Headers responseHeaders;
    const std::string local = encode(name + ":" + tag);
    const std::string repo = encode(std::string(pack::PUSH_HOST) + "/" + name);
    const std::string encodedTag = encode(tag);
    request(pack::DOCKER_HOST, pack::DOCKER_PORT, "POST", std::format("/images/{}/tag?repo={}&tag={}", local, repo, encodedTag), {}, responseHeaders);
    const httplib::Headers headers = {{pack::HEADER_AUTH, pack::REGISTRY_AUTH}};
    request(pack::DOCKER_HOST, pack::DOCKER_PORT, "POST", std::format("/images/{}/push?tag={}", repo, encodedTag), headers, responseHeaders);
}

nlohmann::json PackRoute::manifest(const std::string& name, const std::string& reference, std::string& digest) {
    httplib::Headers responseHeaders;
    const httplib::Headers headers = {{pack::HEADER_ACCEPT, pack::MANIFEST_ACCEPT}};
    const std::string body = request(pack::REGISTRY_HOST, pack::REGISTRY_PORT, "GET", std::format("/v2/{}/manifests/{}", name, reference), headers, responseHeaders);
    digest.clear();
    for (const httplib::Headers::value_type& header : responseHeaders) {
        const bool match = strcasecmp(header.first.c_str(), pack::HEADER_DIGEST) == 0;
        digest = match ? header.second : digest;
    }
    if (digest.empty()) {
        throw std::runtime_error("manifest digest missing");
    }
    const nlohmann::json document = nlohmann::json::parse(body);
    return document;
}

std::vector<nlohmann::json> PackRoute::tree(const std::string& name, const std::string& digest) {
    std::string unused;
    const nlohmann::json document = manifest(name, digest, unused);
    std::vector<nlohmann::json> documents = {document};
    if (document.contains("manifests")) {
        for (const nlohmann::json& child : document["manifests"]) {
            const std::vector<nlohmann::json> nested = tree(name, child["digest"].get<std::string>());
            documents.insert(documents.end(), nested.begin(), nested.end());
        }
    }
    return documents;
}

void PackRoute::save(const std::string& name, const std::string& kind, const std::string& digest, const std::filesystem::path& file) {
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    httplib::Client client(pack::REGISTRY_HOST, pack::REGISTRY_PORT);
    client.set_connection_timeout(pack::CONNECT_TIMEOUT_SECONDS);
    client.set_read_timeout(pack::READ_TIMEOUT_SECONDS);
    const httplib::Headers headers = {{pack::HEADER_ACCEPT, pack::MANIFEST_ACCEPT}};
    const std::string path = std::format("/v2/{}/{}/{}", name, kind, digest);
    const httplib::Result result = client.Get(
        path,
        headers,
        [](const httplib::Response& incoming) { return incoming.status >= 200 && incoming.status < 300; },
        [&output](const char* data, std::size_t length) {
            output.write(data, static_cast<std::streamsize>(length));
            return output.good();
        });
    const bool ok = static_cast<bool>(result) && result->status >= 200 && result->status < 300 && output.good();
    if (!ok) {
        throw std::runtime_error(std::format("GET {} -> {}", path, result ? result->status : 0));
    }
}

std::string PackRoute::request(const std::string& host, int port, const std::string& method, const std::string& path, const httplib::Headers& headers, httplib::Headers& responseHeaders) {
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
    const bool ok = static_cast<bool>(result) && result->status >= 200 && result->status < 300;
    if (!ok) {
        throw std::runtime_error(std::format("{} {} -> {}", method, path, result ? result->status : 0));
    }
    responseHeaders = result->headers;
    return result->body;
}

std::string PackRoute::encode(const std::string& value) {
    std::string encoded;
    for (const unsigned char character : value) {
        const bool plain = std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~';
        encoded += plain ? std::string(1, static_cast<char>(character)) : std::format("%{:02X}", static_cast<int>(character));
    }
    return encoded;
}
