#pragma once

namespace openapi {

inline constexpr const char* SPEC_PATH = "/openapi.json";
inline constexpr const char* DOCS_PATH = "/docs";

inline constexpr const char* SPEC = R"json({
  "openapi": "3.0.3",
  "info": {"title": "export_trt", "version": "1.0.0"},
  "paths": {
    "/ai_stream2/export_trt/health": {
      "get": {
        "summary": "Health",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/export_trt/types": {
      "get": {
        "summary": "Model types",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/TypesEnvelope"}}}}}
      }
    },
    "/ai_stream2/export_trt/export": {
      "post": {
        "summary": "Build a TensorRT engine zip",
        "requestBody": {
          "required": true,
          "content": {
            "multipart/form-data": {
              "schema": {
                "type": "object",
                "required": ["input", "config"],
                "properties": {
                  "input": {"type": "string", "format": "binary", "description": "Zip with one .onnx, labels.txt, meta.json"},
                  "config": {"type": "string", "format": "binary", "description": "YAML with type, precision, batch_size, device"}
                }
              }
            }
          }
        },
        "responses": {
          "200": {"description": "Engine zip", "content": {"multipart/form-data": {"schema": {"$ref": "#/components/schemas/FileEnvelope"}}}},
          "400": {"description": "Error", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}
        }
      }
    },
    "/ai_stream2/export_trt/history": {
      "get": {
        "summary": "List history meta",
        "parameters": [{"name": "filename", "in": "query", "schema": {"type": "string"}}],
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/HistoryEnvelope"}}}}}
      },
      "post": {
        "summary": "Download history zip",
        "parameters": [{"name": "filename", "in": "query", "schema": {"type": "string"}}],
        "responses": {
          "200": {"description": "Nested multipart of zip parts", "content": {"multipart/form-data": {"schema": {"$ref": "#/components/schemas/FileEnvelope"}}}},
          "400": {"description": "Not found", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}
        }
      },
      "delete": {
        "summary": "Delete history zip",
        "parameters": [{"name": "filename", "in": "query", "schema": {"type": "string"}}],
        "responses": {
          "200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}},
          "400": {"description": "Not found", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}
        }
      }
    },
    "/ai_stream2/export_trt/logs": {
      "delete": {
        "summary": "Remove request logs",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    }
  },
  "components": {
    "schemas": {
      "Envelope": {
        "type": "object",
        "properties": {
          "success": {"type": "boolean"},
          "message": {"type": "string"},
          "data": {"type": "object"},
          "file": {"nullable": true}
        }
      },
      "BuildResult": {
        "type": "object",
        "properties": {
          "type": {"type": "string"},
          "batch_size": {"type": "integer"},
          "batch_mode": {"type": "string", "enum": ["static", "dynamic"]},
          "input_tensors": {},
          "output_tensors": {},
          "precision": {"type": "string"},
          "opt_level": {"nullable": true},
          "device": {"type": "integer"},
          "workspace": {"type": "integer"},
          "static_plugin": {"nullable": true},
          "max_det": {"nullable": true},
          "conf": {"nullable": true},
          "iou": {"nullable": true},
          "num_class": {"nullable": true},
          "classes": {"nullable": true},
          "source_path": {"type": "string"},
          "source_name": {"type": "string"},
          "engine_path": {"type": "string"},
          "engine_name": {"type": "string"},
          "status": {"type": "string"},
          "built_at": {"nullable": true},
          "trt_version": {"nullable": true}
        }
      },
      "TypesEnvelope": {
        "allOf": [
          {"$ref": "#/components/schemas/Envelope"},
          {"type": "object", "properties": {"data": {"type": "object", "properties": {"items": {"type": "array", "items": {"type": "string"}}}}}}
        ]
      },
      "HistoryEnvelope": {
        "allOf": [
          {"$ref": "#/components/schemas/Envelope"},
          {"type": "object", "properties": {"data": {"type": "object", "additionalProperties": {"type": "object"}}}}
        ]
      },
      "FileEnvelope": {
        "type": "object",
        "properties": {
          "success": {"type": "string"},
          "message": {"type": "string"},
          "data": {"$ref": "#/components/schemas/BuildResult"},
          "file": {"type": "string", "format": "binary"}
        }
      }
    }
  }
})json";

inline constexpr const char* DOCS = R"html(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <title>export_trt</title>
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css"/>
</head>
<body>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
  <script>
    SwaggerUIBundle({url: "/openapi.json", dom_id: "#swagger-ui"});
  </script>
</body>
</html>
)html";

}
