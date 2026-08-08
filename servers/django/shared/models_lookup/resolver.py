class ModelBuiltResolver:
    resolvers = []

    def register(self, resolver):
        key = getattr(resolver, "__qualname__", id(resolver))
        existing = {getattr(item, "__qualname__", id(item)) for item in self.resolvers}
        if key not in existing:
            self.resolvers.append(resolver)

    def resolve(self, model_id):
        """Return {id, name, status, built: bool} or None."""
        info = None
        for resolver in self.resolvers:
            info = resolver(model_id)
            if info:
                break
        return info

    def is_built(self, model_id):
        info = self.resolve(model_id)
        built = False
        if info is not None:
            built = bool(info.get("built"))
        return built


model_built_resolver = ModelBuiltResolver()
