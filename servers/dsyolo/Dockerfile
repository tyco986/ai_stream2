FROM ultralytics/ultralytics:latest

ENV HOST=0.0.0.0
ENV PORT=8090
ENV LOG_ROOT=/root/logs/dsyolo

WORKDIR /app

COPY servers/dsyolo/requirements.txt .
RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple \
    && pip install --no-cache-dir -r requirements.txt

COPY servers/dsyolo/main.py ./
COPY servers/dsyolo/utils/ ./utils/
COPY attachments/DeepStream-Yolo /root/attachments/DeepStream-Yolo

RUN bash -ec 'mkdir -p /root/src && for f in /root/attachments/DeepStream-Yolo/*.zip; do unzip -o -q "$f" -d /root/src; done' \
    && mkdir -p /root/models/pt /root/models/onnx /root/logs/dsyolo

EXPOSE 8090
CMD ["python3", "main.py"]
