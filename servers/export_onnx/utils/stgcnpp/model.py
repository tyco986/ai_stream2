import torch
import torch.nn as nn

EPS = 1e-4
COCO_INWARD = [
    (15, 13),
    (13, 11),
    (16, 14),
    (14, 12),
    (11, 5),
    (12, 6),
    (9, 7),
    (7, 5),
    (10, 8),
    (8, 6),
    (5, 0),
    (6, 0),
    (1, 0),
    (3, 1),
    (2, 0),
    (4, 2),
]
NTU60_LABELS = [
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
]


def coco_spatial_adjacency(num_node=17):
    identity = torch.eye(num_node)
    inward = torch.zeros(num_node, num_node)
    for src, dst in COCO_INWARD:
        inward[dst, src] = 1
    outward = inward.transpose(0, 1).contiguous()
    adjacency = torch.stack((identity, normalize_digraph(inward), normalize_digraph(outward)))
    return adjacency


def normalize_digraph(adjacency):
    degree = adjacency.sum(dim=0)
    scale = torch.zeros_like(adjacency)
    nonzero = degree > 0
    scale[nonzero, nonzero] = degree[nonzero].pow(-1)
    normalized = adjacency @ scale
    return normalized


class UnitTcn(nn.Module):
    def __init__(self, in_channels, out_channels, kernel_size=9, stride=1, dilation=1, norm=True):
        super().__init__()
        self.in_channels = in_channels
        self.out_channels = out_channels
        pad = (kernel_size + (kernel_size - 1) * (dilation - 1) - 1) // 2
        self.conv = nn.Conv2d(
            in_channels,
            out_channels,
            kernel_size=(kernel_size, 1),
            padding=(pad, 0),
            stride=(stride, 1),
            dilation=(dilation, 1),
        )
        self.bn = nn.BatchNorm2d(out_channels) if norm else nn.Identity()

    def forward(self, x):
        y = self.bn(self.conv(x))
        return y


class UnitGcn(nn.Module):
    def __init__(self, in_channels, out_channels, adjacency, adaptive="init", with_res=False):
        super().__init__()
        self.in_channels = in_channels
        self.out_channels = out_channels
        self.num_subsets = adjacency.size(0)
        self.adaptive = adaptive
        self.with_res = with_res
        self.bn = nn.BatchNorm2d(out_channels)
        self.act = nn.ReLU()
        self.A = nn.Parameter(adjacency.clone())
        self.conv = nn.Conv2d(in_channels, out_channels * adjacency.size(0), 1)
        self.down = nn.Identity()
        if with_res and in_channels != out_channels:
            self.down = nn.Sequential(
                nn.Conv2d(in_channels, out_channels, 1),
                nn.BatchNorm2d(out_channels),
            )

    def forward(self, x):
        n, _, t, v = x.shape
        y = self.conv(x)
        y = y.view(n, self.num_subsets, -1, t, v)
        y = torch.einsum("nkctv,kvw->nctw", y, self.A).contiguous()
        y = self.bn(y)
        if self.with_res:
            y = y + self.down(x)
        y = self.act(y)
        return y


class MsTcn(nn.Module):
    def __init__(
        self,
        in_channels,
        out_channels,
        stride=1,
        ms_cfg=((3, 1), (3, 2), (3, 3), (3, 4), ("max", 3), "1x1"),
    ):
        super().__init__()
        self.ms_cfg = ms_cfg
        num_branches = len(ms_cfg)
        mid_channels = out_channels // num_branches
        rem_mid_channels = out_channels - mid_channels * (num_branches - 1)
        self.act = nn.ReLU()
        branches = []
        for i, cfg in enumerate(ms_cfg):
            branch_c = rem_mid_channels if i == 0 else mid_channels
            branch = self.build_branch(in_channels, branch_c, cfg, stride)
            branches.append(branch)
        self.branches = nn.ModuleList(branches)
        tin_channels = mid_channels * (num_branches - 1) + rem_mid_channels
        self.transform = nn.Sequential(
            nn.BatchNorm2d(tin_channels),
            self.act,
            nn.Conv2d(tin_channels, out_channels, kernel_size=1),
        )
        self.bn = nn.BatchNorm2d(out_channels)

    def build_branch(self, in_channels, branch_c, cfg, stride):
        branch = nn.Conv2d(in_channels, branch_c, kernel_size=1, stride=(stride, 1))
        if cfg == "1x1":
            branch = branch
        elif isinstance(cfg, tuple) and cfg[0] == "max":
            branch = nn.Sequential(
                nn.Conv2d(in_channels, branch_c, kernel_size=1),
                nn.BatchNorm2d(branch_c),
                self.act,
                nn.MaxPool2d(kernel_size=(cfg[1], 1), stride=(stride, 1), padding=(1, 0)),
            )
        else:
            branch = nn.Sequential(
                nn.Conv2d(in_channels, branch_c, kernel_size=1),
                nn.BatchNorm2d(branch_c),
                self.act,
                UnitTcn(branch_c, branch_c, kernel_size=cfg[0], stride=stride, dilation=cfg[1], norm=False),
            )
        return branch

    def forward(self, x):
        branch_outs = [branch(x) for branch in self.branches]
        feat = torch.cat(branch_outs, dim=1)
        feat = self.transform(feat)
        feat = self.bn(feat)
        return feat


class StgcnBlock(nn.Module):
    def __init__(self, in_channels, out_channels, adjacency, stride=1, residual=True):
        super().__init__()
        self.gcn = UnitGcn(in_channels, out_channels, adjacency, adaptive="init", with_res=True)
        self.tcn = MsTcn(out_channels, out_channels, stride=stride)
        self.relu = nn.ReLU()
        self.residual = None
        self.use_identity_residual = False
        if residual and in_channels == out_channels and stride == 1:
            self.use_identity_residual = True
        if residual and not self.use_identity_residual:
            self.residual = UnitTcn(in_channels, out_channels, kernel_size=1, stride=stride)

    def forward(self, x):
        y = self.tcn(self.gcn(x))
        if self.residual is not None:
            y = y + self.residual(x)
        if self.use_identity_residual:
            y = y + x
        y = self.relu(y)
        return y


class StgcnBackbone(nn.Module):
    def __init__(
        self,
        in_channels=3,
        base_channels=64,
        num_person=2,
        num_stages=10,
        inflate_stages=(5, 8),
        down_stages=(5, 8),
    ):
        super().__init__()
        self.in_channels = in_channels
        self.base_channels = base_channels
        self.num_person = num_person
        self.num_stages = num_stages
        self.inflate_stages = inflate_stages
        self.down_stages = down_stages
        adjacency = coco_spatial_adjacency()
        self.data_bn = nn.BatchNorm1d(in_channels * adjacency.size(1))
        modules = [StgcnBlock(in_channels, base_channels, adjacency.clone(), 1, residual=False)]
        inflate_times = 0
        stage_channels = base_channels
        for i in range(2, num_stages + 1):
            stride = 1 + (i in down_stages)
            in_c = stage_channels
            if i in inflate_stages:
                inflate_times += 1
            out_c = int(base_channels * (2**inflate_times) + EPS)
            stage_channels = out_c
            modules.append(StgcnBlock(in_c, out_c, adjacency.clone(), stride))
        self.gcn = nn.ModuleList(modules)

    def forward(self, x):
        n, m, t, v, c = x.size()
        x = x.permute(0, 1, 3, 4, 2).contiguous()
        x = self.data_bn(x.view(n * m, v * c, t))
        x = x.view(n, m, v, c, t).permute(0, 1, 3, 4, 2).contiguous().view(n * m, c, t, v)
        for block in self.gcn:
            x = block(x)
        x = x.reshape((n, m) + x.shape[1:])
        return x


class GcnHead(nn.Module):
    def __init__(self, num_classes=60, in_channels=256):
        super().__init__()
        self.num_classes = num_classes
        self.in_c = in_channels
        self.pool = nn.AdaptiveAvgPool2d(1)
        self.fc_cls = nn.Linear(in_channels, num_classes)

    def forward(self, x):
        n, m, c, t, v = x.shape
        x = x.reshape(n * m, c, t, v)
        x = self.pool(x)
        x = x.reshape(n, m, c).mean(dim=1)
        cls_score = self.fc_cls(x)
        return cls_score


class StgcnppRecognizer(nn.Module):
    def __init__(self, num_classes=60):
        super().__init__()
        self.num_classes = num_classes
        self.backbone = StgcnBackbone()
        self.cls_head = GcnHead(num_classes=num_classes)
        self.softmax = nn.Softmax(dim=-1)

    def forward(self, x):
        feat = self.backbone(x)
        logits = self.cls_head(feat)
        probs = self.softmax(logits)
        return probs
