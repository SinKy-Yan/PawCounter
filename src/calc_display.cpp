#include "calc_display.h"
#include "graphics/animation/Animation.h"
#include "graphics/animation/CharSlideAnim.h"
#include "graphics/animation/MoveToExprAnim.h"
#include "graphics/animation/AnimationManager.h"
#include "graphics/animation/PerformanceMonitor.h"
#include "Logger.h"

#define TAG_CALC_DISPLAY "CalcDisp"

CalcDisplay::CalcDisplay(Arduino_GFX *d, uint16_t w, uint16_t h)
    : tft(d), screenWidth(w), screenHeight(h), _currentAnimation(nullptr) {
    
    // P1阶段：初始化动画系统 - 提升帧率到30FPS获得流畅动效
    _animationManager = new AnimationManager(3, 30);  // 最大3个并发动画，30FPS流畅动效
    _performanceMonitor = new PerformanceMonitor(30.0f, 1000);  // 30FPS目标，1秒更新间隔
    
    // 初始化历史记录
    history[0] = "";  // 最新
    history[1] = "";  // 较旧
    
    // 初始化行配置
    initializeLines();
    
    // 绘制初始界面
    drawFrame();
    refresh();
    
    LOG_I(TAG_CALC_DISPLAY, "CalcDisplay initialized with AnimationManager and PerformanceMonitor");
}

CalcDisplay::~CalcDisplay() {
    // 清理动画系统
    if (_animationManager) {
        delete _animationManager;
        _animationManager = nullptr;
    }
    
    if (_performanceMonitor) {
        delete _performanceMonitor;
        _performanceMonitor = nullptr;
    }
    
    // P0兼容性清理
    if (_currentAnimation) {
        delete _currentAnimation;
        _currentAnimation = nullptr;
    }
    
    LOG_I(TAG_CALC_DISPLAY, "CalcDisplay destroyed");
}

void CalcDisplay::initializeLines() {
    // L0: 历史第2条（最旧）- 部分隐藏营造滚动效果（硬件已扩展5像素）
    lines[0] = {history[1], 3, COLOR_HIST, -6, 24};
    
    // L1: 历史第1条（较旧）
    lines[1] = {history[0], 3, COLOR_HIST, 20, 24};
    
    // L2: 当前输入表达式
    lines[2] = {"", 3, COLOR_FG, 46, 24};
    
    // L3: 计算结果（超大字体）
    lines[3] = {"0", 8, COLOR_FG, 74, 64};
}

void CalcDisplay::drawFrame() {
    // 清屏为黑色
    tft->fillScreen(COLOR_BG);
    
    // 移除白色边框，保持纯黑色背景
    // 简洁无边框设计
}

void CalcDisplay::drawLine(uint8_t lineIndex) {
    if (lineIndex >= 4) return;
    
    LineConfig &line = lines[lineIndex];
    
    // ★ 真正的防闪烁：一次性批量写入
    tft->startWrite();
    
    // 设置文本属性（第二个参数=背景色，可省fillRect）
    tft->setTextColor(line.color, COLOR_BG);
    tft->setTextSize(line.textSize);
    tft->setCursor(PAD_X, line.y);
    tft->print(line.text);
    
    tft->endWrite();
}

void CalcDisplay::pushHistory(const String &line) {
    // 滚动历史记录：旧的向上推
    history[1] = history[0];  // 最新 → 较旧
    history[0] = line;        // 新的成为最新
    
    // 更新行配置
    lines[0].text = history[1];  // L0显示较旧的
    lines[1].text = history[0];  // L1显示最新的
    
    // 全屏刷新
    refresh();
}

void CalcDisplay::setExpr(const String &expr) {
    lines[2].text = expr;
    // 全屏刷新
    refresh();
}

void CalcDisplay::setResult(const String &res) {
    lines[3].text = res;
    // 全屏刷新
    refresh();
}

void CalcDisplay::refresh() {
    // 全屏刷新：先清屏，再重绘所有内容
    tft->fillScreen(COLOR_BG);                                    // 清屏为黑色
    
    // 绘制所有行（无边框）
    for (uint8_t i = 0; i < 4; i++) {
        drawLine(i);
    }
    
    // 如果使用Canvas，需要flush到屏幕
    // 检查tft是否为Arduino_Canvas类型
    extern Arduino_Canvas *canvas;
    if (canvas && tft == canvas) {
        canvas->flush();
    }
}

// 直接数据更新方法（不立即刷新）
void CalcDisplay::updateHistoryDirect(const String &latest, const String &older) {
    history[0] = latest;
    history[1] = older;
    lines[0].text = history[1];  // L0显示较旧的
    lines[1].text = history[0];  // L1显示最新的
}

void CalcDisplay::updateExprDirect(const String &expr) {
    lines[2].text = expr;
}

void CalcDisplay::updateResultDirect(const String &res) {
    lines[3].text = res;
}

// 动画tick更新 - P1阶段：集成AnimationManager和PerformanceMonitor
void CalcDisplay::tick() {
    // 性能监控：开始帧
    if (_performanceMonitor) {
        _performanceMonitor->beginFrame();
    }
    
    // 更新动画管理器
    uint8_t activeAnimCount = 0;
    if (_animationManager) {
        // 简单的垂直同步等待，减少闪烁
        waitForVSync();
        
        activeAnimCount = _animationManager->tick();
        
        // 根据性能监控调整参数
        if (_performanceMonitor) {
            if (_performanceMonitor->shouldReduceFrameRate()) {
                uint8_t recommendedFPS = _performanceMonitor->getRecommendedFPS();
                _animationManager->setTargetFPS(recommendedFPS);
            }
            
            if (_performanceMonitor->shouldReduceAnimations()) {
                uint8_t maxAnims = _performanceMonitor->getRecommendedMaxAnimations();
                _animationManager->setMaxConcurrentAnimations(maxAnims);
            }
            
            if (_performanceMonitor->shouldDisableAnimations()) {
                _animationManager->interruptAllAnimations();
            }
        }
    }
    
    // P0兼容性：处理旧的单一动画（逐步迁移）
    if (_currentAnimation != nullptr) {
        bool continueAnimation = _currentAnimation->tick();
        if (!continueAnimation) {
            delete _currentAnimation;
            _currentAnimation = nullptr;
        }
    }
    
    // 性能监控：结束帧并更新
    if (_performanceMonitor) {
        _performanceMonitor->endFrame();
        _performanceMonitor->update(activeAnimCount);
    }

    // --- 推送 Canvas 缓冲到屏幕 ---
    extern Arduino_Canvas *canvas;
    if (canvas && tft == canvas) {
        canvas->flush();
    }
}

// A1/A2动画：字符滑入/滑出 - P1阶段：使用AnimationManager
void CalcDisplay::animateInputChange(const String& oldTxt, const String& newTxt) {
    if (!_animationManager) {
        LOG_E(TAG_CALC_DISPLAY, "AnimationManager not initialized");
        return;
    }
    
    // 判断是滑入还是滑出
    bool isInsert = newTxt.length() > oldTxt.length();
    
    // 创建新动画
    Animation* newAnim = new CharSlideAnim(this, oldTxt, newTxt, isInsert, 3, 200);
    
    // 添加到动画管理器
    bool added = _animationManager->addAnimation(newAnim, true);
    if (!added) {
        LOG_W(TAG_CALC_DISPLAY, "Failed to add CharSlideAnim to AnimationManager");
        delete newAnim;  // 清理资源
    } else {
        LOG_D(TAG_CALC_DISPLAY, "CharSlideAnim added: %s -> %s", oldTxt.c_str(), newTxt.c_str());
    }
}

// B动画：数字上移缩小到表达式区 - P1阶段：使用AnimationManager
void CalcDisplay::animateMoveInputToExpr(const String& inputTxt, const String& finalExpr) {
    if (!_animationManager) {
        LOG_E(TAG_CALC_DISPLAY, "AnimationManager not initialized");
        return;
    }
    
    // 创建上移缩小动画
    Animation* newAnim = new MoveToExprAnim(this, inputTxt, finalExpr, 250);
    
    // 添加到动画管理器
    bool added = _animationManager->addAnimation(newAnim, true);
    if (!added) {
        LOG_W(TAG_CALC_DISPLAY, "Failed to add MoveToExprAnim to AnimationManager");
        delete newAnim;  // 清理资源
    } else {
        LOG_D(TAG_CALC_DISPLAY, "MoveToExprAnim added: %s -> %s", inputTxt.c_str(), finalExpr.c_str());
    }
}

// 动画辅助方法
void CalcDisplay::clearLineArea(uint8_t lineIndex, bool inWriteBatch) {
    if (lineIndex >= 4) return;
    
    LineConfig &line = lines[lineIndex];
    
    // 计算行区域
    int16_t x = PAD_X;
    int16_t y = line.y;
    uint16_t w = screenWidth - PAD_X * 2;
    uint16_t h = line.charHeight;
    
    // 根据是否在批量写入中决定是否包装startWrite/endWrite
    if (!inWriteBatch) {
        tft->startWrite();
    }
    
    // 清除区域
    tft->fillRect(x, y, w, h, COLOR_BG);
    
    if (!inWriteBatch) {
        tft->endWrite();
    }
}

uint16_t CalcDisplay::getCharWidth(uint8_t textSize) {
    // Arduino_GFX标准字符宽度：6像素 * textSize
    return 6 * textSize;
}

// P1阶段：新增动画管理方法
void CalcDisplay::interruptCurrentAnimation() {
    if (_animationManager) {
        _animationManager->interruptAllAnimations();
        LOG_D(TAG_CALC_DISPLAY, "All animations interrupted");
    }
    
    // P0兼容性
    if (_currentAnimation) {
        _currentAnimation->interrupt();
        delete _currentAnimation;
        _currentAnimation = nullptr;
    }
}

bool CalcDisplay::hasActiveAnimation() const {
    bool hasManagerAnims = _animationManager && _animationManager->getActiveAnimationCount() > 0;
    bool hasLegacyAnim = _currentAnimation != nullptr;
    return hasManagerAnims || hasLegacyAnim;
}

uint8_t CalcDisplay::getActiveAnimationCount() const {
    uint8_t count = 0;
    
    if (_animationManager) {
        count += _animationManager->getActiveAnimationCount();
    }
    
    if (_currentAnimation) {
        count += 1;  // P0兼容性
    }
    
    return count;
}

// P1阶段：简单的垂直同步等待，减少闪烁
void CalcDisplay::waitForVSync() {
    // 简单的延时同步，避免在LCD刷新中途更新
    // 对于大部分LCD，刷新率约60Hz，我们等待刷新间隔
    static unsigned long lastVSync = 0;
    unsigned long currentTime = millis();
    
    // 确保每帧间隔至少16ms（约60FPS上限）
    if (currentTime - lastVSync < 16) {
        delay(1);  // 短暂延时
    }
    
    lastVSync = currentTime;
}