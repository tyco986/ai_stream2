import logging

from pyservicemaker import BatchMetadataOperator

logger = logging.getLogger(__name__)


class AnalyticsMetadataProbe(BatchMetadataOperator):
    """Probe mounted after nvdsanalytics, before tee.

    Reads AnalyticsFrameMeta / AnalyticsObjInfo produced by nvdsanalytics
    and enriches object/frame metadata so that nvmsgconv (msg2p-newapi mode)
    serialises the analytics results into the Kafka JSON payload.

    Mounting:
        pipeline.attach("analytics", Probe("analytics-probe", AnalyticsMetadataProbe()))
    """

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            analytics_frame = self._extract_analytics_frame(frame_meta)

            for obj_meta in frame_meta.object_items:
                self._enrich_object(obj_meta)

            if analytics_frame:
                self._log_frame_analytics(frame_meta, analytics_frame)

        return True

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def _extract_analytics_frame(self, frame_meta):
        """Return the first valid AnalyticsFrameMeta or None."""
        for user_meta in frame_meta.nvdsanalytics_frame_items:
            afm = user_meta.as_nvdsanalytics_frame()
            if afm:
                return afm
        return None

    def _enrich_object(self, obj_meta):
        """Iterate per-object analytics metadata so it is resolved before
        the buffer reaches nvmsgconv.

        With ``msg2p-newapi=True`` nvmsgconv reads NvDsObjectMeta directly;
        nvdsanalytics already attaches AnalyticsObjInfo as user metadata.
        This method is intentionally a read-only pass-through — its role is
        to ensure the iterator is consumed (forcing lazy resolution) and to
        provide a hook for future custom enrichment (e.g. label injection).
        """
        for user_meta in obj_meta.nvdsanalytics_obj_items:
            user_meta.as_nvdsanalytics_obj()

    def _log_frame_analytics(self, frame_meta, afm):
        """Periodically log frame-level analytics for debugging."""
        if frame_meta.frame_number % 300 == 0:
            logger.debug(
                "frame=%d oc_status=%s obj_in_roi=%s lc_curr=%s lc_cum=%s",
                frame_meta.frame_number,
                afm.oc_status,
                afm.obj_in_roi_cnt,
                afm.obj_lc_curr_cnt,
                afm.obj_lc_cum_cnt,
            )
