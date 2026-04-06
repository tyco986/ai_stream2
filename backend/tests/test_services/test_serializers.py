"""Serializer unit tests — validation, required fields, enums."""
import pytest
from rest_framework.test import APIRequestFactory

from apps.cameras.serializers import AnalyticsZoneSerializer, CameraWriteSerializer
from apps.pipelines.serializers import AIModelSerializer, PipelineProfileSerializer
from common.exceptions import ServiceError
from tests.factories import AIModelFactory, OrganizationFactory, UserFactory


@pytest.fixture
def org():
    return OrganizationFactory()


@pytest.fixture
def user(org):
    return UserFactory(organization=org, role="operator")


def _fake_request(user):
    factory = APIRequestFactory()
    request = factory.post("/fake/")
    request.user = user
    return request


@pytest.mark.django_db
class TestCameraWriteSerializer:
    def test_valid_data(self, user):
        data = {"uid": "cam-new", "name": "New Camera", "rtsp_url": "rtsp://1.2.3.4/stream"}
        serializer = CameraWriteSerializer(data=data, context={"request": _fake_request(user)})
        assert serializer.is_valid(), serializer.errors

    def test_missing_name_fails(self, user):
        data = {"uid": "cam-new", "rtsp_url": "rtsp://1.2.3.4/stream"}
        serializer = CameraWriteSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "name" in serializer.errors

    def test_missing_rtsp_url_fails(self, user):
        data = {"uid": "cam-new", "name": "New Camera"}
        serializer = CameraWriteSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "rtsp_url" in serializer.errors


@pytest.mark.django_db
class TestAnalyticsZoneSerializer:
    def test_coordinates_out_of_range(self):
        data = {
            "name": "zone-1",
            "zone_type": "roi",
            "coordinates": [[2000, 100], [500, 100]],
            "config": {},
        }
        serializer = AnalyticsZoneSerializer(data=data)
        assert not serializer.is_valid()
        assert "coordinates" in serializer.errors

    def test_overcrowding_requires_object_threshold(self):
        data = {
            "name": "crowd-zone",
            "zone_type": "overcrowding",
            "coordinates": [[100, 100], [500, 100], [500, 500]],
            "config": {},
        }
        serializer = AnalyticsZoneSerializer(data=data)
        assert not serializer.is_valid()
        assert "config" in str(serializer.errors)

    def test_overcrowding_with_threshold_passes(self):
        data = {
            "name": "crowd-zone",
            "zone_type": "overcrowding",
            "coordinates": [[100, 100], [500, 100], [500, 500]],
            "config": {"object_threshold": 10},
        }
        serializer = AnalyticsZoneSerializer(data=data)
        assert serializer.is_valid(), serializer.errors

    def test_too_few_coordinates(self):
        data = {
            "name": "zone-1",
            "zone_type": "roi",
            "coordinates": [[100, 100]],
        }
        serializer = AnalyticsZoneSerializer(data=data)
        assert not serializer.is_valid()
        assert "coordinates" in serializer.errors


@pytest.mark.django_db
class TestAIModelSerializer:
    def test_detector_requires_num_classes(self, user):
        data = {
            "name": "bad-model",
            "model_type": "detector",
            "model_file": "/models/bad.onnx",
            "config": {},
        }
        serializer = AIModelSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "config" in str(serializer.errors)

    def test_tracker_requires_tracker_type(self, user):
        data = {
            "name": "bad-tracker",
            "model_type": "tracker",
            "model_file": "/models/tracker.onnx",
            "config": {},
        }
        serializer = AIModelSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "config" in str(serializer.errors)

    def test_valid_detector(self, user):
        data = {
            "name": "good-model",
            "model_type": "detector",
            "model_file": "/models/good.onnx",
            "config": {"num_classes": 80},
        }
        serializer = AIModelSerializer(data=data, context={"request": _fake_request(user)})
        assert serializer.is_valid(), serializer.errors


@pytest.mark.django_db
class TestPipelineProfileSerializer:
    def test_detector_must_be_detector_type(self, user):
        tracker_model = AIModelFactory(
            organization=user.organization, model_type="tracker",
            config={"tracker_type": "NvDCF"},
        )
        data = {
            "name": "bad-pipeline",
            "detector": str(tracker_model.id),
        }
        serializer = PipelineProfileSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "detector" in serializer.errors

    def test_cross_org_model_rejected(self, user):
        other_org = OrganizationFactory()
        other_model = AIModelFactory(
            organization=other_org, model_type="detector",
            config={"num_classes": 80},
        )
        data = {
            "name": "cross-org-pipeline",
            "detector": str(other_model.id),
        }
        serializer = PipelineProfileSerializer(data=data, context={"request": _fake_request(user)})
        assert not serializer.is_valid()
        assert "detector" in serializer.errors


@pytest.mark.django_db
class TestServiceError:
    def test_code_and_status_independent(self):
        err = ServiceError("not found", code="CAMERA_NOT_FOUND", http_status=404)
        assert err.code == "CAMERA_NOT_FOUND"
        assert err.http_status == 404
        assert err.message == "not found"
