export const columnPixels = <T extends Record<string, number>>(total: number, percents: T) => {
  const keys = Object.keys(percents) as (keyof T)[];
  const widths = {} as { [K in keyof T]: number | undefined };
  let used = 0;
  for (const key of keys.slice(0, -1)) {
    const width = total ? Math.floor((total * percents[key]) / 100) : undefined;
    widths[key] = width;
    used += width ?? 0;
  }
  const last = keys.at(-1);
  last !== undefined && (widths[last] = total ? total - used : undefined);
  return widths;
};
