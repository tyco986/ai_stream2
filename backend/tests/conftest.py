import pytest
from django.test import override_settings
from rest_framework.test import APIClient
from rest_framework_simplejwt.tokens import RefreshToken

from tests.factories import CameraFactory, OrganizationFactory, UserFactory

IN_MEMORY_CHANNEL_LAYER = {
    "default": {
        "BACKEND": "channels.layers.InMemoryChannelLayer",
    },
}


@pytest.fixture(autouse=True)
def _use_in_memory_channel_layer(settings):
    settings.CHANNEL_LAYERS = IN_MEMORY_CHANNEL_LAYER


@pytest.fixture
def org():
    return OrganizationFactory()


@pytest.fixture
def org_b():
    """Second organization for multi-tenancy isolation tests."""
    return OrganizationFactory(name="Org B")


@pytest.fixture
def admin_user(org):
    return UserFactory(organization=org, role="admin")


@pytest.fixture
def operator_user(org):
    return UserFactory(organization=org, role="operator")


@pytest.fixture
def viewer_user(org):
    return UserFactory(organization=org, role="viewer")


@pytest.fixture
def org_b_user(org_b):
    return UserFactory(organization=org_b, role="operator")


@pytest.fixture
def api_client():
    return APIClient()


@pytest.fixture
def admin_client(admin_user):
    client = APIClient()
    token = RefreshToken.for_user(admin_user)
    client.credentials(HTTP_AUTHORIZATION=f"Bearer {token.access_token}")
    return client


@pytest.fixture
def operator_client(operator_user):
    client = APIClient()
    token = RefreshToken.for_user(operator_user)
    client.credentials(HTTP_AUTHORIZATION=f"Bearer {token.access_token}")
    return client


@pytest.fixture
def viewer_client(viewer_user):
    client = APIClient()
    token = RefreshToken.for_user(viewer_user)
    client.credentials(HTTP_AUTHORIZATION=f"Bearer {token.access_token}")
    return client


@pytest.fixture
def org_b_client(org_b_user):
    client = APIClient()
    token = RefreshToken.for_user(org_b_user)
    client.credentials(HTTP_AUTHORIZATION=f"Bearer {token.access_token}")
    return client


@pytest.fixture
def camera(org):
    return CameraFactory(organization=org)


@pytest.fixture
def camera_b(org_b):
    """Camera belonging to Org B for isolation tests."""
    return CameraFactory(organization=org_b, uid="cam-orgb-001")
