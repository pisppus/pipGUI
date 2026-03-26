SCREEN(mainMenu, 0)
{
    const uint16_t bg565 = ui.rgb(0, 0, 0);
    ui.clear(bg565);
    ui.setTextStyle(H1);

    ui.drawText()
        .text("Main menu")
        .pos(-1, -1)
        .color(ui.rgb(255, 255, 255))
        .bgColor(bg565)
        .align(Center);

    const int16_t ix = 12;
    const int16_t iy = 12;
    const uint16_t is = 48;
    const uint8_t level = g_batteryLevel;
    const uint16_t fill565 = (level <= 20) ? ui.rgb(255, 40, 40) : ui.rgb(80, 255, 120);

    ui.drawIcon()
        .pos(ix, iy)
        .size(is)
        .icon(battery_l2)
        .color(fill565)
        .bgColor(bg565);

    const int16_t cutX = (int16_t)(ix + (int16_t)((uint32_t)is * (uint32_t)level) / 100u);

    ui.drawRect()
        .pos(cutX, iy)
        .size((int16_t)(ix + (int16_t)is - cutX), (int16_t)is)
        .fill(bg565);

    ui.drawIcon()
        .pos(ix, iy)
        .size(is)
        .icon(battery_l0)
        .color(0xFFFF)
        .bgColor(bg565);

    ui.drawIcon()
        .pos(ix, iy)
        .size(is)
        .icon(battery_l1)
        .color(0xFFFF)
        .bgColor(bg565);

    ui.drawIcon()
        .pos(70, 12)
        .size(48)
        .icon(warning)
        .color(0xFF00)
        .bgColor(bg565);
}
