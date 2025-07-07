# project/detect_os.py
import platform
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

# Windows 下启用自定义 build/core 目录
if platform.system() == "Windows":
    # 使用 r"" 以免反斜杠转义
    env.Replace(
        BUILD_DIR=r"C:\build\pawcounter",
        CORE_DIR=r"C:\platformio\pawcounter"
    )
    # 可选：在终端打印，方便调试
    print("⮕ Detected Windows: setting BUILD_DIR and CORE_DIR")
else:
    # macOS/Linux 不做任何替换，使用默认目录
    print(f"⮕ Detected {platform.system()}: using default build/core dirs")