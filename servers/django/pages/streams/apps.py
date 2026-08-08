from django.apps import AppConfig


class StreamsConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "pages.streams"
    label = "streams"

    def ready(self):
        from pages.streams.site_config import register_streams_site_config
        from pages.streams.stream_lookup import register_stream_name_resolver

        register_streams_site_config()
        register_stream_name_resolver()
