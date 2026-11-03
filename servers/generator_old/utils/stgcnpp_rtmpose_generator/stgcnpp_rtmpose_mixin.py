from ..topdown_pose_generator.topdown_pose_mixin import TopdownPoseMixin
from .stgcnpp_core_mixin import StgcnppCoreMixin


class StgcnppRtmposeMixin(StgcnppCoreMixin, TopdownPoseMixin):
    def init_sgie(self) -> None:
        super().init_sgie()
        self.init_stgcnpp()
