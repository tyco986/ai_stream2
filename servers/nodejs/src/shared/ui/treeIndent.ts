/** Shared left padding for group/stream tree rows. Streams align with parent group label. */
export function treeRowPaddingLeft(
  depth: number,
  kind: 'group' | 'stream',
): number {
  const indentDepth = kind === 'stream' ? Math.max(0, depth - 1) : depth
  return 12 + indentDepth * 14
}
