from rest_framework.response import Response


def api_success(data=None, message="", status=200):
    payload = data if data is not None else {}
    return Response(
        {"success": True, "message": message, "data": payload},
        status=status,
    )


def api_error(message, status=400, data=None):
    payload = data if data is not None else {}
    return Response(
        {"success": False, "message": message, "data": payload},
        status=status,
    )
