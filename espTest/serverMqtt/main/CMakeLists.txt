idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES arduino esp_http_client esp_websocket_client nvs_flash esp_wifi esp_event esp_netif)
