<template>
  <div>
    <PageHeader title="仪表盘" subtitle="系统运行概况" />

    <!-- Stat cards -->
    <el-row :gutter="20" style="margin-bottom: 24px">
      <el-col :span="6" v-for="card in statCards" :key="card.label">
        <el-card shadow="hover" style="border-radius: 8px">
          <div style="display: flex; align-items: center; gap: 16px">
            <div
              style="width: 48px; height: 48px; border-radius: 10px; display: flex; align-items: center; justify-content: center; font-size: 24px"
              :style="{ background: card.bg, color: card.color }"
            >
              <el-icon><component :is="card.icon" /></el-icon>
            </div>
            <div>
              <div style="font-size: 28px; font-weight: 700; line-height: 1.2">{{ card.value }}</div>
              <div style="font-size: 13px; color: #909399; margin-top: 2px">{{ card.label }}</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Charts -->
    <el-row :gutter="20">
      <el-col :span="16">
        <el-card shadow="hover" style="border-radius: 8px">
          <template #header>
            <span style="font-weight: 600">检测趋势（近7天）</span>
          </template>
          <div ref="trendChartRef" style="height: 350px" />
        </el-card>
      </el-col>
      <el-col :span="8">
        <el-card shadow="hover" style="border-radius: 8px">
          <template #header>
            <span style="font-weight: 600">摄像头状态分布</span>
          </template>
          <div ref="statusChartRef" style="height: 350px" />
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import * as echarts from 'echarts'
import { VideoCameraFilled, DataAnalysis, Bell, Film } from '@element-plus/icons-vue'
import PageHeader from '@/components/common/PageHeader.vue'

/* @API:DASH_OVERVIEW — GET /api/v1/dashboard/overview/ */
const statCards = ref([
  { label: '在线摄像头', value: 8, icon: VideoCameraFilled, bg: '#ecf5ff', color: '#409eff' },
  { label: '今日检测', value: 1247, icon: DataAnalysis, bg: '#f0f9eb', color: '#67c23a' },
  { label: '未处理报警', value: 5, icon: Bell, bg: '#fef0f0', color: '#f56c6c' },
  { label: '录制总数', value: 32, icon: Film, bg: '#fdf6ec', color: '#e6a23c' },
])

const trendChartRef = ref<HTMLElement>()
const statusChartRef = ref<HTMLElement>()
let trendChart: echarts.ECharts | null = null
let statusChart: echarts.ECharts | null = null

function initTrendChart() {
  if (!trendChartRef.value) return
  trendChart = echarts.init(trendChartRef.value)

  /* @API:DASH_TREND — GET /api/v1/dashboard/detection-trend/ */
  const days = Array.from({ length: 7 }, (_, i) => {
    const d = new Date()
    d.setDate(d.getDate() - (6 - i))
    return `${d.getMonth() + 1}/${d.getDate()}`
  })
  const values = [182, 234, 290, 198, 340, 250, 310]

  trendChart.setOption({
    tooltip: { trigger: 'axis' },
    grid: { left: 50, right: 20, top: 20, bottom: 30 },
    xAxis: { type: 'category', data: days, boundaryGap: false },
    yAxis: { type: 'value' },
    series: [{
      type: 'line',
      data: values,
      smooth: true,
      areaStyle: { color: 'rgba(64,158,255,0.15)' },
      lineStyle: { color: '#409eff', width: 2 },
      itemStyle: { color: '#409eff' },
    }],
  })
}

function initStatusChart() {
  if (!statusChartRef.value) return
  statusChart = echarts.init(statusChartRef.value)

  /* @API:DASH_STATUS — GET /api/v1/dashboard/camera-status/ */
  statusChart.setOption({
    tooltip: { trigger: 'item' },
    legend: { bottom: 0 },
    series: [{
      type: 'pie',
      radius: ['40%', '70%'],
      avoidLabelOverlap: true,
      itemStyle: { borderRadius: 6, borderColor: '#fff', borderWidth: 2 },
      label: { show: true, formatter: '{b}: {c}' },
      data: [
        { value: 8, name: '在线', itemStyle: { color: '#67c23a' } },
        { value: 3, name: '离线', itemStyle: { color: '#909399' } },
        { value: 1, name: '错误', itemStyle: { color: '#f56c6c' } },
      ],
    }],
  })
}

function handleResize() {
  trendChart?.resize()
  statusChart?.resize()
}

onMounted(() => {
  initTrendChart()
  initStatusChart()
  window.addEventListener('resize', handleResize)
})

onBeforeUnmount(() => {
  window.removeEventListener('resize', handleResize)
  trendChart?.dispose()
  statusChart?.dispose()
})
</script>
