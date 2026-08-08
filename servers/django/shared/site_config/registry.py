class SiteConfigSlice:
    def __init__(self, name, export_fn, import_fn):
        self.name = name
        self.export_fn = export_fn
        self.import_fn = import_fn


class SiteConfigRegistry:
    def __init__(self):
        self.slices = []

    def register(self, name, export_fn, import_fn):
        for item in self.slices:
            if item.name == name:
                item.export_fn = export_fn
                item.import_fn = import_fn
                return
        self.slices.append(SiteConfigSlice(name, export_fn, import_fn))

    def export_all(self):
        return {item.name: item.export_fn() for item in self.slices}

    def import_all(self, payload_by_name):
        for item in self.slices:
            if item.name in payload_by_name:
                item.import_fn(payload_by_name[item.name])


site_config_registry = SiteConfigRegistry()
