-- user page

CREATE TABLE user_group (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  stream_crud CHAR(4) NOT NULL,
  model_crud CHAR(4) NOT NULL,
  preview_crud CHAR(4) NOT NULL,
  pipeline_crud CHAR(4) NOT NULL,
  recording_crud CHAR(4) NOT NULL,
  server_crud CHAR(4) NOT NULL,
  user_crud CHAR(4) NOT NULL,
  event_crud CHAR(4) NOT NULL
);

CREATE TABLE "user" (
  id UUID PRIMARY KEY,
  username VARCHAR(150) NOT NULL UNIQUE,
  passwd VARCHAR(128) NOT NULL,
  token VARCHAR(36) UNIQUE,
  active BOOLEAN NOT NULL,
  group_id UUID REFERENCES user_group (id),
  last_login TIMESTAMPTZ
);

INSERT INTO user_group (id, name, stream_crud, model_crud, preview_crud, pipeline_crud, recording_crud, server_crud, user_crud, event_crud)
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

INSERT INTO "user" (id, username, passwd, active, group_id)
VALUES (
  '00000000-0000-4000-8000-000000000004',
  'admin',
  'admin',
  TRUE,
  '00000000-0000-4000-8000-000000000003'
);

CREATE FUNCTION user_group_reject_change() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
  IF OLD.id = '00000000-0000-4000-8000-000000000003' THEN
    RAISE EXCEPTION 'admin group cannot be changed';
  END IF;
  IF TG_OP = 'DELETE' THEN
    RETURN OLD;
  END IF;
  RETURN NEW;
END;
$$;

CREATE TRIGGER user_group_reject_change
BEFORE DELETE OR UPDATE ON user_group
FOR EACH ROW
EXECUTE FUNCTION user_group_reject_change();

CREATE FUNCTION user_reject_change() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
  IF OLD.id = '00000000-0000-4000-8000-000000000004' THEN
    IF TG_OP = 'DELETE' OR NEW.username IS DISTINCT FROM OLD.username
       OR NEW.active IS DISTINCT FROM OLD.active
       OR NEW.group_id IS DISTINCT FROM OLD.group_id THEN
      RAISE EXCEPTION 'admin user cannot be changed';
    END IF;
  END IF;
  IF TG_OP = 'DELETE' THEN
    RETURN OLD;
  END IF;
  RETURN NEW;
END;
$$;

CREATE TRIGGER user_reject_change
BEFORE DELETE OR UPDATE ON "user"
FOR EACH ROW
EXECUTE FUNCTION user_reject_change();

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
  status VARCHAR(16) NOT NULL,
  mount BOOLEAN NOT NULL,
  record BOOLEAN NOT NULL,
  probed_at TIMESTAMPTZ
);

CREATE INDEX stream_group_id_idx ON stream (group_id);

INSERT INTO stream_group (id, name, parent_id)
VALUES
  ('00000000-0000-4000-8000-000000000001', 'All', NULL);

CREATE FUNCTION stream_group_reject_delete() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
  IF OLD.id = '00000000-0000-4000-8000-000000000001' THEN
    RAISE EXCEPTION 'system group cannot be deleted';
  END IF;
  RETURN OLD;
END;
$$;

CREATE TRIGGER stream_group_reject_delete
BEFORE DELETE ON stream_group
FOR EACH ROW
EXECUTE FUNCTION stream_group_reject_delete();

-- model page

CREATE TABLE model (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  type VARCHAR(16) NOT NULL,
  version VARCHAR(64),
  batch_size INT NOT NULL,
  batch_mode VARCHAR(16) NOT NULL,
  input_tensors JSONB,
  output_tensors JSONB,
  precision VARCHAR(16) NOT NULL,
  opt_level INT,
  device INT NOT NULL,
  workspace INT NOT NULL,
  static_plugin TEXT,
  max_det INT,
  conf NUMERIC(4, 2),
  iou NUMERIC(4, 2),
  num_class INT,
  classes JSONB,
  source_path TEXT,
  engine_path TEXT,
  status VARCHAR(16) NOT NULL,
  built_at TIMESTAMPTZ,
  trt_version VARCHAR(64),
  ever_built BOOLEAN NOT NULL
);

-- preview page

CREATE TABLE preview_preset (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES "user" (id),
  name VARCHAR(150) NOT NULL,
  layout VARCHAR(16) NOT NULL,
  mode VARCHAR(16) NOT NULL,
  slots JSONB NOT NULL,
  active BOOLEAN NOT NULL,
  UNIQUE (user_id, name)
);

CREATE UNIQUE INDEX preview_preset_one_active_idx
ON preview_preset (user_id)
WHERE active;

INSERT INTO preview_preset (id, user_id, name, layout, mode, slots, active)
VALUES (
  '00000000-0000-4000-8000-000000000006',
  '00000000-0000-4000-8000-000000000004',
  'Default',
  '2x2',
  'grid',
  '{"1":null,"2":null,"3":null,"4":null}'::jsonb,
  TRUE
);

CREATE FUNCTION preview_preset_reject_delete() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
  IF OLD.id = '00000000-0000-4000-8000-000000000006' THEN
    RAISE EXCEPTION 'admin preset cannot be deleted';
  END IF;
  RETURN OLD;
END;
$$;

CREATE TRIGGER preview_preset_reject_delete
BEFORE DELETE ON preview_preset
FOR EACH ROW
EXECUTE FUNCTION preview_preset_reject_delete();

-- pipeline page

CREATE TABLE pipeline (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  type VARCHAR(64) NOT NULL,
  status VARCHAR(16) NOT NULL,
  input JSONB,
  config_path TEXT,
  stream_ids UUID[] NOT NULL,
  gie_ids UUID[] NOT NULL,
  analyzer_ids UUID[] NOT NULL,
  start_time TIMESTAMPTZ,
  end_time TIMESTAMPTZ,
  duration BIGINT,
  updated_at TIMESTAMPTZ
);

CREATE INDEX pipeline_stream_ids_idx ON pipeline USING GIN (stream_ids);
CREATE INDEX pipeline_gie_ids_idx ON pipeline USING GIN (gie_ids);
CREATE INDEX pipeline_analyzer_ids_idx ON pipeline USING GIN (analyzer_ids);

CREATE TABLE pipeline_gie (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  model_id UUID NOT NULL,
  model_name VARCHAR(150),
  class_attrs JSONB,
  interval INT NOT NULL,
  updated_at TIMESTAMPTZ
);

CREATE TABLE pipeline_analyzer (
  id UUID PRIMARY KEY,
  name VARCHAR(150) NOT NULL UNIQUE,
  source_path TEXT,
  annotations JSONB NOT NULL,
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
  status VARCHAR(16) NOT NULL,
  raw_path TEXT,
  visualization_path TEXT,
  annotations JSONB
);

CREATE INDEX event_occurred_at_id_idx ON event (occurred_at, id);
CREATE INDEX event_stream_id_idx ON event (stream_id);
CREATE INDEX event_pipeline_id_event_code_idx ON event (pipeline_id, event_code);
CREATE INDEX event_status_idx ON event (status);

-- notice

CREATE TABLE notice (
  id UUID PRIMARY KEY,
  created_at TIMESTAMPTZ NOT NULL,
  read_at TIMESTAMPTZ,
  user_id UUID REFERENCES "user" (id) ON DELETE CASCADE,
  level VARCHAR(16) NOT NULL,
  source VARCHAR(64) NOT NULL,
  text TEXT NOT NULL,
  ref_id UUID
);

CREATE INDEX notice_user_id_created_at_idx ON notice (user_id, created_at);

-- server page

CREATE TABLE server (
  id UUID PRIMARY KEY,
  probed_at TIMESTAMPTZ,
  deepstream JSONB,
  generator BOOLEAN NOT NULL,
  export_onnx BOOLEAN NOT NULL,
  export_trt BOOLEAN NOT NULL,
  mediamtx BOOLEAN NOT NULL,
  kafka BOOLEAN NOT NULL,
  postgresql BOOLEAN NOT NULL,
  frontend BOOLEAN NOT NULL,
  backend BOOLEAN NOT NULL
);

CREATE FUNCTION server_reject_delete() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
  RAISE EXCEPTION 'server row cannot be deleted';
END;
$$;

CREATE TRIGGER server_reject_delete
BEFORE DELETE ON server
FOR EACH ROW
EXECUTE FUNCTION server_reject_delete();

INSERT INTO server (id, deepstream, generator, export_onnx, export_trt, mediamtx, kafka, postgresql, frontend, backend)
VALUES (
  '00000000-0000-4000-8000-000000000005',
  '[]'::jsonb,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE
);

-- media page

CREATE TABLE media (
  id UUID PRIMARY KEY,
  type VARCHAR(16) NOT NULL,
  name VARCHAR(150) NOT NULL,
  width INT,
  height INT,
  duration NUMERIC(12, 3),
  fps NUMERIC(6, 2),
  codec VARCHAR(32),
  size BIGINT,
  md5 VARCHAR(32) NOT NULL,
  folder TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL,
  updated_at TIMESTAMPTZ NOT NULL,
  user_id UUID NOT NULL REFERENCES "user" (id),
  stream_id UUID REFERENCES stream (id) ON DELETE SET NULL,
  pipeline_id UUID REFERENCES pipeline (id) ON DELETE SET NULL,
  CONSTRAINT media_type_check CHECK (
    type IN ('image', 'video')
    AND (type = 'video' OR (duration IS NULL AND fps IS NULL AND codec IS NULL))
  )
);

CREATE INDEX media_user_id_idx ON media (user_id);
CREATE INDEX media_stream_id_idx ON media (stream_id);
CREATE INDEX media_pipeline_id_idx ON media (pipeline_id);
