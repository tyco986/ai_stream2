/**
 * Streams API client. Implementation selected by Vite mode (real → http, mock → mock).
 */

import * as api from './runtime'

export type {
  Group,
  GroupsTree,
  ListStreamsQuery,
  PagedStreams,
  PublishResult,
  Stream,
  StreamStatus,
  StreamSummary,
  ProbeResult,
  TreeGroupNode,
  TreeNode,
  TreeStreamNode,
} from '@/api/streams/types'

export { ALL_GROUP_ID } from '@/api/streams/types'
export { collectStreams } from '@/api/streams/utils'

export const STREAMS_API_BASE = api.STREAMS_API_BASE
export const getGroupsTree = api.getGroupsTree
export const getGroupsMap = api.getGroupsMap
export const getGroupsList = api.getGroupsList
export const createGroup = api.createGroup
export const patchGroup = api.patchGroup
export const deleteGroup = api.deleteGroup
export const getGroupMembers = api.getGroupMembers
export const getGroupCandidates = api.getGroupCandidates
export const putGroupMembers = api.putGroupMembers
export const listStreams = api.listStreams
export const getStreamsMap = api.getStreamsMap
export const getStream = api.getStream
export const createStream = api.createStream
export const patchStream = api.patchStream
export const deleteStream = api.deleteStream
export const probeStream = api.probeStream
export const probeStreamUrl = api.probeStreamUrl
export const getStreamLogs = api.getStreamLogs
export const batchDelete = api.batchDelete
export const batchEnable = api.batchEnable
export const batchDisable = api.batchDisable
export const batchRecord = api.batchRecord
export const batchUnrecord = api.batchUnrecord
export const batchProbe = api.batchProbe
export const publishVideo = api.publishVideo
