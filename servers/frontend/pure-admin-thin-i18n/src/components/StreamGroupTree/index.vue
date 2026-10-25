<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
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
import { ALL_GROUP_ID } from "@/constants/streamGroup";
import { $t, transformI18n } from "@/plugins/i18n";

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

onMounted(load);
</script>

<template>
  <el-tree
    class="stream-group-tree"
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
                {{ transformI18n($t("stream.pureRemove")) }}
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
    width="720px"
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
      <div class="pane">
        <label v-for="row in left" :key="row.id" class="stream-row" :class="{ moved: row.moved }">
          <el-checkbox v-model="row.checked" />
          <span class="stream-name">{{ row.name }}</span>
          <span class="stream-group">{{ row.group_name }}</span>
        </label>
      </div>
      <div class="mover">
        <el-button :icon="ArrowRight" @click="moveRight" />
        <el-button :icon="ArrowLeft" @click="moveLeft" />
      </div>
      <div class="pane">
        <label v-for="row in right" :key="row.id" class="stream-row" :class="{ moved: row.moved }">
          <el-checkbox v-model="row.checked" />
          <span class="stream-name">{{ row.name }}</span>
          <span class="stream-group">{{ row.group_name }}</span>
        </label>
      </div>
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
  height: 280px;
  margin-top: 16px;
}

.pane {
  flex: 1;
  overflow: auto;
  border: 1px solid var(--el-border-color);
  border-radius: 4px;
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

.stream-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 8px;
}

.stream-name,
.stream-group {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.moved {
  color: var(--el-color-success);
}

:deep(.el-form-item:last-child) {
  margin-bottom: 12px;
}
</style>
