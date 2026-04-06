"""Pipeline, AI model, and analytics zone API tests."""
import os
import tempfile
from unittest.mock import patch

import pytest

from tests.factories import (
    AIModelFactory,
    CameraFactory,
    CameraModelBindingFactory,
    OrganizationFactory,
    PipelineProfileFactory,
)


@pytest.mark.django_db
class TestAIModelCRUD:
    def test_create_detector_model(self, operator_client, org):
        resp = operator_client.post("/api/v1/ai-models/", {
            "name": "yolov8",
            "model_type": "detector",
            "model_file": "/models/yolov8.onnx",
            "config": {"num_classes": 80},
        }, format="json")
        assert resp.status_code == 201
        assert resp.data["model_type"] == "detector"

    def test_create_detector_without_num_classes_fails(self, operator_client, org):
        resp = operator_client.post("/api/v1/ai-models/", {
            "name": "bad",
            "model_type": "detector",
            "model_file": "/models/bad.onnx",
            "config": {},
        }, format="json")
        assert resp.status_code == 400

    def test_create_tracker_model(self, operator_client, org):
        resp = operator_client.post("/api/v1/ai-models/", {
            "name": "NvDCF",
            "model_type": "tracker",
            "model_file": "/models/tracker.onnx",
            "config": {"tracker_type": "NvDCF_perf"},
        }, format="json")
        assert resp.status_code == 201


@pytest.mark.django_db
class TestPipelineProfileCRUD:
    def test_create_pipeline(self, operator_client, org):
        detector = AIModelFactory(organization=org, model_type="detector")
        resp = operator_client.post("/api/v1/pipeline-profiles/", {
            "name": "Standard Pipeline",
            "detector": str(detector.id),
        }, format="json")
        assert resp.status_code == 201

    def test_cannot_use_tracker_as_detector(self, operator_client, org):
        tracker = AIModelFactory(
            organization=org, model_type="tracker",
            config={"tracker_type": "NvDCF"},
        )
        resp = operator_client.post("/api/v1/pipeline-profiles/", {
            "name": "Bad Pipeline",
            "detector": str(tracker.id),
        }, format="json")
        assert resp.status_code == 400

    def test_cannot_use_other_org_model(self, operator_client, org):
        other_org = OrganizationFactory()
        other_model = AIModelFactory(organization=other_org, model_type="detector")
        resp = operator_client.post("/api/v1/pipeline-profiles/", {
            "name": "Cross-org",
            "detector": str(other_model.id),
        }, format="json")
        assert resp.status_code == 400


@pytest.mark.django_db
class TestCameraPipelineBinding:
    def test_bind_and_get_pipeline(self, operator_client, org, camera):
        profile = PipelineProfileFactory(organization=org)
        resp = operator_client.put(
            f"/api/v1/cameras/{camera.id}/pipeline/",
            {"pipeline_profile_id": str(profile.id)},
            format="json",
        )
        assert resp.status_code == 200

        resp = operator_client.get(f"/api/v1/cameras/{camera.id}/pipeline/")
        assert resp.status_code == 200
        assert resp.data["data"]["pipeline_name"] == profile.name


@pytest.mark.django_db
class TestPipelineDeploy:
    def test_deploy_generates_config_files(self, operator_client, org, camera):
        profile = PipelineProfileFactory(organization=org)
        CameraModelBindingFactory(camera=camera, pipeline_profile=profile)

        with tempfile.TemporaryDirectory() as tmpdir:
            with patch("apps.pipelines.views.PipelineDeployer") as MockDeployer:
                from services.pipeline_deployer import PipelineDeployer
                real = PipelineDeployer(deploy_dir=tmpdir)
                MockDeployer.return_value = real

                resp = operator_client.post(
                    f"/api/v1/pipeline-profiles/{profile.id}/deploy/",
                )
                assert resp.status_code == 200
                assert os.path.exists(os.path.join(tmpdir, "pgie_config.txt"))


@pytest.mark.django_db
class TestAnalyticsZoneCRUD:
    def test_create_roi_zone(self, operator_client, camera):
        resp = operator_client.post(
            f"/api/v1/cameras/{camera.id}/analytics-zones/",
            {
                "name": "entrance",
                "zone_type": "roi",
                "coordinates": [[100, 100], [500, 100], [500, 500], [100, 500]],
                "config": {},
            },
            format="json",
        )
        assert resp.status_code == 201

    def test_reject_out_of_range_coordinates(self, operator_client, camera):
        resp = operator_client.post(
            f"/api/v1/cameras/{camera.id}/analytics-zones/",
            {
                "name": "bad-zone",
                "zone_type": "roi",
                "coordinates": [[2000, 100], [500, 100]],
                "config": {},
            },
            format="json",
        )
        assert resp.status_code == 400

    def test_overcrowding_without_threshold_rejected(self, operator_client, camera):
        resp = operator_client.post(
            f"/api/v1/cameras/{camera.id}/analytics-zones/",
            {
                "name": "crowd",
                "zone_type": "overcrowding",
                "coordinates": [[100, 100], [500, 100], [500, 500]],
                "config": {},
            },
            format="json",
        )
        assert resp.status_code == 400
