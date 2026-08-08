from shared.http.exceptions import AppError


class PaginationService:
    DEFAULT_PAGE = 1
    DEFAULT_PAGE_SIZE = 20
    MAX_PAGE_SIZE = 200

    def parse(self, query_params):
        page = self.parse_int(query_params.get("page"), self.DEFAULT_PAGE, "page")
        page_size = self.parse_int(
            query_params.get("page_size"),
            self.DEFAULT_PAGE_SIZE,
            "page_size",
        )
        if page < 1:
            raise AppError("page must be >= 1", status_code=400)
        if page_size < 1:
            raise AppError("page_size must be >= 1", status_code=400)
        if page_size > self.MAX_PAGE_SIZE:
            raise AppError(
                f"page_size must be <= {self.MAX_PAGE_SIZE}",
                status_code=400,
            )
        return {"page": page, "page_size": page_size}

    def slice_queryset(self, queryset, page, page_size):
        total = queryset.count()
        start = (page - 1) * page_size
        end = start + page_size
        items = list(queryset[start:end])
        return {
            "items": items,
            "total": total,
            "page": page,
            "page_size": page_size,
        }

    def build(self, items, total, page, page_size):
        return {
            "items": items,
            "total": total,
            "page": page,
            "page_size": page_size,
        }

    def parse_int(self, raw, default, field_name):
        value = default
        if raw is not None and raw != "":
            try:
                value = int(raw)
            except (TypeError, ValueError) as exc:
                raise AppError(f"Invalid {field_name}", status_code=400) from exc
        return value
