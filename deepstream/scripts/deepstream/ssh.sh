#!/bin/bash
set -e

# 1. 非交互式安装
export DEBIAN_FRONTEND=noninteractive

# 预先设置时区，避免 apt 安装时询问
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime 2>/dev/null || true
echo "Asia/Shanghai" > /etc/timezone 2>/dev/null || true

# 2. apt update
apt-get update -y

# 3. 安装 openssh-server（无交互）
apt-get install -y openssh-server

# 4. 创建 sshd 所需目录（部分精简镜像可能缺失）
mkdir -p /var/run/sshd

# 5. 设置 root 密码为 0
echo "root:0" | chpasswd

# 6. 允许 root 通过 SSH 登录
sed -i 's/^#*PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
sed -i 's/^#*PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config
grep -q "^PermitRootLogin" /etc/ssh/sshd_config || echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
grep -q "^PasswordAuthentication" /etc/ssh/sshd_config || echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config

# 7. 确保 .bashrc 存在，并添加 service ssh start
touch /root/.bashrc
grep -q "service ssh start" /root/.bashrc || echo "service ssh start" >> /root/.bashrc

# 8. 立即启动 ssh
service ssh start

# 9. source .bashrc
source /root/.bashrc
