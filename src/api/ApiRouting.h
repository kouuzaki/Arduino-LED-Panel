#ifndef SETUP_API_H
#define SETUP_API_H

#include <Ethernet.h>

// API Routes using EthernetServer
// Cocok untuk Arduino UNO dengan W5100 LAN Shield

class ApiRouting
{
private:
    // Singleton instance
    static ApiRouting *instance;

    // EthernetServer instance - port 80 (HTTP standar)
    EthernetServer *server;

    // Flag untuk tracking status server
    bool serverStarted;

    // Private constructor for singleton
    ApiRouting();

    // Private destructor
    ~ApiRouting();

    // Prevent copying and assignment
    ApiRouting(const ApiRouting &) = delete;
    ApiRouting &operator=(const ApiRouting &) = delete;

public:
    // Get singleton instance
    static ApiRouting &getInstance();

    // Initialize API routes (setup saja, tidak buat server di sini)
    void setupRoutes();

    // Start the server
    void start();

    // Stop the server
    void stop();

    // Handle client requests - process incoming HTTP requests
    // HARUS dipanggil dari main loop secara berkala
    void handleClient();

    // Helper function untuk parse HTTP request
    void handleHttpRequest(EthernetClient &client);
};

#endif
