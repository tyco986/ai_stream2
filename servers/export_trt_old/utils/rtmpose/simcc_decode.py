from onnx import TensorProto, helper

from utils.rtmpose.utils.constants import (
    RTMPOSE_BACKBONE_OUTPUT_NAMES,
    RTMPOSE_OUTPUT_NAME,
)


class SimccDecodeGraft:
    """Append SimCC ArgMax decode so the graph outputs keypoints [B, K, 3].

    Score matches MMPose get_simcc_maximum: min of raw SimCC maxima, ReLU clip.
    """

    def __init__(self, model, num_keypoints):
        self.model = model
        self.num_keypoints = num_keypoints
        input_dims = model.graph.input[0].type.tensor_type.shape.dim
        self.input_height = int(input_dims[2].dim_value)
        self.input_width = int(input_dims[3].dim_value)
        outputs = {item.name: item for item in model.graph.output}
        length_x = int(outputs["simcc_x"].type.tensor_type.shape.dim[2].dim_value)
        length_y = int(outputs["simcc_y"].type.tensor_type.shape.dim[2].dim_value)
        self.scale_x = self.input_width / float(length_x)
        self.scale_y = self.input_height / float(length_y)

    def tensor_name(self, suffix):
        name = f"kpt/{suffix}"
        return name

    def append_constant(self, suffix, data_type, dims, values):
        name = self.tensor_name(f"cst/{suffix}")
        tensor = helper.make_tensor(name, data_type, dims, values)
        self.model.graph.initializer.append(tensor)
        return name

    def append_node(self, op_type, inputs, outputs, name, **attributes):
        node = helper.make_node(op_type, inputs, outputs, name=name, **attributes)
        self.model.graph.node.append(node)

    def decode_coord(self, logits_name, scale, suffix):
        argmax_name = self.tensor_name(f"argmax_{suffix}")
        cast_name = self.tensor_name(f"idx_{suffix}")
        coord_name = self.tensor_name(f"coord_{suffix}")
        scale_name = self.append_constant(
            f"scale_{suffix}", TensorProto.FLOAT, [], [scale]
        )
        self.append_node(
            "ArgMax",
            [logits_name],
            [argmax_name],
            self.tensor_name(f"argmax_{suffix}_op"),
            axis=2,
            keepdims=1,
        )
        self.append_node(
            "Cast",
            [argmax_name],
            [cast_name],
            self.tensor_name(f"cast_{suffix}"),
            to=TensorProto.FLOAT,
        )
        self.append_node(
            "Mul",
            [cast_name, scale_name],
            [coord_name],
            self.tensor_name(f"mul_{suffix}"),
        )
        return coord_name

    def reduce_max(self, logits_name, suffix):
        max_name = self.tensor_name(f"max_{suffix}")
        self.append_node(
            "ReduceMax",
            [logits_name],
            [max_name],
            self.tensor_name(f"reducemax_{suffix}"),
            axes=[2],
            keepdims=1,
        )
        return max_name

    def decode_score(self, simcc_x, simcc_y):
        # MMPose get_simcc_maximum: min(raw max_x, raw max_y), clip negatives.
        min_name = self.tensor_name("score_min")
        score_name = self.tensor_name("score")
        self.append_node(
            "Min",
            [self.reduce_max(simcc_x, "x"), self.reduce_max(simcc_y, "y")],
            [min_name],
            self.tensor_name("min_score"),
        )
        self.append_node(
            "Relu",
            [min_name],
            [score_name],
            self.tensor_name("relu_score"),
        )
        return score_name

    def replace_outputs(self):
        batch_dim = self.model.graph.input[0].type.tensor_type.shape.dim[0]
        batch_shape = batch_dim.dim_param or int(batch_dim.dim_value)
        output_info = helper.make_tensor_value_info(
            RTMPOSE_OUTPUT_NAME,
            TensorProto.FLOAT,
            [batch_shape, self.num_keypoints, 3],
        )
        del self.model.graph.output[:]
        self.model.graph.output.append(output_info)

    def graft(self):
        simcc_x, simcc_y = RTMPOSE_BACKBONE_OUTPUT_NAMES
        coord_x = self.decode_coord(simcc_x, self.scale_x, "x")
        coord_y = self.decode_coord(simcc_y, self.scale_y, "y")
        score = self.decode_score(simcc_x, simcc_y)
        self.append_node(
            "Concat",
            [coord_x, coord_y, score],
            [RTMPOSE_OUTPUT_NAME],
            self.tensor_name("concat_keypoints"),
            axis=2,
        )
        self.replace_outputs()
        return self.model
