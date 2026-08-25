class RtmposeSahiMixin:
    def cache_target(self):
        return "nvsahipostprocess"

    def rect_expand_target(self):
        target = "nvsahipostprocess"
        if self.has_tracker():
            target = "nvtracker"
        return target
