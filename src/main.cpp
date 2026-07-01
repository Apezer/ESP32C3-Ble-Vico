#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleKeyboard.h>
#include "font.h"

// Vico Keyboard MVP
// Hardware: 5 keys + SSD1306 OLED
// Key layout: arrow cluster + Enter

#define OLED_SDA 6
#define OLED_SCL 7
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define NUM_KEYS 5
#define DEBOUNCE_MS 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum KeyIndex {
    KEY_UP_INDEX = 0,
    KEY_DOWN_INDEX,
    KEY_LEFT_INDEX,
    KEY_RIGHT_INDEX,
    KEY_ENTER_INDEX,
};

// Keep the existing wiring from the MiniKeyboard project for now.
// You can remap these pins later when the Vico Keyboard PCB is fixed.
const uint8_t KEY_PINS[NUM_KEYS] = {2, 3, 4, 5, 8};

const uint8_t HID_KEYS[NUM_KEYS] = {
    KEY_UP_ARROW,
    KEY_DOWN_ARROW,
    KEY_LEFT_ARROW,
    KEY_RIGHT_ARROW,
    KEY_RETURN,
};

const char *KEY_LABELS[NUM_KEYS] = {
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "ENTER",
};

bool keyState[NUM_KEYS] = {false};
bool lastReportedState[NUM_KEYS] = {false};
unsigned long lastChangeTime[NUM_KEYS] = {0};
String serialLine;
String currentState = "READY";
String currentMessage = "Booting";
uint8_t thinkingFrame = 0;
unsigned long lastThinkingFrameAt = 0;

BleKeyboard bleKeyboard("Vico Keyboard", "Apezer", 100);

void showSplash() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.drawBitmap(
        (SCREEN_WIDTH - CLAUDE_LOGO_WIDTH) / 2,
        (SCREEN_HEIGHT - CLAUDE_LOGO_HEIGHT) / 2,
        CLAUDE_LOGO_64X32,
        CLAUDE_LOGO_WIDTH,
        CLAUDE_LOGO_HEIGHT,
        SSD1306_WHITE
    );

    display.display();
}

void drawLogoBorderPixel(uint8_t pos) {
    const uint8_t x = 96;
    const uint8_t y = 8;
    const uint8_t w = CLAUDE_LOGO_SMALL_WIDTH;
    const uint8_t h = CLAUDE_LOGO_SMALL_HEIGHT;

    if (pos < w) {
        display.drawPixel(x + pos, y, SSD1306_WHITE);
    } else if (pos < w + h - 1) {
        display.drawPixel(x + w - 1, y + pos - w + 1, SSD1306_WHITE);
    } else if (pos < w + h - 1 + w - 1) {
        display.drawPixel(x + w - 2 - (pos - w - h + 1), y + h - 1, SSD1306_WHITE);
    } else {
        display.drawPixel(x, y + h - 2 - (pos - w - h + 1 - w + 1), SSD1306_WHITE);
    }
}

void drawThinkingAnimation() {
    if (currentState != "THINKING") {
        return;
    }

    const uint8_t perimeter = 2 * (CLAUDE_LOGO_SMALL_WIDTH + CLAUDE_LOGO_SMALL_HEIGHT) - 4;
    const uint8_t segmentLength = 14;

    for (uint8_t i = 0; i < segmentLength; i++) {
        drawLogoBorderPixel((thinkingFrame + i) % perimeter);
    }
}

void renderStatus() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("VICO_keyboard");
    display.setCursor(92, 0);
    display.print(bleKeyboard.isConnected() ? "BLE" : "WAIT");

    display.setTextSize(2);
    display.setCursor(0, 8);
    display.print(currentState.substring(0, 8));
    display.drawBitmap(
        96,
        8,
        CLAUDE_LOGO_32X16,
        CLAUDE_LOGO_SMALL_WIDTH,
        CLAUDE_LOGO_SMALL_HEIGHT,
        SSD1306_WHITE
    );
    drawThinkingAnimation();

    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print(currentMessage.substring(0, 20));

    display.display();
}

void showStatus(const String &state, const String &message) {
    currentState = state;
    currentMessage = message;
    thinkingFrame = 0;
    lastThinkingFrameAt = millis();
    renderStatus();
}

void handleStatusLine(String line) {
    line.trim();
    if (line.length() == 0) {
        return;
    }

    Serial.print("VICO RX: ");
    Serial.println(line);

    if (line.startsWith("STATE ")) {
        showStatus(line.substring(6), currentMessage);
    } else if (line.startsWith("TEXT ")) {
        showStatus(currentState, line.substring(5));
    } else if (line.startsWith("SHOW ")) {
        int separator = line.indexOf(' ', 5);
        if (separator > 0) {
            showStatus(line.substring(5, separator), line.substring(separator + 1));
        } else {
            showStatus(line.substring(5), "");
        }
    }
}

void handleSerialStatus() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n') {
            handleStatusLine(serialLine);
            serialLine = "";
        } else if (c != '\r') {
            serialLine += c;
            if (serialLine.length() > 96) {
                serialLine = "";
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    serialLine.reserve(96);

    bleKeyboard.begin();

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed");
        for (;;)
            ;
    }

    for (int i = 0; i < NUM_KEYS; i++) {
        pinMode(KEY_PINS[i], INPUT_PULLUP);
    }

    showSplash();
    delay(1000);
    showStatus("READY", "Waiting for Claude");
}

void loop() {
    handleSerialStatus();

    if (currentState == "THINKING" && millis() - lastThinkingFrameAt > 90) {
        lastThinkingFrameAt = millis();
        const uint8_t perimeter = 2 * (CLAUDE_LOGO_SMALL_WIDTH + CLAUDE_LOGO_SMALL_HEIGHT) - 4;
        thinkingFrame = (thinkingFrame + 4) % perimeter;
        renderStatus();
    }

    if (!bleKeyboard.isConnected()) {
        static unsigned long lastDraw = 0;
        if (millis() - lastDraw > 500) {
            lastDraw = millis();
            renderStatus();
        }
        return;
    }

    unsigned long now = millis();
    bool changed = false;

    for (int i = 0; i < NUM_KEYS; i++) {
        bool raw = (digitalRead(KEY_PINS[i]) == LOW);

        if (raw != keyState[i] && (now - lastChangeTime[i]) >= DEBOUNCE_MS) {
            keyState[i] = raw;
            lastChangeTime[i] = now;
            changed = true;
        }
    }

    if (changed) {
        for (int i = 0; i < NUM_KEYS; i++) {
            if (keyState[i] && !lastReportedState[i]) {
                bleKeyboard.press(HID_KEYS[i]);
            } else if (!keyState[i] && lastReportedState[i]) {
                bleKeyboard.release(HID_KEYS[i]);
            }

            lastReportedState[i] = keyState[i];
        }
    }
}
