#pragma once
#include "Animation.h"

/**
 * @brief 字符滑入/滑出动画
 * @details 实现A1(输入字符滑入)和A2(删除字符滑出)动效
 */
class CharSlideAnim : public Animation {
private:
    String _prevText;       // 原始文本
    String _newText;        // 新文本
    bool _isInsertMode;     // true=滑入模式(A1), false=滑出模式(A2)
    uint8_t _lineIndex;     // 目标行索引(通常是3-结果行)
    
    // 动画计算参数
    uint16_t _charWidth;    // 字符宽度
    int16_t _startX;        // 起始X坐标
    int16_t _endX;          // 结束X坐标
    int16_t _currentX;      // 当前X坐标
    
public:
    /**
     * @brief 构造函数
     * @param display 显示对象指针
     * @param prevText 原始文本
     * @param newText 新文本
     * @param insertMode true=滑入模式, false=滑出模式
     * @param lineIndex 目标行索引（默认3-结果行）
     * @param duration 动画时长(ms)
     */
    CharSlideAnim(CalcDisplay* display, const String& prevText, const String& newText,
                  bool insertMode, uint8_t lineIndex = 3, unsigned long duration = 200);

    /**
     * @brief 开始动画前的准备工作
     */
    void start() override;

protected:
    /**
     * @brief 渲染动画帧
     * @param progress 动画进度 (0.0-1.0)
     */
    void renderFrame(float progress) override;

private:
    /**
     * @brief 计算动画参数
     */
    void calculateAnimationParams();
    
    /**
     * @brief 清除指定行区域
     * @param lineIndex 行索引
     */
    void clearLine(uint8_t lineIndex);
    
    /**
     * @brief 绘制静态文本
     * @param text 要绘制的文本
     * @param lineIndex 行索引
     */
    void drawStaticText(const String& text, uint8_t lineIndex);
    
    /**
     * @brief 绘制运动中的字符
     * @param character 字符
     * @param x X坐标
     * @param lineIndex 行索引
     */
    void drawMovingChar(char character, int16_t x, uint8_t lineIndex);
};