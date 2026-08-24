from utils.tool.parser.rtmpose_parser import RtmposeParser
from utils.tool.preprocessor.rect_expander import RectExpander

INFER_HEIGHT = 256
INFER_WIDTH = 192
SGIE_UNIQUE_ID = 2
STGCNPP_UNIQUE_ID = 4
MISSING_ACTION = ["NA", 0.0, -1]
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


class StgcnppParser(RtmposeParser):
    def __init__(self):
        self.stgcnpp_unique_id = STGCNPP_UNIQUE_ID
        super().__init__(
            infer_height=INFER_HEIGHT,
            infer_width=INFER_WIDTH,
            sgie_unique_id=SGIE_UNIQUE_ID,
        )

    def parse_action_label(self, label) -> list:
        name, conf, class_id = list(MISSING_ACTION)
        parts = str(label).strip().split("|")
        if len(parts) == 2:
            parsed_id = int(parts[0])
            parsed_conf = round(float(parts[1]), 2)
            if 0 <= parsed_id < len(ACTION_LABELS):
                name = ACTION_LABELS[parsed_id]
                conf = parsed_conf
                class_id = parsed_id
        action = [name, conf, class_id]
        return action

    def decode_action(self, object_meta) -> list:
        action = list(MISSING_ACTION)
        for classifier_meta in object_meta.classifier_items:
            if int(classifier_meta.unique_component_id) != self.stgcnpp_unique_id:
                continue
            if int(classifier_meta.n_labels) <= 0:
                continue
            action = self.parse_action_label(classifier_meta.get_n_label(0))
        return action

    def parse_object(self, object_meta, keypoints, action) -> dict:
        item = RtmposeParser.parse_object(self, object_meta, keypoints)
        item["action"] = action
        return item

    def restore_object(self, object_meta, source_id, frame_number, object_index) -> dict:
        rect = object_meta.rect_params
        keypoints = self.decode_keypoints(
            object_meta,
            float(rect.left),
            float(rect.top),
            float(rect.width),
            float(rect.height),
        )
        action = self.decode_action(object_meta)
        RectExpander.restore(rect, source_id, frame_number, object_index)
        item = self.parse_object(object_meta, keypoints, action)
        return item
