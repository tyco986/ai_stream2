class StreamNameResolver:
    resolvers = []

    def register(self, resolver):
        key = getattr(resolver, "__qualname__", id(resolver))
        existing = {getattr(item, "__qualname__", id(item)) for item in self.resolvers}
        if key not in existing:
            self.resolvers.append(resolver)

    def resolve(self, stream_id):
        name = None
        for resolver in self.resolvers:
            name = resolver(stream_id)
            if name:
                break
        return name


stream_name_resolver = StreamNameResolver()
