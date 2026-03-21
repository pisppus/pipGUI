struct FirmwareUpdateLayout
{
  int16_t W = 0;
  int16_t H = 0;
  int16_t m = 12;
  int16_t gap = 6;
  int16_t cardX = 0;
  int16_t cardW = 0;

  int16_t infoY = 54;
  int16_t infoH = 88;

  int16_t notesY = 0;
  int16_t notesH = 40;

  int16_t btnH = 32;
  int16_t btnY = 0;
  int16_t btnW = 0;
  int16_t btnX0 = 0;
  int16_t btnX1 = 0;

  int16_t keyX = 0;
  int16_t valX = 0;
  int16_t valW = 0;

  int16_t rowY0 = 0;
  int16_t rowY1 = 0;
  int16_t rowY2 = 0;

  int16_t notesLabelY = 0;
  int16_t notesTextY = 0;
};

static FirmwareUpdateLayout calcFirmwareUpdateLayout() noexcept
{
  FirmwareUpdateLayout l;
  l.W = (int16_t)ui.screenWidth();
  l.H = (int16_t)ui.screenHeight();
  l.cardX = l.m;
  l.cardW = (int16_t)(l.W - l.m * 2);

  l.notesY = (int16_t)(l.infoY + l.infoH + l.gap);

  l.btnY = (int16_t)(l.H - l.m - l.btnH);
  l.btnW = (int16_t)((l.cardW - l.gap) / 2);
  l.btnX0 = l.cardX;
  l.btnX1 = (int16_t)(l.cardX + l.btnW + l.gap);

  l.keyX = (int16_t)(l.cardX + 10);
  l.valX = (int16_t)(l.cardX + 92);
  l.valW = (int16_t)(l.cardX + l.cardW - 10 - l.valX);

  const int16_t rowStep = 22;
  l.rowY0 = (int16_t)(l.infoY + 12);
  l.rowY1 = (int16_t)(l.rowY0 + rowStep);
  l.rowY2 = (int16_t)(l.rowY1 + rowStep);

  l.notesLabelY = (int16_t)(l.notesY + 10);
  l.notesTextY = (int16_t)(l.notesY + 24);

  return l;
}

SCREEN(firmwareUpdate, 40)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const FirmwareUpdateLayout l = calcFirmwareUpdateLayout();

  ui.setTextStyle(H1);
  ui.drawText().text(PIPGUI_FIRMWARE_TITLE).pos(-1, 18).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  ui.setTextStyle(Caption);
  ui.drawText().text("Prev=action  Next=rollback  Hold=back").pos(-1, 42).color(ui.rgb(170, 170, 170)).bgColor(bg565).align(Center);

  const uint16_t cardBg = ui.rgb(16, 16, 16);
  ui.fillRect().pos(l.cardX, l.infoY).size(l.cardW, l.infoH).radius({14}).color(cardBg);
  ui.fillRect().pos(l.cardX, l.notesY).size(l.cardW, l.notesH).radius({14}).color(cardBg);

  ui.setTextStyle(Caption);
  const uint32_t keyFg = ui.rgb(150, 150, 150);
  ui.drawText().text("Current").pos(l.keyX, l.rowY0).color(keyFg).bgColor(cardBg).align(Left);
  ui.drawText().text("Wi-Fi").pos(l.keyX, l.rowY1).color(keyFg).bgColor(cardBg).align(Left);
  ui.drawText().text("Update").pos(l.keyX, l.rowY2).color(keyFg).bgColor(cardBg).align(Left);
  ui.drawText().text("What's new").pos(l.keyX, l.notesLabelY).color(keyFg).bgColor(cardBg).align(Left);

  ui.drawProgressBar()
      .pos(l.cardX + 10, l.infoY + l.infoH - 16)
      .size(l.cardW - 20, 8)
      .radius(4)
      .value(0)
      .anim(None)
      .baseColor(ui.rgb(18, 18, 18))
      .fillColor(ui.rgb(40, 150, 255));

  ui.drawButton()
      .label("Check update")
      .pos(l.btnX0, l.btnY)
      .size(l.btnW, l.btnH)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(11)
      .state(otaBtnState);

  ui.drawButton()
      .label("Rollback")
      .pos(l.btnX1, l.btnY)
      .size(l.btnW, l.btnH)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(11)
      .state(otaRollbackBtnState);
}
