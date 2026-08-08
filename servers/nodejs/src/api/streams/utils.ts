import type { TreeNode, TreeStreamNode } from '@/api/streams/types'

export function collectStreams(node: TreeNode): TreeStreamNode[] {
  if (node.type === 'stream') {
    return [node]
  }
  return node.children.flatMap((child) => collectStreams(child))
}
