-- user page

CREATE TABLE user_group (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  stream CHAR(4) NOT NULL DEFAULT '0000',
  model CHAR(4) NOT NULL DEFAULT '0000',
  preview CHAR(4) NOT NULL DEFAULT '0000',
  pipeline CHAR(4) NOT NULL DEFAULT '0000',
  recording CHAR(4) NOT NULL DEFAULT '0000',
  server CHAR(4) NOT NULL DEFAULT '0000',
  "user" CHAR(4) NOT NULL DEFAULT '0000',
  event CHAR(4) NOT NULL DEFAULT '0000'
);

CREATE TABLE "user" (
  id UUID PRIMARY KEY,
  username VARCHAR(150) NOT NULL UNIQUE,
  password VARCHAR(128) NOT NULL,
  active BOOLEAN NOT NULL DEFAULT TRUE,
  group_id UUID REFERENCES user_group (id),
  group_name VARCHAR(150),
  last_login TIMESTAMPTZ
);

INSERT INTO user_group (id, name, stream, model, preview, pipeline, recording, server, "user", event)
VALUES (
  '00000000-0000-4000-8000-000000000003',
  'admin',
  '1111',
  '1111',
  '1111',
  '1111',
  '1111',
  '1111',
  '1111',
  '1111'
);

INSERT INTO "user" (id, username, password, group_id, group_name)
VALUES (
  '00000000-0000-4000-8000-000000000004',
  'admin',
  'admin',
  '00000000-0000-4000-8000-000000000003',
  'admin'
);

-- stream page

CREATE TABLE stream_group (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  parent_id UUID REFERENCES stream_group (id) ON DELETE SET NULL
);

CREATE TABLE stream (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  group_id UUID NOT NULL REFERENCES stream_group (id),
  url TEXT NOT NULL UNIQUE,
  width INT,
  height INT,
  fps NUMERIC(6, 2),
  status VARCHAR(16) NOT NULL DEFAULT 'offline',
  enabled BOOLEAN NOT NULL DEFAULT TRUE,
  recording BOOLEAN NOT NULL DEFAULT FALSE,
  probed_at TIMESTAMPTZ
);

CREATE INDEX stream_group_id_idx ON stream (group_id);

INSERT INTO stream_group (id, name, parent_id)
VALUES ('00000000-0000-4000-8000-000000000001', 'All', NULL);

-- model page

CREATE TABLE model (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  version VARCHAR(64),
  batch_size INT NOT NULL,
  batch_mode VARCHAR(16) NOT NULL,
  precision VARCHAR(16) NOT NULL DEFAULT 'fp16',
  num_class INT,
  classes JSONB,
  source_path TEXT,
  engine_path TEXT,
  status VARCHAR(16) NOT NULL DEFAULT 'not_built',
  built_at TIMESTAMPTZ,
  ever_built BOOLEAN NOT NULL DEFAULT FALSE
);

-- preview page

CREATE TABLE preview_preset (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  layout VARCHAR(16) NOT NULL,
  mode VARCHAR(16) NOT NULL DEFAULT 'grid',
  slots JSONB NOT NULL
);

CREATE TABLE preview_active (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL UNIQUE,
  preset_id UUID NOT NULL
);

INSERT INTO preview_preset (id, name, layout, mode, slots)
VALUES (
  '00000000-0000-4000-8000-000000000002',
  'Default',
  '2x2',
  'grid',
  '[null, null, null, null]'::jsonb
);

-- pipeline page

CREATE TABLE pipeline (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  type VARCHAR(64) NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'stopped',
  config JSONB NOT NULL,
  updated_at TIMESTAMPTZ
);

CREATE TABLE pipeline_gie (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  model_id UUID NOT NULL,
  model_name VARCHAR(150),
  class_attrs JSONB NOT NULL,
  updated_at TIMESTAMPTZ
);

CREATE TABLE pipeline_analyzer (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  source_path TEXT,
  annotations JSONB NOT NULL DEFAULT '[]'::jsonb,
  updated_at TIMESTAMPTZ
);

-- event page

CREATE TABLE event (
  id UUID PRIMARY KEY,
  occurred_at TIMESTAMPTZ NOT NULL,
  stream_id UUID NOT NULL,
  stream_name VARCHAR(150) NOT NULL,
  pipeline_id UUID NOT NULL,
  pipeline_name VARCHAR(150) NOT NULL,
  event_code VARCHAR(64) NOT NULL,
  event_label VARCHAR(150) NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'new',
  raw_path TEXT,
  visualization_path TEXT,
  annotations JSONB
);

CREATE INDEX event_occurred_at_id_idx ON event (occurred_at, id);
CREATE INDEX event_stream_id_idx ON event (stream_id);
CREATE INDEX event_pipeline_id_event_code_idx ON event (pipeline_id, event_code);
CREATE INDEX event_status_idx ON event (status);

-- server page

CREATE TABLE server (
  id UUID PRIMARY KEY,
  probed_at TIMESTAMPTZ,
  deepstream JSONB,
  generator BOOLEAN NOT NULL DEFAULT FALSE,
  export_onnx BOOLEAN NOT NULL DEFAULT FALSE,
  export_trt BOOLEAN NOT NULL DEFAULT FALSE,
  mediamtx BOOLEAN NOT NULL DEFAULT FALSE,
  kafka BOOLEAN NOT NULL DEFAULT FALSE,
  postgresql BOOLEAN NOT NULL DEFAULT FALSE,
  frontend BOOLEAN NOT NULL DEFAULT FALSE,
  backend BOOLEAN NOT NULL DEFAULT FALSE
);
