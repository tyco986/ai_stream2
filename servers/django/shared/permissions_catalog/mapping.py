"""
Map transitional PermissionHost codenames (users.<codename>) to API/shell
codenames (streams.view_stream, …). User defaults and users.* group/site-config
perms pass through unchanged.
"""

# PermissionHost.codename → public "app.codename" for catalog + SessionUser.
HOST_CODENAME_TO_PUBLIC = {
    "view_server": "servers.view_server",
    "change_server": "servers.change_server",
    "view_event": "events.view_event",
    "change_event": "events.change_event",
    "view_group": "users.view_group",
    "add_group": "users.add_group",
    "change_group": "users.change_group",
    "delete_group": "users.delete_group",
    "export_site_config": "users.export_site_config",
    "import_site_config": "users.import_site_config",
}

# Catalog matrix only (no export/import).
CATALOG_MODULES = [
    {
        "key": "streams",
        "label": "Streams",
        "actions": [
            ("view", "view", "view_stream"),
            ("add", "add", "add_stream"),
            ("change", "change", "change_stream"),
            ("delete", "delete", "delete_stream"),
        ],
        "source": "stream",
    },
    {
        "key": "previews",
        "label": "Previews",
        "actions": [
            ("view", "view", "view_preview"),
            ("add", "add", "add_preview"),
            ("change", "change", "change_preview"),
            ("delete", "delete", "delete_preview"),
        ],
        "source": "preview",
    },
    {
        "key": "recordings",
        "label": "Recordings",
        "actions": [
            ("view", "view", "view_recording"),
            ("add", "add", "add_recording"),
            ("change", "change", "change_recording"),
            ("delete", "delete", "delete_recording"),
        ],
        "source": "recording",
    },
    {
        "key": "models",
        "label": "Models",
        "actions": [
            ("view", "view", "view_model"),
            ("add", "add", "add_model"),
            ("change", "change", "change_model"),
            ("delete", "delete", "delete_model"),
        ],
        "source": "model",
    },
    {
        "key": "pipelines",
        "label": "Pipelines",
        "actions": [
            ("view", "view", "view_pipeline"),
            ("add", "add", "add_pipeline"),
            ("change", "change", "change_pipeline"),
            ("delete", "delete", "delete_pipeline"),
        ],
        "source": "pipeline",
    },
    {
        "key": "servers",
        "label": "Servers",
        "actions": [
            ("view", "view", "view_server"),
            ("change", "change", "change_server"),
        ],
        "source": "host",
    },
    {
        "key": "events",
        "label": "Events",
        "actions": [
            ("view", "view", "view_event"),
            ("change", "change", "change_event"),
        ],
        "source": "host",
    },
    {
        "key": "users",
        "label": "Users",
        "actions": [
            ("view", "view", "view_user"),
            ("add", "add", "add_user"),
            ("change", "change", "change_user"),
            ("delete", "delete", "delete_user"),
        ],
        "source": "user",
    },
    {
        "key": "groups",
        "label": "Groups",
        "actions": [
            ("view", "view", "view_group"),
            ("add", "add", "add_group"),
            ("change", "change", "change_group"),
            ("delete", "delete", "delete_group"),
        ],
        "source": "host",
    },
]


class PermissionCodenameMapper:
    def to_public(self, django_perm):
        public = django_perm
        if django_perm.startswith("users."):
            host_codename = django_perm[len("users.") :]
            mapped = HOST_CODENAME_TO_PUBLIC.get(host_codename)
            if mapped:
                public = mapped
        return public

    def map_many(self, django_perms):
        return sorted({self.to_public(item) for item in django_perms})
