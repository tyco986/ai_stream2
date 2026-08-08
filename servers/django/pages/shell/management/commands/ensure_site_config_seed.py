from django.core.management.base import BaseCommand

from pages.shell.services import SiteConfigOrchestrator
from pages.users.management.commands.ensure_seed_admin import Command as SeedAdminCommand


class Command(BaseCommand):
    help = "Ensure site config version 0 payload and seed admin user"

    def handle(self, *args, **options):
        SeedAdminCommand().handle()
        SiteConfigOrchestrator().ensure_seed()
        self.stdout.write("Site config seed ensured")
