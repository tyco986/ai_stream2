import cupy

from utils.probe.utils.drawer.pose2d_drawer import KEYPOINTS, Pose2DDrawer
from utils.probe.utils.preprocessor.rect_expander import RectExpander

INFER_HEIGHT = 256
INFER_WIDTH = 192
SGIE_UNIQUE_ID = 2
STGCNPP_UNIQUE_ID = 4
KPT_THRESHOLD = 0.0
CLASSIFIER_THRESHOLD = 0.51
ACTION_LAYER = "output"
FADE_INTERVAL = 50
FADE_TIME = 1
EMPTY_ACTION = [0.0, -1, ""]
ACTION_LABELS = (
    "drink water",
    "eat meal",
    "brush teeth",
    "brush hair",
    "drop",
    "pick up",
    "throw",
    "sit down",
    "stand up",
    "clapping",
    "reading",
    "writing",
    "tear up paper",
    "wear jacket",
    "take off jacket",
    "wear a shoe",
    "take off a shoe",
    "wear glasses",
    "take off glasses",
    "put on a hat/cap",
    "take off a hat/cap",
    "cheer up",
    "hand waving",
    "kicking something",
    "reach into pocket",
    "hopping",
    "jump up",
    "make a phone call",
    "playing with phone/tablet",
    "typing on a keyboard",
    "pointing to something",
    "taking a selfie",
    "check time",
    "rub two hands together",
    "nod head/bow",
    "shake head",
    "wipe face",
    "salute",
    "put the palms together",
    "cross hands in front",
    "sneeze/cough",
    "staggering",
    "falling",
    "touch head",
    "touch chest",
    "touch back",
    "touch neck",
    "nausea or vomiting",
    "use a fan",
    "punching/slapping other person",
    "kicking other person",
    "pushing other person",
    "pat on back of other person",
    "point finger at the other person",
    "hugging other person",
    "giving something to other person",
    "touch other person's pocket",
    "handshaking",
    "walking towards each other",
    "walking apart from each other",
)


class StgcnppDrawer(Pose2DDrawer):
    def __init__(self, show_pose=True):
        self.show_pose = show_pose
        super().__init__(
            show_label=True,
            show_conf=False,
            show_id=False,
            kpt_threshold=KPT_THRESHOLD,
            infer_height=INFER_HEIGHT,
            infer_width=INFER_WIDTH,
            sgie_unique_id=SGIE_UNIQUE_ID,
        )
        self.stgcnpp_unique_id = STGCNPP_UNIQUE_ID
        self.classifier_threshold = CLASSIFIER_THRESHOLD

    def object_outputs(self, object_meta):
        pose_layers = None
        action_layers = None
        for user_meta in object_meta.tensor_items:
            tensor_meta = user_meta.as_tensor_output()
            unique_id = int(tensor_meta.unique_id)
            if unique_id == self.sgie_unique_id:
                pose_layers = tensor_meta.get_layers()
            if unique_id == self.stgcnpp_unique_id:
                action_layers = tensor_meta.get_layers()
        return pose_layers, action_layers

    def decode_action(self, layers) -> list:
        action = list(EMPTY_ACTION)
        if layers is not None:
            scores = cupy.asnumpy(self.layer_array(layers, ACTION_LAYER)).reshape(-1)
            class_id = int(scores.argmax())
            conf = round(float(scores[class_id]), 2)
            if conf >= self.classifier_threshold and 0 <= class_id < len(ACTION_LABELS):
                action = [conf, class_id, ACTION_LABELS[class_id]]
        return action

    def parse_object(self, object_meta, keypoints, action) -> dict:
        item = Pose2DDrawer.parse_object(self, object_meta, keypoints)
        item["action"] = action
        return item

    def restore_object(self, object_meta, source_id, frame_number, object_index) -> dict:
        rect = object_meta.rect_params
        pose_layers, action_layers = self.object_outputs(object_meta)
        keypoints = []
        if pose_layers is not None:
            keypoints = self.map_to_frame(
                self.layer_array(pose_layers, KEYPOINTS),
                float(rect.left),
                float(rect.top),
                float(rect.width),
                float(rect.height),
            )
        action = self.decode_action(action_layers)
        RectExpander.restore(rect, source_id, frame_number, object_index)
        item = self.parse_object(object_meta, keypoints, action)
        return item

    def build_display_text(self, item) -> str:
        action = item["action"]
        display_text = f"{action[2]}|{action[0]:.2f}|{item['object'][7]}"
        return display_text

    def draw_pose(self, batch_meta, frame_meta, item) -> None:
        if self.show_pose:
            Pose2DDrawer.draw_pose(self, batch_meta, frame_meta, item)
