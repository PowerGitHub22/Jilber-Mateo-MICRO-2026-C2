# Medición de la reacción humana (ESP32-S3)

Proyecto en **ESP-IDF 5.4.4** para **ESP32-S3** que mide el **tiempo de reacción**
de una persona ante una señal luminosa y el tiempo que tarda en presionar un
segundo botón. Los resultados se publican **por MQTT** a un panel en el celular.

---

## ¿Cómo funciona la prueba?

1. El usuario **presiona y mantiene presionado** el botón **PB1** (botón de inicio).
2. El micro espera un **tiempo aleatorio** (configurable, p. ej. 1 a 5 s) y luego
   **enciende el LED**.
3. Al ver el LED, el usuario **suelta PB1** → el micro mide el **tiempo de reacción**
   (desde que el LED se enciende hasta que se suelta PB1).
4. Inmediatamente el usuario debe **presionar PB2** → el micro mide el
   **tiempo de movimiento** (desde que soltó PB1 hasta que presiona PB2).
5. Todo se publica por MQTT al celular con precisión de **milisegundos**.

Si el usuario suelta PB1 antes de que el LED se encienda, o tarda demasiado, la
ronda se considera **inválida** y se publica el evento correspondiente.

```
LED ON ──► (usuario suelta PB1) ──► (usuario presiona PB2)
   │              │                         │
   │    TIEMPO DE REACCIÓN         TIEMPO DE MOVIMIENTO
```

---

## Estructura del proyecto

```
Medicion_de_la_reaccion_humana/
|-- CMakeLists.txt            # Proyecto CMake raiz
|-- sdkconfig.defaults        # Target: esp32s3
|-- build.bat                 # Compilar
|-- flash.bat                 # Flashear (usa COM5 por defecto)
|-- menuconfig.bat            # Configuracion (WiFi, MQTT, tiempos)
|-- README.md
|-- .vscode/                  # Configuracion de Visual Studio Code
`-- main/
    |-- CMakeLists.txt
    |-- Kconfig.projbuild     # Parametros WiFi / MQTT / tiempos / topics
    |-- app_pins.h            # Pines: LED (GPIO2), PB1 (GPIO0), PB2 (GPIO1)
    |-- app_events.h          # Eventos del sistema (botones + reset)
    |-- app_main.c            # Integra todos los modulos
    |-- reaction.c/.h         # MAQUINA DE ESTADOS + medicion de tiempos
    |-- buttons.c/.h          # Botones por interrupcion con timestamp (us)
    |-- led.c/.h              # Configuracion del LED
    |-- wifi_app.c/.h         # Conexion WiFi (STA)
    `-- mqtt_app.c/.h         # Cliente MQTT (broker + topics + JSON)
```

---

## Pines (editables en `main/app_pins.h`)

| Elemento | GPIO | Conexión |
|---|---|---|
| LED (señal) | 2 | Ánodo a GPIO2, cátodo a GND (con resistencia) |
| PB1 (inicio / soltar) | 0 | Entre GPIO0 y GND (pull-up interno) |
| PB2 (segundo botón) | 1 | Entre GPIO1 y GND (pull-up interno) |

Los botones cierran a **GND** y usan el **pull-up interno** del ESP32.

> Nota: GPIO0 es un pin de arranque (strapping). Mientras no se presione el botón
> al encender la placa, no hay problema. Si deseas otro pin, cámbialo en
> `main/app_pins.h`.

---

## Cómo compilar y flashear

### Opción A: con los scripts .bat (rápido)

1. Doble clic en `build.bat` (compila).
2. Conecta la placa ESP32-S3 por USB y doble clic en `flash.bat` (flashea).
   - Si tu placa usa otro puerto (ej. COM7): `flash.bat COM7`.
3. `menuconfig.bat` abre la configuración interactiva.

### Opción B: con la terminal ESP-IDF

```bat
set IDF_TOOLS_PATH=C:\Espressif
set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.4.4\venv
powershell -ExecutionPolicy Bypass -File C:\esp\v5.4.4\esp-idf\export.ps1
```

Luego, desde la carpeta del proyecto:

```bat
idf.py set-target esp32s3
idf.py menuconfig     ; configurar WiFi, MQTT y tiempos
idf.py build
idf.py -p COM5 flash  ; cambiar COM5 por el puerto de tu placa
idf.py -p COM5 monitor
```

---

## Configuración obligatoria (menuconfig)

Ejecuta `menuconfig.bat` y ve a **"Configuración de la aplicación"**:

| Opción | Descripción | Valor de ejemplo |
|---|---|---|
| `APP_WIFI_SSID` | Nombre de tu red WiFi | `MiRedWiFi` |
| `APP_WIFI_PASSWORD` | Contraseña de tu red | `MiClave` |
| `APP_MQTT_BROKER_URI` | Broker MQTT | `mqtt://broker.hivemq.com:1883` |
| `APP_MQTT_TOPIC_STATE` | Topic donde se publican los eventos | `reaccion_humana/estado` |
| `APP_MQTT_TOPIC_RESULT` | Topic donde se publican los resultados | `reaccion_humana/resultado` |
| `APP_MQTT_TOPIC_COMMAND` | Topic donde se reciben comandos | `reaccion_humana/comando` |
| `APP_DELAY_MIN_MS` | Tiempo aleatorio mínimo antes del LED | `1000` |
| `APP_DELAY_MAX_MS` | Tiempo aleatorio máximo antes del LED | `5000` |
| `APP_REACTION_TIMEOUT_MS` | Máximo para soltar PB1 tras el LED | `5000` |
| `APP_MOVEMENT_TIMEOUT_MS` | Máximo para presionar PB2 tras soltar PB1 | `5000` |

Guarda con `S` y compila de nuevo. **Sin WiFi configurado, la placa no conectará.**

---

## Protocolo MQTT

### Topic de estado (`reaccion_humana/estado`)

Eventos publicados como texto:

| Evento | Significado |
|---|---|
| `listo` | Sistema arrancó y MQTT conectado |
| `esperando` | Ronda iniciada; LED se encenderá en un tiempo aleatorio |
| `suelta` | LED encendido; el usuario debe soltar PB1 |
| `presiona_segundo` | PB1 soltado; el usuario debe presionar PB2 |
| `invalido` | Ronda inválida (soltó antes de la señal o se agotó el tiempo) |
| `reset` | Ronda cancelada por comando MQTT |

### Topic de resultados (`reaccion_humana/resultado`)

Mensaje **JSON** (con retención, para que el panel lo lea al abrir):

```json
{"t_reaccion_ms": 234, "t_movimiento_ms": 189, "t_total_ms": 423}
```

- `t_reaccion_ms`: tiempo desde que el LED se enciende hasta que suelta PB1.
- `t_movimiento_ms`: tiempo desde que suelta PB1 hasta que presiona PB2.
- `t_total_ms`: suma de ambos.

### Topic de comandos (`reaccion_humana/comando`)

| Comando | Efecto |
|---|---|
| `reset` | Cancela la ronda actual y vuelve al estado inicial |

---

## Panel en el celular

### App recomendada: **MQTT Dash** (Android) o **IoT MQTT Panel** (Android/iOS)

Pasos con **MQTT Dash**:

1. Instala **MQTT Dash** desde Play Store.
2. Crea un nuevo **Dashboard** (`+`).
3. Pulsa `+` para agregar una **suscripción de resultados**:
   - **Broker**: `broker.hivemq.com`, **Puerto**: `1883`.
   - **Topic**: `reaccion_humana/resultado`
   - Tipo: **JSON / String** (verás `t_reaccion_ms`, `t_movimiento_ms`, `t_total_ms`).
4. Pulsa `+` para agregar una **suscripción de estado**:
   - **Topic**: `reaccion_humana/estado`
   - Tipo: **Text / Text value**.
5. (Opcional) Agrega un **publicador** para reiniciar:
   - **Topic**: `reaccion_humana/comando`
   - Valor fijo: `reset`.

### Prueba rápida sin celular (PC)

Abre en el navegador: http://www.hivemq.com/demos/websocket-client/

1. Conecta al broker `broker.hivemq.com` puerto `8000` (WebSocket).
2. Suscríbete a `reaccion_humana/resultado` y `reaccion_humana/estado`.
3. Publica `reset` en `reaccion_humana/comando` si quieres cancelar una ronda.

> Nota: al usar un broker público, cualquier persona puede ver esos topics. Para
> producción usa un broker privado o con credenciales.

---

## Precisión de la medición

- Todas las marcas de tiempo usan `esp_timer_get_time()` (resolución de
  **microsegundos**, µs).
- La detección de los botones se hace por **interrupción GPIO**, no por sondeo,
  por lo que el instante exacto de presionar/soltar se registra en el momento en
  que ocurre.
- El resultado se reporta en **milisegundos** (`/ 1000`).

---

## Solución de problemas

- **No conecta al WiFi**: verifica SSID y clave en `menuconfig` y que la red sea 2.4 GHz.
- **MQTT sin datos**: confirma que el broker esté accesible desde tu red y que los
  topics coincidan exactamente entre placa y celular.
- **El botón no responde**: revisa la conexión (botón entre el GPIO y GND).
- **Ronda "invalido"**: significa que soltaste PB1 antes de que se encienda el LED
  o que tardaste más que el tiempo límite. Es el comportamiento esperado.
- **El puerto COM no aparece**: instala el driver USB del chip (CP210x o CH340
  según tu placa) y revisa que esté conectada.