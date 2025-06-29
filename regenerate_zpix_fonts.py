#!/usr/bin/env python3
"""
重新生成ZPIX字体的脚本，使用更大的尺寸进行测试
"""

import subprocess
import os
import sys

def check_lv_font_conv():
    """检查lv_font_conv是否可用"""
    try:
        result = subprocess.run(['npx', 'lv_font_conv', '--help'], 
                              capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            print("✅ lv_font_conv 可用")
            return True
    except:
        pass
    
    print("❌ lv_font_conv 不可用，尝试安装...")
    return False

def generate_font(size, output_file):
    """生成指定大小的字体文件"""
    if not os.path.exists("zpix.ttf"):
        print("❌ zpix.ttf 文件不存在")
        return False
    
    # 字符范围：数字0-9, 运算符, 字母等
    char_range = "0x30-0x39,0x2B,0x2D,0x2A,0x2F,0x3D,0x2E,0x25,0x28,0x29,0x43,0x45,0x49,0x4E,0x4F,0x52,0x72,0x6F,0x65,0x74,0x61,0x6E,0x20,0x2C,0xA5,0x24"
    
    cmd = [
        'npx', 'lv_font_conv',
        '--font', 'zpix.ttf',
        '--size', str(size),
        '--bpp', '4',
        '--format', 'lvgl',
        '--no-compress',
        '--output', output_file,
        '--range', char_range
    ]
    
    print(f"🔄 生成 {size}px 字体...")
    print(f"命令: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode == 0:
            print(f"✅ {size}px 字体生成成功: {output_file}")
            if os.path.exists(output_file):
                size_kb = os.path.getsize(output_file) / 1024
                print(f"   文件大小: {size_kb:.1f} KB")
            return True
        else:
            print(f"❌ 字体生成失败: {result.stderr}")
            return False
    except subprocess.TimeoutExpired:
        print(f"❌ 字体生成超时")
        return False
    except Exception as e:
        print(f"❌ 字体生成出错: {e}")
        return False

def generate_test_fonts():
    """生成测试用的更大字体"""
    print("🔍 生成测试用的大字体...")
    
    # 生成更大的字体进行测试
    test_sizes = [
        (30, "src/fonts/lv_font_zpix_30.c"),
        (80, "src/fonts/lv_font_zpix_80.c"),
        (100, "src/fonts/lv_font_zpix_100.c")
    ]
    
    success_count = 0
    for size, output in test_sizes:
        if generate_font(size, output):
            success_count += 1
    
    print(f"\n📊 生成结果: {success_count}/{len(test_sizes)} 个字体文件生成成功")
    
    if success_count > 0:
        print("\n💡 建议:")
        print("1. 在 fonts/lv_font_zpix.h 中添加新字体的声明")
        print("2. 在 LVGLDisplay.cpp 中临时使用更大的字体进行测试")
        print("3. 对比不同尺寸的显示效果")

def analyze_zpix_font():
    """分析ZPIX字体特性"""
    print("\n🔍 ZPIX字体分析:")
    
    if not os.path.exists("zpix.ttf"):
        print("❌ zpix.ttf 不存在，无法分析")
        return
    
    print("✅ ZPIX是一个像素艺术字体")
    print("   特点:")
    print("   - 专为像素完美显示设计")
    print("   - 每个字符都是精确的像素网格")
    print("   - 在小尺寸下可能显示效果不明显")
    print("   - 建议使用较大尺寸(40px+)以获得最佳效果")
    
    print("\n💡 优化建议:")
    print("   1. 尝试40px和120px的组合")
    print("   2. 考虑启用LCD子像素渲染")
    print("   3. 调整容器高度以匹配字体尺寸")
    print("   4. 验证字体在目标分辨率下的显示效果")

def main():
    """主函数"""
    print("🎨 ZPIX字体重新生成和测试工具")
    print("=" * 50)
    
    # 分析字体特性
    analyze_zpix_font()
    
    # 检查工具
    if not check_lv_font_conv():
        print("请先安装 lv_font_conv:")
        print("npm install lv_font_conv -g")
        return
    
    # 询问是否生成测试字体
    response = input("\n是否生成测试用的大字体? (y/N): ").strip().lower()
    if response in ['y', 'yes']:
        generate_test_fonts()
    
    print("\n✨ 完成!")

if __name__ == "__main__":
    main()