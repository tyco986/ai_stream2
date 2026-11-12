import { computed, onUnmounted, ref } from "vue";
import { columnPixels } from "@/utils/columnWidth";

export const useColumnWidths = <T extends Record<string, number>>(percents: T) => {
  const target = ref();
  const width = ref(0);
  const observer = new ResizeObserver(entries => {
    width.value = Math.floor(entries[0].contentRect.width);
  });
  const widths = computed(() => columnPixels(width.value, percents));
  const observe = () => {
    const el = target.value?.$el as HTMLElement | undefined;
    el && observer.observe(el);
  };
  onUnmounted(() => observer.disconnect());
  return { target, widths, observe };
};
