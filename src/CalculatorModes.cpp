/**
 * @file CalculatorModes.cpp
 * @brief 计算器模式系统实现 (仅保留基本模式和财务模式)
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "CalculatorModes.h"

// ============================================================================
// CalculatorMode 基类实现
// ============================================================================

CalculatorMode::CalculatorMode(const ModeConfig& config)
    : _config(config)
    , _modeId(0)
    , _isActive(false)
    , _secondFunctionActive(false) {
    
    MODE_LOG_I("Calculator mode created: %s", _config.name.c_str());
}

void CalculatorMode::activate(std::shared_ptr<CalculatorDisplay> display,
                             std::shared_ptr<CalculationEngine> engine) {
    _display = display;
    _engine = engine;
    _isActive = true;
    _secondFunctionActive = false;
    
    // 应用模式特定的配置
    if (_display) {
        _display->setNumberFormat(_config.numberFormat);
        _display->setUnitDisplay(_config.unitDisplay);
        _display->setTheme(_config.theme);
    }
    
    if (_engine) {
        _engine->setPrecisionLevel(_config.defaultPrecision);
    }
    
    MODE_LOG_I("Mode activated: %s", _config.name.c_str());
}

void CalculatorMode::deactivate() {
    _isActive = false;
    _secondFunctionActive = false;
    
    MODE_LOG_I("Mode deactivated: %s", _config.name.c_str());
}

const ModeKeyMapping* CalculatorMode::findKeyMapping(uint8_t keyPosition) const {
    for (const auto& mapping : _config.keyMappings) {
        if (mapping.keyPosition == keyPosition) {
            return &mapping;
        }
    }
    return nullptr;
}

void CalculatorMode::logMessage(const char* level, const char* format, ...) const {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    MODE_LOG_I("[%s] %s", _config.name.c_str(), buffer);
}

// ============================================================================
// BasicMode 实现
// ============================================================================

BasicMode::BasicMode() 
    : CalculatorMode(createBasicConfig())
    , _clearOnNextInput(false)
    , _lastResult(0.0) {
    
    MODE_LOG_I("Basic mode constructor");
}

bool BasicMode::initialize() {
    _currentInput = "0";
    _currentExpression = "";
    _lastResult = 0.0;
    _clearOnNextInput = false;
    
    MODE_LOG_I("Basic mode initialized");
    return true;
}

bool BasicMode::handleKeyInput(uint8_t keyPosition, bool isLongPress, bool isSecondFunction) {
    const ModeKeyMapping* mapping = findKeyMapping(keyPosition);
    if (!mapping) {
        MODE_LOG_W("No key mapping found for position %d", keyPosition);
        return false;
    }
    
    MODE_LOG_D("Basic mode handling key: %s", mapping->primaryLabel.c_str());
    
    // 根据按键类型处理
    switch (mapping->keyType) {
        case KeyType::NUMBER:
            handleDigit(mapping->primaryLabel[0]);
            break;
            
        case KeyType::OPERATOR:
            handleOperator(mapping->operation);
            break;
            
        case KeyType::FUNCTION:
            handleFunction(keyPosition);
            break;
            
        default:
            MODE_LOG_W("Unhandled key type in basic mode");
            return false;
    }
    
    return true;
}

void BasicMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        _display->updateDisplay(_currentInput, _currentExpression, emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String BasicMode::getHelpText() const {
    return "基本计算模式 - 支持四则运算、百分比、平方根等基本功能";
}

void BasicMode::handleDigit(char digit) {
    if (_clearOnNextInput) {
        _currentInput = "";
        _clearOnNextInput = false;
    }
    
    if (_currentInput == "0" && digit != '.') {
        _currentInput = String(digit);
    } else {
        _currentInput += digit;
    }
    
    MODE_LOG_V("Digit input: %c, current: %s", digit, _currentInput.c_str());
}

void BasicMode::handleOperator(Operator op) {
    // 简化的运算符处理
    _currentExpression = _currentInput + " " + String((char)op) + " ";
    _clearOnNextInput = true;
    
    MODE_LOG_D("Operator input: %d", (int)op);
}

void BasicMode::handleFunction(uint8_t keyPosition) {
    // 处理功能键
    MODE_LOG_D("Function key: %d", keyPosition);
    
    if (keyPosition == 19) { // Clear
        _currentInput = "0";
        _currentExpression = "";
        _clearOnNextInput = false;
    } else if (keyPosition == 15) { // Backspace
        if (_currentInput.length() > 1) {
            _currentInput.remove(_currentInput.length() - 1);
        } else {
            _currentInput = "0";
        }
    }
}

std::vector<ModeKeyMapping> BasicMode::createDefaultKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    
    // 添加基本的按键映射
    mappings.push_back({1, "ON", "", KeyType::POWER, Operator::NONE, "", false});
    mappings.push_back({2, "7", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({3, "4", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({4, "1", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({5, "0", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({6, "TAB", "", KeyType::LAYER_SWITCH, Operator::NONE, "", false});
    mappings.push_back({7, "8", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({8, "5", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({9, "2", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({10, "%", "", KeyType::FUNCTION, Operator::PERCENT, "", false});
    mappings.push_back({11, "9", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({12, "6", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({13, "3", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({14, ".", "", KeyType::DECIMAL, Operator::NONE, "", false});
    mappings.push_back({15, "⌫", "", KeyType::DELETE, Operator::NONE, "", false});
    mappings.push_back({16, "×", "", KeyType::OPERATOR, Operator::MULTIPLY, "", false});
    mappings.push_back({17, "−", "", KeyType::OPERATOR, Operator::SUBTRACT, "", false});
    mappings.push_back({18, "+", "", KeyType::OPERATOR, Operator::ADD, "", false});
    mappings.push_back({19, "C", "", KeyType::CLEAR, Operator::NONE, "", false});
    mappings.push_back({20, "±", "", KeyType::FUNCTION, Operator::NONE, "", false});
    mappings.push_back({21, "÷", "", KeyType::OPERATOR, Operator::DIVIDE, "", false});
    mappings.push_back({22, "=", "", KeyType::FUNCTION, Operator::EQUALS, "", false});
    
    return mappings;
}

ModeConfig BasicMode::createBasicConfig() {
    ModeConfig config;
    config.name = "基本模式";
    config.description = "基本计算功能";
    config.type = ModeType::BASIC;
    config.defaultPrecision = PrecisionLevel::STANDARD;
    config.keyMappings = createDefaultKeyMappings();
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = false;
    
    // 基本计算的数字格式
    config.numberFormat.useThousandsSeparator = false;
    config.numberFormat.showTrailingZeros = false;
    config.numberFormat.decimalPlaces = 6;
    config.numberFormat.scientificNotation = false;
    config.numberFormat.showSign = false;
    config.numberFormat.currency = "";
    config.numberFormat.thousandsSeparator = ",";
    config.numberFormat.decimalSeparator = ".";
    
    // 基本计算主题
    config.theme.backgroundColor = 0x0000;
    config.theme.textColor = 0xFFFF;
    config.theme.expressionColor = 0x07E0;  // 绿色
    config.theme.resultColor = 0xFFE0;     // 黄色
    config.theme.errorColor = 0xF800;
    config.theme.statusColor = 0x001F;
    config.theme.borderColor = 0x7BEF;
    config.theme.mainFontSize = 2;
    config.theme.expressionFontSize = 1;
    config.theme.statusFontSize = 1;
    config.theme.padding = 2;
    config.theme.lineSpacing = 1;
    config.theme.showBorders = true;
    
    // 基本单位显示（关闭）
    config.unitDisplay.enabled = false;
    
    return config;
}

// ============================================================================
// FinancialMode 实现
// ============================================================================

FinancialMode::FinancialMode()
    : CalculatorMode(createFinancialConfig())
    , _currencySymbol("¥")
    , _decimalPlaces(2)
    , _showThousandsSeparator(true)
    , _presentValue(0.0)
    , _futureValue(0.0)
    , _payment(0.0)
    , _interestRate(0.0)
    , _periods(0.0) {
    
    MODE_LOG_I("Financial mode constructor");
}

bool FinancialMode::initialize() {
    _presentValue = 0.0;
    _futureValue = 0.0;
    _payment = 0.0;
    _interestRate = 0.0;
    _periods = 0.0;
    
    MODE_LOG_I("Financial mode initialized");
    return true;
}

bool FinancialMode::handleKeyInput(uint8_t keyPosition, bool isLongPress, bool isSecondFunction) {
    const ModeKeyMapping* mapping = findKeyMapping(keyPosition);
    if (!mapping) {
        MODE_LOG_W("No key mapping found for position %d", keyPosition);
        return false;
    }
    
    MODE_LOG_D("Financial mode handling key: %s", mapping->primaryLabel.c_str());
    // 财务模式的按键处理逻辑
    return true;
}

void FinancialMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        String info = "财务模式 " + _currencySymbol;
        _display->updateDisplay("0.00", info, emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String FinancialMode::getHelpText() const {
    return "财务计算模式 - 支持货币格式化、单位显示、财务函数计算";
}

void FinancialMode::setAmount(double amount) {
    MODE_LOG_D("Setting amount: %.2f", amount);
    showUnitLabels(amount);
}

double FinancialMode::calculatePresentValue() const {
    // 简化的现值计算
    return _futureValue / pow(1.0 + _interestRate, _periods);
}

double FinancialMode::calculateFutureValue() const {
    // 简化的终值计算
    return _presentValue * pow(1.0 + _interestRate, _periods);
}

double FinancialMode::calculatePayment() const {
    // 简化的月供计算
    if (_interestRate == 0.0) return _presentValue / _periods;
    return _presentValue * (_interestRate * pow(1.0 + _interestRate, _periods)) / 
           (pow(1.0 + _interestRate, _periods) - 1.0);
}

void FinancialMode::showUnitLabels(double number) {
    if (_display) {
        // 显示金额的单位分解
        String unitDisplay = String(number, (unsigned int)_decimalPlaces);
        MODE_LOG_I("Amount unit display: %s", unitDisplay.c_str());
    }
}

std::vector<ModeKeyMapping> FinancialMode::createFinancialKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    
    // 财务模式的特殊按键映射
    mappings.push_back({1, "PV", "现值", KeyType::FUNCTION, Operator::NONE, "present_value", false});
    mappings.push_back({2, "7", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({3, "4", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({4, "1", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({5, "0", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({6, "TAB", "", KeyType::LAYER_SWITCH, Operator::NONE, "", false});
    mappings.push_back({7, "8", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({8, "5", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({9, "2", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({10, "FV", "终值", KeyType::FUNCTION, Operator::NONE, "future_value", false});
    mappings.push_back({11, "9", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({12, "6", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({13, "3", "", KeyType::NUMBER, Operator::NONE, "", false});
    mappings.push_back({14, ".", "", KeyType::DECIMAL, Operator::NONE, "", false});
    mappings.push_back({15, "⌫", "", KeyType::DELETE, Operator::NONE, "", false});
    mappings.push_back({16, "PMT", "月供", KeyType::FUNCTION, Operator::NONE, "payment", false});
    mappings.push_back({17, "−", "", KeyType::OPERATOR, Operator::SUBTRACT, "", false});
    mappings.push_back({18, "+", "", KeyType::OPERATOR, Operator::ADD, "", false});
    mappings.push_back({19, "C", "", KeyType::CLEAR, Operator::NONE, "", false});
    mappings.push_back({20, "±", "", KeyType::FUNCTION, Operator::NONE, "", false});
    mappings.push_back({21, "I/Y", "利率", KeyType::FUNCTION, Operator::NONE, "interest_rate", false});
    mappings.push_back({22, "=", "", KeyType::FUNCTION, Operator::EQUALS, "", false});
    
    return mappings;
}

UnitDisplay FinancialMode::createFinancialUnitDisplay() {
    UnitDisplay unitDisplay;
    unitDisplay.enabled = true;
    unitDisplay.unitLabels = {"万", "千", "百", "十", "个"};
    unitDisplay.unitValues = {10000, 1000, 100, 10, 1};
    unitDisplay.unitColor = 0x07FF;  // 青色
    unitDisplay.unitFontSize = 1;
    
    return unitDisplay;
}

ModeConfig FinancialMode::createFinancialConfig() {
    ModeConfig config;
    config.name = "财务模式";
    config.description = "财务计算功能";
    config.type = ModeType::FINANCIAL;
    config.defaultPrecision = PrecisionLevel::FINANCIAL;
    config.keyMappings = createFinancialKeyMappings();
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = false;
    
    // 财务计算的数字格式
    config.numberFormat.useThousandsSeparator = true;
    config.numberFormat.showTrailingZeros = true;
    config.numberFormat.decimalPlaces = 2;
    config.numberFormat.scientificNotation = false;
    config.numberFormat.showSign = false;
    config.numberFormat.currency = "¥";
    config.numberFormat.thousandsSeparator = ",";
    config.numberFormat.decimalSeparator = ".";
    
    // 财务计算主题
    config.theme.backgroundColor = 0x0010;  // 深蓝
    config.theme.textColor = 0xFFFF;        // 白色
    config.theme.expressionColor = 0x07FF;  // 青色
    config.theme.resultColor = 0xFFE0;      // 黄色
    config.theme.errorColor = 0xF800;       // 红色
    config.theme.statusColor = 0x001F;      // 蓝色
    config.theme.borderColor = 0x7BEF;      // 灰色
    config.theme.mainFontSize = 2;
    config.theme.expressionFontSize = 1;
    config.theme.statusFontSize = 1;
    config.theme.padding = 2;
    config.theme.lineSpacing = 1;
    config.theme.showBorders = true;
    
    // 财务单位显示
    config.unitDisplay = createFinancialUnitDisplay();
    
    return config;
}

// ============================================================================
// ModeManager 实现
// ============================================================================

ModeManager::ModeManager()
    : _currentModeId(255)  // 无效ID
    , _nextModeId(0) {
}

bool ModeManager::begin() {
    MODE_LOG_I("Initializing mode manager");
    
    // 注册基本模式
    auto basicMode = std::make_shared<BasicMode>();
    registerMode(basicMode);
    
    // 注册财务模式
    auto financialMode = std::make_shared<FinancialMode>();
    registerMode(financialMode);
    
    MODE_LOG_I("Mode manager initialized with %d modes", _modes.size());
    return true;
}

uint8_t ModeManager::registerMode(std::shared_ptr<CalculatorMode> mode) {
    if (!mode) {
        MODE_LOG_E("Cannot register null mode");
        return 255;
    }
    
    mode->setModeId(_nextModeId);
    _modes.push_back(mode);
    
    MODE_LOG_I("Registered mode: %s (ID: %d)", mode->getName().c_str(), _nextModeId);
    
    return _nextModeId++;
}

bool ModeManager::activateMode(uint8_t modeId,
                              std::shared_ptr<CalculatorDisplay> display,
                              std::shared_ptr<CalculationEngine> engine) {
    if (modeId >= _modes.size()) {
        MODE_LOG_E("Invalid mode ID: %d", modeId);
        return false;
    }
    
    // 停用当前模式
    if (_currentModeId < _modes.size()) {
        _modes[_currentModeId]->deactivate();
    }
    
    // 激活新模式
    _modes[modeId]->activate(display, engine);
    _currentModeId = modeId;
    
    MODE_LOG_I("Activated mode: %s", _modes[modeId]->getName().c_str());
    return true;
}

std::shared_ptr<CalculatorMode> ModeManager::getCurrentMode() const {
    if (_currentModeId < _modes.size()) {
        return _modes[_currentModeId];
    }
    return nullptr;
}

std::vector<String> ModeManager::getModeList() const {
    std::vector<String> names;
    for (const auto& mode : _modes) {
        names.push_back(mode->getName());
    }
    return names;
}