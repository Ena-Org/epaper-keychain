import os
import sys
import subprocess
import shutil

# 获取当前项目目录
project_dir = os.path.dirname(os.path.abspath(__file__))

# 创建 .pio 目录（如果不存在）
pio_dir = os.path.join(project_dir, ".pio")
if not os.path.exists(pio_dir):
    os.makedirs(pio_dir)

# 创建虚拟环境目录
venv_dir = os.path.join(pio_dir, "venv")

# 检查虚拟环境是否已经存在
if not os.path.exists(venv_dir):
    print("Creating virtual environment...")
    subprocess.check_call([sys.executable, "-m", "venv", venv_dir])
else:
    print("Virtual environment already exists.")

# 激活虚拟环境并安装 PlatformIO
activate_script = os.path.join(venv_dir, "bin", "activate")

# Windows 下的激活脚本路径
if os.name == "nt":
    activate_script = os.path.join(venv_dir, "Scripts", "activate.bat")

# 安装 PlatformIO
def install_platformio():
    print("Installing PlatformIO in virtual environment...")
    subprocess.check_call([os.path.join(venv_dir, "bin", "pip"), "install", "platformio"])

# 激活虚拟环境并安装 PlatformIO
def run_in_venv():
    if os.name == "nt":
        # 对于 Windows，使用 activate.bat 激活虚拟环境
        subprocess.check_call([os.path.join(venv_dir, "Scripts", "activate.bat"), "&&", "pip", "install", "platformio"], shell=True)
    else:
        # 对于 Linux/MacOS，使用 activate 脚本激活虚拟环境
        subprocess.check_call(f"source {activate_script} && pip install platformio", shell=True, executable="/bin/bash")

# 安装 PlatformIO
install_platformio()

print("PlatformIO installation completed!")
