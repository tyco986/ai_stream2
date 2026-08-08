import os
from pathlib import Path


def env_bool(name, default=False):
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


BASE_DIR = Path(__file__).resolve().parent.parent

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
SECRET_KEY = os.environ.get("DJANGO_SECRET_KEY", "dev-insecure-change-me")
DEBUG = env_bool("DEBUG", True)
ALLOWED_HOSTS = [
    host.strip()
    for host in os.environ.get("DJANGO_ALLOWED_HOSTS", "*").split(",")
    if host.strip()
]

AGE_DEV_PUB_PATH = Path(os.environ.get("AGE_DEV_PUB_PATH", "/app/keys/dev.pub"))
AGE_SITE_KEY_PATH = Path(os.environ.get("AGE_SITE_KEY_PATH", "/secrets/age/site.key"))
AGE_SITE_PUB_PATH = Path(os.environ.get("AGE_SITE_PUB_PATH", "/secrets/age/site.pub"))
TICKET_PUB_PATH = Path(os.environ.get("TICKET_PUB_PATH", "/app/keys/ticket.pub"))
SITE_CONFIG_PAYLOAD_DIR = Path(
    os.environ.get("SITE_CONFIG_PAYLOAD_DIR", "/secrets/age/payloads")
)
SITE_ID = os.environ.get("SITE_ID", "")
SITE_CONFIG_SCHEMA_VERSION = int(os.environ.get("SITE_CONFIG_SCHEMA_VERSION", "3"))
APP_VERSION = os.environ.get("APP_VERSION", "0.0.0")
SEED_ADMIN_USERNAME = os.environ.get("SEED_ADMIN_USERNAME", "admin")
SEED_ADMIN_PASSWORD = os.environ.get("SEED_ADMIN_PASSWORD", "admin")
SEED_ADMIN_GROUP = os.environ.get("SEED_ADMIN_GROUP", "admin")
DEFAULT_SHELL_MODE = "sidebar"

FFMPEG_BASE_URL = os.environ.get(
    "FFMPEG_BASE_URL",
    f"http://{PROJECT_NAME}_ffmpeg:8080",
)
MEDIAMTX_BASE_URL = os.environ.get(
    "MEDIAMTX_BASE_URL",
    f"http://{PROJECT_NAME}_mediamtx:9997",
)
MEDIAMTX_PLAYBACK_URL = os.environ.get(
    "MEDIAMTX_PLAYBACK_URL",
    f"http://{PROJECT_NAME}_mediamtx:9996",
)
FFMPEG_TIMEOUT_SECONDS = float(os.environ.get("FFMPEG_TIMEOUT_SECONDS", "30"))
MEDIAMTX_TIMEOUT_SECONDS = float(os.environ.get("MEDIAMTX_TIMEOUT_SECONDS", "10"))
MEDIAMTX_PLAYBACK_TIMEOUT_SECONDS = float(
    os.environ.get("MEDIAMTX_PLAYBACK_TIMEOUT_SECONDS", "120")
)
MEDIAMTX_RECORD_ROOT = os.environ.get("MEDIAMTX_RECORD_ROOT", "/recordings")
MEDIAMTX_RECORD_SEGMENT_DURATION = os.environ.get(
    "MEDIAMTX_RECORD_SEGMENT_DURATION",
    "1h",
)
MEDIAMTX_RECORD_DELETE_AFTER = os.environ.get("MEDIAMTX_RECORD_DELETE_AFTER", "24h")
RECORDINGS_PLAYBACK_WINDOW_SECONDS = float(
    os.environ.get("RECORDINGS_PLAYBACK_WINDOW_SECONDS", "10")
)
STREAM_LOG_DIR = Path(
    os.environ.get("STREAM_LOG_DIR", str(BASE_DIR / "logs" / "streams"))
)
PREVIEW_SHOT_DIR = Path(
    os.environ.get("PREVIEW_SHOT_DIR", str(BASE_DIR / "media" / "preview_shots"))
)
RECORDINGS_ROOT = Path(
    os.environ.get("RECORDINGS_ROOT", MEDIAMTX_RECORD_ROOT)
)
RECORDINGS_DOWNLOAD_MAX = int(os.environ.get("RECORDINGS_DOWNLOAD_MAX", "100"))
RECORDINGS_DOWNLOAD_TICKET_MAX_AGE = int(
    os.environ.get("RECORDINGS_DOWNLOAD_TICKET_MAX_AGE", "120")
)
RECORDINGS_MEDIA_TICKET_MAX_AGE = int(
    os.environ.get("RECORDINGS_MEDIA_TICKET_MAX_AGE", "14400")
)
# Browser-reachable origin for large downloads (bypass Vite proxy).
PUBLIC_API_ORIGIN = os.environ.get(
    "PUBLIC_API_ORIGIN",
    f"http://127.0.0.1:{os.environ.get('DJANGO_HOST_PORT', '8000')}",
).rstrip("/")
DOCKER_HOST = os.environ.get(
    "DOCKER_HOST",
    f"tcp://{PROJECT_NAME}_docker_socket_proxy:2375",
)
SERVERS_HEALTH_TIMEOUT = float(os.environ.get("SERVERS_HEALTH_TIMEOUT", "5"))
SERVERS_DOCKER_TIMEOUT = float(os.environ.get("SERVERS_DOCKER_TIMEOUT", "30"))
# Paths/ports below mirror servers/* defaults (not env-overridable).
SERVERS_LOG_ROOT = Path("/root/logs")
MODELS_ROOT = Path("/root/models")
MODELS_BUILD_LOG_DIR = Path(
    os.environ.get(
        "MODELS_BUILD_LOG_DIR",
        str(BASE_DIR / "logs" / "models_builds"),
    )
)
EXPORT_ONNX_BASE_URL = os.environ.get(
    "EXPORT_ONNX_BASE_URL",
    f"http://{PROJECT_NAME}_export_onnx:8090",
)
EXPORT_TRT_BASE_URL = os.environ.get(
    "EXPORT_TRT_BASE_URL",
    f"http://{PROJECT_NAME}_export_trt:9000",
)
MODELS_EXPORT_ONNX_ROUTE = "export_yolo11"
MODELS_BUILD_TIMEOUT = float(os.environ.get("MODELS_BUILD_TIMEOUT", "3600"))
GENERATOR_BASE_URL = os.environ.get(
    "GENERATOR_BASE_URL",
    f"http://{PROJECT_NAME}_generator:8091",
)
HOST_PROJECT_ROOT = os.environ.get("HOST_PROJECT_ROOT", "").rstrip("/")
DEEPSTREAM_IMAGE = os.environ.get(
    "DEEPSTREAM_IMAGE",
    f"{PROJECT_NAME}_deepstream_dev" if DEBUG else f"{PROJECT_NAME}_deepstream_prod",
)
DEEPSTREAM_API_PORT = 8092
DEEPSTREAM_HOST_PORT_BASE = int(os.environ.get("DEEPSTREAM_HOST_PORT_BASE", "2000"))
DEEPSTREAM_HEALTH_TIMEOUT = float(
    os.environ.get("DEEPSTREAM_HEALTH_TIMEOUT", "60")
)
DEEPSTREAM_HEALTH_POLL_INTERVAL = float(
    os.environ.get("DEEPSTREAM_HEALTH_POLL_INTERVAL", "1")
)
GENERATOR_CONFIG_ROOT = Path("/root/configs/generator")
DEEPSTREAM_CONFIG_ROOT = Path("/root/configs/deepstream")
DEEPSTREAM_LOG_ROOT = Path("/root/logs/deepstream")
DEEPSTREAM_KAFKA_TOPIC = "deepstream-detections"
DEEPSTREAM_KAFKA_PORT = 9092
PIPELINES_MEDIA_DIR = Path(
    os.environ.get(
        "PIPELINES_MEDIA_DIR",
        str(BASE_DIR / "media" / "analyzer-templates"),
    )
)
PIPELINES_LOG_DIR = Path(
    os.environ.get("PIPELINES_LOG_DIR", str(BASE_DIR / "logs" / "pipelines"))
)
PIPELINES_UPSTREAM_TIMEOUT = float(
    os.environ.get("PIPELINES_UPSTREAM_TIMEOUT", "300")
)
EVENTS_MEDIA_DIR = Path(
    os.environ.get(
        "EVENTS_MEDIA_DIR",
        str(BASE_DIR / "media" / "events"),
    )
)
EVENTS_EXPORT_MAX = int(os.environ.get("EVENTS_EXPORT_MAX", "5000"))
KAFKA_BOOTSTRAP_SERVERS = os.environ.get(
    "KAFKA_BOOTSTRAP_SERVERS",
    f"{PROJECT_NAME}_kafka:9092",
)
EVENTS_KAFKA_TOPIC = os.environ.get("EVENTS_KAFKA_TOPIC", "deepstream-detections")
EVENTS_KAFKA_GROUP_ID = os.environ.get(
    "EVENTS_KAFKA_GROUP_ID",
    f"{PROJECT_NAME}_events",
)

INSTALLED_APPS = [
    "django.contrib.admin",
    "django.contrib.auth",
    "django.contrib.contenttypes",
    "django.contrib.sessions",
    "django.contrib.messages",
    "django.contrib.staticfiles",
    "rest_framework",
    "pages.users.apps.UsersConfig",
    "pages.login.apps.LoginConfig",
    "pages.shell.apps.ShellConfig",
    "pages.streams.apps.StreamsConfig",
    "pages.preview.apps.PreviewConfig",
    "pages.recordings.apps.RecordingsConfig",
    "pages.servers.apps.ServersConfig",
    "pages.models_page.apps.ModelsConfig",
    "pages.pipelines.apps.PipelinesConfig",
    "pages.events.apps.EventsConfig",
    "shared.permissions_catalog.apps.PermissionsCatalogConfig",
]

MIDDLEWARE = [
    "django.middleware.security.SecurityMiddleware",
    "django.contrib.sessions.middleware.SessionMiddleware",
    "django.middleware.common.CommonMiddleware",
    "django.middleware.csrf.CsrfViewMiddleware",
    "django.contrib.auth.middleware.AuthenticationMiddleware",
    "django.contrib.messages.middleware.MessageMiddleware",
    "django.middleware.clickjacking.XFrameOptionsMiddleware",
]

ROOT_URLCONF = "config.urls"
WSGI_APPLICATION = "config.wsgi.application"
ASGI_APPLICATION = "config.asgi.application"

TEMPLATES = [
    {
        "BACKEND": "django.template.backends.django.DjangoTemplates",
        "DIRS": [],
        "APP_DIRS": True,
        "OPTIONS": {
            "context_processors": [
                "django.template.context_processors.request",
                "django.contrib.auth.context_processors.auth",
                "django.contrib.messages.context_processors.messages",
            ],
        },
    },
]

DATABASES = {
    "default": {
        "ENGINE": "django.db.backends.postgresql",
        "HOST": os.environ.get("POSTGRES_HOST", f"{PROJECT_NAME}_postgresql"),
        "PORT": os.environ.get("POSTGRES_PORT", "5432"),
        "NAME": os.environ.get("POSTGRES_DB", PROJECT_NAME),
        "USER": os.environ.get("POSTGRES_USER", PROJECT_NAME),
        "PASSWORD": os.environ.get("POSTGRES_PASSWORD", PROJECT_NAME),
    }
}

AUTH_USER_MODEL = "users.User"

AUTH_PASSWORD_VALIDATORS = [
    {"NAME": "django.contrib.auth.password_validation.UserAttributeSimilarityValidator"},
    {"NAME": "django.contrib.auth.password_validation.MinimumLengthValidator"},
    {"NAME": "django.contrib.auth.password_validation.CommonPasswordValidator"},
    {"NAME": "django.contrib.auth.password_validation.NumericPasswordValidator"},
]

LANGUAGE_CODE = "en-us"
TIME_ZONE = "UTC"
USE_I18N = True
USE_TZ = True

STATIC_URL = "static/"
DEFAULT_AUTO_FIELD = "django.db.models.BigAutoField"

SESSION_COOKIE_HTTPONLY = True
SESSION_COOKIE_SAMESITE = "Lax"
CSRF_COOKIE_SAMESITE = "Lax"
CSRF_TRUSTED_ORIGINS = [
    origin.strip()
    for origin in os.environ.get(
        "CSRF_TRUSTED_ORIGINS",
        "http://127.0.0.1:8000,http://localhost:8000",
    ).split(",")
    if origin.strip()
]

REST_FRAMEWORK = {
    "DEFAULT_AUTHENTICATION_CLASSES": [
        "shared.auth.authentication.SessionAuthenticationWithoutCSRF",
    ],
    "DEFAULT_PERMISSION_CLASSES": [
        "rest_framework.permissions.IsAuthenticated",
    ],
    "EXCEPTION_HANDLER": "shared.http.exceptions.api_exception_handler",
    "UNAUTHENTICATED_USER": None,
}

LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "verbose": {
            "format": "%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        },
    },
    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
            "formatter": "verbose",
        },
    },
    "root": {
        "handlers": ["console"],
        "level": os.environ.get("LOG_LEVEL", "INFO"),
    },
}
