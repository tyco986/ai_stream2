class JpegFilesinkMixin:
    def append_jpeg_filesink(self) -> None:
        self._append_node(
            "capsfilter",
            "capsfilter_jpeg",
            self._add_capsfilter(),
        )
        self._append_node("nvjpegenc", "nvjpegenc", self._add_nvjpegenc())
        self._append_node(
            "filesink",
            "filesink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link_jpeg_from(self, edges: dict, src: str, logger: str) -> None:
        edges[src] = "capsfilter_jpeg"
        edges["capsfilter_jpeg"] = logger
        edges[logger] = "nvjpegenc"
        edges["nvjpegenc"] = "filesink"
