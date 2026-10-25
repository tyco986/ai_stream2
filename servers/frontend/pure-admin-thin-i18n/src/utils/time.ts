const pad = (value: number) => String(value).padStart(2, "0");

export const formatLocalTime = (value: string | null) => {
  const date = value ? new Date(value) : null;
  const text =
    date && !Number.isNaN(date.getTime())
      ? `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`
      : (value ?? "");
  return text;
};
