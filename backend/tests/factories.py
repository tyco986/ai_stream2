import factory
from django.utils.timezone import now

from apps.accounts.models import Organization, User
from apps.alerts.models import Alert, AlertRule
from apps.cameras.models import AnalyticsZone, Camera, CameraGroup
from apps.detections.models import Detection, KafkaDeadLetter
from apps.pipelines.models import AIModel, CameraModelBinding, PipelineProfile


class OrganizationFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Organization

    name = factory.Sequence(lambda n: f"Organization {n}")


class UserFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = User

    username = factory.Sequence(lambda n: f"user{n}")
    password = factory.PostGenerationMethodCall("set_password", "testpass123")
    organization = factory.SubFactory(OrganizationFactory)
    role = "operator"
    is_active = True


class CameraGroupFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = CameraGroup

    name = factory.Sequence(lambda n: f"Group {n}")
    organization = factory.SubFactory(OrganizationFactory)


class CameraFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Camera

    uid = factory.Sequence(lambda n: f"cam-{n:04d}")
    name = factory.Sequence(lambda n: f"Camera {n}")
    rtsp_url = factory.LazyAttribute(lambda o: f"rtsp://192.168.1.100:554/{o.uid}")
    organization = factory.SubFactory(OrganizationFactory)
    status = "offline"


class AnalyticsZoneFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = AnalyticsZone

    camera = factory.SubFactory(CameraFactory)
    name = factory.Sequence(lambda n: f"zone-{n}")
    zone_type = "roi"
    coordinates = factory.LazyFunction(lambda: [[100, 100], [500, 100], [500, 500], [100, 500]])
    config = factory.LazyFunction(dict)


class DetectionFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Detection

    camera = factory.SubFactory(CameraFactory)
    detected_at = factory.LazyFunction(now)
    object_count = 3
    detected_objects = factory.LazyFunction(
        lambda: [
            {"type": "person", "confidence": 0.95, "bbox": [100, 200, 300, 400], "object_id": 1},
            {"type": "person", "confidence": 0.88, "bbox": [400, 200, 600, 400], "object_id": 2},
            {"type": "car", "confidence": 0.92, "bbox": [700, 300, 900, 500], "object_id": 3},
        ]
    )


class AlertRuleFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = AlertRule

    name = factory.Sequence(lambda n: f"Rule {n}")
    organization = factory.SubFactory(OrganizationFactory)
    rule_type = "object_count"
    conditions = factory.LazyFunction(lambda: {"min_count": 5})
    cooldown_seconds = 60


class AlertFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = Alert

    rule = factory.SubFactory(AlertRuleFactory)
    camera = factory.SubFactory(CameraFactory)
    organization = factory.LazyAttribute(lambda o: o.rule.organization)
    triggered_at = factory.LazyFunction(now)
    status = "pending"
    snapshot = factory.LazyFunction(lambda: {"object_count": 5, "objects": []})


class AIModelFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = AIModel

    name = factory.Sequence(lambda n: f"model-{n}")
    organization = factory.SubFactory(OrganizationFactory)
    model_type = "detector"
    framework = "onnx"
    model_file = "/models/yolov8.onnx"
    config = factory.LazyFunction(lambda: {"num_classes": 80})


class PipelineProfileFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = PipelineProfile

    name = factory.Sequence(lambda n: f"Pipeline {n}")
    organization = factory.SubFactory(OrganizationFactory)
    detector = factory.SubFactory(
        AIModelFactory,
        organization=factory.SelfAttribute("..organization"),
        model_type="detector",
    )


class CameraModelBindingFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = CameraModelBinding

    camera = factory.SubFactory(CameraFactory)
    pipeline_profile = factory.SubFactory(PipelineProfileFactory)


class KafkaDeadLetterFactory(factory.django.DjangoModelFactory):
    class Meta:
        model = KafkaDeadLetter

    topic = "deepstream-detections"
    partition_num = 0
    offset = factory.Sequence(lambda n: n)
    raw_message = '{"bad": "json"}'
    error_message = "Missing key: sensorId"
