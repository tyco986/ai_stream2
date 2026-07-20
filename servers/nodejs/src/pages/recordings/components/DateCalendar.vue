<template>
  <div class="date-calendar">
    <div class="date-calendar__nav">
      <button type="button" class="date-calendar__nav-btn" @click="emit('prev-month')">
        &lt;
      </button>
      <span class="date-calendar__month">{{ month }}</span>
      <button type="button" class="date-calendar__nav-btn" @click="emit('next-month')">
        &gt;
      </button>
    </div>
    <div class="date-calendar__week">
      <span v-for="day in weekdays" :key="day">{{ day }}</span>
    </div>
    <div class="date-calendar__grid">
      <button
        v-for="(cell, index) in cells"
        :key="index"
        type="button"
        class="date-calendar__cell"
        :class="{
          'is-empty': cell === null,
          'is-disabled': cell !== null && !availableDates.has(cell),
          'is-selected': cell !== null && cell === selectedDate,
        }"
        :disabled="cell === null || !availableDates.has(cell)"
        @click="cell && emit('select-date', cell)"
      >
        {{ cell ? Number(cell.slice(-2)) : '' }}
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  month: string
  selectedDate: string | null
  availableDates: Set<string>
}>()

const emit = defineEmits<{
  'prev-month': []
  'next-month': []
  'select-date': [date: string]
}>()

const weekdays = ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa']

const cells = computed(() => {
  const [y, m] = props.month.split('-').map(Number)
  const first = new Date(y, m - 1, 1)
  const daysInMonth = new Date(y, m, 0).getDate()
  const startPad = first.getDay()
  const result: (string | null)[] = []
  for (let i = 0; i < startPad; i += 1) {
    result.push(null)
  }
  for (let day = 1; day <= daysInMonth; day += 1) {
    const d = String(day).padStart(2, '0')
    result.push(`${props.month}-${d}`)
  }
  while (result.length < 42) {
    result.push(null)
  }
  return result
})
</script>

<style scoped>
.date-calendar {
  border-top: 1px solid #e4e7ed;
  padding: 8px 10px 10px;
  flex-shrink: 0;
  background: #fff;
}

.date-calendar__nav {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 6px;
}

.date-calendar__month {
  font-weight: 600;
  font-size: 13px;
}

.date-calendar__nav-btn {
  border: none;
  background: transparent;
  cursor: pointer;
  color: #606266;
  width: 24px;
  height: 24px;
}

.date-calendar__week {
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  text-align: center;
  font-size: 11px;
  color: #909399;
  margin-bottom: 4px;
}

.date-calendar__grid {
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  gap: 2px;
}

.date-calendar__cell {
  height: 28px;
  border: none;
  border-radius: 4px;
  background: transparent;
  cursor: pointer;
  font-size: 12px;
  color: #303133;
}

.date-calendar__cell.is-empty {
  cursor: default;
}

.date-calendar__cell.is-disabled {
  color: #c0c4cc;
  cursor: default;
}

.date-calendar__cell.is-selected {
  background: #409eff;
  color: #fff;
}

.date-calendar__cell:not(.is-disabled):not(.is-empty):hover {
  background: #ecf5ff;
}

.date-calendar__cell.is-selected:hover {
  background: #409eff;
}
</style>
