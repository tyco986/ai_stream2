import type { ClassItem, Model } from '@/api/models'
import type { ClassAttrRow } from '@/api/pipelines'

export type ClassSelectOption = {
  label: string
  value: string
  classId: string
  classLabel: string
  disabled?: boolean
}

export const ALL_CLASS_SELECT = 'all'
export const DETECTED_EMPTY_FALLBACK = '-1'

export type DetectedField =
  | 'detected_min_w'
  | 'detected_min_h'
  | 'detected_max_w'
  | 'detected_max_h'

export const DETECTED_FIELDS: readonly DetectedField[] = [
  'detected_min_w',
  'detected_min_h',
  'detected_max_w',
  'detected_max_h',
] as const

/** Editable table row: numeric cells stay strings so typing mid-states (e.g. "-") work. */
export type ClassAttrFormRow = {
  key: string
  classSelect: string
  conf: string
  topk: string
  detected_min_w: string
  detected_min_h: string
  detected_max_w: string
  detected_max_h: string
}

export class GieClassAttrsFormat {
  private labels: Map<number, string>

  constructor(classes: ClassItem[] = []) {
    this.labels = new Map()
    this.setClasses(classes)
  }

  setClasses(classes: ClassItem[]) {
    this.labels = new Map(classes.map((item) => [item.id, item.name]))
  }

  classHead(classValue: ClassAttrRow['class'] | string): string {
    if (classValue === ALL_CLASS_SELECT) {
      return GieClassAttrsFormat.selectText('-1', 'All')
    }
    const id = Number(classValue)
    const label = this.labels.get(id) ?? String(id)
    return GieClassAttrsFormat.selectText(String(id), label)
  }

  summaryLine(row: ClassAttrRow): string {
    return [
      `${this.classHead(row.class)}:`,
      `Confidence=${row.conf},`,
      `Top K=${row.topk},`,
      `MinW=${row.detected_min_w},`,
      `MinH=${row.detected_min_h},`,
      `MaxW=${row.detected_max_w},`,
      `MaxH=${row.detected_max_h}`,
    ].join(' ')
  }

  summaryLines(rows: ClassAttrRow[]): string[] {
    return rows.map((row) => this.summaryLine(row))
  }

  selectOptions(model: Model | null): ClassSelectOption[] {
    const all: ClassSelectOption = GieClassAttrsFormat.selectOption('-1', 'All', ALL_CLASS_SELECT)
    if (!model) {
      return [all]
    }
    if (model.classes?.length) {
      return [
        all,
        ...model.classes.map((item) =>
          GieClassAttrsFormat.selectOption(String(item.id), item.name, String(item.id)),
        ),
      ]
    }
    const count = Math.max(0, model.num_class ?? 0)
    return [
      all,
      ...Array.from({ length: count }, (_, id) =>
        GieClassAttrsFormat.selectOption(String(id), String(id), String(id)),
      ),
    ]
  }

  /** Collapsed select and dropdown option share this exact text. */
  static selectText(classId: string, classLabel: string): string {
    return `${classId} - ${classLabel}`
  }

  static selectOption(classId: string, classLabel: string, value: string): ClassSelectOption {
    return {
      label: GieClassAttrsFormat.selectText(classId, classLabel),
      value,
      classId,
      classLabel,
    }
  }
}

export class GieClassAttrsForm {
  static createRow(key: string, classSelect: string): ClassAttrFormRow {
    return {
      key,
      classSelect,
      conf: '0.25',
      topk: '300',
      detected_min_w: DETECTED_EMPTY_FALLBACK,
      detected_min_h: DETECTED_EMPTY_FALLBACK,
      detected_max_w: DETECTED_EMPTY_FALLBACK,
      detected_max_h: DETECTED_EMPTY_FALLBACK,
    }
  }

  static fromApi(row: ClassAttrRow, key: string): ClassAttrFormRow {
    return {
      key,
      classSelect: row.class === ALL_CLASS_SELECT ? ALL_CLASS_SELECT : String(row.class),
      conf: String(row.conf),
      topk: String(row.topk),
      detected_min_w: String(row.detected_min_w),
      detected_min_h: String(row.detected_min_h),
      detected_max_w: String(row.detected_max_w),
      detected_max_h: String(row.detected_max_h),
    }
  }

  static toApi(row: ClassAttrFormRow): ClassAttrRow {
    const classValue =
      row.classSelect === ALL_CLASS_SELECT ? ALL_CLASS_SELECT : Number(row.classSelect)
    return {
      class: classValue,
      conf: GieClassAttrsForm.parseNumber(row.conf, 0),
      topk: GieClassAttrsForm.parseNumber(row.topk, 0),
      detected_min_w: GieClassAttrsForm.parseDetected(row.detected_min_w),
      detected_min_h: GieClassAttrsForm.parseDetected(row.detected_min_h),
      detected_max_w: GieClassAttrsForm.parseDetected(row.detected_max_w),
      detected_max_h: GieClassAttrsForm.parseDetected(row.detected_max_h),
    }
  }

  static parseNumber(raw: string, fallback: number): number {
    const parsed = Number(raw)
    return Number.isFinite(parsed) ? parsed : fallback
  }

  /** Empty / lone "-" → -1; otherwise finite number or -1. */
  static parseDetected(raw: string): number {
    const text = raw.trim()
    if (text === '' || text === '-') {
      return -1
    }
    return GieClassAttrsForm.parseNumber(text, -1)
  }

  /** Blur: normalize empty draft text back to "-1". */
  static commitDetectedText(raw: string): string {
    return String(GieClassAttrsForm.parseDetected(raw))
  }

  static isClassSelected(row: ClassAttrFormRow): boolean {
    return Boolean(row.classSelect)
  }
}

export function buildClassAttrsSummary(
  rows: ClassAttrRow[],
  classes: ClassItem[] = [],
): string {
  return new GieClassAttrsFormat(classes).summaryLines(rows).join('; ')
}
