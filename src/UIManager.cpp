#include "UIManager.h"
#include "Logger.h"
#include "all_fonts.h"

#define TAG_UI "UIManager"

// 静态实例指针
UIManager* UIManager::_instance = nullptr;
UIManager* g_uiManager = nullptr;

// 主题颜色定义
static const ThemeColors DARK_THEME = {
    .background = 0x000000,      // 黑色背景
    .surface = 0x1a1a1a,         // 深灰表面
    .primary = 0x2196F3,         // 蓝色主调
    .secondary = 0x757575,       // 灰色次要
    .accent = 0xFF9800,          // 橙色强调
    .text_primary = 0xFFFFFF,    // 白色主文本
    .text_secondary = 0xBBBBBB,  // 浅灰次文本
    .border = 0x333333,          // 深灰边框
    .highlight = 0x42A5F5,       // 浅蓝高亮
    .error = 0xF44336            // 红色错误
};

static const ThemeColors LIGHT_THEME = {
    .background = 0xFFFFFF,      // 白色背景
    .surface = 0xF5F5F5,         // 浅灰表面
    .primary = 0x1976D2,         // 深蓝主调
    .secondary = 0x616161,       // 深灰次要
    .accent = 0xFF6F00,          // 深橙强调
    .text_primary = 0x212121,    // 深灰主文本
    .text_secondary = 0x757575,  // 灰色次文本
    .border = 0xE0E0E0,          // 浅灰边框
    .highlight = 0x1E88E5,       // 蓝色高亮
    .error = 0xD32F2F            // 深红错误
};

static const ThemeColors HIGH_CONTRAST_THEME = {
    .background = 0x000000,      // 纯黑背景
    .surface = 0x000000,         // 纯黑表面
    .primary = 0xFFFF00,         // 黄色主调
    .secondary = 0xFFFFFF,       // 白色次要
    .accent = 0x00FFFF,          // 青色强调
    .text_primary = 0xFFFFFF,    // 白色主文本
    .text_secondary = 0xFFFF00,  // 黄色次文本
    .border = 0xFFFFFF,          // 白色边框
    .highlight = 0x00FF00,       // 绿色高亮
    .error = 0xFF0000            // 红色错误
};

UIManager::UIManager(LVGLDisplay* display) 
    : _display(display), _currentPage(UIPage::CALCULATOR), 
      _currentTheme(UITheme::DARK), _currentFont(&WenQuanYi_Bitmap_Song_39px), 
      _currentFontIndex(0) {
    _instance = this;
    g_uiManager = this;
}

UIManager::~UIManager() {
    shutdown();
    _instance = nullptr;
    g_uiManager = nullptr;
}

bool UIManager::begin() {
    if (!_display) {
        LOG_E(TAG_UI, "显示指针为空");
        return false;
    }
    
    LOG_I(TAG_UI, "初始化UI管理器...");
    
    // 初始化主题和字体
    initializeThemes();
    initializeFonts();
    
    // 创建基础样式
    createBaseStyles();
    
    // 创建所有页面
    createAllPages();
    
    // 切换到默认页面
    switchToPage(UIPage::CALCULATOR);
    
    LOG_I(TAG_UI, "UI管理器初始化完成");
    return true;
}

void UIManager::update() {
    // 更新当前活动页面
    if (!_pages.empty()) {
        for (auto& page : _pages) {
            if (page->isActive()) {
                page->update();
                break;
            }
        }
    }
}

void UIManager::shutdown() {
    // 清理所有页面
    _pages.clear();
    _pageStack.clear();
}

bool UIManager::switchToPage(UIPage page) {
    LOG_I(TAG_UI, "切换到页面: %d", static_cast<int>(page));
    
    // 隐藏当前页面
    UIPageBase* currentPageObj = findPage(_currentPage);
    if (currentPageObj) {
        currentPageObj->hide();
    }
    
    // 特殊处理CALCULATOR页面，使用原有的CalculatorCore UI
    if (page == UIPage::CALCULATOR) {
        LOG_I(TAG_UI, "切换到计算器页面（使用CalculatorCore UI）");
        _currentPage = page;
        return true;
    }
    
    // 显示新页面
    UIPageBase* newPageObj = findPage(page);
    if (!newPageObj) {
        LOG_E(TAG_UI, "页面 %d 不存在", static_cast<int>(page));
        return false;
    }
    
    newPageObj->show();
    _currentPage = page;
    
    return true;
}

bool UIManager::goBack() {
    if (_pageStack.empty()) {
        return false;
    }
    
    UIPage previousPage = _pageStack.back();
    _pageStack.pop_back();
    
    return switchToPage(previousPage);
}

void UIManager::pushPage(UIPage page) {
    _pageStack.push_back(_currentPage);
    switchToPage(page);
}

void UIManager::setTheme(UITheme theme) {
    _currentTheme = theme;
    loadThemeColors(theme);
    
    // 重新应用主题到所有页面
    for (auto& page : _pages) {
        if (page->getScreen()) {
            applyThemeToScreen(page->getScreen());
        }
    }
    
    LOG_I(TAG_UI, "切换主题: %d", static_cast<int>(theme));
}

void UIManager::setFont(const lv_font_t* font) {
    if (font) {
        _currentFont = font;
        LOG_I(TAG_UI, "切换字体");
        
        // 触发所有页面重新渲染以应用新字体
        for (auto& page : _pages) {
            if (page->getScreen()) {
                lv_obj_invalidate(page->getScreen());
            }
        }
    }
}

void UIManager::cycleFonts() {
    if (_availableFonts.empty()) return;
    
    _currentFontIndex = (_currentFontIndex + 1) % _availableFonts.size();
    setFont(_availableFonts[_currentFontIndex]);
}

void UIManager::handleKeyEvent(uint8_t key, bool isLongPress) {
    // 将按键事件传递给当前活动页面
    UIPageBase* currentPageObj = findPage(_currentPage);
    if (currentPageObj) {
        currentPageObj->handleKeyEvent(key, isLongPress);
    }
}

void UIManager::initializeThemes() {
    loadThemeColors(_currentTheme);
}

void UIManager::initializeFonts() {
    // 添加可用字体
    _availableFonts.clear();
    _availableFonts.push_back(&WenQuanYi_Bitmap_Song_39px);
    
    _currentFont = _availableFonts[0];
    _currentFontIndex = 0;
    
    LOG_I(TAG_UI, "已加载 %d 个字体", _availableFonts.size());
}

void UIManager::createAllPages() {
    LOG_I(TAG_UI, "创建所有UI页面...");
    
    // 创建设置主菜单页面
    auto settingsMain = std::make_unique<SettingsMainPage>();
    if (settingsMain->create()) {
        _pages.push_back(std::move(settingsMain));
    }
    
    // 创建键盘设置页面
    auto keyboardSettings = std::make_unique<KeyboardSettingsPage>();
    if (keyboardSettings->create()) {
        _pages.push_back(std::move(keyboardSettings));
    }
    
    // 创建显示设置页面
    auto displaySettings = std::make_unique<DisplaySettingsPage>();
    if (displaySettings->create()) {
        _pages.push_back(std::move(displaySettings));
    }
    
    // 创建音效设置页面
    auto audioSettings = std::make_unique<AudioSettingsPage>();
    if (audioSettings->create()) {
        _pages.push_back(std::move(audioSettings));
    }
    
    // 创建系统设置页面
    auto systemSettings = std::make_unique<SystemSettingsPage>();
    if (systemSettings->create()) {
        _pages.push_back(std::move(systemSettings));
    }
    
    LOG_I(TAG_UI, "已创建 %d 个页面", _pages.size());
}

void UIManager::loadThemeColors(UITheme theme) {
    switch (theme) {
        case UITheme::DARK:
            _themeColors = DARK_THEME;
            break;
        case UITheme::LIGHT:
            _themeColors = LIGHT_THEME;
            break;
        case UITheme::HIGH_CONTRAST:
            _themeColors = HIGH_CONTRAST_THEME;
            break;
        case UITheme::CUSTOM:
            // 加载自定义主题配置
            _themeColors = DARK_THEME; // 默认使用深色主题
            break;
        default:
            _themeColors = DARK_THEME;
            break;
    }
}

void UIManager::applyThemeToScreen(lv_obj_t* screen) {
    if (!screen) return;
    
    lv_obj_set_style_bg_color(screen, lv_color_hex(_themeColors.background), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
}

UIPageBase* UIManager::findPage(UIPage pageType) {
    for (auto& page : _pages) {
        if (page->getPageType() == pageType) {
            return page.get();
        }
    }
    return nullptr;
}

void UIManager::createBaseStyles() {
    // 初始化按钮样式
    lv_style_init(&_style_btn);
    lv_style_set_bg_color(&_style_btn, lv_color_hex(_themeColors.surface));
    lv_style_set_bg_opa(&_style_btn, LV_OPA_COVER);
    lv_style_set_border_color(&_style_btn, lv_color_hex(_themeColors.border));
    lv_style_set_border_width(&_style_btn, 1);
    lv_style_set_text_color(&_style_btn, lv_color_hex(_themeColors.text_primary));
    lv_style_set_pad_all(&_style_btn, 8);
    lv_style_set_radius(&_style_btn, 4);
    
    // 初始化按钮按下样式
    lv_style_init(&_style_btn_pressed);
    lv_style_set_bg_color(&_style_btn_pressed, lv_color_hex(_themeColors.primary));
    lv_style_set_text_color(&_style_btn_pressed, lv_color_hex(_themeColors.text_primary));
    
    // 初始化标签样式
    lv_style_init(&_style_label);
    lv_style_set_text_color(&_style_label, lv_color_hex(_themeColors.text_primary));
    lv_style_set_text_font(&_style_label, _currentFont);
    
    // 初始化容器样式
    lv_style_init(&_style_container);
    lv_style_set_bg_color(&_style_container, lv_color_hex(_themeColors.surface));
    lv_style_set_bg_opa(&_style_container, LV_OPA_COVER);
    lv_style_set_border_width(&_style_container, 0);
    lv_style_set_pad_all(&_style_container, 5);
    
    // 初始化滚动条样式（隐藏滚动条）
    lv_style_init(&_style_scrollbar);
    lv_style_set_width(&_style_scrollbar, 0);
    lv_style_set_bg_opa(&_style_scrollbar, LV_OPA_TRANSP);
}

// ===================== 设置主菜单页面实现 =====================

SettingsMainPage::SettingsMainPage() : UIPageBase(UIPage::SETTINGS_MAIN), _selectedIndex(0) {}

bool SettingsMainPage::create() {
    _screen = lv_obj_create(NULL);
    if (!_screen) {
        return false;
    }
    
    // 应用主题
    UI_MANAGER()->applyThemeToScreen(_screen);
    
    // 创建标题
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_label_set_text(_title_label, "设置");
    lv_obj_set_style_text_color(_title_label, lv_color_hex(CURRENT_THEME().primary), LV_PART_MAIN);
    lv_obj_set_style_text_font(_title_label, CURRENT_FONT(), LV_PART_MAIN);
    
    // 创建菜单容器
    _menu_container = lv_obj_create(_screen);
    lv_obj_set_pos(_menu_container, 5, 30);
    lv_obj_set_size(_menu_container, 230, 90);
    lv_obj_set_style_bg_opa(_menu_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_menu_container, 0, LV_PART_MAIN);
    lv_obj_add_style(_menu_container, &UI_MANAGER()->_style_scrollbar, LV_PART_SCROLLBAR);
    
    createMenuItems();
    updateSelection();
    
    return true;
}

void SettingsMainPage::show() {
    if (_screen) {
        lv_scr_load(_screen);
        _isActive = true;
        updateSelection();
    }
}

void SettingsMainPage::hide() {
    _isActive = false;
}

void SettingsMainPage::handleKeyEvent(uint8_t key, bool isLongPress) {
    switch (key) {
        case 2: // 上键
            _selectedIndex = (_selectedIndex - 1 + 4) % 4;
            updateSelection();
            break;
        case 8: // 下键
            _selectedIndex = (_selectedIndex + 1) % 4;
            updateSelection();
            break;
        case 21: // 确认键
            selectMenuItem(_selectedIndex);
            break;
        case 22: // 返回键
            UI_MANAGER()->switchToPage(UIPage::CALCULATOR);
            break;
    }
}

void SettingsMainPage::createMenuItems() {
    const char* menuItems[] = {
        "Keyboard",
        "Display", 
        "Audio",
        "System"
    };
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_btn_create(_menu_container);
        lv_obj_set_pos(btn, 10, i * 20);
        lv_obj_set_size(btn, 200, 18);
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, menuItems[i]);
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, CURRENT_FONT(), LV_PART_MAIN);
        
        _menu_buttons.push_back(btn);
    }
}

void SettingsMainPage::updateSelection() {
    for (int i = 0; i < _menu_buttons.size(); i++) {
        if (i == _selectedIndex) {
            lv_obj_set_style_bg_color(_menu_buttons[i], lv_color_hex(CURRENT_THEME().highlight), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(_menu_buttons[i], lv_color_hex(CURRENT_THEME().surface), LV_PART_MAIN);
        }
    }
}

void SettingsMainPage::selectMenuItem(int index) {
    switch (index) {
        case 0:
            UI_MANAGER()->pushPage(UIPage::SETTINGS_KEYBOARD);
            break;
        case 1:
            UI_MANAGER()->pushPage(UIPage::SETTINGS_DISPLAY);
            break;
        case 2:
            UI_MANAGER()->pushPage(UIPage::SETTINGS_AUDIO);
            break;
        case 3:
            UI_MANAGER()->pushPage(UIPage::SETTINGS_SYSTEM);
            break;
    }
}

// ===================== 键盘设置页面实现 =====================

KeyboardSettingsPage::KeyboardSettingsPage() : UIPageBase(UIPage::SETTINGS_KEYBOARD), _selectedIndex(0) {}

bool KeyboardSettingsPage::create() {
    _screen = lv_obj_create(NULL);
    if (!_screen) {
        return false;
    }
    
    UI_MANAGER()->applyThemeToScreen(_screen);
    
    // 创建标题
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_label_set_text(_title_label, "Keyboard Settings");
    lv_obj_set_style_text_color(_title_label, lv_color_hex(CURRENT_THEME().primary), LV_PART_MAIN);
    lv_obj_set_style_text_font(_title_label, CURRENT_FONT(), LV_PART_MAIN);
    
    // 创建设置容器
    _settings_container = lv_obj_create(_screen);
    lv_obj_set_pos(_settings_container, 5, 25);
    lv_obj_set_size(_settings_container, 230, 95);
    lv_obj_set_style_bg_opa(_settings_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_settings_container, 0, LV_PART_MAIN);
    lv_obj_add_style(_settings_container, &UI_MANAGER()->_style_scrollbar, LV_PART_SCROLLBAR);
    
    createKeyboardSettings();
    updateSettingsDisplay();
    
    return true;
}

void KeyboardSettingsPage::show() {
    if (_screen) {
        lv_scr_load(_screen);
        _isActive = true;
    }
}

void KeyboardSettingsPage::hide() {
    _isActive = false;
}

void KeyboardSettingsPage::handleKeyEvent(uint8_t key, bool isLongPress) {
    switch (key) {
        case 2: // 上键
            _selectedIndex = (_selectedIndex - 1 + _settings.size()) % _settings.size();
            updateSettingsDisplay();
            break;
        case 8: // 下键
            _selectedIndex = (_selectedIndex + 1) % _settings.size();
            updateSettingsDisplay();
            break;
        case 4: // 左键
            modifySelectedSetting(false);
            break;
        case 6: // 右键
            modifySelectedSetting(true);
            break;
        case 22: // 返回键
            UI_MANAGER()->goBack();
            break;
    }
}

void KeyboardSettingsPage::createKeyboardSettings() {
    ConfigManager& config = ConfigManager::getInstance();
    
    // 按键重复延迟
    SettingsItem repeatDelay = {
        .title = "Repeat Delay",
        .description = "Key repeat trigger delay",
        .currentValue = String(config.getRepeatDelay()) + "ms",
        .options = {"250ms", "500ms", "750ms", "1000ms"},
        .onChange = [&config](int index) {
            uint16_t values[] = {250, 500, 750, 1000};
            config.setRepeatDelay(values[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(repeatDelay);
    
    // 按键重复速率
    SettingsItem repeatRate = {
        .title = "Repeat Rate",
        .description = "Key repeat trigger rate",
        .currentValue = String(config.getRepeatRate()) + "ms",
        .options = {"50ms", "100ms", "150ms", "200ms"},
        .onChange = [&config](int index) {
            uint16_t values[] = {50, 100, 150, 200};
            config.setRepeatRate(values[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(repeatRate);
    
    // 长按延迟
    SettingsItem longPressDelay = {
        .title = "Long Press",
        .description = "Long press trigger delay",
        .currentValue = String(config.getLongPressDelay()) + "ms",
        .options = {"500ms", "1000ms", "1500ms", "2000ms"},
        .onChange = [&config](int index) {
            uint16_t values[] = {500, 1000, 1500, 2000};
            config.setLongPressDelay(values[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(longPressDelay);
}

void KeyboardSettingsPage::updateSettingsDisplay() {
    // 清除现有显示对象
    _setting_objects.clear();
    lv_obj_clean(_settings_container);
    
    // 重新创建设置项显示
    for (size_t i = 0; i < _settings.size(); i++) {
        lv_obj_t* item_container = lv_obj_create(_settings_container);
        lv_obj_set_pos(item_container, 5, i * 25);
        lv_obj_set_size(item_container, 220, 22);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);
        
        if (i == _selectedIndex) {
            lv_obj_set_style_bg_opa(item_container, LV_OPA_20, LV_PART_MAIN);
            lv_obj_set_style_bg_color(item_container, lv_color_hex(CURRENT_THEME().highlight), LV_PART_MAIN);
        }
        
        // 标题标签
        lv_obj_t* title_label = lv_label_create(item_container);
        lv_obj_set_pos(title_label, 5, 2);
        lv_label_set_text(title_label, _settings[i].title.c_str());
        lv_obj_set_style_text_font(title_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(CURRENT_THEME().text_primary), LV_PART_MAIN);
        
        // 值标签
        lv_obj_t* value_label = lv_label_create(item_container);
        lv_obj_set_pos(value_label, 160, 2);
        lv_label_set_text(value_label, _settings[i].currentValue.c_str());
        lv_obj_set_style_text_font(value_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(value_label, lv_color_hex(CURRENT_THEME().accent), LV_PART_MAIN);
        
        _setting_objects.push_back(item_container);
    }
}

void KeyboardSettingsPage::modifySelectedSetting(bool increase) {
    if (_selectedIndex >= _settings.size()) return;
    
    // 这里可以添加设置值修改逻辑
    LOG_I(TAG_UI, "修改设置项 %d, 增加: %s", _selectedIndex, increase ? "是" : "否");
    
    // 触发设置保存
    ConfigManager::getInstance().save();
    updateSettingsDisplay();
}

// ===================== 显示设置页面实现 =====================

DisplaySettingsPage::DisplaySettingsPage() : UIPageBase(UIPage::SETTINGS_DISPLAY), _selectedIndex(0) {}

bool DisplaySettingsPage::create() {
    _screen = lv_obj_create(NULL);
    if (!_screen) {
        return false;
    }
    
    UI_MANAGER()->applyThemeToScreen(_screen);
    
    // 创建标题
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_label_set_text(_title_label, "Display Settings");
    lv_obj_set_style_text_color(_title_label, lv_color_hex(CURRENT_THEME().primary), LV_PART_MAIN);
    lv_obj_set_style_text_font(_title_label, CURRENT_FONT(), LV_PART_MAIN);
    
    // 创建字体预览
    _font_preview = lv_label_create(_screen);
    lv_obj_set_pos(_font_preview, 10, 25);
    lv_label_set_text(_font_preview, "字体预览: 123+456=579");
    lv_obj_set_style_text_font(_font_preview, CURRENT_FONT(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_font_preview, lv_color_hex(CURRENT_THEME().text_primary), LV_PART_MAIN);
    
    // 创建设置容器
    _settings_container = lv_obj_create(_screen);
    lv_obj_set_pos(_settings_container, 5, 45);
    lv_obj_set_size(_settings_container, 230, 75);
    lv_obj_set_style_bg_opa(_settings_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_settings_container, 0, LV_PART_MAIN);
    lv_obj_add_style(_settings_container, &UI_MANAGER()->_style_scrollbar, LV_PART_SCROLLBAR);
    
    createDisplaySettings();
    updateSettingsDisplay();
    
    return true;
}

void DisplaySettingsPage::show() {
    if (_screen) {
        lv_scr_load(_screen);
        _isActive = true;
        updateFontPreview();
    }
}

void DisplaySettingsPage::hide() {
    _isActive = false;
}

void DisplaySettingsPage::handleKeyEvent(uint8_t key, bool isLongPress) {
    switch (key) {
        case 2: // 上键
            _selectedIndex = (_selectedIndex - 1 + _settings.size()) % _settings.size();
            updateSettingsDisplay();
            break;
        case 8: // 下键
            _selectedIndex = (_selectedIndex + 1) % _settings.size();
            updateSettingsDisplay();
            break;
        case 4: // 左键
            modifySelectedSetting(false);
            break;
        case 6: // 右键
            modifySelectedSetting(true);
            break;
        case 21: // 确认键
            if (_selectedIndex == 0) { // 字体切换
                UI_MANAGER()->cycleFonts();
                updateFontPreview();
            }
            break;
        case 22: // 返回键
            UI_MANAGER()->goBack();
            break;
    }
}

void DisplaySettingsPage::createDisplaySettings() {
    ConfigManager& config = ConfigManager::getInstance();
    
    // 字体选择
    SettingsItem fontSetting = {
        .title = "Font",
        .description = "Select display font",
        .currentValue = "Default Font",
        .options = {"Default Font", "Chill 7px", "Chill 16px"},
        .onChange = [](int index) {
            // 字体切换在按键处理中实现
        },
        .isEnabled = true
    };
    _settings.push_back(fontSetting);
    
    // 背光亮度
    SettingsItem brightness = {
        .title = "Backlight",
        .description = "Adjust backlight brightness",
        .currentValue = String(config.getBacklightBrightness()) + "%",
        .options = {"25%", "50%", "75%", "100%"},
        .onChange = [&config](int index) {
            uint8_t values[] = {25, 50, 75, 100};
            config.setBacklightBrightness(values[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(brightness);
    
    // 主题选择
    SettingsItem theme = {
        .title = "Theme",
        .description = "Select UI theme",
        .currentValue = "Dark Theme",
        .options = {"Dark Theme", "Light Theme", "High Contrast"},
        .onChange = [](int index) {
            UITheme themes[] = {UITheme::DARK, UITheme::LIGHT, UITheme::HIGH_CONTRAST};
            UI_MANAGER()->setTheme(themes[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(theme);
}

void DisplaySettingsPage::updateFontPreview() {
    if (_font_preview) {
        lv_obj_set_style_text_font(_font_preview, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_invalidate(_font_preview);
    }
}

void DisplaySettingsPage::updateSettingsDisplay() {
    // 实现与KeyboardSettingsPage类似的显示更新逻辑
    _setting_objects.clear();
    lv_obj_clean(_settings_container);
    
    for (size_t i = 0; i < _settings.size(); i++) {
        lv_obj_t* item_container = lv_obj_create(_settings_container);
        lv_obj_set_pos(item_container, 5, i * 22);
        lv_obj_set_size(item_container, 220, 20);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);
        
        if (i == _selectedIndex) {
            lv_obj_set_style_bg_opa(item_container, LV_OPA_20, LV_PART_MAIN);
            lv_obj_set_style_bg_color(item_container, lv_color_hex(CURRENT_THEME().highlight), LV_PART_MAIN);
        }
        
        lv_obj_t* title_label = lv_label_create(item_container);
        lv_obj_set_pos(title_label, 5, 2);
        lv_label_set_text(title_label, _settings[i].title.c_str());
        lv_obj_set_style_text_font(title_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(CURRENT_THEME().text_primary), LV_PART_MAIN);
        
        lv_obj_t* value_label = lv_label_create(item_container);
        lv_obj_set_pos(value_label, 160, 2);
        lv_label_set_text(value_label, _settings[i].currentValue.c_str());
        lv_obj_set_style_text_font(value_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(value_label, lv_color_hex(CURRENT_THEME().accent), LV_PART_MAIN);
        
        _setting_objects.push_back(item_container);
    }
}

void DisplaySettingsPage::modifySelectedSetting(bool increase) {
    LOG_I(TAG_UI, "修改显示设置项 %d", _selectedIndex);
    updateSettingsDisplay();
}

// ===================== 音效设置页面实现 =====================

AudioSettingsPage::AudioSettingsPage() : UIPageBase(UIPage::SETTINGS_AUDIO), _selectedIndex(0) {}

bool AudioSettingsPage::create() {
    _screen = lv_obj_create(NULL);
    if (!_screen) {
        return false;
    }
    
    UI_MANAGER()->applyThemeToScreen(_screen);
    
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_label_set_text(_title_label, "Audio Settings");
    lv_obj_set_style_text_color(_title_label, lv_color_hex(CURRENT_THEME().primary), LV_PART_MAIN);
    lv_obj_set_style_text_font(_title_label, CURRENT_FONT(), LV_PART_MAIN);
    
    _settings_container = lv_obj_create(_screen);
    lv_obj_set_pos(_settings_container, 5, 25);
    lv_obj_set_size(_settings_container, 230, 95);
    lv_obj_set_style_bg_opa(_settings_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_settings_container, 0, LV_PART_MAIN);
    lv_obj_add_style(_settings_container, &UI_MANAGER()->_style_scrollbar, LV_PART_SCROLLBAR);
    
    createAudioSettings();
    updateSettingsDisplay();
    
    return true;
}

void AudioSettingsPage::show() {
    if (_screen) {
        lv_scr_load(_screen);
        _isActive = true;
    }
}

void AudioSettingsPage::hide() {
    _isActive = false;
}

void AudioSettingsPage::handleKeyEvent(uint8_t key, bool isLongPress) {
    switch (key) {
        case 2: // 上键
            _selectedIndex = (_selectedIndex - 1 + _settings.size()) % _settings.size();
            updateSettingsDisplay();
            break;
        case 8: // 下键
            _selectedIndex = (_selectedIndex + 1) % _settings.size();
            updateSettingsDisplay();
            break;
        case 4: // 左键
            modifySelectedSetting(false);
            break;
        case 6: // 右键
            modifySelectedSetting(true);
            break;
        case 21: // 确认键
            playTestBeep();
            break;
        case 22: // 返回键
            UI_MANAGER()->goBack();
            break;
    }
}

void AudioSettingsPage::createAudioSettings() {
    ConfigManager& config = ConfigManager::getInstance();
    
    // 蜂鸣器开关
    SettingsItem buzzerEnabled = {
        .title = "Buzzer",
        .description = "Enable/disable buzzer",
        .currentValue = config.getBuzzerEnabled() ? "Enabled" : "Disabled",
        .options = {"Disabled", "Enabled"},
        .onChange = [&config](int index) {
            config.setBuzzerEnabled(index == 1);
        },
        .isEnabled = true
    };
    _settings.push_back(buzzerEnabled);
    
    // 音量设置
    const char* volumeNames[] = {"Mute", "Low", "Medium", "High"};
    SettingsItem volume = {
        .title = "Volume",
        .description = "Adjust buzzer volume",
        .currentValue = volumeNames[config.getBuzzerVolume()],
        .options = {"Mute", "Low", "Medium", "High"},
        .onChange = [&config](int index) {
            config.setBuzzerVolume(index);
        },
        .isEnabled = config.getBuzzerEnabled()
    };
    _settings.push_back(volume);
}

void AudioSettingsPage::updateSettingsDisplay() {
    // 实现音效设置显示更新
    _setting_objects.clear();
    lv_obj_clean(_settings_container);
    
    for (size_t i = 0; i < _settings.size(); i++) {
        lv_obj_t* item_container = lv_obj_create(_settings_container);
        lv_obj_set_pos(item_container, 5, i * 25);
        lv_obj_set_size(item_container, 220, 22);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);
        
        if (i == _selectedIndex) {
            lv_obj_set_style_bg_opa(item_container, LV_OPA_20, LV_PART_MAIN);
            lv_obj_set_style_bg_color(item_container, lv_color_hex(CURRENT_THEME().highlight), LV_PART_MAIN);
        }
        
        lv_obj_t* title_label = lv_label_create(item_container);
        lv_obj_set_pos(title_label, 5, 2);
        lv_label_set_text(title_label, _settings[i].title.c_str());
        lv_obj_set_style_text_font(title_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(CURRENT_THEME().text_primary), LV_PART_MAIN);
        
        lv_obj_t* value_label = lv_label_create(item_container);
        lv_obj_set_pos(value_label, 160, 2);
        lv_label_set_text(value_label, _settings[i].currentValue.c_str());
        lv_obj_set_style_text_font(value_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(value_label, lv_color_hex(CURRENT_THEME().accent), LV_PART_MAIN);
        
        _setting_objects.push_back(item_container);
    }
}

void AudioSettingsPage::modifySelectedSetting(bool increase) {
    LOG_I(TAG_UI, "修改音效设置项 %d", _selectedIndex);
    updateSettingsDisplay();
}

void AudioSettingsPage::playTestBeep() {
    LOG_I(TAG_UI, "播放测试蜂鸣音");
    // 这里应该调用蜂鸣器播放测试音
}

// ===================== 系统设置页面实现 =====================

SystemSettingsPage::SystemSettingsPage() : UIPageBase(UIPage::SETTINGS_SYSTEM), _selectedIndex(0) {}

bool SystemSettingsPage::create() {
    _screen = lv_obj_create(NULL);
    if (!_screen) {
        return false;
    }
    
    UI_MANAGER()->applyThemeToScreen(_screen);
    
    _title_label = lv_label_create(_screen);
    lv_obj_set_pos(_title_label, 10, 5);
    lv_label_set_text(_title_label, "System Settings");
    lv_obj_set_style_text_color(_title_label, lv_color_hex(CURRENT_THEME().primary), LV_PART_MAIN);
    lv_obj_set_style_text_font(_title_label, CURRENT_FONT(), LV_PART_MAIN);
    
    _settings_container = lv_obj_create(_screen);
    lv_obj_set_pos(_settings_container, 5, 25);
    lv_obj_set_size(_settings_container, 230, 70);
    lv_obj_set_style_bg_opa(_settings_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_settings_container, 0, LV_PART_MAIN);
    lv_obj_add_style(_settings_container, &UI_MANAGER()->_style_scrollbar, LV_PART_SCROLLBAR);
    
    _info_container = lv_obj_create(_screen);
    lv_obj_set_pos(_info_container, 5, 100);
    lv_obj_set_size(_info_container, 230, 20);
    lv_obj_set_style_bg_opa(_info_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(_info_container, 0, LV_PART_MAIN);
    
    createSystemSettings();
    createSystemInfo();
    updateSettingsDisplay();
    
    return true;
}

void SystemSettingsPage::show() {
    if (_screen) {
        lv_scr_load(_screen);
        _isActive = true;
    }
}

void SystemSettingsPage::hide() {
    _isActive = false;
}

void SystemSettingsPage::handleKeyEvent(uint8_t key, bool isLongPress) {
    switch (key) {
        case 2: // 上键
            _selectedIndex = (_selectedIndex - 1 + _settings.size()) % _settings.size();
            updateSettingsDisplay();
            break;
        case 8: // 下键
            _selectedIndex = (_selectedIndex + 1) % _settings.size();
            updateSettingsDisplay();
            break;
        case 4: // 左键
            modifySelectedSetting(false);
            break;
        case 6: // 右键
            modifySelectedSetting(true);
            break;
        case 21: // 确认键
            if (_selectedIndex == _settings.size() - 1) { // 恢复出厂设置
                performFactoryReset();
            }
            break;
        case 22: // 返回键
            UI_MANAGER()->goBack();
            break;
    }
}

void SystemSettingsPage::createSystemSettings() {
    ConfigManager& config = ConfigManager::getInstance();
    
    // 休眠时间
    SettingsItem sleepTimeout = {
        .title = "Sleep Timeout",
        .description = "Auto sleep timeout duration",
        .currentValue = String(config.getSleepTimeout() / 1000) + "s",
        .options = {"5s", "10s", "30s", "60s", "Off"},
        .onChange = [&config](int index) {
            uint32_t values[] = {5000, 10000, 30000, 60000, 0};
            config.setSleepTimeout(values[index]);
        },
        .isEnabled = true
    };
    _settings.push_back(sleepTimeout);
    
    // 自动保存
    SettingsItem autoSave = {
        .title = "Auto Save",
        .description = "Auto save configuration",
        .currentValue = config.getAutoSave() ? "Enabled" : "Disabled",
        .options = {"Disabled", "Enabled"},
        .onChange = [&config](int index) {
            config.setAutoSave(index == 1);
        },
        .isEnabled = true
    };
    _settings.push_back(autoSave);
    
    // 恢复出厂设置
    SettingsItem factoryReset = {
        .title = "Factory Reset",
        .description = "Reset to default settings",
        .currentValue = "Execute",
        .options = {"Execute"},
        .onChange = [](int index) {
            // 恢复出厂设置在按键处理中实现
        },
        .isEnabled = true
    };
    _settings.push_back(factoryReset);
}

void SystemSettingsPage::createSystemInfo() {
    lv_obj_t* info_label = lv_label_create(_info_container);
    lv_obj_set_pos(info_label, 5, 2);
    lv_label_set_text(info_label, "PawCounter v1.0 - ESP32");
    lv_obj_set_style_text_font(info_label, CURRENT_FONT(), LV_PART_MAIN);
    lv_obj_set_style_text_color(info_label, lv_color_hex(CURRENT_THEME().text_secondary), LV_PART_MAIN);
}

void SystemSettingsPage::updateSettingsDisplay() {
    _setting_objects.clear();
    lv_obj_clean(_settings_container);
    
    for (size_t i = 0; i < _settings.size(); i++) {
        lv_obj_t* item_container = lv_obj_create(_settings_container);
        lv_obj_set_pos(item_container, 5, i * 22);
        lv_obj_set_size(item_container, 220, 20);
        lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(item_container, 0, LV_PART_MAIN);
        
        if (i == _selectedIndex) {
            lv_obj_set_style_bg_opa(item_container, LV_OPA_20, LV_PART_MAIN);
            lv_obj_set_style_bg_color(item_container, lv_color_hex(CURRENT_THEME().highlight), LV_PART_MAIN);
        }
        
        lv_obj_t* title_label = lv_label_create(item_container);
        lv_obj_set_pos(title_label, 5, 2);
        lv_label_set_text(title_label, _settings[i].title.c_str());
        lv_obj_set_style_text_font(title_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(CURRENT_THEME().text_primary), LV_PART_MAIN);
        
        lv_obj_t* value_label = lv_label_create(item_container);
        lv_obj_set_pos(value_label, 160, 2);
        lv_label_set_text(value_label, _settings[i].currentValue.c_str());
        lv_obj_set_style_text_font(value_label, CURRENT_FONT(), LV_PART_MAIN);
        lv_obj_set_style_text_color(value_label, lv_color_hex(CURRENT_THEME().accent), LV_PART_MAIN);
        
        _setting_objects.push_back(item_container);
    }
}

void SystemSettingsPage::modifySelectedSetting(bool increase) {
    LOG_I(TAG_UI, "修改系统设置项 %d", _selectedIndex);
    updateSettingsDisplay();
}

void SystemSettingsPage::performFactoryReset() {
    LOG_I(TAG_UI, "执行恢复出厂设置");
    ConfigManager::getInstance().reset();
    keyboardConfig.resetToDefault();
    
    // 显示重置确认信息
    lv_obj_t* msgbox = lv_msgbox_create(NULL, "确认", "已恢复出厂设置", NULL, true);
    lv_obj_center(msgbox);
}