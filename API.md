# pipKit API

Этот файл описывает актуальный публичный API `pipGUI` и `pipCore`, который есть в коде проекта.

---

# 1. Build-time флаги

Низкий слой `pipCore` выбирает платформу и драйвер дисплея на этапе компиляции.
Эти флаги задаются в `include/config.hpp`.

- `PIPCORE_DISPLAY`
  - пример: `#define PIPCORE_DISPLAY ST7789`
- `PIPCORE_PLATFORM`
  - пример: `#define PIPCORE_PLATFORM ESP32`

Пример через `include/config.hpp`:

```cpp
#define PIPCORE_PLATFORM ESP32
#define PIPCORE_DISPLAY ST7789
```

Если флаги не заданы, а в проекте собран ровно один backend платформы и ровно один backend дисплея, они выбираются автоматически.

---

# 2. Инициализация

## 2.1. Конфигурация дисплея

Полный пример:

```cpp
ui.configureDisplay()
    .pins(11, 12, 10, 9, -1)   // mosi, sclk, cs, dc, rst
    .size(240, 320)            // width, height
    .hz(80000000)              // частота SPI
    .order("RGB")              // "RGB" или "BGR"
    .invert(true)              // инверсия панели
    .swap(false)               // swap байтов RGB565
    .offset(0, 0);             // смещение активной области
```

Минимальный пример:

```cpp
ui.configureDisplay()
    .pins(11, 12, 10, 9, -1)
    .size(240, 320);
```

Обязательно указать:

- `pins(...)` задаёт пины SPI и управляющие пины дисплея.
- `size(width, height)` задаёт логический размер панели.

Можно не писать и оставить дефолт библиотеки:

- `hz(freq)` по умолчанию `80000000`
- `order("RGB")` по умолчанию `RGB`
- `invert(bool)` по умолчанию `true`
- `swap(bool)` по умолчанию `false`
- `offset(x, y)` по умолчанию `(0, 0)`

## 2.2. Запуск GUI

После конфигурации дисплея вызывается `begin()`:

```cpp
ui.begin(
    0,                  // rotation: 0..3
    ui.rgb(0, 0, 0)   // bgColor
);
```

- `rotation` принимает значения `0..3`.
- `bgColor` задаёт цвет фона GUI при старте.

## 2.3. Подсветка и яркость

```cpp
ui.setBacklight()
    .pin(48)          // pin
    .channel(0)       // PWM channel
    .freq(5000)       // PWM frequency
    .resolution(12);  // PWM resolution bits
```

Что важно:

- `pin` - обязательный
- `channel`, `frequency` и `resolution` можно не указывать
- после `setBacklight(...)` boot-анимация `LightFade` уже сама управляет яркостью
- если backlight не настроен, `LightFade` управлять нечем

### Максимальная яркость

```cpp
ui.setMaxBrightness(70);   // ограничить максимум 70%
```

- диапазон `0..100`
- это верхний лимит яркости, библиотека использует этот лимит, когда сама меняет яркость
- на текущей ESP32-платформе значение сохраняется и потом подгружается автоматически

### Текущая яркость

```cpp
ui.setBrightness(35);
uint8_t now = ui.brightness();
```
- диапазон `0..100`
- меняет текущую яркость сразу
- значение ограничивается `maxBrightness()`
- текущее значение не сохраняется в prefs

---

# 3. Базовые helpers

## 3.1. Размеры

```cpp
ui.screenWidth();   // ширина вашего экрана
ui.screenHeight();  // высота вашего экрана
```

## 3.2. Цвет

```cpp
ui.rgb(255, 255, 255);
```

## 3.3. Очистка экрана

```cpp
ui.clear(ui.rgb(0, 0, 0));
```

## 3.4. Клип

```cpp
ui.setClip()
    .pos(10, 20)
    .size(120, 80);

// ... рисование только внутри области ...

ui.clearClip();
```

---

# 4. Экраны и цикл

## 4.1. Регистрация экранов

Экраны регистрируются через макрос `SCREEN(name, order)`:

```cpp
SCREEN(ScreenHome, 0)
{
    ui.clear(ui.rgb(0, 0, 0));
}

SCREEN(ScreenSettings, 1)
{
    ui.clear(ui.rgb(8, 8, 8));
}
```

Что создаёт макрос:

- callback-функцию экрана;
- числовой id экрана;
- автоматическую регистрацию экрана в таблице GUI.

Что важно:

- `name` становится константой id, её потом можно передавать в `setScreen(...)`, `configureList(...)`, `configureTile(...)` и другие API
- `order` задаёт порядок регистрации экрана
- `order` должен быть уникальным для каждого экрана
- обычно первый экран делают с `order = 0`

После этого экран можно активировать:

```cpp
ui.setScreen(ScreenHome);
```

## 4.2. Основной цикл

Базовый вариант:

```cpp
void loop()
{
    ui.loop();
}
```

Вспомогательный вариант, если есть две кнопки `Button`:

```cpp
Button btnNext(1, Pullup);
Button btnPrev(2, Pullup);

void setup()
{
    btnNext.begin();
    btnPrev.begin();
}

void loop()
{
    ui.loopWithInput(btnNext, btnPrev);
}
```

`loopWithInput(...)` только обновляет сами объекты `Button` и потом вызывает `ui.loop()`.
Логику экранов, списков и плиток вы всё равно задаёте отдельно.

Если `loopWithInput(...)` не используется, вызывайте `btn.update()` сами в начале каждого `loop()`.
После этого `wasPressed()` и `isDown()` читают уже обновлённое состояние кнопки.

## 4.3. Управление экранами

```cpp
ui.setScreen(ScreenHome);
ui.nextScreen();
ui.prevScreen();
ui.screenTransitionActive();
```

## 4.4. Принудительная перерисовка

```cpp
ui.requestRedraw();
```

Нужно, когда вы изменили данные экрана извне и хотите гарантированно перерисовать следующий кадр.

## 4.5. Анимация переходов

```cpp
ui.setScreenAnim(SlideX, 250);
```

Доступные режимы:

- `None`
- `SlideX`
- `SlideY`

---

# 5. Текст, шрифты и иконки

## 5.1. Встроенные шрифты

Выбирать шрифт можно так:

```cpp
ui.setFont(WixMadeForDisplay);
```

Доступны встроенные в библиотеку:
- `WixMadeForDisplay`
- `KronaOne`

Текущие значения можно читать:

```cpp
ui.fontSize();    // текущий размер шрифта
ui.fontWeight();  // текущая толщина шрифта
```

Доступные токены толщины:

```cpp
ui.setFontWeight(Semibold);
```

- `Thin`
- `Light`
- `Regular`
- `Medium`
- `Semibold`
- `Bold`
- `Black`

## 5.2. Текстовые стили

```cpp
ui.setTextStyle(H1);
```

`TextStyle`:

- `H1`
- `H2`
- `Body`
- `Caption`

## 5.3. Обычный текст

```cpp
ui.drawText()
    .pos(center, 32)
    .font(WixMadeForDisplay)
    .size(18)
    .weight(Semibold)
    .text("Hello")
    .color(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
    .align(AlignCenter)
```

Для in-place обновления есть такой же builder:

```cpp
ui.updateText()
    .pos(center, 32)
    .text("Updated")
    .color(ui.rgb(255, 255, 255))
    .align(AlignCenter)
```

## 5.4. Бегущая строка и многоточие

```cpp
ui.drawTextMarquee()
    .pos(20, 80)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
    .speed(30)
    .holdStart(700)
```

```cpp
ui.drawTextEllipsized()
    .pos(20, 110)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
```

## 5.5. Иконки

```cpp
ui.drawIcon()
    .pos(20, 20)
    .size(18)
    .icon(WarningLayer0)
    .color(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
```

Иконки берутся из вашего набора `IconId`.

## 5.6. Логотип

```cpp
ui.logoSizesPx(36, 20);

ui.showLogo()
    .title("PISPPUS")
    .subtitle("Digital Thermometer")
    .anim(ZoomIn)
    .fgColor(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
    .duration(1800)
    .pos(center, 40);
```

Параметры:

- `title` и `subtitle` — строки логотипа;
- `animation` — `None`, `FadeIn`, `SlideUp`, `LightFade`, `ZoomIn`;
- `fgColor` и `bgColor` — цвета;
- `durationMs` — длительность анимации;
- `x`, `y` — позиция.

Отдельно можно настроить размеры:

```cpp
ui.logoTitleSizePx(36);
ui.logoSubtitleSizePx(20);
```

---

# 6. Фигуры

Приставка fill - это фигуры с заливкой, draw - с контуром

## 6.1 Линия

```cpp
ui.drawLine()
    .from(20, 20)
    .to(140, 60)
    .color(ui.rgb(255, 255, 255))
```

Для fluent API `draw()` / `show()` / `apply()` не обязательны:

- fluent-объект сам коммитится в конце выражения
- явный `draw()` нужен только если вы хотите сделать commit сразу или подчеркнуть это в коде

## 6.2 Прямоугольник

```cpp
ui.fillRect()
    .pos(20, 40)
    .size(100, 40)
    .color(ui.rgb(0, 120, 255))
```

```cpp
ui.drawRect()
    .pos(20, 90)
    .size(100, 40)
    .color(ui.rgb(255, 255, 255))
    .radius({10})
```

`drawRect()` и `fillRect()` умеют:

- `radius({r})` — один радиус для всех углов;
- `radius({tl, tr, br, bl})` — отдельные радиусы по углам.

## 6.3 Круг

```cpp
ui.fillCircle()
    .pos(50, 50)
    .radius(18)
    .color(ui.rgb(0, 87, 250))
```

```cpp
ui.drawCircle()
    .pos(50, 50)
    .radius(18)
    .color(ui.rgb(255, 255, 255))
```

## 6.4 Треугольник

```cpp
ui.fillTriangle()
    .point0(40, 120)
    .point1(70, 80)
    .point2(100, 120)
    .color(ui.rgb(0, 200, 120))
```

```cpp
ui.drawTriangle()
    .point0(40, 120)
    .point1(70, 80)
    .point2(100, 120)
    .color(ui.rgb(255, 255, 255))
```

## 6.5 Скруглённый треугольник

```cpp
ui.fillTriangle()
    .point0(140, 120)
    .point1(170, 80)
    .point2(200, 120)
    .radius(10)
    .color(ui.rgb(0, 120, 255))
```

## 6.6 Дуга

```cpp
ui.drawArc()
    .pos(100, 80)
    .radius(28)
    .startDeg(-90.0f)
    .endDeg(90.0f)
    .color(ui.rgb(80, 255, 120))
```

## 6.7 Эллипс

```cpp
ui.fillEllipse()
    .pos(120, 50)
    .radiusX(28)
    .radiusY(16)
    .color(ui.rgb(255, 0, 72))
```

```cpp
ui.drawEllipse()
    .pos(120, 50)
    .radiusX(28)
    .radiusY(16)
    .color(ui.rgb(255, 255, 255))
```

## 6.8 Сквиркль

```cpp
ui.fillSquircle()
    .pos(80, 160)
    .radius(26)
    .color(ui.rgb(255, 128, 0))
```

# 7. Градиенты

## 7.1 Вертикальный

```cpp
ui.gradientVertical()
    .pos(20, 20)
    .size(120, 40)
    .topColor(ui.rgb(255, 0, 72))
    .bottomColor(ui.rgb(0, 87, 250))
```

## 7.2 Горизонтальный

```cpp
ui.gradientHorizontal()
    .pos(20, 70)
    .size(120, 40)
    .leftColor(ui.rgb(255, 128, 0))
    .rightColor(ui.rgb(80, 255, 120))
```

## 7.3 4 угла

```cpp
ui.gradientCorners()
    .pos(20, 120)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 0, 72))
    .topRightColor(ui.rgb(0, 87, 250))
    .bottomLeftColor(ui.rgb(80, 255, 120))
    .bottomRightColor(ui.rgb(255, 128, 0))
```

## 7.4 Диагональный

```cpp
ui.gradientDiagonal()
    .pos(20, 170)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 255, 255))
    .bottomRightColor(ui.rgb(30, 30, 30))
```

## 7.5 Радиальный

```cpp
ui.gradientRadial()
    .pos(20, 220)
    .size(120, 60)
    .center(80, 250)
    .radius(40)
    .innerColor(ui.rgb(255, 255, 255))
    .outerColor(ui.rgb(0, 87, 250))
```

# 8. Эффекты

## 8.1 Blur

```cpp
ui.drawBlur()
    .pos(0, 180)
    .size(240, 40)
    .radius(10)
    .direction(TopDown)
    .gradient(true)
    .materialStrength(160)
    .materialColor(-1)
```

Для обновления части экрана:

```cpp
ui.updateBlur()
    .pos(0, 180)
    .size(240, 40)
    .radius(10)
    .direction(TopDown)
```

Что делают параметры:

- `materialStrength(...)` задаёт силу цветного материала поверх blur
- `materialColor(...)` задаёт цвет материала в `RGB565`; `-1` значит взять текущий цвет фона библиотеки

## 8.2. Glow

Круг:

```cpp
ui.drawGlowCircle()
    .pos(60, 90)
    .radius(18)
    .fillColor(ui.rgb(255, 255, 255))
    .glowColor(ui.rgb(0, 120, 255))
    .glowSize(16)
    .glowStrength(220)
    .anim(Pulse)
    .pulsePeriodMs(1200)
```

Прямоугольник:

```cpp
ui.drawGlowRect()
    .pos(30, 140)
    .size(120, 46)
    .radius(12)
    .fillColor(ui.rgb(20, 20, 20))
    .glowColor(ui.rgb(0, 120, 255))
    .glowSize(14)
    .glowStrength(220)
```

Для in-place обновления есть `updateGlowCircle()` и `updateGlowRect()`.

---

# 8. Виджеты

## 8.1. Scroll dots

```cpp
ui.drawScrollDots()
    .pos(center, 220)
    .count(5)
    .activeIndex(2)
    .prevIndex(1)
    .animProgress(0.5f)
    .animate(true)
    .animDirection(1)
    .activeColor(ui.rgb(0, 87, 250))
    .inactiveColor(ui.rgb(60, 60, 60))
    .dotRadius(3)
    .spacing(14)
    .activeWidth(18)
    .draw();
```

Есть и `updateScrollDots()` с теми же параметрами.

## 8.2. Универсальная кнопка

Состояние:

```cpp
static ButtonVisualState saveBtn{};
```

Обновление анимации:

```cpp
ui.updateButtonPress(saveBtn, isDown);
```

Отрисовка:

```cpp
ui.drawButton()
    .label("Save")
    .pos(center, 180)
    .size(120, 40)
    .baseColor(ui.rgb(0, 120, 255))
    .radius(10)
    .state(saveBtn)
    .draw();
```

Для частичного обновления есть `updateButton()`.

## 8.3. Toggle switch

Состояние:

```cpp
static ToggleSwitchState sw{};
```

Обновление логики:

```cpp
// btn.update() уже вызван в этом кадре
bool changed = ui.updateToggleSwitch(sw, btn.wasPressed());
if (changed)
    ui.requestRedraw();
```

Отрисовка:

```cpp
ui.drawToggleSwitch()
    .pos(center, 140)
    .size(78, 36)
    .state(sw)
    .activeColor(ui.rgb(21, 180, 110))
    .draw();
```

`inactiveColor(...)` и `knobColor(...)` задаются по желанию.

## 8.4. Прогресс-бар

```cpp
ui.drawProgressBar()
    .pos(20, 220)
    .size(180, 16)
    .value(65)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .radius(8)
    .anim(Shimmer)
    .draw();
```

Текст над прогрессом:

```cpp
ui.drawProgressText(20, 246, 180, 20, "Downloading",
                    ui.rgb(255, 255, 255), ui.rgb(0, 0, 0),
                    AlignCenter, 14);

ui.drawProgressPercent(20, 268, 180, 20, 65,
                       ui.rgb(255, 255, 255), ui.rgb(0, 0, 0),
                       AlignCenter, 14);
```

## 8.5. Круговой прогресс-бар

```cpp
ui.drawCircularProgressBar()
    .pos(center, 140)
    .radius(34)
    .thickness(8)
    .value(72)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .anim(None)
    .draw();
```

## 8.6. Drum roll

Горизонтальный:

```cpp
String options[] = {"Low", "Medium", "High"};

ui.drawDrumRollHorizontal(
    20, 60, 200, 40,
    options, 3,
    1, 0,          // selectedIndex, prevIndex
    0.4f,          // animProgress
    ui.rgb(255, 255, 255),
    ui.rgb(0, 0, 0),
    16
);
```

Вертикальный есть отдельной функцией `drawDrumRollVertical(...)`.

---

# 9. Списки и плитки

## 9.1. Списочное меню

Конфигурация:

```cpp
ui.configureList()
    .screen(ScreenMainMenu)
    .parent(ScreenHome)
    .items({
        {"Settings", "Device configuration", ScreenSettings},
        {"About",    "Firmware info",        ScreenAbout},
        {"Restart",  "Reboot device",        ScreenRestart}
    })
    .cardColor(ui.rgb(12, 12, 12))
    .activeColor(ui.rgb(0, 120, 255))
    .radius(8)
    .cardSize(0, 0)
    .mode(Cards)
    .apply();
```

Ввод в `loop()`:

```cpp
// btnNext.update() и btnPrev.update() уже вызваны в начале этого loop()
ui.listInput(ScreenMainMenu)
    .nextDown(btnNext.isDown())
    .prevDown(btnPrev.isDown())
    .apply();
```

Отрисовка в screen-callback:

```cpp
SCREEN(ScreenMainMenu, 1)
{
    ui.updateList(ScreenMainMenu);
}
```

Поведение:

- короткое отпускание `NEXT` переключает пункт вперёд;
- короткое отпускание `PREV` переключает пункт назад;
- удержание `NEXT` открывает `targetScreen` выбранного пункта;
- удержание `PREV` возвращает на `parentScreen`.

Режимы списка:

- `Cards`
- `Plain`

## 9.2. Плиточное меню

Конфигурация:

```cpp
ui.configureTile()
    .screen(ScreenTiles)
    .parent(ScreenHome)
    .items({
        {"Main",     "Главный экран", ScreenHome},
        {"Settings", "Настройки",     ScreenSettings},
        {"Info",     "Инфо",          ScreenInfo},
        {"Graph",    "Графики",       ScreenGraph}
    })
    .cardColor(ui.rgb(16, 16, 16))
    .activeColor(ui.rgb(0, 120, 255))
    .radius(12)
    .spacing(10)
    .columns(2)
    .tileSize(100, 70)
    .lineGap(5)
    .mode(TextSubtitle)
    .apply();
```

Ввод:

```cpp
// btnNext.update() и btnPrev.update() уже вызваны в начале этого loop()
ui.tileInput(ScreenTiles)
    .nextDown(btnNext.isDown())
    .prevDown(btnPrev.isDown())
    .apply();
```

Отрисовка:

```cpp
SCREEN(ScreenTiles, 2)
{
    ui.renderTile(ScreenTiles);
}
```

Режимы плитки:

- `TextOnly`
- `TextSubtitle`

## 9.3. Кастомная раскладка плиток

Если стандартной сетки мало, можно описать layout строками:

```cpp
ui.configureTile()
    .screen(ScreenTiles)
    .items({
        {"Main",     "Главный экран", ScreenHome},
        {"Settings", "Настройки",     ScreenSettings},
        {"Info",     "Инфо",          ScreenInfo},
        {"Graph",    "Графики",       ScreenGraph}
    })
    .layout({
        "AA",
        "BC",
        "BD"
    })
    .apply();
```

Это advanced-режим. Для большинства экранов хватает обычных `columns(...)`, `spacing(...)` и `tileSize(...)`.

---

# 10. Графики

Фон и сетка:

```cpp
ui.drawGraphGrid(
    10, 50, 220, 120,
    8,
    LeftToRight,
    ui.rgb(10, 10, 10),
    ui.rgb(40, 40, 40),
    1.0f
);
```

Auto-scale:

```cpp
ui.setGraphAutoScale(true);
```

Линия графика:

```cpp
ui.drawGraphLine(
    0,                  // index линии
    sensorValue,
    ui.rgb(0, 255, 140),
    0, 100
);
```

Есть и `updateGraphGrid(...)` / `updateGraphLine(...)`.

Режимы направления:

- `LeftToRight`
- `RightToLeft`
- `Oscilloscope`

---

# 11. Статус-бар

## 11.1. Включение

```cpp
ui.configureStatusBar(
    true,
    ui.rgb(0, 0, 0),
    18,
    Top
);
```

Позиции:

- `Top`
- `Bottom`

## 11.2. Стиль

```cpp
ui.setStatusBarStyle(StatusBarStyleSolid);
// или
ui.setStatusBarStyle(StatusBarStyleBlurGradient);
```

## 11.3. Текст

```cpp
ui.setStatusBarText(
    "pipGUI",
    "12:34",
    "Wi-Fi"
);
```

## 11.4. Батарея

```cpp
ui.setStatusBarBattery(87, Numeric);
ui.setStatusBarBattery(100, Bar);
ui.setStatusBarBattery(-1, Hidden);
```

`BatteryStyle`:

- `Hidden`
- `Numeric`
- `Bar`
- `WarningBar`
- `ErrorBar`

## 11.5. Кастомная дорисовка

```cpp
void statusBarCustom(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h)
{
    ui.drawLine()
        .from(x, y + h - 1)
        .to(x + w, y + h - 1)
        .color(ui.rgb(70, 70, 70))
        .draw();
}

ui.setStatusBarCustom(statusBarCustom);
```

Вспомогательные методы:

```cpp
int16_t h = ui.statusBarHeight();
ui.updateStatusBar();
ui.renderStatusBar();
```

---

# 12. Уведомления, toast, ошибки

## 12.1. Toast

```cpp
ui.showToast()
    .text("Saved")
    .duration(1500)
    .fromTop()
    .show();
```

Проверка активности:

```cpp
bool active = ui.toastActive();
```

## 12.2. Notification

```cpp
ui.showNotification()
    .title("Settings")
    .message("Changes applied")
    .button("OK")
    .delay(0)
    .type(Normal)
    .show();
```

Типы уведомления:

- `Normal`
- `Warning`
- `Error`

Управление:

```cpp
bool active = ui.notificationActive();
ui.setNotificationButtonDown(btnOk.isDown());
```

## 12.3. Popup menu

```cpp
ui.showPopupMenu()
    .items(&itemProvider, userData, itemCount)
    .pos(160, 40)
    .width(120)
    .selected(0)
    .maxVisible(6);
```

Во время активности:

```cpp
if (ui.popupMenuActive())
{
    ui.popupMenuInput().nextDown(btnNext.isDown()).prevDown(btnPrev.isDown());

    if (ui.popupMenuHasResult())
    {
        int16_t picked = ui.popupMenuTakeResult();
    }
}
```

- `popupMenuActive()` показывает, что меню перехватило ввод
- `popupMenuHasResult()` сообщает, что выбор уже готов
- `popupMenuTakeResult()` возвращает индекс и одновременно сбрасывает флаг результата

## 12.4. Ошибки

```cpp
ui.showError("Low battery", "Please charge device", Warning, "OK");
ui.showError("LittleFS mount failed", "Code: 0xLFS", Error, "Retry");
```

Управление:

```cpp
bool active = ui.errorActive();
ui.setErrorButtonDown(btnOk.isDown());
ui.nextError();
```

---

# 13. Layout helpers

Это лёгкие helper-ы для раскладки UI без тяжёлой layout-системы.

Базовые типы:

```cpp
UiSize   size{120, 40};
UiRect   rect{0, 0, 240, 320};
UiInsets insets{10, 10, 10, 10};
```

## 13.1. Slicing API

```cpp
UiRect root{0, 0, 240, 320};
UiRect work = inset(root, 10);

UiRect header = takeTop(work, 24, 8);
UiRect footer = takeBottom(work, 28, 8);
UiRect content = work;
```

Доступно:

- `inset(rect, all)`
- `inset(rect, l, t, r, b)`
- `inset(rect, UiInsets{...})`
- `takeTop(...)`
- `takeBottom(...)`
- `takeLeft(...)`
- `takeRight(...)`
- `placeInside(...)`
- `centerIn(...)`

## 13.2. Flow API

```cpp
UiSize sizes[3] = {{40, 20}, {60, 20}, {40, 20}};
UiRect out[3];

flowRow(area, sizes, out, 3, 10, Center, Center);
flowColumn(area, sizes, out, 3, 8, Center, Center);
```

Для распределения доступны:

- `Start`
- `Center`
- `End`
- `layout::SpaceBetween`
- `layout::SpaceEvenly`

## 13.3. Cursor-based API

```cpp
UiFlowRow<3> row(area, 10, layout::SpaceEvenly, Center);

row.next(40, 24);
row.next(60, 24);
row.next(40, 24);
row.finish();
```

Аналогично работает `UiFlowColumn<N>`.

Как этим пользоваться правильно:

- `next(w, h)` резервирует следующий slot
- после всех `next(...)` нужно вызвать `finish()`
- итоговые прямоугольники потом берутся через `row[i]` или `column[i]`

Для позиционирования и выравнивания часто используются короткие шорткаты:

- `center` для автоматического центрирования по оси
- `Left`, `Center`, `Right`
- `Top`, `Bottom`

---

# 14. Скриншоты

## 14.1. Быстрый старт

Минимум для работы:

```cpp
ui.setScreenshotShortcut(&btnNext, &btnPrev, 500);
```

Теперь удержание двух кнопок 500мс делает скрин. В момент удержания UI не реагирует на кнопки. После завершения будет toast "Screenshot saved".

## 14.2. Программный запуск

```cpp
bool ok = ui.startScreenshot(); // асинхронно, не блокирует GUI
```

Если запускаете вручную — тост можно показать своим кодом (встроенный тост ставится только при захвате через shortcut).

## 14.3. Галерея миниатюр

```cpp
ui.configureScreenshotGallery(12, 64, 40, 6);

SCREEN(ScreenScreenshots, 10)
{
    ui.clear(ui.rgb(10, 10, 10));
    ui.drawScreenshot()
        .pos(8, 28)
        .size(224, 284)
        .grid(3, 5)
        .padding(8);
}
```

Поведение зависит от `PIPGUI_SCREENSHOT_MODE`.

В режиме `1` (Serial) миниатюры создаются из текущего sprite и живут только в RAM (после перезагрузки не сохраняются).

В режиме `2` (LittleFS) скрины сохраняются во flash в `/pipKit/screenshots/`. Галерея показывает миниатюры: для каждого размера (`thumbW x thumbH`) она один раз генерирует и сохраняет их в `/pipKit/thumbnails/<WxH>/` (Area resampling), а при следующих входах читает готовые без повторной генерации.

Количество можно узнать так:

```cpp
uint8_t n = ui.screenshotCount();
```

## 14.4. Build-time флаги

- `PIPGUI_SCREENSHOTS`
  - `1` по умолчанию
  - `0` полностью выключает систему скриншотов
- `PIPGUI_SCREENSHOT_MODE`
  - `1` — Serial стрим (PSCR)
  - `2` — запись в LittleFS (flash)
Для режима `2` нужен LittleFS (в `platformio.ini`: `board_build.filesystem = littlefs`). Библиотека сама вызывает `LittleFS.begin(false)` при первом скриншоте/открытии галереи и создаёт структуру `/pipKit/` (включая папки `screenshots/` и `thumbnails/`).

Важно: библиотека не делает auto-format (не стирает данные). Если mount LittleFS не удался — скриншоты/галерея просто не будут работать, а причина будет выведена в `Serial`.

## 14.5. Формат PSCR (Serial)

Для захвата на ПК используйте скрипт:

```
python tools/screenshots/bin/capture.py --port COM9 --baud 1000000
```

PSCR по Serial — это простой контейнер:

- Header (13 байт): `PSCR` + `w(uint16 LE)` + `h(uint16 LE)` + `format(uint8)` + `payloadSize(uint32 LE)`
- Payload: байтовый поток QOI-opcodes (для `format = QoiRgb`)

Для Serial допускается `payloadSize = 0` (приёмник декодит до восстановления `w*h` пикселей).

## 14.6. Скриншоты во flash (LittleFS)

В режиме `2` скриншоты сохраняются как файлы:

- Full: `/pipKit/screenshots/pscr_00000001.pscr`
- Thumbnails: `/pipKit/thumbnails/<WxH>/pscr_00000001.pscr` (имя такое же, как у full-скрина)

Галерея сортирует по stamp: **новые сверху**, старые ниже.

### 14.6.1. Надёжная запись (tmp → rename)

Все записи делаются через временный файл:

- запись в `.../pscr_XXXXXXXX.tmp`
- `flush()` + дописывание trailer/CRC
- `close()`
- `rename(tmp, final)`

Если на любом шаге произошёл сбой — финальный файл не создаётся, а `.tmp` удаляется. Оставшиеся `.tmp` (включая `.counter.tmp`) очищаются при сканировании директории.

### 14.6.2. Формат `.pscr` (LittleFS)

Файл во flash — это PSCR header + payload + trailer:

- Header (13 байт): `PSCR` + `w(uint16 LE)` + `h(uint16 LE)` + `format(uint8)` + `payloadSize(uint32 LE)`
- Payload: QOI-opcodes stream (для `format = QoiRgb`)
- Trailer (16 байт): `PSCT` + `ver(uint8)` + `flags(uint8)` + `reserved(2)` + `payloadSize(uint32 LE)` + `crc32(payload)(uint32 LE)`

Файл считается валидным только если:

- `fileSize == header + payloadSize + trailer`
- trailer присутствует и версия совпадает
- `payloadSize` в header и trailer совпадают
- CRC32 payload совпадает

Старые файлы без trailer/CRC (созданные до добавления CRC) не поддерживаются и будут игнорироваться.

---

# 15. Wi‑Fi

Минимальное управление:

```cpp
ui.requestWiFi(true);

if (ui.wifiConnected())
{
    uint32_t ip = ui.wifiLocalIpV4();
}
```

- `requestWiFi(true)` запускает подключение
- `requestWiFi(false)` останавливает Wi‑Fi
- `wifiConnected()` возвращает текущее состояние
- `wifiLocalIpV4()` возвращает IPv4 в packed `uint32_t`

---

# 16. OTA в `pipGUI` и tooling

> Важно: для OTA нужна A/B разметка (два OTA app-слота) в partition table.

## 16.1. Конфигурация (`config.hpp`)

- `PIPGUI_OTA` — `1` включает OTA модуль
- `PIPGUI_OTA_PROJECT_URL` — HTTPS base на `/fw/<project>` (без `/index.json` в конце)
- `PIPGUI_OTA_ED25519_PUBKEY_HEX` — публичный ключ Ed25519 (32 байта, hex)
- `PIPGUI_FIRMWARE_TITLE` — имя прошивки (показывается в UI)
- `PIPGUI_FIRMWARE_VER_MAJOR`, `PIPGUI_FIRMWARE_VER_MINOR`, `PIPGUI_FIRMWARE_VER_PATCH` — текущая версия `X.Y.Z`

## 16.2. Manifest

Поля:

- `title` (string) — имя прошивки (показывается в UI)
- `version` (string `X.Y.Z`)
- `build` (number) — монотонный номер версии, **должен совпадать с**:
  - `build = major*1_000_000 + minor*1_000 + patch`
- `size` (number, bytes)
- `sha256` (hex, 64 chars)
- `url` (https)
- `desc` (string) — описание обновления (можно `""`)
- `sig_ed25519` (hex, 128 chars) — обязательная подпись Ed25519

Подпись считается **не от JSON**, а от payload v5, собранного из полей манифеста:
`prefix + title + 0x00 + version + 0x00 + build(u64 BE) + size(u32 BE) + sha256(32) + url(utf-8) + 0x00 + desc(utf-8)`.

`prefix` = `pipgui-ota-manifest-v5`.

## 16.3. Использование (UI)

OTA — неблокирующая state-machine, обслуживается из `GUI::loop()`:

- Инициализация (один раз на старте): `ui.otaConfigure()`
- Проверка обновления: `ui.otaRequestCheck()`
  - порядок: **stable -> beta** (если в stable нет апдейта, проверяется beta)
- Установка: `ui.otaRequestInstall()`
- История stable (rollback UI):
  - запросить список: `ui.otaRequestStableList()`
  - читать: `ui.otaStableListReady()`, `ui.otaStableListCount()`, `ui.otaStableListVersion(i)`
  - установить конкретную stable-версию: `ui.otaRequestInstallStableVersion("1.2.3")`
- Статус: `ui.otaStatus()`

После установки и перезапуска прошивка стартует в режиме pending-verify (`pendingVerify = true`).
Вы можете явно подтвердить успех через `ui.otaMarkAppValid()` после своего health-check, либо оставить это на встроенную автоматическую логику.

## 16.4. Тулинг (`tools/ota/`)

- Сгенерировать ключ: `python tools/ota/keygen.py` (публичный ключ вставить в `PIPGUI_OTA_ED25519_PUBKEY_HEX`)
- Подготовить релиз (bin + manifest) в `tools/ota/out/`:
  - stable: `python tools/ota/make_release.py --bin .pio/build/<env>/firmware.bin --channel stable --title "Pipboy OS" --version 1.3.8 --site-base https://<domain>/fw --desc "Bugfixes and improvements"`
  - beta: `python tools/ota/make_release.py --bin .pio/build/<env>/firmware.bin --channel beta --title "Pipboy OS" --version 1.3.9 --site-base https://<domain>/fw --desc-file notes.txt`
- Структура на сайте (после деплоя `tools/ota/out/<project>/...` в `/fw/<project>/...`):
  - `/fw/<project>/index.json`
  - `/fw/<project>/stable/index.json`, `/fw/<project>/beta/index.json`
  - `/fw/<project>/<channel>/<version>/manifest.json`
  - `/fw/<project>/<channel>/<version>/fw.bin`
- Быстрая проверка: `python tools/ota/verify_manifest.py --manifest tools/ota/out/<project>/stable/1.3.8/manifest.json --bin tools/ota/out/<project>/stable/1.3.8/fw.bin --pubkey <PIPGUI_OTA_ED25519_PUBKEY_HEX>`



