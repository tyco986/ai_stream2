FROM ultralytics/ultralytics:latest

ENV HOST=0.0.0.0
ENV PORT=8090
ENV LOG_ROOT=/root/logs/export_onnx

WORKDIR /app

COPY servers/export_onnx/requirements.txt .
RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple \
    && pip install --no-cache-dir -r requirements.txt

COPY servers/export_onnx/main.py ./
COPY servers/export_onnx/utils/ ./utils/
COPY servers/export_onnx/modules/ ./modules/

RUN mkdir -p /root/models/pt /root/models/onnx /root/logs/export_onnx

EXPOSE 8090
CMD ["python3", "main.py"]
