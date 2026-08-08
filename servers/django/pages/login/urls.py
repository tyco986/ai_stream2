from django.urls import path

from pages.login.views import LoginView

urlpatterns = [
    path("", LoginView.as_view(), name="login"),
]
