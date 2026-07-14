# NVIDIA TAO Model Zoo

来源：[TAO Model Zoo Overview](https://docs.nvidia.com/tao/tao-toolkit/latest/text/model_zoo/overview.html) · [NGC TAO Computer Vision](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/tao/collections/tao_computervision)（约 76 个模型条目）

TAO 分三类：**Foundation**（基础大模型）、**Purpose-built**（领域预训练）、**Pre-trained weights**（训练起点）。另可通过 **General-purpose** 自选骨干训练（100+ 架构组合）。

---

## Foundation Models

| 模型 | 任务 | TAO 微调 |
|------|------|----------|
| C-RADIOv2 | 视觉表征 / 下游适配 | ✅ |
| ConvNeXt v2 | 表征 / RT-DETR 等 | ✅ |
| NV-DINOv2 | 视觉基础模型 | ✅ |
| NV-CLIP | 图文对齐 | ❌ |
| RADIO-CLIP | 开放词汇检索 | ✅ |
| SigLIPv2 | 图文检索 | ✅ |
| Cosmos-Embed1 | 视频-文本嵌入 | ✅ |
| SegIC | In-context 分割 | ❌ |
| Grounding DINO | 开放词汇检测 | ✅ |
| Mask Grounding DINO | 开放词汇实例分割 | ✅ |
| Mask Auto Label | 自动标注 | ✅ |
| FoundationPose | 6DoF 物体位姿 | ❌ |
| ODISE | 开放词汇分割 | ✅（源码） |

---

## Purpose-built Models

| 模型 | 任务 | TAO 微调 |
|------|------|----------|
| PeopleNet | 人 / 包 / 脸检测 | ✅ |
| PeopleNet Transformer v2 | 人检测（Transformer） | ✅ |
| TrafficCamNet | 车辆检测 | ✅ |
| DashCamNet | 车载检测 | ✅ |
| FaceDetect / FaceDetect-IR | 人脸检测 | ✅ |
| Facial Landmarks | 人脸关键点 | ✅ |
| EmotionNet | 表情识别 | ✅ |
| GazeNet | 视线估计 | ✅ |
| GestureNet | 手势分类 | ✅ |
| HeartRateNet | 心率（rPPG） | ✅ |
| BodyPoseNet | 2D 多人姿态（18 关键点） | ✅ |
| BodyPose3DNet | 3D 人体姿态（34 关键点） | ❌ 仅部署 |
| PoseClassificationNet | 骨架动作分类（ST-GCN，时域） | ✅ |
| ActionRecognitionNet | RGB / 光流视频动作（时域） | ✅ |
| ReIdentificationNet | 行人 ReID | ✅ |
| ReIdentificationNet Transformer | ReID（Swin） | ✅ |
| PeopleSegNet | 人体实例分割 | ✅ |
| PeopleSemSegFormer | 人体语义分割 | ✅ |
| PeopleSemSegNet | 人体语义分割（UNet） | ✅ |
| CitySemSegFormer | 城市场景语义分割 | ✅ |
| BEVFusion | 3D 检测（点云 + RGB） | ✅ |
| PointPillarNet | 点云 3D 检测 | ✅ |
| NVPanoptix3D | 3D 全景重建 | ✅ |
| RT-DETR 2D Warehouse | 仓储 2D 检测 | ✅ |
| Retail Object Detection | 零售目标检测 | ✅ |
| Retail Object Recognition | 零售目标识别 | ✅ |
| CenterPose / CenterPose ROS | 零售物体 6D 姿态 | ✅ |
| Multi-class 3D CenterPose | 类别级 3D 姿态 | ✅ |
| OCDNet | 文字检测 | ✅ |
| OCRNet | 文字识别 | ✅ |
| LPDNet | 车牌检测 | ✅ |
| LPRNet | 车牌识别 | ✅ |
| VehicleMakeNet / VehicleTypeNet | 车型 / 品牌分类 | ✅ |
| Visual ChangeNet（分类 / 分割） | 变化检测 | ✅ |
| PCB Classification / Optical Inspection | PCB 缺陷 | ✅ |
| Mask2Former（预训练权重） | 实例分割起点 | 权重 |

---

## General-purpose（自选架构训练）

| 任务 | 可选架构 / 模型 |
|------|-----------------|
| 分类 | ResNet, EfficientNet, ViT, FAN, GCViT, FasterViT, Swin, NV-CLIP, C-RADIOv2, NvDINOv2 |
| 检测 | DetectNet_v2, YOLOv3/v4/tiny, Faster R-CNN, SSD, RetinaNet, DSSD, EfficientDet, DINO, Deformable DETR, RT-DETR, Grounding DINO |
| 实例 / 全景 / 语义分割 | Mask R-CNN, UNet, Mask2Former, SegFormer |
| 点云 | PointPillars |
| OCR | OCD + OCR（ResNet / FAN 骨干） |
| ReID / 度量学习 | ResNet, ViT, Swin, NvDINOv2 |
| 姿态动作 | ST-GCN（PoseClassificationNet） |

---

## 较新补充（TAO 6.25+ Release Notes）

| 模型 | 任务 |
|------|------|
| Sparse4D | 稀疏 4D 感知 |
| DepthNet | 深度估计 |
| OneFormer | 统一分割 |
| C-RADIOv3 | 基础模型 |

---

## Pre-trained Weights（训练起点，节选）

| 模型 | 说明 |
|------|------|
| Mask2Former | COCO 实例分割 |
| SegFormer | ImageNet / Cityscapes / NvImageNet 权重 |
| EfficientDet-TF2 | COCO / NvImageNet 骨干 |
| Deformable DETR / DINO | COCO 检测 |
| DINO + NvDINOv2 backbone | COCO 检测 |
| FAN / GCViT / FasterViT | ImageNet / NvImageNet 分类 |
| pretrained_imagenet_backbones / nvimagenet_backbones | 通用骨干 |
| pretrained_classification_tf2 / efficientdet_tf2 | TF2 分类 / 检测骨干 |
