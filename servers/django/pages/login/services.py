from django.contrib.auth import authenticate, get_user_model, login

from pages.shell.services import AuthTicketService
from shared.audit import AuditService
from shared.http.exceptions import AppError

ADMIN_LOGIN_ACTION = "admin_login"


class LoginService:
    def authenticate_and_login(self, request, username, password, new_password=None):
        cleaned_username = (username or "").strip()
        result = {}
        if self.is_jwt_token(password):
            self.login_with_ticket(
                request,
                cleaned_username,
                password,
                new_password,
            )
        else:
            result = self.login_with_password(
                request,
                cleaned_username,
                password,
                new_password,
            )
        return result

    def is_jwt_token(self, value):
        parts = (value or "").split(".")
        looks_like_jwt = (
            len(parts) == 3 and bool(parts[0]) and bool(parts[1]) and bool(parts[2])
        )
        return looks_like_jwt

    def login_with_password(self, request, username, password, new_password=None):
        user = authenticate(request, username=username, password=password)
        if user is None or not user.is_active:
            raise AppError("Invalid username or password", status_code=401)
        result = {}
        if user.must_change_password and not new_password:
            result = {"must_change_password": True}
        else:
            if user.must_change_password:
                self.apply_new_password(user, password, new_password)
            login(request, user)
            AuditService().record(user, "Login", f"Login {user.username}")
        return result

    def login_with_ticket(self, request, username, password, new_password=None):
        user_model = get_user_model()
        user = user_model.objects.filter(username=username).first()
        if user is None or not user.is_active or not user.is_superuser:
            raise AppError("Invalid username or password", status_code=401)
        tickets = AuthTicketService()
        ticket = tickets.validate(password, ADMIN_LOGIN_ACTION, pkg_hash=None)
        tickets.consume(ticket["jti"], ADMIN_LOGIN_ACTION)
        if new_password:
            cleaned = new_password.strip()
            if not cleaned:
                raise AppError("new_password is required", status_code=400)
            user.set_password(cleaned)
            user.must_change_password = False
            user.save(update_fields=["password", "must_change_password"])
        login(request, user)
        AuditService().record(user, "Login", f"Ticket login {user.username}")

    def apply_new_password(self, user, old_password, new_password):
        cleaned = (new_password or "").strip()
        if not cleaned:
            raise AppError("new_password is required", status_code=400)
        if cleaned == old_password:
            raise AppError("new_password must differ from password", status_code=400)
        user.set_password(cleaned)
        user.must_change_password = False
        user.save(update_fields=["password", "must_change_password"])
