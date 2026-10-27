FROM ultralytics/ultralytics:latest AS build

WORKDIR /app

COPY servers/export_onnx/requirements.txt .
RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple \
    && pip install --no-cache-dir -r requirements.txt

COPY servers/export_onnx/main.py ./
COPY servers/export_onnx/utils/ ./utils/
COPY servers/export_onnx/modules/ ./modules/
COPY servers/export_onnx/scripts/4_nuitka.sh /tmp/4_nuitka.sh
RUN bash /tmp/4_nuitka.sh && rm /tmp/4_nuitka.sh


FROM ultralytics/ultralytics:latest

ENV HOST=0.0.0.0
ENV PORT=8090
ENV LOG_ROOT=/root/logs/export_onnx

WORKDIR /app

RUN --mount=type=bind,source=servers/export_onnx/requirements.txt,target=/tmp/requirements.txt \
    pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple \
    && pip install --no-cache-dir -r /tmp/requirements.txt

COPY --from=build /app/export_onnx ./export_onnx
RUN mkdir -p /root/history /root/logs/export_onnx

EXPOSE 8090
CMD ["./export_onnx"]
