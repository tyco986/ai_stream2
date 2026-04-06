from rest_framework.pagination import CursorPagination, PageNumberPagination


class StandardPagination(PageNumberPagination):
    page_size = 20
    page_size_query_param = "page_size"
    max_page_size = 100


class DetectionCursorPagination(CursorPagination):
    """Cursor-based pagination for Detection — avoids deep-offset perf issues."""

    page_size = 50
    ordering = "-detected_at"
    page_size_query_param = "page_size"
    max_page_size = 200
