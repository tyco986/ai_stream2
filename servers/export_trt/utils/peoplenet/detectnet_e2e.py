from onnx import TensorProto, helper

from utils.peoplenet.utils.constants import (
    PEOPLENET_BBOX_NORM,
    PEOPLENET_CLASSES,
    PEOPLENET_E2E_OUTPUT_NAME,
    PEOPLENET_GRID_HW,
    PEOPLENET_GRID_OFFSET,
    PEOPLENET_INPUT_CHW,
    PEOPLENET_OUTPUT_NAMES,
    PEOPLENET_STRIDE,
)


class DetectNetE2EGraft:
    """Append DetectNet decode + EfficientNMS_TRT + packed output0 to PeopleNet ONNX."""

    def __init__(self, model, conf, iou, max_det):
        self.model = model
        self.conf = conf
        self.iou = iou
        self.max_det = max_det
        self.grid_h, self.grid_w = PEOPLENET_GRID_HW
        self.num_classes = len(PEOPLENET_CLASSES)
        self.num_cells = self.grid_h * self.grid_w
        self.num_boxes = self.num_classes * self.num_cells

    def tensor_name(self, suffix):
        name = f"e2e/{suffix}"
        return name

    def append_constant(self, suffix, data_type, dims, values):
        name = self.tensor_name(f"cst/{suffix}")
        tensor = helper.make_tensor(name, data_type, dims, values)
        self.model.graph.initializer.append(tensor)
        return name

    def append_node(self, op_type, inputs, outputs, name, domain="", **attributes):
        node = helper.make_node(op_type, inputs, outputs, name=name, **attributes)
        if domain:
            node.domain = domain
        self.model.graph.node.append(node)

    def decode_boxes(self, bbox_name):
        boxes_name = self.tensor_name("boxes")
        bbox_shape = self.append_constant(
            "bbox_shape",
            TensorProto.INT64,
            [5],
            [0, self.num_classes, 4, self.grid_h, self.grid_w],
        )
        sl_axes = self.append_constant("sl_axes", TensorProto.INT64, [1], [2])
        sl_0 = self.append_constant("sl_0", TensorProto.INT64, [1], [0])
        sl_1 = self.append_constant("sl_1", TensorProto.INT64, [1], [1])
        sl_2 = self.append_constant("sl_2", TensorProto.INT64, [1], [2])
        sl_3 = self.append_constant("sl_3", TensorProto.INT64, [1], [3])
        sl_4 = self.append_constant("sl_4", TensorProto.INT64, [1], [4])
        grid_x = self.append_constant(
            "grid_x",
            TensorProto.FLOAT,
            [1, 1, 1, self.grid_w],
            [i * PEOPLENET_STRIDE + PEOPLENET_GRID_OFFSET for i in range(self.grid_w)],
        )
        grid_y = self.append_constant(
            "grid_y",
            TensorProto.FLOAT,
            [1, 1, self.grid_h, 1],
            [i * PEOPLENET_STRIDE + PEOPLENET_GRID_OFFSET for i in range(self.grid_h)],
        )
        bbox_norm = self.append_constant(
            "bbox_norm", TensorProto.FLOAT, [], [PEOPLENET_BBOX_NORM]
        )
        zero = self.append_constant("zero", TensorProto.FLOAT, [], [0.0])
        net_w = self.append_constant(
            "net_w", TensorProto.FLOAT, [], [float(PEOPLENET_INPUT_CHW[2])]
        )
        net_h = self.append_constant(
            "net_h", TensorProto.FLOAT, [], [float(PEOPLENET_INPUT_CHW[1])]
        )
        boxes_shape = self.append_constant(
            "boxes_shape", TensorProto.INT64, [3], [0, self.num_boxes, 4]
        )
        self.append_node(
            "Reshape",
            [bbox_name, bbox_shape],
            [self.tensor_name("bbox_nchw4")],
            self.tensor_name("reshape_bbox"),
        )
        self.append_node(
            "Slice",
            [self.tensor_name("bbox_nchw4"), sl_0, sl_1, sl_axes],
            [self.tensor_name("x1_off_5d")],
            self.tensor_name("slice_x1"),
        )
        self.append_node(
            "Slice",
            [self.tensor_name("bbox_nchw4"), sl_1, sl_2, sl_axes],
            [self.tensor_name("y1_off_5d")],
            self.tensor_name("slice_y1"),
        )
        self.append_node(
            "Slice",
            [self.tensor_name("bbox_nchw4"), sl_2, sl_3, sl_axes],
            [self.tensor_name("x2_off_5d")],
            self.tensor_name("slice_x2"),
        )
        self.append_node(
            "Slice",
            [self.tensor_name("bbox_nchw4"), sl_3, sl_4, sl_axes],
            [self.tensor_name("y2_off_5d")],
            self.tensor_name("slice_y2"),
        )
        self.append_node(
            "Squeeze",
            [self.tensor_name("x1_off_5d")],
            [self.tensor_name("x1_off")],
            self.tensor_name("squeeze_x1"),
            axes=[2],
        )
        self.append_node(
            "Squeeze",
            [self.tensor_name("y1_off_5d")],
            [self.tensor_name("y1_off")],
            self.tensor_name("squeeze_y1"),
            axes=[2],
        )
        self.append_node(
            "Squeeze",
            [self.tensor_name("x2_off_5d")],
            [self.tensor_name("x2_off")],
            self.tensor_name("squeeze_x2"),
            axes=[2],
        )
        self.append_node(
            "Squeeze",
            [self.tensor_name("y2_off_5d")],
            [self.tensor_name("y2_off")],
            self.tensor_name("squeeze_y2"),
            axes=[2],
        )
        self.append_node(
            "Mul",
            [self.tensor_name("x1_off"), bbox_norm],
            [self.tensor_name("x1_off_n")],
            self.tensor_name("mul_x1"),
        )
        self.append_node(
            "Mul",
            [self.tensor_name("y1_off"), bbox_norm],
            [self.tensor_name("y1_off_n")],
            self.tensor_name("mul_y1"),
        )
        self.append_node(
            "Mul",
            [self.tensor_name("x2_off"), bbox_norm],
            [self.tensor_name("x2_off_n")],
            self.tensor_name("mul_x2"),
        )
        self.append_node(
            "Mul",
            [self.tensor_name("y2_off"), bbox_norm],
            [self.tensor_name("y2_off_n")],
            self.tensor_name("mul_y2"),
        )
        self.append_node(
            "Sub",
            [grid_x, self.tensor_name("x1_off_n")],
            [self.tensor_name("x1")],
            self.tensor_name("sub_x1"),
        )
        self.append_node(
            "Sub",
            [grid_y, self.tensor_name("y1_off_n")],
            [self.tensor_name("y1")],
            self.tensor_name("sub_y1"),
        )
        self.append_node(
            "Add",
            [grid_x, self.tensor_name("x2_off_n")],
            [self.tensor_name("x2")],
            self.tensor_name("add_x2"),
        )
        self.append_node(
            "Add",
            [grid_y, self.tensor_name("y2_off_n")],
            [self.tensor_name("y2")],
            self.tensor_name("add_y2"),
        )
        self.append_node(
            "Clip",
            [self.tensor_name("x1"), zero, net_w],
            [self.tensor_name("x1c")],
            self.tensor_name("clip_x1"),
        )
        self.append_node(
            "Clip",
            [self.tensor_name("y1"), zero, net_h],
            [self.tensor_name("y1c")],
            self.tensor_name("clip_y1"),
        )
        self.append_node(
            "Clip",
            [self.tensor_name("x2"), zero, net_w],
            [self.tensor_name("x2c")],
            self.tensor_name("clip_x2"),
        )
        self.append_node(
            "Clip",
            [self.tensor_name("y2"), zero, net_h],
            [self.tensor_name("y2c")],
            self.tensor_name("clip_y2"),
        )
        self.append_node(
            "Unsqueeze",
            [self.tensor_name("x1c")],
            [self.tensor_name("x1u")],
            self.tensor_name("unsq_x1"),
            axes=[-1],
        )
        self.append_node(
            "Unsqueeze",
            [self.tensor_name("y1c")],
            [self.tensor_name("y1u")],
            self.tensor_name("unsq_y1"),
            axes=[-1],
        )
        self.append_node(
            "Unsqueeze",
            [self.tensor_name("x2c")],
            [self.tensor_name("x2u")],
            self.tensor_name("unsq_x2"),
            axes=[-1],
        )
        self.append_node(
            "Unsqueeze",
            [self.tensor_name("y2c")],
            [self.tensor_name("y2u")],
            self.tensor_name("unsq_y2"),
            axes=[-1],
        )
        self.append_node(
            "Concat",
            [
                self.tensor_name("x1u"),
                self.tensor_name("y1u"),
                self.tensor_name("x2u"),
                self.tensor_name("y2u"),
            ],
            [self.tensor_name("boxes_5d")],
            self.tensor_name("concat_xyxy"),
            axis=-1,
        )
        self.append_node(
            "Reshape",
            [self.tensor_name("boxes_5d"), boxes_shape],
            [boxes_name],
            self.tensor_name("reshape_boxes"),
        )
        return boxes_name

    def decode_scores(self, cov_name):
        scores_name = self.tensor_name("scores")
        cov_shape = self.append_constant(
            "cov_shape",
            TensorProto.INT64,
            [3],
            [0, self.num_classes, self.num_cells],
        )
        eye = [float(i == j) for i in range(self.num_classes) for j in range(self.num_classes)]
        eye_name = self.append_constant(
            "eye", TensorProto.FLOAT, [1, self.num_classes, 1, self.num_classes], eye
        )
        scores_shape = self.append_constant(
            "scores_shape", TensorProto.INT64, [3], [0, self.num_boxes, self.num_classes]
        )
        self.append_node(
            "Reshape",
            [cov_name, cov_shape],
            [self.tensor_name("cov_flat")],
            self.tensor_name("reshape_cov"),
        )
        self.append_node(
            "Unsqueeze",
            [self.tensor_name("cov_flat")],
            [self.tensor_name("cov_u")],
            self.tensor_name("unsq_cov"),
            axes=[-1],
        )
        self.append_node(
            "Mul",
            [self.tensor_name("cov_u"), eye_name],
            [self.tensor_name("scores_4d")],
            self.tensor_name("mul_eye"),
        )
        self.append_node(
            "Reshape",
            [self.tensor_name("scores_4d"), scores_shape],
            [scores_name],
            self.tensor_name("reshape_scores"),
        )
        return scores_name

    def append_nms(self, boxes_name, scores_name):
        num_dets = self.tensor_name("num_dets")
        det_boxes = self.tensor_name("det_boxes")
        det_scores = self.tensor_name("det_scores")
        det_classes = self.tensor_name("det_classes")
        self.append_node(
            "EfficientNMS_TRT",
            [boxes_name, scores_name],
            [num_dets, det_boxes, det_scores, det_classes],
            self.tensor_name("EfficientNMS_TRT"),
            domain="TRT",
            background_class=-1,
            box_coding=0,
            class_agnostic=0,
            score_activation=0,
            max_output_boxes=self.max_det,
            score_threshold=self.conf,
            iou_threshold=self.iou,
        )
        return det_boxes, det_scores, det_classes

    def pack_output(self, det_boxes, det_scores, det_classes):
        self.append_node(
            "Unsqueeze",
            [det_scores],
            [self.tensor_name("det_scores_u")],
            self.tensor_name("unsq_scores"),
            axes=[-1],
        )
        self.append_node(
            "Unsqueeze",
            [det_classes],
            [self.tensor_name("det_classes_u")],
            self.tensor_name("unsq_classes"),
            axes=[-1],
        )
        self.append_node(
            "Cast",
            [self.tensor_name("det_classes_u")],
            [self.tensor_name("det_classes_f")],
            self.tensor_name("cast_classes"),
            to=TensorProto.FLOAT,
        )
        self.append_node(
            "Concat",
            [
                det_boxes,
                self.tensor_name("det_scores_u"),
                self.tensor_name("det_classes_f"),
            ],
            [PEOPLENET_E2E_OUTPUT_NAME],
            self.tensor_name("concat_output0"),
            axis=-1,
        )

    def replace_outputs(self):
        batch_dim = self.model.graph.input[0].type.tensor_type.shape.dim[0]
        batch_shape = batch_dim.dim_param or int(batch_dim.dim_value)
        output_info = helper.make_tensor_value_info(
            PEOPLENET_E2E_OUTPUT_NAME,
            TensorProto.FLOAT,
            [batch_shape, self.max_det, 6],
        )
        del self.model.graph.output[:]
        self.model.graph.output.append(output_info)
        has_trt = any(opset.domain == "TRT" for opset in self.model.opset_import)
        if not has_trt:
            self.model.opset_import.append(helper.make_opsetid("TRT", 1))

    def graft(self):
        cov_name, bbox_name = PEOPLENET_OUTPUT_NAMES
        boxes_name = self.decode_boxes(bbox_name)
        scores_name = self.decode_scores(cov_name)
        det_boxes, det_scores, det_classes = self.append_nms(boxes_name, scores_name)
        self.pack_output(det_boxes, det_scores, det_classes)
        self.replace_outputs()
        return self.model
