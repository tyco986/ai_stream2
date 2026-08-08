from django.conf import settings

from pages.pipelines.models import Pipeline


class DeepStreamPortManager:
    def allocate(self):
        base = int(settings.DEEPSTREAM_HOST_PORT_BASE)
        used = set(
            Pipeline.objects.exclude(host_port=None).values_list("host_port", flat=True)
        )
        port = base
        while port in used:
            port += 1
        return port
