from pages.streams.models import Stream
from shared.streams_lookup import stream_name_resolver


class StreamsNameLookup:
    def resolve(self, stream_id):
        name = None
        row = Stream.objects.filter(pk=stream_id).only("name").first()
        if row is not None:
            name = row.name
        return name


def register_stream_name_resolver():
    stream_name_resolver.register(StreamsNameLookup().resolve)
