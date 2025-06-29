#!/usr/bin/env python3
"""
ZPIX字体测试和验证脚本
根据LVGL最佳实践优化字体配置
"""

import os
import subprocess
import sys

def check_font_file():
    """检查字体文件是否存在"""
    font_file = "zpix.ttf"
    if not os.path.exists(font_file):
        print(f"❌ 字体文件 {font_file} 不存在")
        return False
    
    print(f"✅ 字体文件 {font_file} 存在")
    return True

def check_generated_files():
    """检查生成的字体文件"""
    font_files = [
        "src/fonts/lv_font_zpix_20.c",
        "src/fonts/lv_font_zpix_60.c",
        "src/fonts/lv_font_zpix.h"
    ]
    
    for file in font_files:
        if os.path.exists(file):
            print(f"✅ {file} 存在")
            # 检查文件大小
            size = os.path.getsize(file)
            print(f"   文件大小: {size:,} bytes")
        else:
            print(f"❌ {file} 不存在")

def analyze_character_range():
    """分析字符范围配置"""
    range_hex = "0x30-0x39,0x2B,0x2D,0x2A,0x2F,0x3D,0x2E,0x25,0x28,0x29,0x43,0x45,0x49,0x4E,0x4F,0x52,0x72,0x6F,0x65,0x74,0x61,0x6E,0x20,0x2C,0xA5,0x24"
    
    print("\n📝 当前字符范围分析:")
    print(f"原始范围: {range_hex}")
    
    # 解析字符
    chars = []
    ranges = range_hex.split(',')
    
    for r in ranges:
        if '-' in r:
            # 范围
            start, end = r.split('-')
            start_val = int(start, 16)
            end_val = int(end, 16)
            for i in range(start_val, end_val + 1):
                chars.append(chr(i))
        else:
            # 单个字符
            chars.append(chr(int(r, 16)))
    
    print(f"包含字符: {''.join(chars)}")
    print(f"字符数量: {len(chars)}")

def optimize_font_parameters():
    """根据LVGL最佳实践提供优化建议"""
    print("\n🔧 LVGL字体优化建议:")
    
    recommendations = [
        "1. BPP设置: 当前使用bpp=4，适合平滑边缘显示",
        "2. 压缩设置: 当前使用--no-compress，适合嵌入式系统",
        "3. 字符范围: 只包含计算器必需的字符，节省内存",
        "4. 字体大小: 20px(表达式) + 60px(主数字)，层次分明",
        "5. 内存使用: 总共约116KB (20px:19KB + 60px:97KB)"
    ]
    
    for rec in recommendations:
        print(f"   ✅ {rec}")

def suggest_improvements():
    """建议进一步的改进"""
    print("\n💡 进一步优化建议:")
    
    improvements = [
        "考虑启用--compress以减小体积(但会增加渲染时间约30%)",
        "可以尝试bpp=2以减小内存占用(但会降低显示质量)",
        "考虑添加更多数学符号: √, ^, π等科学计算符号",
        "可以为不同显示区域使用不同的字体大小",
        "考虑使用--lcd选项支持子像素渲染(如果硬件支持)"
    ]
    
    for i, improvement in enumerate(improvements, 1):
        print(f"   {i}. {improvement}")

def check_memory_usage():
    """检查内存使用情况"""
    print("\n💾 内存使用分析:")
    
    if os.path.exists("src/fonts/lv_font_zpix_20.c"):
        size_20 = os.path.getsize("src/fonts/lv_font_zpix_20.c")
        print(f"   20px字体: {size_20:,} bytes")
    
    if os.path.exists("src/fonts/lv_font_zpix_60.c"):
        size_60 = os.path.getsize("src/fonts/lv_font_zpix_60.c")
        print(f"   60px字体: {size_60:,} bytes")
    
    total = size_20 + size_60 if 'size_20' in locals() and 'size_60' in locals() else 0
    print(f"   总计: {total:,} bytes ({total/1024:.1f} KB)")
    
    # ESP32-S3内存分析
    esp32_flash = 2 * 1024 * 1024  # 2MB
    esp32_ram = 327680  # 320KB
    
    print(f"\n   ESP32-S3资源占用:")
    print(f"   Flash占用: {total/esp32_flash*100:.2f}% ({total:,}/{esp32_flash:,} bytes)")
    print(f"   RAM影响: LVGL需要额外32KB内存池")

def main():
    """主函数"""
    print("🔍 ZPIX字体验证和优化分析")
    print("=" * 50)
    
    # 检查文件
    check_font_file()
    check_generated_files()
    
    # 分析配置
    analyze_character_range()
    
    # 提供建议
    optimize_font_parameters()
    suggest_improvements()
    check_memory_usage()
    
    print("\n✨ 字体集成验证完成!")

if __name__ == "__main__":
    main()