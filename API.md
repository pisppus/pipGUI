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
ui.configDisplay()
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
ui.configDisplay()
    .pins(11, 12, 10, 9, -1)
    .size(240, 320);
```

Можно не писать и оставить дефолт библиотеки:

- `hz(freq)` по умолчанию `80000000`
- `order("RGB")` по умолчанию `RGB`
- `invert(bool)` по умолчанию `true`
- `swap(bool)` по умолчанию `false`
- `offset(x, y)` по умолчанию `(0, 0)`

Обязательно указать:

- `pins(...)` задаёт пины SPI и управляющие пины дисплея.
- `size(width, height)` задаёт логический размер панели.

## 2.2. Запуск GUI

После конфигурации дисплея вызывается `begin()`:

```cpp
ui.begin(0);   // rotation: 0..3
```
 
- `rotation` принимает значения `0..3`.
- стартовый фон GUI фиксированно чёрный (`0x0000`).

### Runtime-поворот экрана

```cpp
ui.setRotation(0);        // повернуть экран сразу или с анимацией по умолчанию
ui.setRotation(1, 620);   // тот же поворот, но со своей длительностью

ui.screenRotation();              // текущая ориентация 0..3
ui.rotationTransitionActive();    // идёт ли сейчас анимация переворота
```

- `setRotation(...)` меняет ориентацию уже после `begin()`, без повторного `configDisplay()`
- библиотека держит физическую конфигурацию дисплея как есть и поворачивает GUI логически, пересобирая внутренний sprite под новый размер
- если GUI сейчас в обычном steady-state, поворот идёт с анимацией как у телефона: текущий кадр схлопывается, в середине меняется ориентация, затем новый кадр раскрывается
- если в этот момент активны boot, error, screen transition или overlay-состояния, библиотека делает безопасный мгновенный поворот без анимации
- `screenRotation()` возвращает текущий runtime rotation `0..3`
- `rotationTransitionActive()` позволяет не запускать свой второй переход поверх уже идущего переворота

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
```

- диапазон `0..100`
- сразу отправляет новое значение в текущий backlight runtime, то есть эффект виден без перезапуска GUI
- если backlight настроен через `setBacklight(...)` или `setBacklightHandler(...)`, библиотека сразу применяет новый уровень к активной подсветке
- значение всегда clamp-ится сверху через `maxBrightness()`, поэтому `setBrightness(90)` при `setMaxBrightness(70)` даст фактические `70`

```cpp
ui.brightness();
```

- возвращает именно текущее runtime-значение, а не сохранённый лимит
- это временное пользовательское состояние на текущую сессию: после reboot оно не восстанавливается из prefs автоматически
- если нужен сохраняемый предел яркости, для этого используется `setMaxBrightness(...)`

### Низкоуровневые helpers

```cpp
void myBacklight(uint16_t level) {
    // свой способ применить яркость
}

ui.setBacklightHandler(myBacklight);
```

- `setBacklightHandler(...)` позволяет подставить свой handler для управления подсветкой
- callback имеет сигнатуру `void (*)(uint16_t level)`
- `level` это целевой уровень яркости, который библиотека хочет применить
- это low-level хук для своего драйвера подсветки, если стандартного `setBacklight()` недостаточно
- обычно этот API не нужен, если хватает `setBacklight()`

---

# 3. Логотип

```cpp
ui.showLogo()
    .text("PISPPUS", "Digital Thermometer")
    .anim(FadeIn);
```

`showLogo()` запускает полноэкранную boot-анимацию с логотипом.
Размер текста библиотека подбирает сама под текущее разрешение экрана.

Параметры:

- `text(title, subtitle)` — две строки логотипа;
- `anim(...)` — `None`, `FadeIn`, `LightFade`;

Что делают анимации:

- `None` - просто сразу показывает логотип без анимации
- `FadeIn` - плавно проявляет текст из цвета фона
- `LightFade` - плавно поднимает яркость подсветки; для неё backlight должен быть заранее настроен

---

# 4. Базовые helpers

## 4.1. Размеры

```cpp
ui.screenWidth();   // ширина вашего экрана
ui.screenHeight();  // высота вашего экрана
```

- `screenWidth()` и `screenHeight()` возвращают уже активный логический размер экрана
- удобно для центровки, адаптивных отступов и расчёта layout без хардкода

## 4.2. Цвет

```cpp
ui.rgb(255, 255, 255);
0xFFFF;     // RGB565
0xFF6200;   // RGB888
```

- Библиотека поддерживает два практических способа задания цвета:
- `ui.rgb(r, g, b)` - обычный и основной способ. На вход подаётся `RGB888`, на выходе получается `RGB565`.
- прямой hex-литерал - если ты уже знаешь нужное значение заранее
- если метод принимает `uint16_t`, туда нужно передавать `RGB565`
- если метод принимает `uint32_t`, туда нужно передавать `RGB888` в виде `0xRRGGBB`

Для повторно используемых системных accent-цветов есть короткие semantic-токены:

```cpp
Warning; // #FF6200
Error;   // #FF0048
```

- их можно передавать в `color(...)`, `fillColor(...)`, `bgColor(...)` и другие места, где ожидается `RGB565`

## 4.3. Очистка экрана

```cpp
ui.clear(ui.rgb(0, 0, 0));
```

- очищает весь текущий draw target указанным цветом

## 4.4. Клип

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

## 4.5. Наследование стиля fluent

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

## 4.6. Adaptive preview

```cpp
ui.setAdaptivePreview(240, 135, 7200);
ui.clearAdaptivePreview();
```

- это debug-helper для проверки адаптивности интерфейса
- `setAdaptivePreview(minWidth, minHeight, cycleMs)` плавно гоняет логический размер GUI между текущим физическим экраном и указанным минимумом
- при этом `screenWidth()` и `screenHeight()` реально меняются, поэтому layout пересчитывается по-настоящему
- это не замена `configDisplay().size(...)` и не перенастройка самой панели: физический дисплей остаётся тем же, меняется только логический viewport GUI
- `clearAdaptivePreview()` выключает этот режим и возвращает обычный размер экрана

# 5. Layout helpers

Это лёгкие helper-ы для раскладки UI без тяжёлой layout-системы.

Базовые типы:

```cpp
UiSize   size{120, 40};
UiRect   rect{0, 0, 240, 320};
UiInsets insets{10, 10, 10, 10};
```

## 5.1. Slicing API

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

## 5.2. Flow API

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

## 5.3. Cursor-based API

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

# 6. Текст, шрифты и иконки

## 6.1. Встроенные шрифты

Текущий шрифт настраивается отдельными методами:

```cpp
ui.setFont(WixMadeForDisplay);   // выбирает семейство шрифта
ui.setFontSize(18);              // задаёт текущий размер в пикселях
```

В библиотеке сейчас есть два встроенных семейства:

- `WixMadeForDisplay`
- `KronaOne`

Для веса можно передавать либо число, либо готовый токен:

```cpp
ui.setFontWeight(450);        // обычно это диапазон `100..900`
ui.setFontWeight(Semibold);
```

- `Thin`
- `Light`
- `Regular`
- `Medium`
- `Semibold`
- `Bold`
- `Black`

Текущее состояние тоже можно читать:

```cpp
ui.fontId();      // текущий FontId
ui.fontSize();    // текущий размер шрифта
ui.fontWeight();  // текущая толщина шрифта
```

## 6.2. Текстовые стили

```cpp
ui.setTextStyle(H1);
```

`setTextStyle(...)` быстро выставляет готовый пресет под тип текста.

Доступные стили:

- `H1` - крупный заголовок
- `H2` - подзаголовок или вторичный заголовок
- `Body` - основной текст интерфейса
- `Caption` - мелкие подписи и пояснения

## 6.3. Обычный текст

```cpp
ui.drawText()
    .pos(center, 32)                   // точка привязки текста
    .font(WixMadeForDisplay, 18)       // конкретный шрифт, размер и насыщенность
    .weight(Semibold)                  // вес текста
    .text("Hello")                     // сама строка
    .color(ui.rgb(255, 255, 255))      // цвет текста
    .bgColor(ui.rgb(0, 0, 0))          // фон под текстом
    .align(Center);                    // выравнивание относительно точки `pos(...)`
```

`drawText()` рисует строку сразу в текущий кадр.

Если текст обновляется на одном и том же месте, используйте `updateText()`:

```cpp
ui.updateText()
    .pos(center, 32)
    .text("Updated")
    .color(ui.rgb(255, 255, 255))
    .align(Center);
```

- `drawText()` рисует текст как есть
- `updateText()` сначала очищает прошлую область и потом рисует новое значение на том же месте

## 6.4. Бегущая строка и многоточие

```cpp
ui.drawTextMarquee()
    .pos(20, 80)                                  // точка привязки строки
    .width(140)                                   // ширина области, внутри которой идёт прокрутка
    .font(WixMadeForDisplay, 18)                  // конкретный шрифт и его размер
    .weight(Semibold)                             // насыщенность начертания
    .text("Very long text that does not fit")     // сама строка
    .color(ui.rgb(255, 255, 255))                 // цвет текста
    .speed(30)                                    // скорость движения в пикселях в секунду
    .holdStart(700)                               // пауза перед началом движения
    .phaseStart(0)                                // стартовая фаза, если нужна синхронизация
    .align(Left);                                 // выравнивание строки внутри заданной ширины
```

`drawTextMarquee()` нужен для длинной строки, которая должна прокручиваться внутри ограниченной ширины.

```cpp
ui.drawTextEllipsized()
    .pos(20, 110)                                 // точка привязки строки
    .width(140)                                   // ширина области, в которую текст должен уместиться
    .font(WixMadeForDisplay, 18)                  // конкретный шрифт и его размер
    .weight(Semibold)                             // насыщенность начертания
    .text("Very long text that does not fit")     // сама строка
    .color(ui.rgb(255, 255, 255));                // цвет текста
```

`drawTextEllipsized()` обрезает строку по `width(...)` и добавляет многоточие

## 6.5. Иконки

### Обычные иконки берутся из набора `IconId`

```cpp
ui.drawIcon()
    .pos(20, 20)                      // позиция иконки
    .size(18)                         // итоговый размер в пикселях
    .icon(warning)                    // конкретный `IconId`
    .color(ui.rgb(255, 255, 255))     // основной цвет слоя
    .bgColor(ui.rgb(0, 0, 0));        // фон под иконкой
```

Отдельного `updateIcon()` нет. 

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

Для animated icons используются отдельные runtime-вызовы.

Обычная отрисовка:

```cpp
ui.drawAnimIcon()
    .pos(92, 120)
    .size(56)
    .icon(setting_anim)
    .color(ui.rgb(235, 235, 235));
```

`drawAnimIcon()` рисует анимированную иконку как есть

Локальное обновление поверх известного фона:

```cpp
ui.updateAnimIcon()
    .pos(92, 120)
    .size(56)
    .icon(setting_anim)
    .color(ui.rgb(235, 235, 235))
    .bgColor(ui.rgb(10, 10, 10));
```

`updateAnimIcon()` сначала очищает область через `bgColor`, потом рисует новый кадр

### Создание иконок из `sources`

Чтобы добавить свою иконку:

1. положить source-файл в `tools/icons/sources/`
2. пересобрать проект — генератор сам обновит готовые файлы в `lib/pipKit/pipGUI/Graphics/Text/Icons/`
3. использовать иконку по имени файла

Пример:

- файл `tools/icons/sources/checkmark.svg`
- в коде: `.icon(checkmark)`

Что важно:

- генератор читает и `.svg`, и `.json` рекурсивно
- файлы могут лежать как прямо в корне `sources`, так и в любых подпапках
- имя для C++ генерируется из относительного пути, так что вложенные папки тоже поддерживаются

Что появится в коде:

- для обычной однослойной иконки из `name.svg` появляется alias `name`
- для многослойной иконки появляются alias вида `name_l0`, `name_l1`, `name_l2`
- дополнительно экспортируются и enum-константы `IconName`, `IconNameL0`, `IconNameL1` и т.д.
- для анимированной иконки из `name.json` появляется alias `name_anim`

Если в имени файла есть `-`, пробелы или другие неподходящие символы, генератор сам приводит имя к валидному C++-идентификатору через `_`.

- `.svg` идёт в обычный static PSDF pipeline.
- `.json` идёт в animated PSDF pipeline.

---

# 7. Фигуры

```cpp
ui.drawRect()
    .pos(20, 40)
    .size(100, 40)
    .radius(10)
    .fill(ui.rgb(0, 120, 255))
    .border(1, ui.rgb(255, 255, 255));
```

Что важно:

- `fill(color565)` — задаёт заливку; если не вызывать, фигура остаётся без заливки
- `border(widthPx, color565)` — задаёт контур; если не вызывать, контура не будет
- `fill(...)` и `border(...)` можно использовать вместе или по отдельности

## 7.1 Линия 

```cpp
ui.drawLine()
    .from(20, 20)                      // начало линии
    .to(140, 60)                       // конец линии
    .thickness(2)                      // толщина линии; по умолчанию `1`
    .color(ui.rgb(255, 255, 255))      // цвет линии
```

## 7.2 Прямоугольник

```cpp
ui.drawRect()
    .pos(20, 40)                          // левый верхний угол прямоугольника
    .size(100, 40)                        // ширина и высота
    .radius(10)                           // один радиус для всех углов
    // .radius(10, 14, 18, 22)            // или отдельные радиусы: tl, tr, br, bl
    .fill(ui.rgb(0, 120, 255))            // цвет заливки
    .border(1, ui.rgb(255, 255, 255))     // толщина и цвет контура
```

## 7.3 Круг

```cpp
ui.drawCircle()
    .pos(50, 50)                          // центр круга
    .radius(18)                           // радиус
    .fill(ui.rgb(0, 87, 250))             // цвет заливки
    .border(1, ui.rgb(255, 255, 255))     // толщина и цвет контура
```

## 7.4 Эллипс

```cpp
ui.drawEllipse()
    .pos(120, 50)                         // центр эллипса
    .radiusX(28)                          // горизонтальная полуось
    .radiusY(16)                          // вертикальная полуось
    .fill(ui.rgb(255, 0, 72))             // цвет заливки
    .border(1, ui.rgb(255, 255, 255))     // толщина и цвет контура
```

## 7.5 Треугольник

```cpp
ui.drawTriangle()
    .point0(40, 120)                      // первая вершина
    .point1(70, 80)                       // вторая вершина
    .point2(100, 120)                     // третья вершина
    // .radius(8)                         // опционально: скругление углов
    .fill(ui.rgb(0, 200, 120))            // цвет заливки
    .border(1, ui.rgb(255, 255, 255))     // толщина и цвет контура
```

## 7.6 Дуга

```cpp
ui.drawArc()
    .pos(100, 80)                     // центр дуги
    .radius(28)                       // внешний радиус
    .thickness(6)                     // толщина; по умолчанию 1
    .start(-90.0f)                    // начальный угол в градусах
    .end(90.0f)                       // конечный угол в градусах
    .color(ui.rgb(80, 255, 120))      // цвет дуги
```
У дуги концы всегда скруглённые, а диапазон `0..360` рисует полный круг.

## 7.7 Сквиркль

```cpp
ui.drawSquircleRect()
    .pos(54, 134)                         // левый верхний угол области
    .size(52, 52)                         // ширина и высота
    .radius(26)                           // один радиус для всех углов
    // .radius(18, 22, 26, 30)            // или отдельные радиусы: tl, tr, br, bl
    .fill(ui.rgb(255, 128, 0))            // цвет заливки
    .border(1, ui.rgb(255, 255, 255))     // толщина и цвет контура
```

# 8. Градиенты

У всех градиентов `pos(...)` и `size(...)` задают прямоугольную область рисования.

## 8.1. Вертикальный

```cpp
ui.gradientVertical()
    .pos(20, 20)
    .size(120, 40)
    .TColor(ui.rgb(255, 0, 72))
    .BColor(ui.rgb(0, 87, 250))
```

- цвет плавно меняется сверху вниз

## 8.2. Горизонтальный

```cpp
ui.gradientHorizontal()
    .pos(20, 70)
    .size(120, 40)
    .LColor(ui.rgb(255, 128, 0))
    .RColor(ui.rgb(80, 255, 120))
```

- цвет плавно меняется слева направо

## 8.3. Диагональный

```cpp
ui.gradientDiagonal()
    .pos(20, 170)
    .size(120, 40)
    .TLColor(ui.rgb(255, 255, 255))
    .BRColor(ui.rgb(30, 30, 30))
```

- плавный переход по диагонали от верхнего левого к нижнему правому углу

## 8.4. 4 угла

```cpp
ui.gradientCorners()
    .pos(20, 120)
    .size(120, 40)
    .TLColor(ui.rgb(255, 0, 72))
    .TRColor(ui.rgb(0, 87, 250))
    .BLColor(ui.rgb(80, 255, 120))
    .BRColor(ui.rgb(255, 128, 0))
```

- каждая вершина прямоугольника получает свой цвет, внутри идёт двунаправленная интерполяция между четырьмя углами

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
    .pos(0, 180)                  // левый верхний угол blur-области
    .size(240, 40)                // ширина и высота области
    .radius(10)                   // сила blur; чем больше, тем мягче и тяжелее эффект
    .direction(TopDown)           // если указать, material пойдёт по этому направлению
    .material(160, -1)            // сила material и его цвет; -1 = цвет фона библиотеки
```

Для обновления части экрана:

```cpp
ui.updateBlur()
    .pos(0, 180)                  // та же область, которую нужно перерисовать
    .size(240, 40)                // тот же размер blur-региона
    .radius(10)                   // тот же blur-радиус
    .direction(TopDown)           // то же направление материала
    .material(160, -1)            // те же параметры material
```
`updateBlur()` нужен для in-place перерисовки той же blur-области без полной перерисовки экрана.

`direction(...)` поддерживает:

- `TopDown` - материал и его усиление идут сверху вниз
- `BottomUp` - материал и его усиление идут снизу вверх
- `LeftRight` - материал и его усиление идут слева направо
- `RightLeft` - материал и его усиление идут справа налево

`material(strength, color)` управляет только верхним tinted-слоем поверх blur:

- чем больше `strength`, тем заметнее tinted-слой поверх blur
- `color = -1` - взять текущий цвет фона библиотеки

То есть blur остаётся тем же, а меняется только то, как поверх него распределяется tinted-слой.

## 9.2. Glow

Круг:

```cpp
ui.drawGlowCircle()
    .pos(60, 90)                          // центр круга
    .radius(18)                           // радиус фигуры
    .fillColor(ui.rgb(255, 255, 255))     // цвет самой фигуры
    .glowColor(ui.rgb(0, 120, 255))       // цвет свечения вокруг
    .glowSize(16)                         // толщина зоны свечения
    .glowStrength(220)                    // интенсивность свечения
    .anim(Pulse)                          // тип анимации свечения
    .pulseMs(1200)                        // период пульсации в миллисекундах
```

Для in-place обновления есть `updateGlowCircle()`.
Если glow нужно обновлять на месте без грязных хвостов, добавь `bgColor(...)` с цветом фона под фигурой.

`anim(...)` поддерживает:

- `None` - свечение статичное, без анимации
- `Pulse` - свечение плавно пульсирует по силе

---

# 10. Экраны и цикл

## 10.1. Регистрация экранов

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

- `name` становится константой id, её потом можно передавать в `setScreen(...)`, `updateList(...)`, `updateTile(...)` и другие API
- `order` задаёт порядок регистрации экрана
- `order` должен быть уникальным для каждого экрана
- обычно первый экран делают с `order = 0`

После этого экран можно активировать:

```cpp
ui.setScreen(ScreenHome);
```

## 10.2. Основной цикл

Базовый вариант:

```cpp
void loop()
{
    ui.loop();
}
```

Вспомогательный вариант, если есть две кнопки `Button`:

```cpp
Button Next(1, Pullup);
Button Prev(2, Pullup);

void setup()
{
    Next.begin();
    Prev.begin();
}

void loop()
{
    ui.loopWithInput(Next, Prev);
}
```

`loopWithInput(...)` только обновляет сами объекты `Button` и потом вызывает `ui.loop()`.
Логику экранов, списков и плиток вы всё равно задаёте отдельно.

Если `loopWithInput(...)` не используется, вызывайте `btn.update()` сами в начале каждого `loop()`.
После этого `wasPressed()` и `isDown()` читают уже обновлённое состояние кнопки.

Если нужен готовый snapshot ввода для собственного state-machine:

```cpp
InputState input = ui.pollInput(Next, Prev);
```

`pollInput(...)` сам обновляет обе кнопки и возвращает готовый снимок их состояния на текущий тик.

Поля `InputState`:

- `nextDown` - кнопка `Next` сейчас удерживается
- `prevDown` - кнопка `Prev` сейчас удерживается
- `nextPressed` - кнопка `Next` была нажата именно в этом тике
- `prevPressed` - кнопка `Prev` была нажата именно в этом тике
- `comboDown` - обе кнопки сейчас зажаты одновременно

Разница простая:

- `Down` - состояние удержания
- `Pressed` - одноразовое событие нажатия

## 10.3. Управление экранами

Эти методы управляют активным экраном и переходами между экранами.

```cpp
ui.setScreen(ScreenHome);      // сразу переключает на указанный экран
ui.currentScreen();            // возвращает id текущего активного экрана
ui.nextScreen();               // переходит на следующий экран по порядку регистрации
ui.prevScreen();               // возвращает назад по navigation-history
ui.screenTransitionActive();   // показывает, что переход между экранами сейчас ещё идёт
```

Что важно:

- библиотека сама ведёт history переходов между экранами
- если экран меняется через `setScreen(...)`, текущий экран добавляется в history автоматически

## 10.4. Принудительная перерисовка

```cpp
ui.requestRedraw();
```

Нужно, когда данные экрана изменились вне обычного render-flow, и вы хотите гарантированно перерисовать следующий кадр.

## 10.5. Анимация переходов

```cpp
ui.setScreenAnim(SlideX, 250);
```

`setScreenAnim(mode, durationMs)` задаёт стиль и длительность перехода между экранами.

Доступные режимы:

- `None` - переход без анимации
- `SlideX` - горизонтальный слайд
- `SlideY` - вертикальный слайд

---

# 11. Статус-бар

## 11.1. Build-time флаги

- `PIPGUI_STATUS_BAR`
  - `1` включает код статус-бара
  - `0` вырезает runtime-реализацию, публичные методы остаются no-op

Обычно эти флаги задаются в `include/config.hpp`.

## 11.2. Включение

```cpp
ui.configStatusBar()
    .height(18)
    .pos(Top)
    .style(Blur);
```

Позиции:

- `Top`
- `Bottom`

Стили:

- `Solid` — обычная непрозрачная полоса; layout резервирует под неё высоту
- `Blur` — блюр-полоса поверх контента; layout не должен откусывать под неё safe area

## 11.3. Текст

```cpp
ui.setStatusBarText()
    .left("pipGUI")
    .center("12:34")
    .right("Wi-Fi");
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

Что важно:

- `statusBarHeight()` возвращает ненулевую высоту только для `Solid`
- при `Blur` helper возвращает `0`, потому что layout не должен резервировать fixed-height safe area под blur-панель

## 11.6. Иконки слотов

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

# 12. Виджеты

## 12.1. Scroll dots

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

- при `count > 7` включается оконный режим: показывается компактное окно точек с taper по краям

## 12.2. Buttons

Обычная отрисовка:

```cpp
ui.drawButton()
    .label("Save")                       // текст внутри кнопки
    .pos(center, 180)                    // центр по X, координата Y
    .size(120, 40)                       // ширина и высота кнопки
    .baseColor(ui.rgb(0, 120, 255))      // основной цвет кнопки
    .radius(10);                         // радиус
```

Обновление с состоянием кнопки:

```cpp
ui.updateButton()
    .label("Save")                       // текст внутри кнопки
    .pos(center, 180)                    // центр по X, координата Y
    .size(120, 40)                       // ширина и высота кнопки
    .baseColor(ui.rgb(0, 120, 255))      // основной цвет кнопки
    .radius(10)                          // радиус
    .icon(warning)                       // иконка внутри кнопки
    .mode(true, false)                   // enabled, loading
    .down(isDown)                        // текущее физическое нажатие для press-анимации
;
```

Кнопка с прогрессом:

```cpp
ui.updateButton()
    .label("Updating 56%")               // текст внутри кнопки
    .pos(center, 180)                    // центр по X, координата Y
    .size(170, 38)                       // ширина и высота кнопки
    .baseColor(ui.rgb(24, 24, 24))       // базовый цвет корпуса кнопки
    .fillColor(ui.rgb(0, 120, 255))      // цвет progress-заливки
    .value(56)                           // значение встроенного progress: 0..100
    .radius(12)                          // радиус
    .icon(battery_l1);                   // иконка внутри кнопки
```

`drawButton()` и `updateButton()` используют один и тот же API. Для обычных статичных экранов достаточно `drawButton()`. Для анимируемой или интерактивной кнопки используй `updateButton()`.

Если заданы и текст, и иконка - иконка рисуется слева от текста как единый центрированный блок. Если текст пустой, иконка рисуется по центру кнопки.
Если задан `value(...)`, кнопка рисует встроенный progress-fill под текстом и иконкой. `loading` и `progress` одновременно не используются: progress-режим приоритетнее.

## 12.3. Toggle switch

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
    .value(wifiEnabled)                   // текущий bool; библиотека сама обновит его при нажатии
    .pressed(btn.wasPressed())            // событие нажатия этого кадра
    .changed(changed)                     // сюда вернется true, если значение переключилось
    .enabled(!wifiBusy)                   // можно временно отключить ввод, пока идет внешняя операция
    .activeColor(ui.rgb(21, 180, 110))    // цвет включенного track
    .inactiveColor(ui.rgb(46, 46, 46))    // цвет выключенного track
    .knobColor(0xFFFF);                   // цвет бегунка
```

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

## 12.4. Slider

Слайдер подходит для настроек вроде громкости, яркости и подобных значений.

```cpp
ui.updateSlider()
    .pos(center, 114)                     // центр по X, координата Y
    .size(186, 24)                        // ширина и высота трека
    .bind(value)                          // привязка переменной; библиотека обновляет ее сама
    .activeColor(ui.rgb(0, 87, 250))      // цвет заполненной части
    .inactiveColor(ui.rgb(36, 36, 36))    // цвет неактивной части трека
    // .enabled(false)                    // опционально: показать slider без реакции на ввод
    .thumbColor(0xFFFF);                  // цвет бегунка
```

`drawSlider()` и `updateSlider()` используют один и тот же fluent API. Для интерактивного сценария нужен `updateSlider()`.
Ввод `Next/Prev` slider берет сам из последнего `pollInput(...)`.

Поведение:

- удержание кнопки сначала двигает значение обычным шагом, потом ускоряет и частоту, и величину шага;
- по умолчанию бегунок белый, трек темнее активной части.

## 12.5. Прогресс

```cpp
ui.drawProgress()
    .pos(20, 220)                        // левый верхний угол
    .size(180, 16)                       // ширина и высота
    .value(65)                           // значение progress: 0..100
    .baseColor(ui.rgb(30, 30, 30))       // цвет пустой части
    .fillColor(ui.rgb(0, 120, 255))      // цвет заполненной части
    .radius(8)                           // скругление краев
    .anim(Shimmer);                      // тип анимации progress
```

Для локального обновления без полной перерисовки есть `updateProgress()`:

```cpp
ui.updateProgress()
    .pos(20, 220)                        // та же область, которую нужно обновить
    .size(180, 16)                       // тот же размер
    .value(65)                           // новое значение: 0..100
    .baseColor(ui.rgb(30, 30, 30))       // цвет пустой части
    .fillColor(ui.rgb(0, 120, 255))      // цвет заполненной части
    .radius(8)                           // скругление краев
    .anim(Shimmer);                      // тип анимации progress
```

Текст у линейного прогресса задаётся прямо на самом progress:

```cpp
ui.drawProgress()
    .pos(20, 246)                            // левый верхний угол
    .size(180, 14)                           // ширина и высота
    .value(65)                               // значение progress: 0..100
    .baseColor(ui.rgb(20, 20, 20))           // цвет пустой части
    .fillColor(ui.rgb(0, 120, 255))          // цвет заполненной части
    .label("Downloading", Left)              // текст и его выравнивание
    .labelColor(ui.rgb(255, 255, 255))       // цвет label
    .percent(Right)                          // показать текущее значение как процент и задать выравнивание
    .percentColor(ui.rgb(200, 200, 200));    // цвет процента
```
Текст поддерживается только у линейного progress; у `drawCircleProgress()` его нет.

Для `label(...)` и `percent(...)` доступны:

- `Left` - прижать текст к левому краю progress
- `Center` - выровнять текст по центру progress
- `Right` - прижать текст к правому краю progress

`anim(...)` у progress поддерживает:

- `None` - progress статичный, без анимации
- `Shimmer` - по заполненной части идет мягкий движущийся блик

## 12.6. Круговой прогресс

```cpp
ui.drawCircleProgress()
    .pos(center, 140)                    // центр кольца
    .radius(34)                          // внешний радиус
    .thickness(8)                        // толщина кольца
    .value(72)                           // значение progress: 0..100
    .baseColor(ui.rgb(30, 30, 30))       // цвет пустой части
    .fillColor(ui.rgb(0, 120, 255))      // цвет заполненной части
    .anim(None);                         // тип анимации progress
```

Локальное обновление:

```cpp
ui.updateCircleProgress()
    .pos(center, 140)                    // тот же центр
    .radius(34)                          // тот же внешний радиус
    .thickness(8)                        // та же толщина кольца
    .value(72)                           // новое значение: 0..100
    .baseColor(ui.rgb(30, 30, 30))       // цвет пустой части
    .fillColor(ui.rgb(0, 120, 255))      // цвет заполненной части
    .anim(None);                         // тип анимации progress
```

## 12.7. Drum roll

Горизонтальный:

```cpp
ui.drawDrumRoll()
    .pos(20, 60)                            // левый верхний угол
    .size(200, 40)                          // ширина и высота области
    .options(16, "Low", "Medium", "High")   // шрифт и список опций; количество считается автоматически
    .selected(1)                            // текущий выбранный индекс
    .fgColor(ui.rgb(255, 255, 255))         // цвет текста
    .bgColor(ui.rgb(0, 0, 0));              // цвет фона
```

Вертикальный:

```cpp
ui.drawDrumRoll()
    .pos(220, 100)                          // левый верхний угол
    .size(70, 90)                           // ширина и высота области
    .options(16, "Low", "Medium", "High")   // шрифт и список опций; количество считается автоматически
    .selected(1)                            // текущий выбранный индекс
    .fgColor(ui.rgb(255, 255, 255))         // цвет текста
    .bgColor(ui.rgb(0, 0, 0))               // цвет фона
    .vertical();                            // включить вертикальный режим
```

---

# 13. Списки и плитки

## 13.1. Списочное меню

Прямо в `SCREEN(...)`:

```cpp
SCREEN(ScreenMainMenu, 1)
{
    ui.clear(0x0000);

    ui.updateList()
    .items(
        listItem("Settings", "Device configuration", ScreenSettings),
        listItem("About",    "Firmware info",        ScreenAbout),
        listItem("Restart",  "Reboot device",        ScreenRestart))
        .inactive(ui.rgb(12, 12, 12))      // цвет обычной карточки
        .active(ui.rgb(0, 120, 255))       // цвет активной карточки
        // .checked(1)                        // опционально: справа рисовать галочку у этого пункта
        .radius(8)                         // радиус карточек
        .cardSize(0, 0)                    // 0,0 = размер подберётся автоматически
        .mode(Cards);                      // режим списка
}
```

Режимы списка:

- `Cards`
- `Plain`

Ввод в `loop()`:

```cpp
ui.listInput()
    .nextDown(btnNext.isDown())            // кнопка NEXT сейчас удерживается
    .prevDown(btnPrev.isDown());           // кнопка PREV сейчас удерживается
```

Поведение:

- короткое отпускание `NEXT` переключает пункт вперёд;
- короткое отпускание `PREV` переключает пункт назад;
- удержание `NEXT` открывает `targetScreen` выбранного пункта;
- удержание `PREV` возвращает на предыдущий экран из navigation-history.

## 13.2. Плиточное меню

Прямо в `SCREEN(...)`:

```cpp
SCREEN(ScreenTiles, 2)
{
    ui.updateTile()
    .items(
        tileItem("Main",     "Главный экран", ScreenHome),
        tileItem("Settings", "Настройки",     ScreenSettings),
        tileItem("Info",     "Инфо",          ScreenInfo),
        tileItem("Graph",    "Графики",       ScreenGraph))
        .inactive(ui.rgb(16, 16, 16))     // обычная плитка
        .active(ui.rgb(0, 120, 255))      // активная плитка
        .radius(12)                       // радиус плитки
        .spacing(10)                      // расстояние между плитками
        .columns(2)                       // количество колонок в обычной сетке
        .tileSize(100, 70)                // желаемый размер плитки
        .mode(TextSubtitle);              // контент плитки
}
```

Ввод:

```cpp
ui.tileInput()
    .nextDown(btnNext.isDown())           // кнопка NEXT сейчас удерживается
    .prevDown(btnPrev.isDown());          // кнопка PREV сейчас удерживается
```

Режимы плитки:

- `TextOnly`
- `TextSubtitle`

## 13.3. Кастомная раскладка плиток

Если обычной сетки мало, можно задать свою понятную раскладку:

```cpp
ui.updateTile()
    .grid(2, 3)                                      // сетка: 2 колонки, 3 строки
    .tile("Main", "Главный экран", ScreenHome)
    .at(0, 0)                                        // колонка 0, строка 0
    .span(2, 1)                                      // ширина 2 клетки, высота 1 клетка
    .tile("Settings", "Настройки", ScreenSettings)
    .at(0, 1)                                        // колонка 0, строка 1
    .tile("Info", "Инфо", ScreenInfo)
    .at(1, 1)                                        // колонка 1, строка 1
    .tile("Graph", "Графики", ScreenGraph)
    .at(1, 2)                                        // колонка 1, строка 2
```

Что важно:

- `grid(cols, rows)` включает кастомную сетку
- `tile(...)` добавляет очередную плитку
- `at(col, row)` ставит последнюю добавленную плитку в нужную клетку
- `span(cols, rows)` растягивает последнюю добавленную плитку на несколько клеток
- в этом режиме `columns(...)` уже не влияет на раскладку

---

# 14. Графики

## 14.1. Фон и сетка:

```cpp
ui.drawGraphGrid()
    .pos(10, 50)
    .size(220, 120)
    .radius(8)
    .direction(LeftToRight)           // задаёт режим движения и раскладки данных внутри этой graph-area
    .bgColor(ui.rgb(10, 10, 10))      // цвет сетки вычисляется автоматически из bgColor()
    .speed(1.0f);
```

- `drawGraphGrid()` / `updateGraphGrid()` должны использовать тот же `direction(...)`, в котором потом рисуются линии этого графа

Направления:

- `LeftToRight` - новые точки приходят справа, старая история уезжает влево
- `RightToLeft` - новые точки приходят слева, старая история уезжает вправо
- `Oscilloscope` - фиксированное окно по всей ширине графика без rolling-shift; точки раскладываются по видимому буферу как осциллограф

Для локального dirty-redraw доступен тот же fluent через `updateGraphGrid()`:

```cpp
ui.updateGraphGrid()
    .pos(10, 50)
    .size(220, 120)
    .radius(8)
    .direction(LeftToRight)
    .bgColor(ui.rgb(10, 10, 10))
    .speed(1.0f);
```

## 14.2. Линия графика:

```cpp
ui.drawGraphLine()
    .line(0)
    .value(sensorValue)
    .thickness(2)                   // задаёт толщину линии графика; по умолчанию `1`
    .color(ui.rgb(0, 255, 140))
    .range(0, 100);
    .scale(true);                   // вкл/выкл автоматический диапазон по данным для этой graph-area
```

- `drawGraphLine()` добавляет новую точку в уже настроенный график
- `updateGraphLine()` подходит для in-place обновления, когда графику нужно самому зачистить и перерисовать нужную область

## 14.3. Пакетная отрисовка готового массива:

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
- `updateGraphSamples()` использует тот же API, но подходит для локального in-place обновления

Для `Oscilloscope` эти настройки задаются прямо у сетки:

```cpp
ui.drawGraphGrid()
    .pos(center, center)
    .size(200, 170)
    .direction(Oscilloscope)
    .scope(2000, 100)                  // частота входных samples и длительность окна в мс
    .visible(0);
```

Что важно по lifecycle:

- `drawGraphGrid()` задаёт активную область графика и должен вызываться в screen-callback этого экрана
- buffered-график (`drawGraphLine()`) живёт только пока экран реально рисует граф в текущих кадрах
- если экран перестал вызывать graph API или вы ушли на другой screen, внутренние буферы графа освобождаются
- при возврате граф начинает собирать историю заново

Что важно по режимам:

- `LeftToRight` и `RightToLeft` используют rolling-history
- `Oscilloscope` использует фиксированное окно видимых samples
- если `visible(0)`, окно для `Oscilloscope` вычисляется из `rateHz * timebaseMs`

---

# 15. Уведомления, toast, ошибки

## 15.1. Toast

```cpp
ui.showToast()
    .text("Saved")    // основной текст toast; можно оставить пустым, если нужен только значок
    .icon(error)      // необязательная иконка слева
    .pos(top);        // `top` или `down`; по умолчанию используется `down`
```

Позиции toast:

- `top` — появляется сверху
- `down` — появляется снизу

Проверка активности:

```cpp
bool active = ui.toastActive();    // `true`, пока toast еще анимируется или висит на экране
```

Поведение:

- если не заданы и текст, и иконка, toast не показывается
- toast сам показывается, держится на экране и затем скрывается с анимацией

## 15.2. Notification

```cpp
ui.showNotification()
    .text("Settings", "Changes applied") // заголовок и основной текст карточки
    .button("OK")                        // подпись кнопки подтверждения
    .delay(0)                            // автозакрытие в секундах; `0` значит без таймера
    .type(Normal)                        // semantic-тип уведомления
    .icon(warning);                      // необязательная иконка; без нее библиотека берет visual от `type(...)`
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

## 15.3. Popup menu

Самый обычный сценарий:

```cpp
static const char *menuItems[] = {"Edit", "Rename", "Delete"};

auto button = ui.updateButton()
    .label("Open menu")
    .pos(center, 188)
    .size(180, 32)
    .baseColor(ui.rgb(0, 87, 250))
    .radius(10);

ui.showPopupMenu()
    .items(menuItems)                  // обычный массив строк
    .anchor(button)                    // привязка к fluent-компоненту; меню само откроется над ним или под ним
    .width(120)                        // ширина меню; если не задать, библиотека подберет сама
    .selected(0);                      // стартовый выделенный пункт; если не задавать, берется дефолт
```

Во время активности меню:

```cpp
if (ui.popupMenuActive())                             // значит, что меню сейчас открыто и перехватило ввод
{
    ui.popupMenuInput()
        .nextDown(btnNext.isDown())                   // перемещение вперед по пунктам
        .prevDown(btnPrev.isDown());                  // перемещение назад по пунктам

    if (ui.popupMenuHasResult())                      // говорит, что пользователь уже выбрал пункт
    {
        int16_t picked = ui.popupMenuTakeResult();    // возвращает индекс выбранного пункта и сразу сбрасывает флаг результата
    }
}
```

Что важно:

- `.selected(index)` нужен только если хочешь вручную задать стартовый курсор
- `.anchor(component)` берет прямоугольник fluent-компонента и открывает меню по центру над ним или под ним
- если использовать короткий паттерн без `popupMenuHasResult()`, то `popupMenuTakeResult() == -1` значит, что результата пока нет
 
## 15.4. Ошибки

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
- `Warning` можно закрыть
- `Error` не закрывается пользовательской кнопкой

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

- при запуске без параметров tool покажет меню: port, baud и output directory

Быстрый прямой запуск тоже можно:

```bash
python tools/screenshots/bin/capture.py --port COM9 --baud 1000000
```

## 16.3. Flash (LittleFS)

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

## 16.5. Галерея миниатюр

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
- сам вычисляет размер миниатюр и вместимость галереи из `size(...)`, `grid(...)` и `padding(...)`
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
python tools/ota/key.py
```

Если запустить без параметров, tool покажет меню и даст выбрать путь сохранения числом.

Сделать stable release:

```bash
python tools/ota/release.py
```

Сделать beta release:

```bash
python tools/ota/release.py --beta --interactive
```

Проверка manifest + bin:

```bash
python tools/ota/verify.py
```

При запуске без параметров tool покажет меню: manifest, firmware source и signature check.

## 18.2. Конфигурация

Обычно всё задаётся в `include/config.hpp`: 

```cpp
#define PIPGUI_OTA 1
#define PIPGUI_OTA_PROJECT_URL "https://example.com/fw/pipGUI"
#define PIPGUI_OTA_ED25519_PUBKEY_HEX "..."

#define PIPGUI_FIRMWARE_TITLE "pipGUI"
#define PIPGUI_FIRMWARE_VER_MAJOR 1
#define PIPGUI_FIRMWARE_VER_MINOR 0
#define PIPGUI_FIRMWARE_VER_PATCH 2
```

Что это значит:

- `PIPGUI_OTA` включает сам OTA subsystem
- `PIPGUI_OTA_PROJECT_URL` это базовый URL проекта, откуда берутся manifest и бинарники
- `PIPGUI_OTA_ED25519_PUBKEY_HEX` это публичный ключ, которым проверяется подпись manifest и firmware
- `PIPGUI_FIRMWARE_TITLE` это имя прошивки в UI
- `PIPGUI_FIRMWARE_VER_MAJOR / MINOR / PATCH` это текущая версия прошивки, с которой backend сравнивает remote release

Что обычно важно:

- без `PIPGUI_OTA_ED25519_PUBKEY_HEX` нормальная защищенная OTA-схема не имеет смысла
- URL и версия это то, что меняют чаще всего
- если OTA не нужен, достаточно держать `PIPGUI_OTA 0`

## 18.3. Использование

### Минимальный сценарий

```cpp
ui.otaConfigure();                    // один раз на старте
ui.otaRequestCheck();                 // запустить проверку обновления

const OtaStatus& st = ui.otaStatus(); // текущее состояние OTA state-machine
```

Этого уже хватает для обычного сценария, потому что `GUI::loop()` сам обслуживает OTA backend.

### Проверка обновления

```cpp
ui.otaRequestCheck();
ui.otaRequestCheck(NewerOnly);
ui.otaRequestCheck(AllowDowngrade);
```

- `ui.otaRequestCheck()` это обычная проверка
- `NewerOnly` разрешает только обновление вверх
- `AllowDowngrade` разрешает откат на более старую версию
- backend идет по каналам в порядке `stable -> beta`
- если в `stable` ничего подходящего нет, потом проверяется `beta`

### Установка и отмена

```cpp
ui.otaRequestInstall();   // начать установку уже найденного релиза
ui.otaCancel();           // отменить текущую OTA-операцию
```

### История stable-версий

```cpp
ui.otaRequestStableList();                    // запросить список stable-версий
bool ready = ui.otaStableListReady();         // список уже загружен
uint8_t count = ui.otaStableListCount();      // сколько stable-версий доступно
const char* ver = ui.otaStableListVersion(i); // строка версии по индексу

ui.otaRequestInstallStableVersion("1.2.3");
```

Это нужно только если хочешь показывать пользователю список старых stable-сборок и давать выбрать rollback вручную.

### Дополнительно

```cpp
ui.otaService();
ui.otaMarkAppValid();
```

- `otaService()` публичный, но обычно вручную не нужен, потому что `GUI::loop()` уже вызывает его сам
- `otaMarkAppValid()` нужен после reboot, если новая прошивка стартовала в режиме ожидания подтверждения
- в обычном UI-коде чаще всего достаточно `otaConfigure()`, `otaRequestCheck()`, `otaRequestInstall()` и `otaStatus()`

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

На практике чаще всего встречаются:

- `WifiNotConnected` — нет соединения
- `HttpStatusNotOk` — сервер не отдал корректный HTTP-ответ
- `ManifestParseFailed` — manifest битый или неожиданный по формату
- `SignatureInvalid` — подпись не сошлась
- `HashMismatch` — firmware скачалась, но checksum не совпала
