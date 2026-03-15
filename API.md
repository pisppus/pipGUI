# pipKit API

Этот файл описывает актуальный публичный API `pipGUI` и `pipCore`, который есть в коде проекта.

---

# 1. Build-time флаги

Низкий слой `pipCore` выбирает платформу и драйвер дисплея на этапе компиляции. 

- `PIPCORE_DISPLAY`
  - пример: `-DPIPCORE_DISPLAY=ST7789`
- `PIPCORE_PLATFORM`
  - пример: `-DPIPCORE_PLATFORM=ESP32`

Если в сборке доступен только один backend, `pipCore` может выбрать его автоматически. В текущем проекте это обычно значит:

- платформа: `ESP32`
- дисплей: `ST7789`

Явно задавать флаги имеет смысл, если вы хотите жёстко зафиксировать конфигурацию в `platformio.ini` и не полагаться на auto-detect.

Отдельно есть diagnostic-флаг `pipGUI` для автологирования ошибок в `Serial`:

- `PIPGUI_SERIAL_ERRORS`
  - по умолчанию: `1`
  - чтобы выключить: `-DPIPGUI_SERIAL_ERRORS=0`

Он не выбирает платформу или дисплей. Он только включает или отключает автоматический вывод ошибок `configureDisplay()`, `begin()` и display I/O в `Serial`.

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

Если удобнее держать пины одной структурой, можно так:

```cpp
DisplayPins dispPins{11, 12, 10, 9, -1};

ui.configureDisplay()
    .pins(dispPins)
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

Что делает каждая настройка:

- `pins(...)` задаёт пины SPI и управляющие пины дисплея
- `size(width, height)` задаёт логический размер панели
- `hz(freq)` задаёт частоту SPI
- `order("RGB")` / `order("BGR")` переключает порядок цветовых каналов контроллера
- `invert(bool)` включает или отключает инверсию
- `swap(bool)` включает swap байтов RGB565 перед отправкой
- `offset(x, y)` задаёт сдвиг видимой области в памяти контроллера

## 2.2. Запуск GUI

После конфигурации дисплея вызывается `begin()`:

```cpp
ui.begin(
    0,                  // rotation: 0..3
    ui.rgb(0, 0, 0)     // bgColor
);
```

- `rotation` принимает значения `0..3`.
- `bgColor` задаёт цвет фона GUI при старте.

## 2.3. Подсветка и яркость

```cpp
ui.setBacklightPin(
    48,    // pin
    0,     // PWM channel
    5000,  // PWM frequency
    12     // PWM resolution bits
);
```

Что важно:

- обязательный только `pin`
- `channel`, `freqHz` и `resolutionBits` можно не указывать
- после `setBacklightPin(...)` библиотека сама подключает стандартное PWM-управление подсветкой через платформу
- если вы используете `LightFade` в boot-анимации, вызовите `setBacklightPin(...)` до этой анимации

### Максимальная яркость

```cpp
ui.setMaxBrightness(70);   // ограничить максимум 70%
```

Что это значит:

- диапазон `0..100`
- это верхний лимит яркости, а не команда “установить текущую яркость прямо сейчас”
- библиотека использует этот лимит, когда сама меняет яркость
- на текущей ESP32-платформе значение сохраняется и потом подгружается автоматически

### Своя логика управления подсветкой

Если стандартный PWM-путь не подходит, можно передать свой callback:

```cpp
ui.setBacklightCallback([](uint16_t level) {
    // level: 0..100
    // здесь своя логика управления подсветкой
});
```

Это полезно, если у вас:

- нестандартная схема управления подсветкой
- внешний драйвер яркости
- своя таблица уровней или нелинейная шкала

Обычно используют что-то одно:

- либо `setBacklightPin(...)` для стандартного пути
- либо `setBacklightCallback(...)` для своей логики

### Когда `pipGUI` сам меняет яркость

Сейчас это используется в первую очередь для boot-анимации `LightFade`:

- во время анимации яркость плавно растёт
- в конце выставляется `maxBrightness()`

Если вам нужен не лимит, а отдельный публичный API вида “поставить яркость 35% прямо сейчас”, это уже другая задача и сейчас такого метода в `pipGUI` нет.

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

Если `loopWithInput(...)` не используется, вызовите `begin()` в `setup()`, потом `update()` один раз в начале каждого `loop()`.
После этого `wasPressed()` и `isDown()` просто читают уже обновлённое состояние кнопки.

## 4.3. Управление экранами

```cpp
ui.setScreen(ScreenHome);
ui.nextScreen();
ui.prevScreen();

uint8_t current = ui.currentScreen();
bool animActive = ui.screenTransitionActive();
```

## 4.4. Принудительная перерисовка

```cpp
ui.requestRedraw();
```

Нужно, когда вы изменили данные экрана извне и хотите гарантированно перерисовать следующий кадр.

## 4.5. Анимация переходов

```cpp
ui.setScreenAnimation(SlideX, 250);
```

Доступные режимы:

- `ScreenAnimNone`
- `SlideX`
- `SlideY`

Через токен `None` тоже можно:

```cpp
ui.setScreenAnimation(None, 0);
```

---

# 5. Базовые helpers

## 5.1. Размеры и доступ к низкому уровню

```cpp
uint16_t w = ui.screenWidth();
uint16_t h = ui.screenHeight();

pipcore::Display &disp = ui.display();
pipcore::Platform *plat = ui.platform();
```

`display()` и `platform()` нужны в основном для advanced-кейсов.

## 5.2. Цвет

Обычный способ:

```cpp
uint16_t c = ui.rgb(255, 0, 72);
```

Есть и `GUI::rgb888(...)` для API, которым нужен 24-битный цвет, но в большинстве пользовательских кейсов хватает `ui.rgb(...)`.

## 5.3. Очистка экрана

```cpp
ui.clear(ui.rgb(0, 0, 0));
```

## 5.4. Клип

```cpp
ui.setClip(10, 20, 120, 80);
// ... рисование только внутри области ...
ui.clearClip();
```

---

# 6. Текст, шрифты и иконки

## 6.1. Встроенные шрифты

Встроенные `FontId`:

- `WixMadeForDisplay`
- `KronaOne`

Выбирать шрифт можно так:

```cpp
ui.setFont(WixMadeForDisplay);
ui.setFontSize(18);
ui.setFontWeight(Semibold);
```

Текущие значения можно читать:

```cpp
uint16_t px = ui.fontSize();
uint16_t weight = ui.fontWeight();
```

Доступные токены толщины:

- `Thin`
- `Light`
- `Regular`
- `Medium`
- `Semibold`
- `Bold`
- `Black`

## 6.2. Текстовые стили

```cpp
ui.configureTextStyles(
    24, // H1
    18, // H2
    14, // Body
    12  // Caption
);

ui.setTextStyle(H1);
```

`TextStyle`:

- `H1`
- `H2`
- `Body`
- `Caption`

## 6.3. Обычный текст

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
    .draw();
```

Для in-place обновления есть такой же builder:

```cpp
ui.updateText()
    .pos(center, 32)
    .text("Updated")
    .color(ui.rgb(255, 255, 255))
    .align(AlignCenter)
    .draw();
```

## 6.4. Бегущая строка и ellipsis

```cpp
ui.drawTextMarquee()
    .pos(20, 80)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
    .speed(30)
    .holdStart(700)
    .draw();
```

```cpp
ui.drawTextEllipsized()
    .pos(20, 110)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
    .draw();
```

## 6.5. Иконки

```cpp
ui.drawIcon()
    .pos(20, 20)
    .size(18)
    .icon(WarningLayer0)
    .color(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
    .draw();
```

Иконки берутся из вашего набора `IconId`.

## 6.6. Логотип

```cpp
ui.logoSizesPx(36, 20);

ui.showLogo(
    "PISPPUS",
    "Digital Thermometer",
    ZoomIn,
    ui.rgb(255, 255, 255),
    ui.rgb(0, 0, 0),
    1800,
    center,
    40
);
```

Параметры:

- `title` и `subtitle` — строки логотипа;
- `animation` вЂ” `None`, `FadeIn`, `SlideUp`, `LightFade`, `ZoomIn`;
- `fgColor` и `bgColor` — цвета;
- `durationMs` — длительность анимации;
- `x`, `y` — позиция.

Отдельно можно настроить размеры:

```cpp
ui.logoTitleSizePx(36);
ui.logoSubtitleSizePx(20);
```

## 6.7. Кастомные шрифты

Для обычного использования это не нужно, но API есть:

```cpp
FontId id = ui.registerFont(
    atlasData,
    atlasWidth, atlasHeight,
    distanceRange,
    nominalSizePx,
    ascender, descender, lineHeight,
    glyphs,
    glyphCount
);

ui.setFont(id);
```

---

# 7. Рисование: Fluent API

Общий стиль у builder-ов один:

```cpp
ui.fillRect()
    .pos(20, 40)
    .size(100, 40)
    .color(ui.rgb(0, 120, 255))
    .draw();
```

Рекомендуемый стиль:

- `pos(...)` или `from(...)`
- потом `size(...)` / `radius(...)`
- потом цвет и опции
- в конце `.draw()`

## 7.1. Прямоугольники

Заливка:

```cpp
ui.fillRect()
    .pos(20, 40)
    .size(100, 40)
    .color(ui.rgb(0, 120, 255))
    .draw();
```

Контур:

```cpp
ui.drawRect()
    .pos(20, 90)
    .size(100, 40)
    .color(ui.rgb(255, 255, 255))
    .radius({10})
    .draw();
```

`drawRect()` и `fillRect()` умеют:

- `radius({r})` — один радиус для всех углов;
- `radius({tl, tr, br, bl})` — отдельные радиусы по углам.

## 7.2. Градиенты

Вертикальный:

```cpp
ui.gradientVertical()
    .pos(20, 20)
    .size(120, 40)
    .topColor(ui.rgb(255, 0, 72))
    .bottomColor(ui.rgb(0, 87, 250))
    .draw();
```

Горизонтальный:

```cpp
ui.gradientHorizontal()
    .pos(20, 70)
    .size(120, 40)
    .leftColor(ui.rgb(255, 128, 0))
    .rightColor(ui.rgb(80, 255, 120))
    .draw();
```

4 угла:

```cpp
ui.gradientCorners()
    .pos(20, 120)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 0, 72))
    .topRightColor(ui.rgb(0, 87, 250))
    .bottomLeftColor(ui.rgb(80, 255, 120))
    .bottomRightColor(ui.rgb(255, 128, 0))
    .draw();
```

Диагональный:

```cpp
ui.gradientDiagonal()
    .pos(20, 170)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 255, 255))
    .bottomRightColor(ui.rgb(30, 30, 30))
    .draw();
```

Радиальный:

```cpp
ui.gradientRadial()
    .pos(20, 220)
    .size(120, 60)
    .center(80, 250)
    .radius(40)
    .innerColor(ui.rgb(255, 255, 255))
    .outerColor(ui.rgb(0, 87, 250))
    .draw();
```

## 7.3. Линия и фигуры

Линия:

```cpp
ui.drawLine()
    .from(20, 20)
    .to(140, 60)
    .color(ui.rgb(255, 255, 255))
    .draw();
```

Круг:

```cpp
ui.fillCircle()
    .pos(50, 50)
    .radius(18)
    .color(ui.rgb(0, 87, 250))
    .draw();

ui.drawCircle()
    .pos(50, 50)
    .radius(18)
    .color(ui.rgb(255, 255, 255))
    .draw();
```

Дуга:

```cpp
ui.drawArc()
    .pos(100, 80)
    .radius(28)
    .startDeg(-90.0f)
    .endDeg(90.0f)
    .color(ui.rgb(80, 255, 120))
    .draw();
```

Эллипс:

```cpp
ui.fillEllipse()
    .pos(120, 50)
    .radiusX(28)
    .radiusY(16)
    .color(ui.rgb(255, 0, 72))
    .draw();
```

Треугольник:

```cpp
ui.fillTriangle()
    .point0(40, 120)
    .point1(70, 80)
    .point2(100, 120)
    .color(ui.rgb(0, 200, 120))
    .draw();
```

Сквиркль:

```cpp
ui.fillSquircle()
    .pos(80, 160)
    .radius(26)
    .color(ui.rgb(255, 128, 0))
    .draw();
```

## 7.4. Blur

Рисование blur-области:

```cpp
ui.drawBlur()
    .pos(0, 180)
    .size(240, 40)
    .radius(10)
    .direction(TopDown)
    .gradient(true)
    .materialStrength(160)
    .materialColor(-1)
    .draw();
```

Для обновления части экрана:

```cpp
ui.updateBlur()
    .pos(0, 180)
    .size(240, 40)
    .radius(10)
    .direction(TopDown)
    .draw();
```

Что делают параметры:

- `materialStrength(...)` задаёт силу цветного материала поверх blur
- `materialColor(...)` задаёт цвет материала в `RGB565`; `-1` значит взять текущий цвет фона библиотеки

## 7.5. Glow

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
    .draw();
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
    .draw();
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

## 12.3. Ошибки

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
Для режима `2` нужен LittleFS (в `platformio.ini`: `board_build.filesystem = littlefs`). Библиотека сама вызывает `LittleFS.begin(true)` при первом скриншоте/открытии галереи и создаёт структуру `/pipKit/` (включая папки `screenshots/` и `thumbnails/`).

## 14.5. Формат PSCR (Serial)

Для захвата на ПК используйте скрипт:

```
python tools/screenshots/bin/capture.py --port COM9 --baud 1000000
```
