#include <SPI.h>
#include <Ethernet.h>

// ===============
// KONFIGURASI
// ===============

// Gunakan MAC address random (bisa apa saja, tapi unik)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Pilihan 1: DHCP (otomatis)
// EthernetClient client;

// Pilihan 2: STATIC IP
IPAddress ip(192, 168, 1, 200);   // sesuaikan jaringan LAN kamu
IPAddress dns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Web server port 80
EthernetServer server(80);

void setup() {
  pinMode(13, OUTPUT);  // LED terpasang pada pin 13
  digitalWrite(13, LOW);

  Serial.begin(9600);
  while (!Serial) {}

  Serial.println("Initializing Ethernet...");

  // ---- DHCP ----
  // if (Ethernet.begin(mac) == 0) {
  //   Serial.println("DHCP Failed, switching to STATIC IP");
  //   Ethernet.begin(mac, ip, dns, gateway, subnet);
  // }

  // ---- STATIC IP ----
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  delay(1000);

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  EthernetClient client = server.available();

  if (client) {
    Serial.println("New Client Connected");
    boolean currentLineIsBlank = true;
    String request = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        // Cek end of header (baris kosong)
        if (c == '\n' && currentLineIsBlank) {
          
          // ======================
          // PROSES REQUEST
          // ======================
          if (request.indexOf("GET /on") >= 0) {
            digitalWrite(13, HIGH);
            Serial.println("LED ON");
          }

          if (request.indexOf("GET /off") >= 0) {
            digitalWrite(13, LOW);
            Serial.println("LED OFF");
          }

          // ======================
          // RESPON WEB
          // ======================
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<html><body>");
          client.println("<h2>Arduino W5100 LED Control</h2>");
          client.println("<a href='/on'><button>LED ON</button></a><br/><br/>");
          client.println("<a href='/off'><button>LED OFF</button></a>");
          client.println("</body></html>");

          break;
        }

        if (c == '\n') currentLineIsBlank = true;
        else if (c != '\r') currentLineIsBlank = false;
      }
    }

    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
}
