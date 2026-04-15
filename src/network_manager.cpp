#include "network_manager.h"
#include <WiFiManager.h>
#include <Ticker.h>

Ticker ticker;

// ── RGB LED Pins (4-pin common cathode) ─────────────────────────────────────
// Wiring: R → GPIO25, G → GPIO26, B → GPIO27, Common (−) → GND
const int LED_R = 25;
const int LED_G = 26;
const int LED_B = 27;

const int RESET_PIN = 0;
unsigned long pressStartTime = 0;
bool isPressing    = false;
int currentBlinkPhase = -1; // -1: Normal, 0-2: Button hold progress, 3: Reset

// WiFi state tracking
bool wifiConnected = false;

// Toggle state used inside ticker callbacks
bool toggleState = false;

// ── RGB Helper ───────────────────────────────────────────────────────────────
void setColor(bool r, bool g, bool b) {
    digitalWrite(LED_R, r ? HIGH : LOW);
    digitalWrite(LED_G, g ? HIGH : LOW);
    digitalWrite(LED_B, b ? HIGH : LOW);
}

// ── Ticker Callbacks ─────────────────────────────────────────────────────────

// Blinking Red (0.5s intervals)
void tickRed() {
    toggleState = !toggleState;
    if (toggleState) setColor(true, false, false);  // Red ON
    else             setColor(false, false, false);  // OFF
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setupNetwork() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLUP);

    // Initial state: Red blinking while connecting to WiFi (or after reset)
    toggleState = false;
    ticker.attach(0.5, tickRed);

    WiFiManager wm;
    // CSS custom untuk portal WiFi
    const char* custom_html = R"rawliteral(

<style>

/* ===== GLOBAL SCALE ===== */
body {
    font-family: sans-serif;
    background-color: #f4f4f4;
    padding: 25px;
    font-size: 26px !important;
}

/* ===== WIFI SSID LIST ===== */
a[data-ssid] {
    display: block !important;
    font-size: 44px !important;
    font-weight: 900 !important;
    padding: 55px 30px !important;
    margin: 30px 0 !important;
    line-height: 1.4 !important;
    border-radius: 22px;
    background: #ffffff;
    border: 3px solid #ccc;
}

/* Hide signal bars */
div.q {
    display: none !important;
}

/* ===== FORM LABELS ===== */
label {
    display: block;
    font-size: 34px !important;
    font-weight: 800 !important;
    margin-top: 50px;
    margin-bottom: 20px;
}

/* ===== INPUT FIELDS ===== */
input[type="text"],
input[type="password"] {
    height: 120px !important;
    font-size: 34px !important;
    border-radius: 18px;
    width: 100%;
    border: 3px solid #999;
    padding-left: 25px;
}

/* ===== BUTTONS ===== */
button,
input[type="submit"] {
    background-color: #007bff !important;
    height: 140px !important;
    font-size: 36px !important;
    font-weight: 900 !important;
    border-radius: 26px !important;
    margin-top: 45px;
    width: 100%;
    color: #fff !important;
}

/* ===== CHECKBOX (Show Password) ===== */
input[type="checkbox"] {
    transform: scale(2.2);
    margin-right: 15px;
}

/* ===== GENERAL TEXT (messages like "No AP set") ===== */
.msg,
div,
p,
span {
    font-size: 30px !important;
}

/* ===== TOUCH FRIENDLY SPACING ===== */
br {
    display: block;
    margin-bottom: 20px;
}

</style>

<script>
document.addEventListener('DOMContentLoaded', function () {

    /* Rename SSID label to Nama WiFi */
    document.querySelectorAll('label').forEach(function(label) {
        if (label.innerText.includes('SSID')) {
            label.innerText = 'Nama WiFi';
        }
    });

});
</script>
)rawliteral";

    wm.setCustomHeadElement(custom_html);

    // Autoconnect akan memblokir sampai terhubung atau timeout
    if (!wm.autoConnect("SmartRemote_Setup")) {
        Serial.println("Gagal terhubung, restarting...");
        ESP.restart();
    }

    // WiFi connected -> Solid green
    ticker.detach();
    toggleState = false;
    setColor(false, true, false);
    wifiConnected = true;
    Serial.println("Terhubung ke WiFi!");
}

// ── Reset Button ─────────────────────────────────────────────────────────────
void checkResetButton() {
    if (digitalRead(RESET_PIN) == LOW) {
        if (!isPressing) {
            pressStartTime = millis();
            isPressing = true;
            Serial.println("Tombol ditekan, tahan 5 detik untuk reset...");
            // Show solid yellow while button is pressed
            ticker.detach();
            toggleState = false;
            setColor(true, true, false);  // Yellow ON
            currentBlinkPhase = 0;
        }

        unsigned long elapsed = millis() - pressStartTime;

        // Check if button held for 5 seconds
        if (elapsed >= 5000 && currentBlinkPhase != 3) {
            currentBlinkPhase = 3;
            ticker.detach();
            toggleState = false;
            // Start blinking red (0.5s intervals)
            ticker.attach(0.5, tickRed);
            Serial.println("5 detik tercapai! Reset WiFi...");
            delay(1500); // Beri waktu untuk melihat efek blinking red
            WiFiManager wm;
            wm.resetSettings();
            delay(500);
            ESP.restart();
        }

    } else {
        // Button released before 5s -> cancel, return to solid green
        if (isPressing) {
            Serial.println("Reset dibatalkan.");
            isPressing = false;
            currentBlinkPhase = -1;
            ticker.detach();
            toggleState = false;
            setColor(false, true, false);  // Solid green (normal state)
        }
    }
}

// ── Device ID ────────────────────────────────────────────────────────────────
String getDeviceID() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return mac;
}