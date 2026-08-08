from django.core.management.base import BaseCommand

from pages.events.consumers import EventKafkaConsumer


class Command(BaseCommand):
    help = "Consume Kafka event envelopes (skips non-envelope messages)"

    def handle(self, *args, **options):
        EventKafkaConsumer().run_with_backoff()
