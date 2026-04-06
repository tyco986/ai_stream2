from datetime import timedelta
from pathlib import Path

import environ
import structlog

BASE_DIR = Path(__file__).resolve().parent.parent.parent

env = environ.Env(
    DEBUG=(bool, False),
    DEEPSTREAM_MOCK=(bool, False),
    ACCESS_TOKEN_LIFETIME_MINUTES=(int, 30),
    DETECTION_RETENTION_MONTHS=(int, 1),
    DEAD_LETTER_RETENTION_DAYS=(int, 90),
    HEALTH_CHECK_KAFKA=(bool, True),
)

SECRET_KEY = env("SECRET_KEY", default="dev-insecure-secret-key-change-in-production")

DEBUG = env("DEBUG")

ALLOWED_HOSTS = env.list("ALLOWED_HOSTS", default=["*"])

# ---------------------------------------------------------------------------
# Application definition
# ---------------------------------------------------------------------------

INSTALLED_APPS = [
    "django.contrib.admin",
    "django.contrib.auth",
    "django.contrib.contenttypes",
    "django.contrib.sessions",
    "django.contrib.messages",
    "django.contrib.staticfiles",
    # Third-party
    "rest_framework",
    "rest_framework_simplejwt",
    "rest_framework_simplejwt.token_blacklist",
    "corsheaders",
    "channels",
    "drf_spectacular",
    # Project apps
    "apps.accounts",
    "apps.cameras",
    "apps.detections",
    "apps.alerts",
    "apps.pipelines",
    "apps.dashboard",
]

MIDDLEWARE = [
    "corsheaders.middleware.CorsMiddleware",
    "django.middleware.security.SecurityMiddleware",
    "django.contrib.sessions.middleware.SessionMiddleware",
    "django.middleware.common.CommonMiddleware",
    "django.middleware.csrf.CsrfViewMiddleware",
    "django.contrib.auth.middleware.AuthenticationMiddleware",
    "django.contrib.messages.middleware.MessageMiddleware",
    "django.middleware.clickjacking.XFrameOptionsMiddleware",
    "common.middleware.RequestIDMiddleware",
    "common.middleware.AccessLogMiddleware",
]

ROOT_URLCONF = "config.urls"

TEMPLATES = [
    {
        "BACKEND": "django.template.backends.django.DjangoTemplates",
        "DIRS": [],
        "APP_DIRS": True,
        "OPTIONS": {
            "context_processors": [
                "django.template.context_processors.debug",
                "django.template.context_processors.request",
                "django.contrib.auth.context_processors.auth",
                "django.contrib.messages.context_processors.messages",
            ],
        },
    },
]

ASGI_APPLICATION = "config.asgi.application"
WSGI_APPLICATION = "config.wsgi.application"

# ---------------------------------------------------------------------------
# Database
# ---------------------------------------------------------------------------

DATABASES = {"default": env.db("DATABASE_URL", default="postgres://postgres:postgres@localhost:5432/ai_stream")}
DATABASES["default"]["CONN_MAX_AGE"] = env.int("DB_CONN_MAX_AGE", default=600)

DEFAULT_AUTO_FIELD = "django.db.models.BigAutoField"

# ---------------------------------------------------------------------------
# Auth
# ---------------------------------------------------------------------------

AUTH_USER_MODEL = "accounts.User"

AUTH_PASSWORD_VALIDATORS = [
    {"NAME": "django.contrib.auth.password_validation.UserAttributeSimilarityValidator"},
    {"NAME": "django.contrib.auth.password_validation.MinimumLengthValidator"},
    {"NAME": "django.contrib.auth.password_validation.CommonPasswordValidator"},
    {"NAME": "django.contrib.auth.password_validation.NumericPasswordValidator"},
]

# ---------------------------------------------------------------------------
# Redis — single URL, each component appends its own DB number
# ---------------------------------------------------------------------------

_redis_url = env("REDIS_URL", default="redis://localhost:6379")

CACHES = {
    "default": {
        "BACKEND": "django.core.cache.backends.redis.RedisCache",
        "LOCATION": f"{_redis_url}/0",
    },
}

# ---------------------------------------------------------------------------
# Celery
# ---------------------------------------------------------------------------

CELERY_BROKER_URL = f"{_redis_url}/1"
CELERY_RESULT_BACKEND = f"{_redis_url}/2"
CELERY_RESULT_EXPIRES = 3600
CELERY_TASK_ACKS_LATE = True
CELERY_TASK_REJECT_ON_WORKER_LOST = True

CELERY_BEAT_SCHEDULE = {
    "cleanup-detections": {
        "task": "tasks.maintenance.cleanup_old_detections",
        "schedule": timedelta(days=1),
        "options": {"expires": 3600},
    },
    "create-next-partition": {
        "task": "tasks.maintenance.create_next_partition",
        "schedule": timedelta(days=1),
        "options": {"expires": 3600},
    },
    "sync-camera-status": {
        "task": "tasks.maintenance.sync_camera_status",
        "schedule": 60.0,
    },
    "cleanup-dead-letters": {
        "task": "tasks.maintenance.cleanup_dead_letters",
        "schedule": timedelta(days=1),
        "options": {"expires": 3600},
    },
}

# ---------------------------------------------------------------------------
# Django Channels
# ---------------------------------------------------------------------------

CHANNEL_LAYERS = {
    "default": {
        "BACKEND": "channels_redis.core.RedisChannelLayer",
        "CONFIG": {
            "hosts": [f"{_redis_url}/3"],
        },
    },
}

# ---------------------------------------------------------------------------
# DRF
# ---------------------------------------------------------------------------

REST_FRAMEWORK = {
    "DEFAULT_AUTHENTICATION_CLASSES": [
        "rest_framework_simplejwt.authentication.JWTAuthentication",
    ],
    "DEFAULT_PERMISSION_CLASSES": [
        "rest_framework.permissions.IsAuthenticated",
    ],
    "DEFAULT_PAGINATION_CLASS": "common.pagination.StandardPagination",
    "PAGE_SIZE": 20,
    "DEFAULT_THROTTLE_CLASSES": [
        "rest_framework.throttling.AnonRateThrottle",
        "rest_framework.throttling.UserRateThrottle",
    ],
    "DEFAULT_THROTTLE_RATES": {
        "anon": "20/minute",
        "user": "200/minute",
        "login": "5/minute",
    },
    "EXCEPTION_HANDLER": "common.exceptions.custom_exception_handler",
    "DEFAULT_SCHEMA_CLASS": "drf_spectacular.openapi.AutoSchema",
}

# ---------------------------------------------------------------------------
# SimpleJWT
# ---------------------------------------------------------------------------

SIMPLE_JWT = {
    "ACCESS_TOKEN_LIFETIME": timedelta(minutes=env("ACCESS_TOKEN_LIFETIME_MINUTES")),
    "REFRESH_TOKEN_LIFETIME": timedelta(days=7),
    "ROTATE_REFRESH_TOKENS": True,
    "BLACKLIST_AFTER_ROTATION": True,
}

# ---------------------------------------------------------------------------
# drf-spectacular
# ---------------------------------------------------------------------------

SPECTACULAR_SETTINGS = {
    "TITLE": "AI Stream Backend API",
    "DESCRIPTION": "Video analytics platform — backend API gateway",
    "VERSION": "1.0.0",
    "SERVE_INCLUDE_SCHEMA": False,
}

# ---------------------------------------------------------------------------
# Internationalization
# ---------------------------------------------------------------------------

LANGUAGE_CODE = "en-us"
TIME_ZONE = "UTC"
USE_I18N = True
USE_TZ = True

# ---------------------------------------------------------------------------
# Static files
# ---------------------------------------------------------------------------

STATIC_URL = "static/"
STATIC_ROOT = BASE_DIR / "staticfiles"

# ---------------------------------------------------------------------------
# DeepStream
# ---------------------------------------------------------------------------

DEEPSTREAM_REST_URL = env("DEEPSTREAM_REST_URL", default="http://localhost:9000")
DEEPSTREAM_MOCK = env("DEEPSTREAM_MOCK")

# ---------------------------------------------------------------------------
# Kafka
# ---------------------------------------------------------------------------

KAFKA_BOOTSTRAP_SERVERS = env("KAFKA_BOOTSTRAP_SERVERS", default="localhost:9092")
KAFKA_DETECTION_TOPIC = env("KAFKA_DETECTION_TOPIC", default="deepstream-detections")
KAFKA_EVENT_TOPIC = env("KAFKA_EVENT_TOPIC", default="deepstream-events")
KAFKA_COMMAND_TOPIC = env("KAFKA_COMMAND_TOPIC", default="deepstream-commands")
KAFKA_CONSUMER_GROUP = env("KAFKA_CONSUMER_GROUP", default="backend-consumer")
KAFKA_BATCH_SIZE = env.int("KAFKA_BATCH_SIZE", default=100)
KAFKA_FLUSH_INTERVAL = env.float("KAFKA_FLUSH_INTERVAL", default=2.0)

# ---------------------------------------------------------------------------
# Business config
# ---------------------------------------------------------------------------

DETECTION_RETENTION_MONTHS = env("DETECTION_RETENTION_MONTHS")
DEAD_LETTER_RETENTION_DAYS = env("DEAD_LETTER_RETENTION_DAYS")
HEALTH_CHECK_KAFKA = env("HEALTH_CHECK_KAFKA")

# ---------------------------------------------------------------------------
# Structlog
# ---------------------------------------------------------------------------

structlog.configure(
    processors=[
        structlog.contextvars.merge_contextvars,
        structlog.stdlib.filter_by_level,
        structlog.stdlib.add_logger_name,
        structlog.stdlib.add_log_level,
        structlog.stdlib.PositionalArgumentsFormatter(),
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.StackInfoRenderer(),
        structlog.processors.UnicodeDecoder(),
        structlog.processors.JSONRenderer(),
    ],
    wrapper_class=structlog.stdlib.BoundLogger,
    context_class=dict,
    logger_factory=structlog.stdlib.LoggerFactory(),
    cache_logger_on_first_use=True,
)

LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
        },
    },
    "root": {
        "handlers": ["console"],
        "level": "INFO",
    },
}
