import yaml
from fastapi import UploadFile

from utils.api.schemas import ApiEnvelope, AppError, TrtTestConfig
from utils.tester.rtmpose.constants import DEFAULT_POSE_THRESHOLD
from utils.tester.rtmpose.rtmpose_image_tester import RtmposeImageTester
from utils.tester.rtmpose.rtmpose_video_tester import RtmposeVideoTester


class TrtTesterManager:
    TESTERS = {
        "RTMPOSE-IMAGE": RtmposeImageTester,
        "RTMPOSE-VIDEO": RtmposeVideoTester,
    }

    @classmethod
    def types(cls) -> list[str]:
        names = list(cls.TESTERS)
        return names

    def test(self, config: UploadFile) -> ApiEnvelope:
        payload = yaml.safe_load(config.file.read())
        if not isinstance(payload, dict):
            raise AppError("config YAML must be a mapping")
        cfg = TrtTestConfig.model_validate(payload)
        tester_cls = self.TESTERS.get(cfg.type)
        if tester_cls is None:
            raise AppError(f"unknown type {cfg.type!r}")
        out = tester_cls(cfg.engine, DEFAULT_POSE_THRESHOLD)(cfg.input)
        envelope = ApiEnvelope.ok(data=str(out))
        return envelope
