/**
 * @file CalculatorModes.cpp
 * @brief 计算器模式系统实现
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
    
    switch (mapping->keyType) {
        case KeyType::NUMBER:
            handleDigit(mapping->primaryLabel[0]);
            break;
            
        case KeyType::DECIMAL:
            if (_currentInput.indexOf('.') == -1) {
                handleDigit('.');
            }
            break;
            
        case KeyType::OPERATOR:
            handleOperator(mapping->operation);
            break;
            
        case KeyType::FUNCTION:
            handleFunction(keyPosition);
            break;
            
        default:
            return false;
    }
    
    updateDisplay();
    return true;
}

void BasicMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        _display->updateDisplay(_currentInput, _currentExpression, emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String BasicMode::getHelpText() const {
    return "基本计算模式 - 支持四则运算 (+, -, *, /)";
}

void BasicMode::handleDigit(char digit) {
    if (_clearOnNextInput) {
        _currentInput = "";
        _clearOnNextInput = false;
    }
    
    if (digit == '.') {
        if (_currentInput.isEmpty()) {
            _currentInput = "0.";
        } else {
            _currentInput += digit;
        }
    } else {
        if (_currentInput == "0") {
            _currentInput = String(digit);
        } else {
            _currentInput += digit;
        }
    }
}

void BasicMode::handleOperator(Operator op) {
    // 基本运算符处理逻辑
    if (!_currentInput.isEmpty()) {
        _currentExpression = _currentInput + " " + String((char)op) + " ";
        _clearOnNextInput = true;
    }
}

void BasicMode::handleFunction(uint8_t keyPosition) {
    // 基本功能键处理逻辑
    if (keyPosition == 17) { // Clear
        _currentInput = "0";
        _currentExpression = "";
        _clearOnNextInput = false;
    }
}

std::vector<ModeKeyMapping> BasicMode::createDefaultKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    // 简化的基本模式按键映射
    // 这里可以根据需要添加具体的按键映射
    return mappings;
}

ModeConfig BasicMode::createBasicConfig() {
    ModeConfig config;
    config.name = "基本模式";
    config.description = "基础四则运算";
    config.type = ModeType::BASIC;
    config.defaultPrecision = PrecisionLevel::STANDARD;
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = false;
    
    // 创建基本的数字格式
    config.numberFormat.useThousandsSeparator = false;
    config.numberFormat.showTrailingZeros = false;
    config.numberFormat.decimalPlaces = 6;
    config.numberFormat.scientificNotation = false;
    config.numberFormat.showSign = false;
    config.numberFormat.currency = "";
    config.numberFormat.thousandsSeparator = ",";
    config.numberFormat.decimalSeparator = ".";
    
    // 创建基本的主题
    config.theme.backgroundColor = 0x0000;
    config.theme.textColor = 0xFFFF;
    config.theme.expressionColor = 0x7BEF;
    config.theme.resultColor = 0x07E0;
    config.theme.errorColor = 0xF800;
    config.theme.statusColor = 0x001F;
    config.theme.borderColor = 0x7BEF;
    config.theme.mainFontSize = 3;
    config.theme.expressionFontSize = 2;
    config.theme.statusFontSize = 1;
    config.theme.padding = 5;
    config.theme.lineSpacing = 2;
    config.theme.showBorders = false;
    
    // 基本单位显示（关闭）
    config.unitDisplay.enabled = false;
    
    return config;
}

// ============================================================================
// ScientificMode 实现
// ============================================================================

ScientificMode::ScientificMode()
    : CalculatorMode(createScientificConfig())
    , _angleInDegrees(true)
    , _inverseMode(false) {
    
    MODE_LOG_I("Scientific mode constructor");
}

bool ScientificMode::initialize() {
    _angleInDegrees = true;
    _inverseMode = false;
    _pendingFunction = "";
    
    MODE_LOG_I("Scientific mode initialized");
    return true;
}

bool ScientificMode::handleKeyInput(uint8_t keyPosition, bool isLongPress, bool isSecondFunction) {
    MODE_LOG_D("Scientific mode handling key position: %d", keyPosition);
    // 科学计算模式的按键处理逻辑
    return true;
}

void ScientificMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        String info = _angleInDegrees ? "DEG" : "RAD";
        if (_inverseMode) info += " INV";
        _display->updateDisplay("0", info, emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String ScientificMode::getHelpText() const {
    return "科学计算模式 - 支持三角函数、对数、指数等科学函数";
}

void ScientificMode::handleScientificFunction(const String& functionName) {
    MODE_LOG_D("Executing scientific function: %s", functionName.c_str());
}

void ScientificMode::toggleAngleMode() {
    _angleInDegrees = !_angleInDegrees;
    MODE_LOG_I("Angle mode changed to: %s", _angleInDegrees ? "Degrees" : "Radians");
}

std::vector<ModeKeyMapping> ScientificMode::createScientificKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    // 简化的科学模式按键映射
    // 这里可以根据需要添加具体的按键映射
    return mappings;
}

ModeConfig ScientificMode::createScientificConfig() {
    ModeConfig config;
    config.name = "科学模式";
    config.description = "科学计算功能";
    config.type = ModeType::SCIENTIFIC;
    config.defaultPrecision = PrecisionLevel::HIGH_PRECISION;
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = true;
    
    // 科学计算的数字格式
    config.numberFormat.useThousandsSeparator = false;
    config.numberFormat.showTrailingZeros = false;
    config.numberFormat.decimalPlaces = 10;
    config.numberFormat.scientificNotation = true;
    config.numberFormat.showSign = false;
    config.numberFormat.currency = "";
    config.numberFormat.thousandsSeparator = ",";
    config.numberFormat.decimalSeparator = ".";
    
    // 科学计算主题
    config.theme.backgroundColor = 0x0000;
    config.theme.textColor = 0xFFFF;
    config.theme.expressionColor = 0x07FF;  // 青色
    config.theme.resultColor = 0xF81F;     // 紫色
    config.theme.errorColor = 0xF800;
    config.theme.statusColor = 0x001F;
    config.theme.borderColor = 0x7BEF;
    config.theme.mainFontSize = 2;
    config.theme.expressionFontSize = 1;
    config.theme.statusFontSize = 1;
    config.theme.padding = 3;
    config.theme.lineSpacing = 1;
    config.theme.showBorders = true;
    
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
    MODE_LOG_D("Financial mode handling key position: %d", keyPosition);
    // 财务计算模式的按键处理逻辑
    return true;
}

void FinancialMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        String amount = String(_presentValue, 2);
        _display->updateDisplay(amount, "财务计算", emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String FinancialMode::getHelpText() const {
    return "财务计算模式 - 支持现值、终值、月供计算和中文单位显示";
}

void FinancialMode::setAmount(double amount) {
    _presentValue = amount;
    updateDisplay();
    
    // 演示单位显示
    if (_display) {
        showUnitLabels(amount);
    }
}

double FinancialMode::calculatePresentValue() const {
    // PV = FV / (1 + r)^n
    if (_interestRate == 0.0) {
        return _futureValue;
    }
    return _futureValue / pow(1.0 + _interestRate, _periods);
}

double FinancialMode::calculateFutureValue() const {
    // FV = PV * (1 + r)^n
    return _presentValue * pow(1.0 + _interestRate, _periods);
}

double FinancialMode::calculatePayment() const {
    // PMT = PV * r * (1 + r)^n / ((1 + r)^n - 1)
    if (_interestRate == 0.0) {
        return _presentValue / _periods;
    }
    
    double factor = pow(1.0 + _interestRate, _periods);
    return _presentValue * _interestRate * factor / (factor - 1.0);
}

void FinancialMode::showUnitLabels(double number) {
    if (!_display) return;
    
    // 显示中文货币单位分解
    MODE_LOG_I("=== 金额单位分解 ===");
    MODE_LOG_I("总金额: %s%.2f", _currencySymbol.c_str(), number);
    
    if (number >= 100000000) {
        int yi = (int)(number / 100000000);
        MODE_LOG_I("  %d 亿", yi);
        number -= yi * 100000000;
    }
    
    if (number >= 10000) {
        int wan = (int)(number / 10000);
        MODE_LOG_I("  %d 万", wan);
        number -= wan * 10000;
    }
    
    if (number >= 1000) {
        int qian = (int)(number / 1000);
        MODE_LOG_I("  %d 千", qian);
        number -= qian * 1000;
    }
    
    if (number >= 100) {
        int bai = (int)(number / 100);
        MODE_LOG_I("  %d 百", bai);
        number -= bai * 100;
    }
    
    if (number >= 10) {
        int shi = (int)(number / 10);
        MODE_LOG_I("  %d 十", shi);
        number -= shi * 10;
    }
    
    if (number >= 1) {
        int ge = (int)number;
        MODE_LOG_I("  %d 个", ge);
        number -= ge;
    }
    
    if (number > 0.01) {
        MODE_LOG_I("  %.2f 角分", number);
    }
    
    MODE_LOG_I("==================");
}

std::vector<ModeKeyMapping> FinancialMode::createFinancialKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    // 简化的财务模式按键映射
    // 这里可以根据需要添加具体的按键映射
    return mappings;
}

ModeConfig FinancialMode::createFinancialConfig() {
    ModeConfig config;
    config.name = "财务模式";
    config.description = "财务计算和货币格式化";
    config.type = ModeType::FINANCIAL;
    config.defaultPrecision = PrecisionLevel::FINANCIAL;
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = true;
    
    // 财务计算的数字格式
    config.numberFormat.useThousandsSeparator = true;
    config.numberFormat.showTrailingZeros = false;
    config.numberFormat.decimalPlaces = 2;
    config.numberFormat.scientificNotation = false;
    config.numberFormat.showSign = false;
    config.numberFormat.currency = "¥";
    config.numberFormat.thousandsSeparator = ",";
    config.numberFormat.decimalSeparator = ".";
    
    // 财务主题
    config.theme.backgroundColor = 0x0000;
    config.theme.textColor = 0xFFFF;
    config.theme.expressionColor = 0xFFE0;  // 黄色
    config.theme.resultColor = 0x07E0;     // 绿色
    config.theme.errorColor = 0xF800;
    config.theme.statusColor = 0xFBE0;     // 橙色
    config.theme.borderColor = 0x7BEF;
    config.theme.mainFontSize = 3;
    config.theme.expressionFontSize = 2;
    config.theme.statusFontSize = 1;
    config.theme.padding = 5;
    config.theme.lineSpacing = 2;
    config.theme.showBorders = false;
    
    // 创建财务单位显示
    config.unitDisplay = createFinancialUnitDisplay();
    
    return config;
}

UnitDisplay FinancialMode::createFinancialUnitDisplay() {
    UnitDisplay unitDisplay;
    unitDisplay.enabled = true;
    unitDisplay.unitColor = 0x07E0;  // 绿色
    unitDisplay.unitFontSize = 1;
    
    // 中文数位单位
    unitDisplay.unitLabels = {
        "个", "十", "百", "千", "万", 
        "十万", "百万", "千万", "亿"
    };
    
    unitDisplay.unitValues = {
        1, 10, 100, 1000, 10000,
        100000, 1000000, 10000000, 100000000
    };
    
    return unitDisplay;
}

// ============================================================================
// ProgrammerMode 实现
// ============================================================================

ProgrammerMode::ProgrammerMode()
    : CalculatorMode(createProgrammerConfig())
    , _currentBase(NumberBase::DECIMAL)
    , _currentValue(0) {
    
    MODE_LOG_I("Programmer mode constructor");
}

bool ProgrammerMode::initialize() {
    _currentBase = NumberBase::DECIMAL;
    _currentValue = 0;
    
    MODE_LOG_I("Programmer mode initialized");
    return true;
}

bool ProgrammerMode::handleKeyInput(uint8_t keyPosition, bool isLongPress, bool isSecondFunction) {
    MODE_LOG_D("Programmer mode handling key position: %d", keyPosition);
    // 程序员模式的按键处理逻辑
    return true;
}

void ProgrammerMode::updateDisplay() {
    if (_display) {
        std::vector<CalculationHistory> emptyHistory;
        String valueStr = convertToBase(_currentValue, _currentBase);
        String info = "Base: " + String((int)_currentBase);
        _display->updateDisplay(valueStr, info, emptyHistory, CalculatorState::INPUT_NUMBER);
    }
}

String ProgrammerMode::getHelpText() const {
    return "程序员模式 - 支持二进制、八进制、十进制、十六进制转换和位运算";
}

void ProgrammerMode::switchBase(NumberBase base) {
    _currentBase = base;
    MODE_LOG_I("Switched to base: %d", (int)base);
}

String ProgrammerMode::convertToBase(int64_t value, NumberBase base) const {
    if (value == 0) return "0";
    
    String result = "";
    int64_t absValue = abs(value);
    
    while (absValue > 0) {
        int digit = absValue % base;
        if (digit < 10) {
            result = String((char)('0' + digit)) + result;
        } else {
            result = String((char)('A' + digit - 10)) + result;
        }
        absValue /= base;
    }
    
    if (value < 0) result = "-" + result;
    return result;
}

void ProgrammerMode::performBitwiseOperation(const String& operation) {
    MODE_LOG_D("Performing bitwise operation: %s", operation.c_str());
}

std::vector<ModeKeyMapping> ProgrammerMode::createProgrammerKeyMappings() {
    std::vector<ModeKeyMapping> mappings;
    // 简化的程序员模式按键映射
    // 这里可以根据需要添加具体的按键映射
    return mappings;
}

ModeConfig ProgrammerMode::createProgrammerConfig() {
    ModeConfig config;
    config.name = "程序员模式";
    config.description = "进制转换和位运算";
    config.type = ModeType::PROGRAMMER;
    config.defaultPrecision = PrecisionLevel::STANDARD;
    config.supportsMemory = true;
    config.supportsHistory = true;
    config.supportsSecondFunction = true;
    
    // 程序员模式的数字格式
    config.numberFormat.useThousandsSeparator = false;
    config.numberFormat.showTrailingZeros = false;
    config.numberFormat.decimalPlaces = 0;
    config.numberFormat.scientificNotation = false;
    config.numberFormat.showSign = true;
    config.numberFormat.currency = "";
    config.numberFormat.thousandsSeparator = "";
    config.numberFormat.decimalSeparator = "";
    
    // 程序员主题
    config.theme.backgroundColor = 0x0841;  // 深蓝色
    config.theme.textColor = 0xFFFF;
    config.theme.expressionColor = 0x07FF;  // 青色
    config.theme.resultColor = 0xFFE0;     // 黄色
    config.theme.errorColor = 0xF800;
    config.theme.statusColor = 0x07E0;     // 绿色
    config.theme.borderColor = 0x7BEF;
    config.theme.mainFontSize = 2;
    config.theme.expressionFontSize = 1;
    config.theme.statusFontSize = 1;
    config.theme.padding = 3;
    config.theme.lineSpacing = 1;
    config.theme.showBorders = true;
    
    config.unitDisplay.enabled = false;
    
    return config;
}

// ============================================================================
// ModeManager 实现
// ============================================================================

ModeManager::ModeManager()
    : _currentModeId(0)
    , _nextModeId(0) {
    
    MODE_LOG_I("Mode manager created");
}

bool ModeManager::begin() {
    _modes.clear();
    _currentModeId = 0;
    _nextModeId = 0;
    
    MODE_LOG_I("Mode manager initialized");
    return true;
}

uint8_t ModeManager::registerMode(std::shared_ptr<CalculatorMode> mode) {
    if (!mode) {
        MODE_LOG_E("Cannot register null mode");
        return 255;
    }
    
    mode->setModeId(_nextModeId);
    _modes.push_back(mode);
    
    MODE_LOG_I("Mode registered: %s (ID: %d)", mode->getName().c_str(), _nextModeId);
    return _nextModeId++;
}

bool ModeManager::activateMode(uint8_t modeId,
                              std::shared_ptr<CalculatorDisplay> display,
                              std::shared_ptr<CalculationEngine> engine) {
    
    if (modeId >= _modes.size()) {
        MODE_LOG_E("Invalid mode ID: %d", modeId);
        return false;
    }
    
    // 去激活当前模式
    if (_currentModeId < _modes.size()) {
        _modes[_currentModeId]->deactivate();
    }
    
    // 激活新模式
    _currentModeId = modeId;
    _modes[_currentModeId]->activate(display, engine);
    
    MODE_LOG_I("Mode activated: %s", _modes[_currentModeId]->getName().c_str());
    return true;
}

std::shared_ptr<CalculatorMode> ModeManager::getCurrentMode() const {
    if (_currentModeId >= _modes.size()) {
        return nullptr;
    }
    return _modes[_currentModeId];
}

std::vector<String> ModeManager::getModeList() const {
    std::vector<String> modeList;
    for (const auto& mode : _modes) {
        modeList.push_back(mode->getName());
    }
    return modeList;
}