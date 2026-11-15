#pragma once

namespace openapi {

inline constexpr const char* SPEC_PATH = "/openapi.json";
inline constexpr const char* DOCS_PATH = "/docs";

inline constexpr const char* SPEC = R"json({
  "openapi": "3.0.3",
  "info": {"title": "generator", "version": "1.0.0"},
  "paths": {
    "/ai_stream2/generator/health": {
      "get": {
        "summary": "Health",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/generator/types": {
      "get": {
        "summary": "Pipeline types",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/TypesEnvelope"}}}}}
      }
    },
    "/ai_stream2/generator/generate": {
      "post": {
        "summary": "Generate a pipeline config zip",
        "requestBody": {
          "required": true,
          "content": {
            "multipart/form-data": {
              "schema": {
                "type": "object",
                "required": ["input"],
                "properties": {
                  "input": {"type": "string", "format": "binary", "description": "YAML with generator"}
                }
              }
            }
          }
        },
        "responses": {
          "200": {"description": "Config zip", "content": {"multipart/form-data": {"schema": {"$ref": "#/components/schemas/FileEnvelope"}}}}
        }
      }
    },
    "/ai_stream2/generator/history/enable": {
      "post": {
        "summary": "Enable history",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/generator/history/disable": {
      "post": {
        "summary": "Disable history",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/generator/history": {
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
    "/ai_stream2/generator/logs/enable": {
      "post": {
        "summary": "Enable request logs",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/generator/logs/disable": {
      "post": {
        "summary": "Disable request logs",
        "responses": {"200": {"description": "OK", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/Envelope"}}}}}
      }
    },
    "/ai_stream2/generator/logs": {
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
      "TypesEnvelope": {
        "allOf": [
          {"$ref": "#/components/schemas/Envelope"},
          {"type": "object", "properties": {"data": {"type": "object", "additionalProperties": {"type": "object"}}}}
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
          "data": {"type": "object"},
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
  <title>generator</title>
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
