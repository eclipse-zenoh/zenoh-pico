#include <Arduino.h>
#include <WiFi.h>

extern "C" {
  #include "zenoh-pico.h"
}

#define SSID "SSID_GOES_HERE" // CHECK: Network SSID
#define PASS "PASS_GOES_HERE" // CHECK: Network password

// Zenoh-specific parameters
#define MODE "client"
#define PEER "tcp/zenoh-router.local:7447" // CHECK: replace IP address / hostname if needed
#define URI "/demo/example/zenoh-pico-pub"

void setup() {
  Serial.begin(9600);

  // Set WiFi in STA mode and trigger attachment
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  // Keep trying until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

    Serial.print(String((const char*)sample->key.val).substring(0, sample->key.len));
    Serial.print("=>");
    Serial.println(String((const char*)sample->value.val).substring(0, sample->value.len));
}

void loop()
{
    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    zn_session_t *s = zn_open(config);
    if (s == 0) {
        return;
    }

    znp_start_read_task(s);
    znp_start_lease_task(s);

    zn_subscriber_t *sub = zn_declare_subscriber(s, zn_rname(URI), zn_subinfo_default(), data_handler, NULL);
    if (sub == 0) {
        return;
    }

    while(true) {
        delay(10000);
    }

    zn_undeclare_subscriber(sub);
    zn_close(s);
}

