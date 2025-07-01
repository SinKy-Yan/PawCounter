#pragma once
#include "Animation.h"

/**
 * @brief 数字上移缩小动画
 * @details 实现B动效：输入区数字上移并缩小至表达式区
 */
class MoveToExprAnim : public Animation {
private:
    String _inputText;      // 要移动的输入文本
    String _finalExpr;      // 最终表达式文本
    String _operatorSuffix; // 运算符后缀
    
    // 动画起始和目标参数
    struct AnimParams {
        int16_t startY, endY;       // Y坐标
        uint8_t startSize, endSize; // 文本大小
        uint16_t startColor, endColor; // 文本颜色
    } _params;
    
    // 当前动画状态
    int16_t _currentY;
    uint8_t _currentSize;
    uint16_t _currentColor;
    
public:
    /**
     * @brief 构造函数
     * @param display 显示对象指针
     * @param inputText 输入区的文本
     * @param finalExpr 最终表达式
     * @param duration 动画时长(ms)
     */
    MoveToExprAnim(CalcDisplay* display, const String& inputText, const String& finalExpr,
                   unsigned long duration = 250);

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
     * @brief 提取运算符后缀
     */
    void extractOperatorSuffix();
    
    /**
     * @brief 插值计算
     */
    template<typename T>
    T interpolate(T start, T end, float progress) {
        return start + (T)((float)(end - start) * progress);
    }
};