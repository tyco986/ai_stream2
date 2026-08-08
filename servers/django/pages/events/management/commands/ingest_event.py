import json
from pathlib import Path

from django.core.management.base import BaseCommand, CommandError

from pages.events.services import EventIngestService


class Command(BaseCommand):
    help = "Ingest one event envelope JSON file"

    def add_arguments(self, parser):
        parser.add_argument("--file", required=True, help="Path to envelope JSON")

    def handle(self, *args, **options):
        path = Path(options["file"])
        if not path.is_file():
            raise CommandError(f"File not found: {path}")
        envelope = json.loads(path.read_text(encoding="utf-8"))
        row = EventIngestService().ingest(envelope)
        self.stdout.write(self.style.SUCCESS(str(row.id)))
