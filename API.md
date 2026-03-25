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

### Низкоуровневые helpers подсветки и дисплея

```cpp
bool ok = ui.displayReady();
ui.setBacklightCallback(myBacklightFn);
```

- `displayReady()` показывает, что backend дисплея уже поднят и готов к выводу
- `setBacklightCallback(...)` позволяет подставить свой callback для управления подсветкой
- обычно этот API не нужен, если хватает `setBacklight()`

---

# 3. Базовые helpers

## 3.1. Размеры

```cpp
ui.screenWidth();   // ширина вашего экрана
ui.screenHeight();  // высота вашего экрана
```

- `screenWidth()` и `screenHeight()` возвращают уже активный логический размер экрана
- удобно для центровки, адаптивных отступов и расчёта layout без хардкода

## 3.2. Цвет

```cpp
ui.rgb(255, 255, 255);
```

- переводит обычный `RGB888` в внутренний `RGB565`
- это основной helper для всех цветов в fluent API

Для повторно используемых системных accent-цветов есть короткие semantic-токены:

```cpp
Warning; // #FF6200
Error;   // #FF0048
```

- их можно передавать в `color(...)`, `fillColor(...)`, `bgColor(...)` и другие места, где ожидается `RGB565`
- те же токены уже используются и как semantic-type в API уведомлений/ошибок, так что отдельный namespace для цветов не нужен

## 3.3. Очистка экрана

```cpp
ui.clear(ui.rgb(0, 0, 0));
```

- очищает весь текущий draw target указанным цветом
- обычно используется в начале screen-callback перед остальной отрисовкой

## 3.4. Клип

```cpp
ui.setClip()
    .pos(10, 20)
    .size(120, 80);

// ... рисование только внутри области ...

ui.clearClip();
```

- `setClip()` ограничивает всю последующую отрисовку прямоугольной областью
- удобно для локальных redraw, списков, анимаций и виджетов в карточках
- `clearClip()` возвращает обычную отрисовку без ограничений

Если прямоугольник уже посчитан вручную, можно передать его одной fluent-операцией:

```cpp
ui.setClip().rect(10, 20, 120, 80);
```

## 4. Логотип

```cpp
ui.showLogo()
    .title("PISPPUS")
    .subtitle("Digital Thermometer")
    .anim(FadeIn)
    .fgColor(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
    .duration(1800)
    .pos(center, 40);
```

`showLogo()` запускает полноэкранную boot-анимацию с логотипом.
Размер текста библиотека подбирает сама под текущее разрешение экрана.

Параметры:

- `title` и `subtitle` — строки логотипа;
- `anim(...)` — `None`, `FadeIn`, `LightFade`;
- `fgColor` и `bgColor` — цвета логотипа и фона; можно передавать как `ui.rgb(...)`, так и `RGB565`
- `duration(...)` — длительность анимации;
- `pos(x, y)` — позиция.

Что делают анимации:

- `None` - просто сразу показывает логотип без анимации
- `FadeIn` - плавно проявляет текст из цвета фона
- `LightFade` - плавно поднимает яркость подсветки; для неё backlight должен быть заранее настроен

---

# 5. Экраны и цикл

## 5.1. Регистрация экранов

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

## 5.2. Основной цикл

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

Если нужен готовый snapshot ввода для собственного state-machine:

```cpp
GUI::InputState input = ui.pollInput(btnNext, btnPrev);
```

В `InputState` есть поля:

- `nextDown`
- `prevDown`
- `nextPressed`
- `prevPressed`
- `comboDown`

## 5.3. Управление экранами

```cpp
ui.setScreen(ScreenHome);
uint8_t cur = ui.currentScreen();
ui.nextScreen();
ui.prevScreen();
ui.screenTransitionActive();
```

- `currentScreen()` возвращает id текущего активного экрана
- библиотека автоматически ведёт navigation-history при переходах между экранами
- `prevScreen()` возвращает по navigation-history, а не по порядку

## 5.4. Принудительная перерисовка

```cpp
ui.requestRedraw();
```

Нужно, когда вы изменили данные экрана извне и хотите гарантированно перерисовать следующий кадр.

## 5.5. Анимация переходов

```cpp
ui.setScreenAnim(SlideX, 250);
```

Доступные режимы:

- `None`
- `SlideX`
- `SlideY`

---

# 6. Текст, шрифты и иконки

## 6.1. Встроенные шрифты

Выбирать шрифт можно так:

```cpp
ui.setFont(WixMadeForDisplay);
ui.setFontSize(18);
```

Доступны встроенные в библиотеку:
- `WixMadeForDisplay`
- `KronaOne`

Что за что отвечает:

- `setFont(...)` выбирает семейство шрифта
- `setFontSize(...)` задаёт текущий размер в пикселях
- `setFontWeight(...)` задаёт насыщенность начертания для PSDF-рендера

Текущие значения можно читать:

```cpp
ui.fontId();      // текущий FontId
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

## 6.2. Текстовые стили

```cpp
ui.setTextStyle(H1);
```

`TextStyle`:

- `H1` - крупный заголовок
- `H2` - подзаголовок или вторичный заголовок
- `Body` - основной текст интерфейса
- `Caption` - мелкие подписи и пояснения

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
    .align(Center)
```

Что задают параметры:

- `pos(x, y)` - точка привязки текста
- `font(...)`, `size(...)`, `weight(...)` - конкретный шрифт, размер и насыщенность
- `text(...)` - сама строка
- `color(...)` - цвет текста
- `bgColor(...)` - фон под текстом; особенно полезен для `updateText()`, чтобы корректно затирать старое значение
- `align(...)` - выравнивание относительно точки `pos(...)`

Для in-place обновления есть такой же builder:

```cpp
ui.updateText()
    .pos(center, 32)
    .text("Updated")
    .color(ui.rgb(255, 255, 255))
    .align(Center)
```

- `drawText()` рисует текст как есть
- `updateText()` сначала чистит прошлую область и потом рисует новое значение на том же месте

## 6.4. Бегущая строка и многоточие

```cpp
ui.drawTextMarquee()
    .pos(20, 80)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
    .speed(30)
    .holdStart(700)
```

- `maxWidth(...)` задаёт ширину области, внутри которой строка должна уместиться
- `speed(...)` задаёт скорость прокрутки в пикселях в секунду
- `holdStart(...)` задаёт паузу перед началом движения
- `phaseStart(...)` позволяет стартовать marquee не с нуля, если нужна синхронизация
- `align(...)` определяет якорь строки внутри области

```cpp
ui.drawTextEllipsized()
    .pos(20, 110)
    .maxWidth(140)
    .text("Very long text that does not fit")
    .color(ui.rgb(255, 255, 255))
```

- `drawTextEllipsized()` обрезает строку по `maxWidth(...)` и добавляет многоточие

## 6.5. Иконки

```cpp
ui.drawIcon()
    .pos(20, 20)
    .size(18)
    .icon(warning)
    .color(ui.rgb(255, 255, 255))
    .bgColor(ui.rgb(0, 0, 0))
```

Иконки берутся из вашего набора `IconId`.

Что задают параметры:

- `pos(...)` - позиция иконки
- `size(...)` - итоговый размер в пикселях
- `icon(...)` - конкретный `IconId`
- `color(...)` - основной цвет слоя
- `bgColor(...)` - фон под иконкой, полезен для чистого обновления поверх уже нарисованного интерфейса

Если иконка многослойная, слои можно рисовать отдельно разными цветами:

```cpp
ui.drawIcon()
    .pos(200, 10)
    .size(18)
    .icon(battery_l0)
    .color(ui.rgb(255, 255, 255));

ui.drawIcon()
    .pos(200, 10)
    .size(18)
    .icon(battery_l1)
    .color(ui.rgb(0, 200, 120));
```

---

# 7. Фигуры

Приставка fill - это фигуры с заливкой, draw - с контуром

## 7.1 Линия

```cpp
ui.drawLine()
    .from(20, 20)
    .to(140, 60)
    .thickness(2)
    .color(ui.rgb(255, 255, 255))
```

- `from(...)` и `to(...)` задают начало и конец линии
- `thickness(...)` задаёт толщину линии; по умолчанию `1`
- `color(...)` задаёт цвет линии

## 7.2 Прямоугольник

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
- `pos(...)` и `size(...)` задают левый верхний угол и размер
- `fillRect()` рисует сплошную фигуру, `drawRect()` только контур

## 7.3 Круг

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

- `pos(...)` - центр круга
- `radius(...)` - радиус
- `fillCircle()` рисует диск, `drawCircle()` только окружность

## 7.4 Треугольник

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

- `point0/1/2(...)` задают три вершины
- `radius(...)` можно добавить для скругления углов
- `fillTriangle()` заливает фигуру, `drawTriangle()` рисует контур

## 7.5 Скруглённый треугольник

```cpp
ui.fillTriangle()
    .point0(140, 120)
    .point1(170, 80)
    .point2(200, 120)
    .radius(10)
    .color(ui.rgb(0, 120, 255))
```

- `radius(...)` здесь задаёт степень скругления вершин

## 7.6 Дуга

```cpp
ui.drawArc()
    .pos(100, 80)
    .radius(28)
    .thickness(6)
    .startDeg(-90.0f)
    .endDeg(90.0f)
    .color(ui.rgb(80, 255, 120))
```

- `pos(...)` - центр дуги
- `radius(...)` - внешний радиус дуги
- `thickness(...)` - толщина дуги; по умолчанию `1`
- `startDeg(...)` и `endDeg(...)` задают углы в градусах
- концы дуги всегда скруглённые
- `0..360` рисует полный круг

## 7.7 Эллипс

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

- `radiusX(...)` и `radiusY(...)` задают полуоси
- `fillEllipse()` заливает фигуру, `drawEllipse()` рисует контур

## 7.8 Сквиркль

```cpp
ui.fillSquircleRect()
    .pos(54, 134)
    .size(52, 52)
    .radius({26})
    .color(ui.rgb(255, 128, 0))
```

```cpp
ui.drawSquircleRect()
    .pos(124, 134)
    .size(52, 52)
    .radius({26})
    .color(ui.rgb(255, 255, 255))
```

- `pos(...)` - левый верхний угол области
- `size(...)` - размер прямоугольного squircle
- `radius({r})` - один радиус на все углы
- `radius({tl, tr, br, bl})` - отдельные радиусы по углам
- `fillSquircleRect()` рисует заливку, `drawSquircleRect()` только контур

# 8. Градиенты

У всех градиентов `pos(...)` и `size(...)` задают прямоугольную область рисования.

## 8.1. Вертикальный

```cpp
ui.gradientVertical()
    .pos(20, 20)
    .size(120, 40)
    .topColor(ui.rgb(255, 0, 72))
    .bottomColor(ui.rgb(0, 87, 250))
```

- цвет плавно меняется сверху вниз

## 8.2. Горизонтальный

```cpp
ui.gradientHorizontal()
    .pos(20, 70)
    .size(120, 40)
    .leftColor(ui.rgb(255, 128, 0))
    .rightColor(ui.rgb(80, 255, 120))
```

- цвет плавно меняется слева направо

## 8.3. 4 угла

```cpp
ui.gradientCorners()
    .pos(20, 120)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 0, 72))
    .topRightColor(ui.rgb(0, 87, 250))
    .bottomLeftColor(ui.rgb(80, 255, 120))
    .bottomRightColor(ui.rgb(255, 128, 0))
```

- каждая вершина прямоугольника получает свой цвет
- внутри идёт двунаправленная интерполяция между четырьмя углами

## 8.4. Диагональный

```cpp
ui.gradientDiagonal()
    .pos(20, 170)
    .size(120, 40)
    .topLeftColor(ui.rgb(255, 255, 255))
    .bottomRightColor(ui.rgb(30, 30, 30))
```

- плавный переход по диагонали от верхнего левого к нижнему правому углу

## 8.5. Радиальный

```cpp
ui.gradientRadial()
    .pos(20, 220)
    .size(120, 60)
    .center(80, 250)
    .radius(40)
    .innerColor(ui.rgb(255, 255, 255))
    .outerColor(ui.rgb(0, 87, 250))
```

- `center(...)` задаёт центр градиента в координатах экрана
- `radius(...)` задаёт радиус перехода
- `innerColor(...)` и `outerColor(...)` задают цвета в центре и на периферии

# 9. Эффекты

## 9.1. Blur

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
- `direction(...)` задаёт направление усиления blur-материала
- `gradient(true)` делает материал не плоским, а с мягким переходом

## 9.2. Glow

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

Что задают параметры glow:

- `fillColor(...)` - цвет самой фигуры
- `bgColor(...)` - цвет фона под фигурой, нужен для чистого in-place обновления
- `glowColor(...)` - цвет свечения
- `glowSize(...)` - толщина зоны свечения
- `glowStrength(...)` - интенсивность свечения
- `anim(...)` и `pulsePeriodMs(...)` - анимация свечения

---

# 10. Виджеты

## 10.1. Scroll dots

```cpp
ui.drawScrollDots()
    .pos(center, 220)
    .count(5)
    .activeIndex(2)
    .activeColor(ui.rgb(0, 87, 250))
    .inactiveColor(ui.rgb(60, 60, 60)) 
    .radius(3)
    .spacing(14);
```

Есть и `updateScrollDots()` с теми же параметрами.

Что задают параметры:

- `count(...)` - общее число страниц/точек
- `activeIndex(...)` - текущая активная страница
- `radius(...)` - базовый радиус точки
- `spacing(...)` - шаг между центрами соседних точек

Поведение:

- при смене `activeIndex` индикатор сам (time-based) анимирует активную точку как `circle -> capsule -> circle`
- при `count > 7` включается оконный режим: показывается компактное окно точек с taper по краям

## 10.2. Наследование стиля fluent

У fluent-объектов есть `derive()`. Это позволяет собрать базовый стиль один раз, а потом сделать на его основе несколько вариантов без копирования всей цепочки.

Пример:

```cpp
auto base = ui.drawButton()
    .size(120, 40)
    .baseColor(ui.rgb(0, 120, 255))
    .radius(10);

auto compact = base.derive()
    .size(96, 34);

base
    .label("Main")
    .pos(20, 160);

compact
    .label("Mini")
    .pos(20, 210);
```

После `derive()` исходный fluent становится шаблоном и сам больше не коммитится. Рисуется уже производная цепочка или финальная донастройка исходного объекта.

## 10.3. Универсальная кнопка

Обычная отрисовка:

```cpp
ui.drawButton()
    .label("Save")
    .pos(center, 180)
    .size(120, 40)
    .baseColor(ui.rgb(0, 120, 255))
    .radius(10);
```

Обновление с состоянием кнопки:

```cpp
ui.updateButton()
    .label("Save")
    .pos(center, 180)
    .size(120, 40)
    .baseColor(ui.rgb(0, 120, 255))
    .radius(10)
    .icon(warning)
    .mode(true, false)
    .down(isDown)
;
```

Параметры:

- `label(...)` - текст внутри кнопки
- `pos(x, y)` - позиция левого верхнего угла;
- `size(w, h)` - размер кнопки
- `baseColor(...)` - основной цвет кнопки 
- `radius(...)` - squircle-радиус кнопки
- `icon(...)` - иконка внутри кнопки
- `mode(enabled, loading)` - сразу задает активное и loading-состояние
- `down(bool)` - текущее физическое нажатие для press-анимации

`drawButton()` и `updateButton()` используют один и тот же API. Для обычных статичных экранов достаточно `drawButton()`. Для анимируемой или интерактивной кнопки используй `updateButton()`.

Если заданы и текст, и иконка, иконка рисуется слева от текста как единый центрированный блок. Если текст пустой, иконка рисуется по центру кнопки.

## 10.4. Toggle switch

Снаружи нужен только обычный `bool`:

```cpp
bool wifiEnabled = false;
bool changed = false;
```

Отрисовка:

```cpp
ui.updateToggleSwitch()
    .pos(center, 140)
    .size(78, 36)
    .value(wifiEnabled)                // текущий bool; библиотека сама обновит его при нажатии
    .pressed(btn.wasPressed())         // событие нажатия этого кадра
    .changed(changed)                  // сюда вернется true, если значение переключилось
    .enabled(!wifiBusy)                // можно временно отключить ввод, пока идет внешняя операция
    .activeColor(ui.rgb(21, 180, 110));
```

Дополнительно можно задать:

- `inactiveColor(...)` - цвет выключенного track
- `knobColor(...)` - цвет бегунка
- `enabled(false)` - временно делает переключатель неактивным, но сохраняет текущее состояние

`drawToggleSwitch()` и `updateToggleSwitch()` используют один и тот же fluent API. Разница только в режиме вывода:

- `drawToggleSwitch()` - обычная отрисовка
- `updateToggleSwitch()` - локальный dirty-update

Если хочется просто показать состояние без ввода, можно не передавать `pressed(...)` и `changed(...)`:

```cpp
ui.drawToggleSwitch()
    .pos(center, 140)
    .size(78, 36)
    .value(wifiEnabled)
    .activeColor(ui.rgb(21, 180, 110));
```

## 10.5. Прогресс

```cpp
ui.drawProgress()
    .pos(20, 220)
    .size(180, 16)
    .value(65)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .radius(8)
    .anim(Shimmer);
```

Для локального обновления без полной перерисовки есть `updateProgress()`:

```cpp
ui.updateProgress()
    .pos(20, 220)
    .size(180, 16)
    .value(65)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .radius(8)
    .anim(Shimmer);
```

Текст у линейного прогресса задаётся прямо на самом progress:

```cpp
ui.drawProgress()
    .pos(20, 246)
    .size(180, 14)
    .value(65)
    .baseColor(ui.rgb(20, 20, 20))
    .fillColor(ui.rgb(0, 120, 255))
    .label("Downloading")
    .labelAlign(Left)
    .labelColor(ui.rgb(255, 255, 255))
    .percent()
    .percentAlign(Right)
    .percentColor(ui.rgb(200, 200, 200));
```

- `label(...)` привязывает произвольный текст к конкретному линейному progress
- `percent()` выводит текущее значение этого же progress в формате `65%`
- `labelAlign(...)`, `labelColor(...)`, `labelFont(...)` настраивают label
- `percentAlign(...)`, `percentColor(...)`, `percentFont(...)` настраивают числовой индикатор
- текст поддерживается только у линейного progress; у `drawCircleProgress()` его нет

## 10.6. Круговой прогресс

```cpp
ui.drawCircleProgress()
    .pos(center, 140)
    .radius(34)
    .thickness(8)
    .value(72)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .anim(None);
```

Локальное обновление:

```cpp
ui.updateCircleProgress()
    .pos(center, 140)
    .radius(34)
    .thickness(8)
    .value(72)
    .baseColor(ui.rgb(30, 30, 30))
    .fillColor(ui.rgb(0, 120, 255))
    .anim(None);
```

## 10.7. Drum roll

Горизонтальный:

```cpp
String options[] = {"Low", "Medium", "High"};

ui.drawDrumRollHorizontal(
    20, 60, 200, 40,
    options, 3,
    1,             // selectedIndex
    ui.rgb(255, 255, 255),
    ui.rgb(0, 0, 0),
    16,            // fontPx
    280            // animDurationMs
);
```

Вертикальный есть отдельной функцией `drawDrumRollVertical(...)`.

- анимация у `DrumRoll` теперь внутренняя time-based
- снаружи передаётся только новый `selectedIndex`
- `animDurationMs` задаёт длительность перелистывания в миллисекундах

---

# 11. Списки и плитки

## 11.1. Списочное меню

Конфигурация:

```cpp
ui.configureList()
    .screen(ScreenMainMenu)
    .items({
        {"Settings", "Device configuration", ScreenSettings},
        {"About",    "Firmware info",        ScreenAbout},
        {"Restart",  "Reboot device",        ScreenRestart}
    })
    .inactive(ui.rgb(12, 12, 12))
    .active(ui.rgb(0, 120, 255))
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
- удержание `PREV` возвращает на предыдущий экран из navigation-history.

Режимы списка:

- `Cards`
- `Plain`

Дефолты:

- `radius = 17`

## 11.2. Плиточное меню

Конфигурация:

```cpp
ui.configureTile()
    .screen(ScreenTiles)
    .items({
        {"Main",     "Главный экран", ScreenHome},
        {"Settings", "Настройки",     ScreenSettings},
        {"Info",     "Инфо",          ScreenInfo},
        {"Graph",    "Графики",       ScreenGraph}
    })
    .inactive(ui.rgb(16, 16, 16))
    .active(ui.rgb(0, 120, 255))
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

Что важно:

- `columns(...)` управляет обычной равномерной сеткой
- `tileSize(...)` задаёт желаемый размер плитки, но layout всё равно ужимается в доступную область экрана
- `lineGap(...)` влияет только на расстояние между title и subtitle внутри плитки
- `mode(TextSubtitle)` не гарантирует subtitle любой ценой — если по высоте не влезает, subtitle скрывается

## 11.3. Кастомная раскладка плиток

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
```

Это advanced-режим. Для большинства экранов хватает обычных `columns(...)`, `spacing(...)` и `tileSize(...)`.

Что важно:

- `layout(...)` переключает плитки в кастомную сетку
- в этом режиме `columns(...)` уже не управляет раскладкой
- каждая буква — отдельная плитка, прямоугольный блок одинаковых букв задаёт её span

---

# 12. Графики

## Фон и сетка:

```cpp
ui.drawGraphGrid()
    .pos(10, 50)
    .size(220, 120)
    .radius(8)
    .direction(LeftToRight)
    .bgColor(ui.rgb(10, 10, 10))
    .speed(1.0f);
```

- цвет сетки вычисляется автоматически из `bgColor()`

## Линия графика:

```cpp
ui.drawGraphLine()
    .line(0)
    .value(sensorValue)
    .thickness(2)
    .color(ui.rgb(0, 255, 140))
    .range(0, 100);
```

- `drawGraphLine()` добавляет новую точку в уже настроенный график
- `updateGraphLine()` подходит для in-place обновления, когда графику нужно самому зачистить и перерисовать нужную область
- `thickness(...)` задаёт толщину линии графика; по умолчанию `1`

## Auto-scale:

```cpp
ui.setGraphScale(true);
```

- `setGraphScale(true)` включает автоматический диапазон по данным, auto-scale применяется ко всем buffered-линиям графика

## Пакетная отрисовка готового массива:

```cpp
int16_t samples[] = {10, 15, 12, 18, 20, 17};

ui.drawGraphSamples()
    .line(0)
    .samples(samples, 6)
    .thickness(2)
    .color(ui.rgb(0, 255, 140))
    .range(0, 100);
```

- `drawGraphSamples()` рисует переданный массив сразу, не накапливает внутреннюю историю точек. Для streaming-режима с накоплением используйте `drawGraphLine()`
- `thickness(...)` задаёт толщину линии; по умолчанию `1`

Осциллограф можно настроить отдельно:

```cpp
ui.configGraphScope()
    .screen(ScreenGraph)
    .rate(2000)
    .timebase(100)
    .visible(0);
```

Чтение текущих настроек:

```cpp
uint16_t hz = ui.graphRate(ScreenGraph);
uint16_t timeMs = ui.graphTimebase(ScreenGraph);
uint16_t visible = ui.graphSamplesVisible(ScreenGraph);
```

Что важно по lifecycle:

- `drawGraphGrid()` задаёт активную область графика и должен вызываться в screen-callback этого экрана
- buffered-график (`drawGraphLine()`) живёт только пока экран реально рисует граф в текущих кадрах
- если экран перестал вызывать graph API или вы ушли на другой screen, внутренние буферы графа освобождаются
- при возврате граф начинает собирать историю заново

Что важно по режимам:

- `LeftToRight` и `RightToLeft` используют rolling-history
- `Oscilloscope` использует фиксированное окно видимых samples
- если `visibleSamples == 0`, окно для `Oscilloscope` вычисляется из `sampleRateHz * timebaseMs`

Режимы направления:

- `LeftToRight`
- `RightToLeft`
- `Oscilloscope`

---

# 13. Статус-бар

## 13.0. Build-time флаги

- `PIPGUI_STATUS_BAR`
  - `1` включает код статус-бара
  - `0` вырезает runtime-реализацию, публичные методы остаются no-op

Обычно эти флаги задаются в `include/config.hpp`.

## 13.1. Включение

```cpp
ui.configureStatusBar()
    .enabled(true)
    .bgColor(ui.rgb(0, 0, 0))
    .height(18)
    .position(Top);
```

Позиции:

- `Top`
- `Bottom`

## 13.2. Стиль

```cpp
ui.setStatusBarStyle(StatusBarStyleSolid);
// или
ui.setStatusBarStyle(StatusBarStyleBlur);
```

Режимы:

- `StatusBarStyleSolid` — обычная непрозрачная полоса; layout резервирует под неё высоту
- `StatusBarStyleBlur` — блюр-полоса поверх контента; layout не должен откусывать под неё safe area

## 13.3. Текст

```cpp
ui.setStatusBarText()
    .left("pipGUI")
    .center("12:34")
    .right("Wi-Fi");
```

## 13.4. Батарея

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

## 13.5. Кастомная дорисовка

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

Что важно:

- `statusBarHeight()` возвращает ненулевую высоту только для `StatusBarStyleSolid`
- при `StatusBarStyleBlur` helper возвращает `0`, потому что layout не должен резервировать fixed-height safe area под blur-панель

## 13.6. Иконки слотов

Можно повесить отдельную иконку в левый, центральный или правый слот статус-бара:

```cpp
ui.setStatusBarIcon()
    .side(Left)
    .icon(warning);

ui.setStatusBarIcon()
    .side(Center)
    .icon(error)
    .color(ui.rgb(255, 80, 80))
    .size(16);

ui.setStatusBarIcon()
    .side(Right)
    .icon(warning)
    .color(ui.rgb(255, 220, 120))
    .size(14);
```

Параметры:

- первый аргумент - слот: `Left`, `Center` или `Right`
- второй аргумент - `IconId`
- `color` необязателен; если не задан, берётся foreground-цвет статус-бара
- `sizePx` необязателен; если `0`, размер подбирается автоматически от высоты панели

Удаление иконки:

```cpp
ui.clearStatusBarIcon(Left);
ui.clearStatusBarIcon(Center);
ui.clearStatusBarIcon(Right);
```

Поведение:

- иконки появляются и исчезают с короткой fade-анимацией
- левая иконка живёт в одном block-е с левым текстом
- центральная иконка центрируется вместе с центральным текстом как одна группа
- правая иконка живёт в правом block-е вместе с правым текстом

---

# 14. Уведомления, toast, ошибки

## 14.1. Toast

```cpp
ui.showToast()
    .text("Saved")
    .icon(error)
    .pos(top);
```

Параметры:

- `text(...)` — текст toast (можно не задавать, если toast только с иконкой)
- `pos(top | down)` — появление сверху или снизу (`down` по умолчанию)
- `icon(id)` — иконка (если не задана, toast будет только текстом)

Проверка активности:

```cpp
bool active = ui.toastActive();
```

## 14.2. Notification

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

## 14.3. Popup menu

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

Можно использовать и более короткий паттерн без `popupMenuHasResult()`:

- `popupMenuTakeResult() == -1` — результата пока нет
- `popupMenuTakeResult() >= 0` — это индекс выбранного пункта

## 14.4. Ошибки

```cpp
ui.showError()
    .message("Low battery")
    .code("0xLOWBAT")
    .type(Warning)
    .button("OK");

ui.showError()
    .message("LittleFS mount failed")
    .code("0xLFS")
    .type(Error)
    .button("Retry");
```

Управление:

```cpp
ui.nextError();                                                                 // переключает на следующую ошибку
ui.prevError();                                                                 // переключает на предыдущую ошибку
ui.errorActive();                                                               // сообщает, активен ли сейчас error overlay
ui.setErrorButtonDown(btnOk.isDown());                                          // короткий совместимый wrapper для простого сценария с одной кнопкой
ui.setErrorButtonsDown(btnNext.isDown(), btnPrev.isDown(), btnCombo.isDown());  // полный input API для error overlay
```

Поведение:

- если ошибок несколько, первой показывается последняя добавленная
- header берется из `type(...)`: `Warning` или `Error/Crash`
- пользователь задает только `message(...)` и `code(...)`; строка `Code:` собирается библиотекой сама
- `Warning` можно закрыть
- `Error` не закрывается пользовательской кнопкой

---

# 15. Layout helpers

Это лёгкие helper-ы для раскладки UI без тяжёлой layout-системы.

Базовые типы:

```cpp
UiSize   size{120, 40};
UiRect   rect{0, 0, 240, 320};
UiInsets insets{10, 10, 10, 10};
```

## 15.1. Slicing API

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

## 15.2. Flow API

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

## 15.3. Cursor-based API

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

# 16. Скриншоты

## 16.1. Build-time флаги

- `PIPGUI_SCREENSHOTS`
  - `1` по умолчанию
  - `0` полностью выключает систему скриншотов
- `PIPGUI_SCREENSHOT_MODE`
  - `1` — serial capture
  - `2` — запись в LittleFS

Для режима `2` нужен LittleFS. Библиотека сама создаёт `/pipKit/`, `/pipKit/screenshots/` и `/pipKit/thumbnails/`.

## 16.2. Serial capture

Скрипт на ПК:

```bash
python tools/screenshots/bin/capture.py
```

Что важно:

- serial header теперь фиксированный: magic `PSCR`, затем `width`, `height`, `payloadSize`
- формат payload один: lossless `Packed565`
- отдельного поля `format` в header больше нет
- при запуске без параметров tool покажет меню: port, baud и output directory

Быстрый прямой запуск тоже можно:

```bash
python tools/screenshots/bin/capture.py --port COM9 --baud 1000000
```

## 16.3. Скриншоты во flash (LittleFS)

В режиме `2` сохраняются:

- full screenshot: `/pipKit/screenshots/pscr_00000001.pscr`
- thumbnail: `/pipKit/thumbnails/<WxH>/pscr_00000001.pscr`

Галерея показывает новые сверху.

## 16.4. Запуск скриншота

```cpp
bool started = ui.startScreenshot();
```

- `startScreenshot()` запускает захват асинхронно
- если snapshot-buffer не удалось выделить, функция вернёт `false`
- built-in shortcut фиксированный: удержание `Next + Prev` `300 ms`
- shortcut настраивать нельзя и отдельного API для него больше нет

## 16.5. Галерея миниатюр

Настройка кеша и thumb-геометрии:

```cpp
ui.configureScreenshotGallery(
    12, // maxShots: сколько screenshot entries держать в галерее
    64, // thumbW: ширина одной миниатюры
    40, // thumbH: высота одной миниатюры
    6   // padding: внутренний отступ между миниатюрами
);
```

Отрисовка галереи:

```cpp
ui.drawScreenshot()
    .pos(8, 28)     // x, y области галереи
    .size(224, 284) // w, h области галереи
    .grid(3, 5)     // cols, rows видимой сетки
    .padding(8);    // отступ между ячейками в самой сетке
```

Что делает `drawScreenshot()`:

- рисует текущую screenshot gallery в заданной области
- сам берёт entries из внутреннего screenshot store
- в serial mode просто покажет пустое состояние, если gallery backend не используется

Дополнительно:

```cpp
uint8_t count = ui.screenshotCount();
```

---

# 17. Wi‑Fi

## 17.1. Build-time флаги

- `PIPGUI_WIFI`
  - `1` — `GUI::loop()` обслуживает standalone Wi‑Fi wrapper
  - `0` — standalone Wi‑Fi path не обслуживается автоматически
- `PIPGUI_WIFI_SSID`
- `PIPGUI_WIFI_PASSWORD`

Важно:

- OTA продолжает работать и при `PIPGUI_WIFI = 0`
- у OTA свой runtime-path, он не зависит от standalone Wi‑Fi servicing в `GUI::loop()`

## 17.2. API

```cpp
ui.requestWiFi(true);   // включить или выключить standalone Wi‑Fi request
ui.wifiState();         // текущее состояние backend-а
ui.wifiConnected();     // true только в Connected
ui.wifiLocalIpV4();     // IPv4 как packed uint32_t
```

---

# 18. OTA

> Для OTA нужна A/B partition table с двумя OTA app-слотами.

## 18.1. Тулинг (`tools/ota/`)

Сгенерировать ключ:

```bash
python tools/ota/keygen.py
```

Если запустить без параметров, tool покажет меню и даст выбрать путь сохранения числом.

Сделать stable release:

```bash
python tools/ota/make_release.py
```

Сделать beta release:

```bash
python tools/ota/make_release.py --beta --interactive
```

Проверка manifest + bin:

```bash
python tools/ota/verify_manifest.py
```

При запуске без параметров tool покажет меню: manifest, firmware source и signature check.

## 18.2. Конфигурация

- `PIPGUI_OTA` — `1` включает OTA subsystem
- `PIPGUI_OTA_PROJECT_URL` — base URL на `/fw/<project>`
- `PIPGUI_OTA_ED25519_PUBKEY_HEX` — обязательный Ed25519 public key в hex
- `PIPGUI_FIRMWARE_TITLE` — имя прошивки для UI
- `PIPGUI_FIRMWARE_VER_MAJOR`, `PIPGUI_FIRMWARE_VER_MINOR`, `PIPGUI_FIRMWARE_VER_PATCH` — текущая версия

## 18.3. Использование

Типы `OtaChannel`, `OtaCheckMode`, `OtaState`, `OtaError`, `OtaManifest`, `OtaStatus`, `OtaStatusCallback` экспортируются из `pipGUI/Systems/Update/Ota.hpp`.

### Базовый lifecycle

```cpp
ui.otaConfigure();
ui.otaRequestCheck();
const OtaStatus& st = ui.otaStatus();
```

- `otaConfigure()` вызывается один раз на старте
- `otaRequestCheck()` запускает проверку обновления
- `otaStatus()` возвращает live status state-machine

### Проверка и установка

```cpp
ui.otaRequestCheck();
ui.otaRequestCheck(OtaCheckMode::NewerOnly);
ui.otaRequestCheck(OtaCheckMode::AllowDowngrade);
ui.otaRequestInstall();
ui.otaCancel();
```

- порядок каналов: `stable -> beta`
- если в `stable` ничего нового нет, backend проверяет `beta`
- `otaCancel()` отменяет текущую OTA-операцию

### История stable-версий

```cpp
ui.otaRequestStableList();
bool ready = ui.otaStableListReady();
uint8_t count = ui.otaStableListCount();
const char* ver = ui.otaStableListVersion(i);
ui.otaRequestInstallStableVersion("1.2.3");
```

### Дополнительно

```cpp
ui.otaService();
ui.otaMarkAppValid();
```

- `otaService()` публичный, но обычно вручную не нужен: `GUI::loop()` уже его вызывает сам
- после reboot в `pendingVerify` можно явно подтвердить новую прошивку через `otaMarkAppValid()`

### Ошибки `OtaError`

- `None`
- `WifiNotEnabled`
- `WifiNotConnected`
- `HttpBeginFailed`
- `HttpStatusNotOk`
- `ManifestTooLarge`
- `ManifestParseFailed`
- `ManifestReplay`
- `SignatureMissing`
- `SignatureInvalid`
- `FlashLayoutInvalid`
- `RollbackUnavailable`
- `UpdateBeginFailed`
- `UpdateWriteFailed`
- `HashPipelineFailed`
- `DownloadTruncated`
- `PayloadSizeMismatch`
- `HashMismatch`
- `UpdateEndFailed`
- `UrlTooLong`
