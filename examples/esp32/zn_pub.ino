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
#define DATA "Publishing data from ESP-32"

void setup() {
  // Set WiFi in STA mode and trigger attachment
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  // Keep trying until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop()
{
    delay(5000);
    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    zn_session_t *s = zn_open(config);
    if (s == 0) {
      return;
    }

    znp_start_read_task(s);
    znp_start_lease_task(s);

    unsigned long rid = zn_declare_resource(s, zn_rname(URI));
    zn_reskey_t reskey = zn_rid(rid);

    zn_publisher_t *pub = zn_declare_publisher(s, reskey);
    if (pub == 0) {
      return;
    }

    char buf[30];
    delay(1000);
    sprintf(buf, "%s", DATA);
    zn_write(s, reskey, (const uint8_t *)buf, strlen(buf));

    zn_undeclare_publisher(pub);
    zn_close(s);
}

