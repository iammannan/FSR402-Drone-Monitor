#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// ── WiFi AP ──────────────────────────────────────────────────────────────────
const char* AP_SSID = "Data Plotter for Drone";
const char* AP_PASS = "00009999";

// ── ADC1 pins for 8x FSR402 ──────────────────────────────────────────────────
const gpio_num_t SENSOR_PINS[8] = {
    GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4,
    GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8
};

AsyncWebServer server(80);
AsyncWebSocket  ws("/ws");

int tare[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// ── 1 kHz batch sampler ───────────────────────────────────────────────────────
// Collect BATCH_SIZE samples then send one WebSocket frame (20 frames/sec)
#define BATCH_SIZE  50      // 50 samples × 20 packets = 1000 samples/sec
#define N_SENSORS   8

struct Sample {
    uint32_t t;             // millis() timestamp
    int16_t  v[N_SENSORS];
};

static Sample batchA[BATCH_SIZE], batchB[BATCH_SIZE];
static Sample* writeBuf = batchA;
static Sample* sendBuf  = batchB;
static int     writeIdx = 0;
static bool    batchReady = false;

// ── WebSocket event handler ──────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->opcode == WS_TEXT && len >= 4) {
            String msg((char*)data, len);
            if (msg == "tare") {
                for (int i = 0; i < N_SENSORS; i++)
                    tare[i] = analogRead(SENSOR_PINS[i]);
                Serial.println("[WS] Tare applied");
                ws.textAll("{\"tared\":true}");
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    for (int i = 0; i < N_SENSORS; i++) pinMode(SENSOR_PINS[i], INPUT);

    if (!LittleFS.begin(true)) {
        Serial.println("[FS] Mount failed!");
        while (true) delay(1000);
    }
    Serial.println("[FS] LittleFS OK");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(200);
    Serial.printf("[AP] SSID: \"%s\"  IP: %s\n", AP_SSID,
                  WiFi.softAPIP().toString().c_str());

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });
    server.begin();
    Serial.println("[HTTP] Server ready → http://192.168.4.1");
}

// ── Build and send one batch frame ───────────────────────────────────────────
// Compact format: {"b":[[t,v0,v1,v2,v3,v4,v5,v6,v7],...]}
static void sendBatch()
{
    // Each row: [t,v0..v7] ≈ 55 chars  →  55*50 + wrapper ≈ 2.8 KB
    static char buf[3200];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "{\"b\":[");
    for (int i = 0; i < BATCH_SIZE; i++) {
        const Sample& s = sendBuf[i];
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "[%lu,%d,%d,%d,%d,%d,%d,%d,%d]%s",
            (unsigned long)s.t,
            s.v[0], s.v[1], s.v[2], s.v[3],
            s.v[4], s.v[5], s.v[6], s.v[7],
            i < BATCH_SIZE - 1 ? "," : "");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    ws.textAll(buf);
}

// ── Main loop: sample at 1 kHz, send batch every 50 ms ──────────────────────
unsigned long lastSample_us = 0;
unsigned long lastCleanup   = 0;

void loop()
{
    unsigned long now_us = micros();
    unsigned long now_ms = millis();

    // 1 kHz sampling — runs every 1000 µs
    if (now_us - lastSample_us >= 1000) {
        lastSample_us += 1000;  // drift-free increment

        if (ws.count() > 0) {
            Sample& s = writeBuf[writeIdx];
            s.t = now_ms;
            for (int i = 0; i < N_SENSORS; i++)
                s.v[i] = (int16_t)(analogRead(SENSOR_PINS[i]) - tare[i]);

            if (++writeIdx >= BATCH_SIZE) {
                // swap buffers
                Sample* tmp = writeBuf;
                writeBuf  = sendBuf;
                sendBuf   = tmp;
                writeIdx  = 0;
                batchReady = true;
            }
        }
    }

    // Send pending batch (no blocking inside, just formats + queues)
    if (batchReady) {
        batchReady = false;
        sendBatch();
    }

    // Cleanup stale WebSocket clients once per second
    if (now_ms - lastCleanup >= 1000) {
        ws.cleanupClients();
        lastCleanup = now_ms;
    }
}
