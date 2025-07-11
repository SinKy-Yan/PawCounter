#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <memory>
#include <functional>
#include "LVGLDisplay.h"
#include "ChillBitmapFonts.h"
#include "ConfigManager.h"
#include "KeyboardConfig.h"

/**
 * @brief UI页面类型枚举
 */
enum class UIPage {
    CALCULATOR = 0,      ///< 计算器主页面
    SETTINGS_MAIN,       ///< 设置主菜单
    SETTINGS_KEYBOARD,   ///< 键盘设置
    SETTINGS_DISPLAY,    ///< 显示设置  
    SETTINGS_AUDIO,      ///< 音效设置
    SETTINGS_SYSTEM,     ///< 系统设置
    FONT_TEST,          ///< 字体测试
    MAX_PAGES
};

/**
 * @brief UI主题枚举
 */
enum class UITheme {
    DARK = 0,           ///< 深色主题
    LIGHT,              ///< 浅色主题
    HIGH_CONTRAST,      ///< 高对比度主题
    CUSTOM              ///< 自定义主题
};

/**
 * @brief 主题颜色配置
 */
struct ThemeColors {
    uint32_t background;        ///< 背景色
    uint32_t surface;          ///< 表面色
    uint32_t primary;          ///< 主色调
    uint32_t secondary;        ///< 次要色调
    uint32_t accent;           ///< 强调色
    uint32_t text_primary;     ///< 主要文本色
    uint32_t text_secondary;   ///< 次要文本色
    uint32_t border;           ///< 边框色
    uint32_t highlight;        ///< 高亮色
    uint32_t error;            ///< 错误色
};

/**
 * @brief 页面基类
 */
class UIPageBase {
public:
    UIPageBase(UIPage pageType) : _pageType(pageType), _screen(nullptr), _isActive(false) {}
    virtual ~UIPageBase() = default;
    
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update() {}
    virtual void handleKeyEvent(uint8_t key, bool isLongPress) {}
    
    UIPage getPageType() const { return _pageType; }
    bool isActive() const { return _isActive; }
    lv_obj_t* getScreen() const { return _screen; }
    
protected:
    UIPage _pageType;
    lv_obj_t* _screen;
    bool _isActive;
};

/**
 * @brief 设置项配置
 */
struct SettingsItem {
    String title;                           ///< 设置项标题
    String description;                     ///< 设置项描述
    String currentValue;                    ///< 当前值
    std::vector<String> options;           ///< 可选值列表
    std::function<void(int)> onChange;     ///< 值改变回调
    bool isEnabled;                        ///< 是否启用
};

/**
 * @brief UI管理器类
 */
class UIManager {
public:
    UIManager(LVGLDisplay* display);
    ~UIManager();
    
    // 初始化和管理
    bool begin();
    void update();
    void shutdown();
    
    // 页面管理
    bool switchToPage(UIPage page);
    UIPage getCurrentPage() const { return _currentPage; }
    bool goBack();
    void pushPage(UIPage page);
    
    // 主题管理
    void setTheme(UITheme theme);
    UITheme getCurrentTheme() const { return _currentTheme; }
    const ThemeColors& getThemeColors() const { return _themeColors; }
    
    // 字体管理
    const lv_font_t* getCurrentFont() const { return _currentFont; }
    void setFont(const lv_font_t* font);
    void cycleFonts();
    
    // 主题应用
    void applyThemeToScreen(lv_obj_t* screen);
    
    // 按键处理
    void handleKeyEvent(uint8_t key, bool isLongPress);
    
    // 设置页面创建
    void createSettingsPages();
    
    // 静态实例访问
    static UIManager* getInstance() { return _instance; }
    
private:
    LVGLDisplay* _display;
    UIPage _currentPage;
    UITheme _currentTheme;
    ThemeColors _themeColors;
    const lv_font_t* _currentFont;
    
    // 页面管理
    std::vector<std::unique_ptr<UIPageBase>> _pages;
    std::vector<UIPage> _pageStack;
    
    // 可用字体列表
    std::vector<const lv_font_t*> _availableFonts;
    size_t _currentFontIndex;
    
    // 静态实例
    static UIManager* _instance;
    
    // 初始化方法
    void initializeThemes();
    void initializeFonts();
    void createAllPages();
    
    // 主题相关
    void loadThemeColors(UITheme theme);
    
    // 页面查找
    UIPageBase* findPage(UIPage pageType);
    
    // 样式创建辅助方法
    void createBaseStyles();
    lv_style_t* createButtonStyle();
    lv_style_t* createLabelStyle();
    lv_style_t* createContainerStyle();
    
    // 全局样式对象
    lv_style_t _style_btn;
    lv_style_t _style_btn_pressed;
    lv_style_t _style_label;
    lv_style_t _style_container;
    lv_style_t _style_scrollbar;
};

/**
 * @brief 设置主菜单页面
 */
class SettingsMainPage : public UIPageBase {
public:
    SettingsMainPage();
    virtual ~SettingsMainPage() = default;
    
    bool create() override;
    void show() override;
    void hide() override;
    void handleKeyEvent(uint8_t key, bool isLongPress) override;
    
private:
    lv_obj_t* _menu_container;
    lv_obj_t* _title_label;
    std::vector<lv_obj_t*> _menu_buttons;
    int _selectedIndex;
    
    void createMenuItems();
    void updateSelection();
    void selectMenuItem(int index);
};

/**
 * @brief 键盘设置页面
 */
class KeyboardSettingsPage : public UIPageBase {
public:
    KeyboardSettingsPage();
    virtual ~KeyboardSettingsPage() = default;
    
    bool create() override;
    void show() override;
    void hide() override;
    void handleKeyEvent(uint8_t key, bool isLongPress) override;
    
private:
    lv_obj_t* _settings_container;
    lv_obj_t* _title_label;
    std::vector<SettingsItem> _settings;
    std::vector<lv_obj_t*> _setting_objects;
    int _selectedIndex;
    
    void createKeyboardSettings();
    void updateSettingsDisplay();
    void modifySelectedSetting(bool increase);
};

/**
 * @brief 显示设置页面
 */
class DisplaySettingsPage : public UIPageBase {
public:
    DisplaySettingsPage();
    virtual ~DisplaySettingsPage() = default;
    
    bool create() override;
    void show() override;
    void hide() override;
    void handleKeyEvent(uint8_t key, bool isLongPress) override;
    
private:
    lv_obj_t* _settings_container;
    lv_obj_t* _title_label;
    lv_obj_t* _font_preview;
    std::vector<SettingsItem> _settings;
    std::vector<lv_obj_t*> _setting_objects;
    int _selectedIndex;
    
    void createDisplaySettings();
    void updateFontPreview();
    void updateSettingsDisplay();
    void modifySelectedSetting(bool increase);
};

/**
 * @brief 音效设置页面
 */
class AudioSettingsPage : public UIPageBase {
public:
    AudioSettingsPage();
    virtual ~AudioSettingsPage() = default;
    
    bool create() override;
    void show() override;
    void hide() override;
    void handleKeyEvent(uint8_t key, bool isLongPress) override;
    
private:
    lv_obj_t* _settings_container;
    lv_obj_t* _title_label;
    std::vector<SettingsItem> _settings;
    std::vector<lv_obj_t*> _setting_objects;
    int _selectedIndex;
    
    void createAudioSettings();
    void updateSettingsDisplay();
    void modifySelectedSetting(bool increase);
    void playTestBeep();
};

/**
 * @brief 系统设置页面
 */
class SystemSettingsPage : public UIPageBase {
public:
    SystemSettingsPage();
    virtual ~SystemSettingsPage() = default;
    
    bool create() override;
    void show() override;
    void hide() override;
    void handleKeyEvent(uint8_t key, bool isLongPress) override;
    
private:
    lv_obj_t* _settings_container;
    lv_obj_t* _title_label;
    lv_obj_t* _info_container;
    std::vector<SettingsItem> _settings;
    std::vector<lv_obj_t*> _setting_objects;
    int _selectedIndex;
    
    void createSystemSettings();
    void createSystemInfo();
    void updateSettingsDisplay();
    void modifySelectedSetting(bool increase);
    void performFactoryReset();
};

// 全局UI管理器实例声明
extern UIManager* g_uiManager;

// 便捷宏定义
#define UI_MANAGER() (UIManager::getInstance())
#define CURRENT_THEME() (UI_MANAGER()->getThemeColors())
#define CURRENT_FONT() (UI_MANAGER()->getCurrentFont())