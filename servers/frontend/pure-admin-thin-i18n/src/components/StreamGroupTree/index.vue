<script setup lang="ts">
import { computed, nextTick, onMounted, ref, watch } from "vue";
import ArrowLeft from "~icons/ri/arrow-left-s-line";
import ArrowRight from "~icons/ri/arrow-right-s-line";
import More from "~icons/ri/more-2-fill";
import { getStreams } from "@/api/stream";
import {
  createGroup,
  getGroupCandidates,
  getGroupMap,
  getGroupMembers,
  getGroupTree,
  groupById,
  parentGroupId,
  removeGroup,
  setGroupMembers,
  subtreeGroupIds,
  updateGroup,
  type GroupStream,
  type GroupTreeNode,
  type TreeNode
} from "@/api/streamGroup";
import { ALL_GROUP_ID, STREAM_GROUP_COLUMN_ALIGNS, STREAM_GROUP_COLUMN_PERCENTS, STREAM_GROUP_COLUMN_RESIZABLE, STREAM_GROUP_DIALOG_WIDTH, STREAM_GROUP_ROW_HEIGHT, STREAM_GROUP_TREE_INDENT } from "@/constants/streamGroup";
import { $t, transformI18n } from "@/plugins/i18n";
import { useColumnWidths } from "@/views/pipeline/columnWidth";

defineOptions({
  name: "StreamGroupTree"
});

type PickerRow = GroupStream & {
  checked: boolean;
  moved: boolean;
};

const emit = defineEmits<{
  select: [node: TreeNode];
}>();

const data = ref<TreeNode[]>([]);
const visible = ref(false);
const mode = ref<"new" | "edit">("new");
const target = ref<GroupTreeNode | null>(null);
const name = ref("");
const parentId = ref("");
const parentOptions = ref<{ id: string; name: string }[]>([]);
const left = ref<PickerRow[]>([]);
const right = ref<PickerRow[]>([]);
const canMoveRight = computed(() => left.value.some(row => row.checked));
const canMoveLeft = computed(() => right.value.some(row => row.checked));
const { target: leftRef, widths: leftWidths, observe: observeLeft } = useColumnWidths(STREAM_GROUP_COLUMN_PERCENTS);
const { target: rightRef, widths: rightWidths, observe: observeRight } = useColumnWidths(STREAM_GROUP_COLUMN_PERCENTS);

const blockedParents = computed(() => {
  const node = mode.value === "edit" && target.value ? groupById(target.value.id, data.value) : null;
  return new Set(node ? subtreeGroupIds(node) : []);
});

const toRow = (item: GroupStream): PickerRow => ({
  ...item,
  checked: false,
  moved: false
});

const load = () => {
  return getGroupTree().then(res => {
    data.value = [res.data];
  });
};

const loadStreams = () => {
  return getStreams({ page: 1, page_size: 1 }).then(first =>
    getStreams({ page: 1, page_size: Math.max(first.data.total, 1) }).then(res => {
      left.value = res.data.items.map(item =>
        toRow({
          id: item.id,
          name: item.name,
          group_id: item.group_id || "",
          group_name: item.group_name || ""
        })
      );
      right.value = [];
    })
  );
};

const loadMembers = (groupId: string) => {
  return Promise.all([getGroupCandidates(groupId), getGroupMembers(groupId)]).then(
    ([candidates, members]) => {
      left.value = candidates.data.map(toRow);
      right.value = members.data.map(toRow);
    }
  );
};

const onNodeClick = (node: TreeNode) => {
  emit("select", node);
};

const nodeLabel = (node: TreeNode) =>
  node.id === ALL_GROUP_ID ? transformI18n($t("stream.pureAll")) : node.name;

const openDialog = (next: "new" | "edit", node: GroupTreeNode) => {
  mode.value = next;
  target.value = node;
  name.value = next === "edit" ? node.name : "";
  const parent = next === "edit" ? parentGroupId(node.id, data.value) : node.id;
  const parentNode = groupById(parent, data.value);
  parentId.value = parent;
  parentOptions.value = parentNode ? [{ id: parentNode.id, name: parentNode.name }] : [];
  left.value = [];
  right.value = [];
  visible.value = true;
  const loading = next === "new" ? loadStreams() : loadMembers(node.id);
  return loading;
};

const commands: Record<string, (node: GroupTreeNode) => void> = {
  new: node => openDialog("new", node),
  edit: node => openDialog("edit", node),
  remove: node => {
    removeGroup(node.id).then(res => {
      data.value = [res.data];
    });
  }
};

const onCommand = (command: string, node: GroupTreeNode) => {
  commands[command](node);
};

const onParentOpen = (opened: boolean) => {
  const loading = opened && mode.value === "edit" ? getGroupMap() : null;
  if (loading) {
    loading.then(res => {
      parentOptions.value = Object.entries(res.data).map(([label, id]) => ({
        id,
        name: label
      }));
    });
  }
};

const currentGroupName = () => name.value || target.value?.name || "";

const markChecked = (rows: PickerRow[], selected: PickerRow[]) => {
  const ids = new Set(selected.map(row => row.id));
  rows.forEach(row => {
    row.checked = ids.has(row.id);
  });
};

const onLeftSelect = (selected: PickerRow[]) => {
  markChecked(left.value, selected);
};

const onRightSelect = (selected: PickerRow[]) => {
  markChecked(right.value, selected);
};

const rowClass = ({ row }: { row: PickerRow }) => (row.moved ? "moved" : "");

const observePicker = () => {
  observeLeft();
  observeRight();
};

const moveRight = () => {
  const picked = left.value.filter(row => row.checked);
  left.value = left.value.filter(row => !row.checked);
  right.value = [
    ...right.value,
    ...picked.map(row => ({
      ...row,
      checked: false,
      moved: true,
      group_id: mode.value === "edit" ? target.value?.id || "" : "",
      group_name: currentGroupName()
    }))
  ];
};

const moveLeft = () => {
  const picked = right.value.filter(row => row.checked);
  right.value = right.value.filter(row => !row.checked);
  left.value = [
    ...left.value,
    ...picked.map(row => ({
      ...row,
      checked: false,
      moved: true,
      group_id: ALL_GROUP_ID,
      group_name: "All"
    }))
  ];
};

const applyMembers = (groupId: string) => {
  return setGroupMembers(
    groupId,
    right.value.map(row => row.id)
  ).then(res => {
    data.value = [res.data];
  });
};

const submitters = {
  new: () => createGroup(parentId.value, name.value).then(res => applyMembers(res.data.id)),
  edit: () =>
    updateGroup(target.value.id, name.value, parentId.value).then(() => applyMembers(target.value.id))
};

const onConfirm = () => {
  submitters[mode.value]().then(() => {
    visible.value = false;
  });
};

const onCancel = () => {
  visible.value = false;
};

watch(visible, value => {
  value && nextTick(observePicker);
});

onMounted(load);

defineExpose({ load });
</script>

<template>
  <el-tree
    class="stream-group-tree"
    :indent="STREAM_GROUP_TREE_INDENT"
    :data="data"
    node-key="id"
    default-expand-all
    :props="{ label: 'name', children: 'children' }"
    @node-click="onNodeClick"
  >
    <template #default="{ data: node }">
      <div class="node-row">
        <span class="node-name">{{ nodeLabel(node) }}</span>
        <el-dropdown
          v-if="node.type === 'group'"
          trigger="click"
          @command="(command: string) => onCommand(command, node)"
        >
          <span class="node-more" @click.stop>
            <More />
          </span>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="new">
                {{ transformI18n($t("stream.pureNewGroup")) }}
              </el-dropdown-item>
              <el-dropdown-item command="edit" :disabled="node.id === ALL_GROUP_ID">
                {{ transformI18n($t("stream.pureEditGroup")) }}
              </el-dropdown-item>
              <el-dropdown-item command="remove" :disabled="node.id === ALL_GROUP_ID">
                {{ transformI18n($t("stream.pureRemoveGroup")) }}
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </template>
  </el-tree>
  <el-dialog
    v-model="visible"
    :title="transformI18n($t(mode === 'new' ? 'stream.pureNewGroup' : 'stream.pureEditGroup'))"
    :width="STREAM_GROUP_DIALOG_WIDTH"
  >
    <el-form label-width="72px" label-position="left">
      <el-form-item :label="transformI18n($t('stream.pureName'))">
        <el-input v-model="name" />
      </el-form-item>
      <el-form-item :label="transformI18n($t('stream.pureParent'))">
        <el-select
          v-model="parentId"
          :disabled="mode === 'new'"
          class="parent-select"
          @visible-change="onParentOpen"
        >
          <el-option
            v-for="option in parentOptions"
            :key="option.id"
            :label="option.name"
            :value="option.id"
            :disabled="blockedParents.has(option.id)"
          />
        </el-select>
      </el-form-item>
    </el-form>
    <div class="picker">
      <el-table ref="leftRef" :data="left" border height="280" table-layout="fixed" class="pane picker-table" :row-class-name="rowClass" @selection-change="onLeftSelect">
        <el-table-column type="selection" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="leftWidths.selection" :header-align="STREAM_GROUP_COLUMN_ALIGNS.selection.header" :align="STREAM_GROUP_COLUMN_ALIGNS.selection.content" />
        <el-table-column prop="name" :label="transformI18n($t('stream.pureName'))" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="leftWidths.name" :header-align="STREAM_GROUP_COLUMN_ALIGNS.name.header" :align="STREAM_GROUP_COLUMN_ALIGNS.name.content" />
        <el-table-column prop="group_name" :label="transformI18n($t('stream.pureGroup'))" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="leftWidths.group" :header-align="STREAM_GROUP_COLUMN_ALIGNS.group.header" :align="STREAM_GROUP_COLUMN_ALIGNS.group.content" />
      </el-table>
      <div class="mover">
        <el-button :icon="ArrowRight" :disabled="!canMoveRight" @click="moveRight" />
        <el-button :icon="ArrowLeft" :disabled="!canMoveLeft" @click="moveLeft" />
      </div>
      <el-table ref="rightRef" :data="right" border height="280" table-layout="fixed" class="pane picker-table" :row-class-name="rowClass" @selection-change="onRightSelect">
        <el-table-column type="selection" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="rightWidths.selection" :header-align="STREAM_GROUP_COLUMN_ALIGNS.selection.header" :align="STREAM_GROUP_COLUMN_ALIGNS.selection.content" />
        <el-table-column prop="name" :label="transformI18n($t('stream.pureName'))" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="rightWidths.name" :header-align="STREAM_GROUP_COLUMN_ALIGNS.name.header" :align="STREAM_GROUP_COLUMN_ALIGNS.name.content" />
        <el-table-column prop="group_name" :label="transformI18n($t('stream.pureGroup'))" :resizable="STREAM_GROUP_COLUMN_RESIZABLE" :width="rightWidths.group" :header-align="STREAM_GROUP_COLUMN_ALIGNS.group.header" :align="STREAM_GROUP_COLUMN_ALIGNS.group.content" />
      </el-table>
    </div>
    <template #footer>
      <el-button @click="onCancel">
        {{ transformI18n($t("stream.pureCancel")) }}
      </el-button>
      <el-button type="primary" @click="onConfirm">
        {{ transformI18n($t("stream.pureConfirm")) }}
      </el-button>
    </template>
  </el-dialog>
</template>

<style scoped>
.stream-group-tree {
  height: 100%;
  overflow: auto;
}

.node-row {
  display: flex;
  flex: 1;
  align-items: center;
  min-width: 0;
  padding-right: 4px;
}

.node-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.node-more {
  display: inline-flex;
  align-items: center;
  padding: 2px;
  font-size: 16px;
  color: var(--el-text-color-secondary);
}

.parent-select {
  width: 100%;
}

.picker {
  display: flex;
  gap: 8px;
  margin-top: 16px;
}

.pane {
  flex: 1;
  min-width: 0;
}

.mover {
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 8px;
}

.mover :deep(.el-button + .el-button) {
  margin-left: 0;
}

.picker-table :deep(.el-table__cell) {
  height: v-bind('STREAM_GROUP_ROW_HEIGHT + "px"');
  padding-top: 0;
  padding-bottom: 0;
}

.pane :deep(.moved) {
  color: var(--el-color-success);
}

:deep(.el-form-item:last-child) {
  margin-bottom: 12px;
}
</style>
