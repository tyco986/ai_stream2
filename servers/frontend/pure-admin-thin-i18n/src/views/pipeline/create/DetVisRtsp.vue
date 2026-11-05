<script setup lang="ts">
import { ref } from "vue";
import {
  PIPELINE_CLASS_DIALOG_WIDTH,
  PIPELINE_FORM_DIALOG_WIDTH,
  PIPELINE_STREAM_DIALOG_WIDTH
} from "@/constants/pipeline";
import { $t, transformI18n } from "@/plugins/i18n";

const visible = defineModel<boolean>({ required: true });

const name = ref("yolo26n_det_vis_rtsp");
const type = ref("DetVisRTSPPipeline");
const pgie = ref("yolo26n");
const drawerInterval = ref(50);
const fadeTime = ref(1);
const showLabel = ref(true);
const trackerOn = ref(false);
const trackerClasses = ref("person");
const streamsOpen = ref(false);
const classesOpen = ref(false);

const types = ["DetVisRTSPPipeline"];
const pgies = ["yolo26n"];
const streams = [
  { name: "video1", url: "rtsp://ai_stream2_mediamtx:8554/video1", width: 1920, height: 1080, fps: 25, analyzer: "" },
  { name: "video2", url: "rtsp://ai_stream2_mediamtx:8554/video2", width: 1920, height: 1080, fps: 25, analyzer: "" }
];
const analyzers = ["floor-roi"];
const classes = [
  { id: 0, name: "person" },
  { id: 1, name: "bicycle" },
  { id: 2, name: "car" }
];
</script>

<template>
  <el-dialog
    v-model="visible"
    append-to-body
    class="create-dialog"
    :title="transformI18n($t('pipeline.pureCreate'))"
    :width="PIPELINE_FORM_DIALOG_WIDTH"
  >
    <el-form label-width="88px" label-position="left">
      <el-form-item :label="transformI18n($t('pipeline.pureName'))">
        <el-input v-model="name" />
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureType'))">
        <el-select v-model="type" class="pipeline-field">
          <el-option v-for="item in types" :key="item" :label="item" :value="item" />
        </el-select>
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.purePgie'))">
        <el-select v-model="pgie" class="pipeline-field">
          <el-option v-for="item in pgies" :key="item" :label="item" :value="item" />
        </el-select>
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureDrawer'))">
        <div class="drawer-fields">
          <span>{{ transformI18n($t("pipeline.pureInterval")) }}</span>
          <el-input-number v-model="drawerInterval" :min="0" :precision="0" :step="1" />
          <span>{{ transformI18n($t("pipeline.pureFadeTime")) }}</span>
          <el-input-number v-model="fadeTime" :min="0" :precision="0" :step="1" />
          <el-checkbox v-model="showLabel">{{ transformI18n($t("pipeline.pureShowLabel")) }}</el-checkbox>
        </div>
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureStreams'))">
        <el-input model-value="video1, video2" readonly @click="streamsOpen = true" />
      </el-form-item>
      <el-form-item :label="transformI18n($t('pipeline.pureTracker'))">
        <div class="tracker-fields">
          <el-switch v-model="trackerOn" />
          <el-input v-model="trackerClasses" readonly :disabled="!trackerOn" @click="trackerOn && (classesOpen = true)" />
        </div>
      </el-form-item>
    </el-form>
    <template #footer>
      <el-button @click="visible = false">{{ transformI18n($t("pipeline.pureCancel")) }}</el-button>
      <el-button type="primary" @click="visible = false">{{ transformI18n($t("pipeline.pureConfirm")) }}</el-button>
    </template>
  </el-dialog>
  <el-dialog
    v-model="streamsOpen"
    append-to-body
    class="create-dialog"
    :title="transformI18n($t('pipeline.pureStreams'))"
    :width="PIPELINE_STREAM_DIALOG_WIDTH"
  >
    <el-table :data="streams" border>
      <el-table-column type="selection" width="48" />
      <el-table-column prop="name" :label="transformI18n($t('stream.pureName'))" />
      <el-table-column prop="url" :label="transformI18n($t('stream.pureUrl'))" />
      <el-table-column prop="width" :label="transformI18n($t('stream.pureWidth'))" width="90" />
      <el-table-column prop="height" :label="transformI18n($t('stream.pureHeight'))" width="90" />
      <el-table-column prop="fps" :label="transformI18n($t('stream.pureFps'))" width="80" />
      <el-table-column :label="transformI18n($t('pipeline.pureAnalyzer'))" width="180">
        <template #default="{ row }">
          <el-select v-model="row.analyzer" clearable :placeholder="transformI18n($t('pipeline.pureSelect'))">
            <el-option v-for="item in analyzers" :key="item" :label="item" :value="item" />
          </el-select>
        </template>
      </el-table-column>
    </el-table>
    <template #footer>
      <el-button @click="streamsOpen = false">{{ transformI18n($t("pipeline.pureCancel")) }}</el-button>
      <el-button type="primary" @click="streamsOpen = false">{{ transformI18n($t("pipeline.pureConfirm")) }}</el-button>
    </template>
  </el-dialog>
  <el-dialog
    v-model="classesOpen"
    append-to-body
    class="create-dialog"
    :title="transformI18n($t('pipeline.pureClass'))"
    :width="PIPELINE_CLASS_DIALOG_WIDTH"
  >
    <el-table :data="classes" border>
      <el-table-column type="selection" width="48" />
      <el-table-column prop="id" :label="transformI18n($t('model.pureClassId'))" width="120" />
      <el-table-column prop="name" :label="transformI18n($t('model.pureClassName'))" />
    </el-table>
    <template #footer>
      <el-button @click="classesOpen = false">{{ transformI18n($t("pipeline.pureCancel")) }}</el-button>
      <el-button type="primary" @click="classesOpen = false">{{ transformI18n($t("pipeline.pureConfirm")) }}</el-button>
    </template>
  </el-dialog>
</template>

<style scoped>
.pipeline-field {
  width: 100%;
}

.drawer-fields,
.tracker-fields {
  display: flex;
  gap: 12px;
  align-items: center;
  width: 100%;
}

.tracker-fields .el-input {
  flex: 1;
}
</style>
