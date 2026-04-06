from rest_framework.response import Response


def success_response(data=None, message="success", status=200):
    return Response(
        {"code": "OK", "message": message, "data": data},
        status=status,
    )


def created_response(data=None, message="created"):
    return success_response(data=data, message=message, status=201)
