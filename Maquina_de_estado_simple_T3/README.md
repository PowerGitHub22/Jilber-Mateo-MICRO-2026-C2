# Maquina de estado simple T3 (ESP32-S3)

Proyecto en **ESP-IDF 5.4.4** para **ESP32-S3** que implementa una maquina de
estado de **dos estados** (LED_OFF / LED_ON) controlada por:

- Un **boton fisico** (alterna el estado).
- **Comandos MQTT** recibidos desde el celular u otro dispositivo.

El estado actual se refleja en el **LED** y se **publica por MQTT** en tiempo real.

---

## Estructura del proyecto

```
Maquina_de_estado_simple_T3/
|-- CMakeLists.txt            # Proyecto CMake raiz
|-- sdkconfig.defaults        # Target: esp32s3
|-- build.bat                 # Compilar
|-- flash.bat                 # Flashear (usa COM5 por defecto)
|-- menuconfig.bat            # Configuracion (WiFi, MQTT, topics)
|-- README.md
|-- .vscode/                  # Configuracion de Visual Studio Code
`-- main/
    |-- CMakeLists.txt
    |-- Kconfig.projbuild     # Parametros WiFi / MQTT / topics
    |-- app_pins.h            # Pines: LED (GPIO2) y boton (GPIO0)
    |-- app_main.c            # Integra todos los modulos
    |-- state_machine.c/.h    # MAQUINA DE ESTADO (2 estados)
    |-- led.c/.h              # Configuracion del LED
    |-- button.c/.h           # Configuracion del boton (antirrebote)
    |-- wifi_app.c/.h         # Ejemplo de conexion WiFi
    `-- mqtt_app.c/.h         # Cliente MQTT (broker + topics)
```

---

## Como compilar y flashear

### Opcion A: con los scripts .bat (rapido)

1. Doble clic en `build.bat` (compila).
2. Conecta la placa ESP32-S3 por USB y doble clic en `flash.bat` (flashea).
   - Si tu placa usa otro puerto (ej. COM7): `flash.bat COM7`.
3. `menuconfig.bat` abre la configuracion interactiva.

### Opcion B: con la terminal ESP-IDF

```bat
set IDF_TOOLS_PATH=C:\Espressif
set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.4.4\venv
powershell -ExecutionPolicy Bypass -File C:\esp\v5.4.4\esp-idf\export.ps1
```

Luego, desde la carpeta del proyecto:

```bat
idf.py set-target esp32s3
idf.py menuconfig     ; configurar WiFi y MQTT
idf.py build
idf.py -p COM5 flash  ; cambiar COM5 por el puerto de tu placa
idf.py -p COM5 monitor
```

---

## Configuracion obligatoria (menuconfig)

Ejecuta `menuconfig.bat` y ve a **"Configuracion de la aplicacion"**:

| Opcion | Descripcion | Valor de ejemplo |
|---|---|---|
| `APP_WIFI_SSID` | Nombre de tu red WiFi | `MiRedWiFi` |
| `APP_WIFI_PASSWORD` | Contrasena de tu red | `MiClave` |
| `APP_MQTT_BROKER_URI` | Broker MQTT | `mqtt://broker.hivemq.com:1883` |
| `APP_MQTT_TOPIC_STATE` | Topic donde se publica el estado | `maquina_estado_simple/estado` |
| `APP_MQTT_TOPIC_COMMAND` | Topic donde se reciben comandos | `maquina_estado_simple/comando` |

Guarda con `S` y compila de nuevo. **Sin WiFi configurado, la placa no conectara.**

---

## Pines (editable en `main/app_pins.h`)

| Elemento | GPIO |
|---|---|
| LED | 2 |
| Boton (pulsador a GND, con pull-up interno) | 0 |

---

## Maquina de estado

Estados: `STATE_LED_OFF` y `STATE_LED_ON`.

Eventos que provocan transicion:

| Evento | Origen | Efecto |
|---|---|---|
| `EVENT_BUTTON_TOGGLE` | Boton fisico | Alterna el estado |
| `EVENT_MQTT_TOGGLE` | Comando MQTT `toggle` | Alterna el estado |
| `EVENT_MQTT_SET_ON` | Comando MQTT `on` | Pasa a LED_ON |
| `EVENT_MQTT_SET_OFF` | Comando MQTT `off` | Pasa a LED_OFF |

Al cambiar de estado la placa: enciende/apaga el LED y publica en el topic de
estado el mensaje `on` o `off`.

Comandos validos en `maquina_estado_simple/comando`: **on**, **off**, **toggle**.

---

## Configuracion de la app en el celular

### App recomendada: **MQTT Dash** (Android) o **IoT MQTT Panel** (Android/iOS)

Pasos con MQTT Dash:

1. Instala **MQTT Dash** desde Play Store.
2. Crea un nuevo **Dashboard** (`+`).
3. Pulsa `+` para agregar una **suscripcion** (leer el estado):
   - **Broker**: `broker.hivemq.com`, **Puerto**: `1883`.
   - **Topic**: `maquina_estado_simple/estado`
   - Tipo: **Text / Text value**.
4. Pulsa `+` de nuevo para agregar un **publicador** (enviar comandos):
   - **Broker**: `broker.hivemq.com`, **Puerto**: `1883`.
   - **Topic**: `maquina_estado_simple/comando`
   - Escribe como valor fijo: `toggle` (boton) o usa los campos para enviar `on` / `off`.

> Nota: al usar un broker publico, cualquier persona en el mundo puede ver/enviar
> mensajes a esos topics. Para pruebas esta bien; para produccion usa un broker
> privado o con credenciales.

### Prueba rapida sin celular (PC)

Abre en el navegador: http://www.hivemq.com/demos/websocket-client/

1. Conecta al broker `broker.hivemq.com` puerto `8000` (WebSocket).
2. Suscribete a `maquina_estado_simple/estado`.
3. Publica `toggle` en `maquina_estado_simple/comando` y observa como cambia el LED.

---

## Visual Studio Code

1. Instala la extension **Espressif IDF** (id: `espressif.esp-idf-extension`).
2. Abre esta carpeta en VS Code.
3. Pulsa `F1` → **"ESP-IDF: Configure ESP-IDF extension"** y acepta las rutas que
   ya estan predefinidas en `.vscode/settings.json`:
   - IDF path: `C:\esp\v5.4.4\esp-idf`
   - Tools path: `C:\Espressif`
4. En la barra inferior: elige el puerto COM de la placa y usa los botones
   **Build**, **Flash** y **Monitor**.
5. La primera vez, si pide configurar `idf.port`, selecciona el COM de tu placa.

> El IntelliSense ya esta configurado para usar `build/compile_commands.json`
> (se genera al compilar).

---

## Solucion de problemas

- **No conecta al WiFi**: verifica SSID y clave en `menuconfig` y que la red sea 2.4 GHz.
- **MQTT sin datos**: confirma que el broker este accesible desde tu red y revisa
  que topics coincidan exactamente entre placa y celular.
- **El boton no responde**: revisa la conexion (boton entre GPIO0 y GND).
- **El puerto COM no aparece**: instala el driver USB del chip (CP210x o CH340
  segun tu placa) y revisa que la placa este conectada.
