"""
Publicador MQTT para comandos dirigidos a dispositivos (AUTO-04).

Utiliza paho.mqtt.publish.single() para realizar publicaciones
desconectadas (connect → publish → disconnect), evitando mantener
una conexión persistente. Esto es intencional: el servidor de
desarrollo de Django con autoreload puede crear múltiples hilos,
y una conexión MQTT persistente desde un hilo podría no ser
correctamente gestionada al recargar.

Cada publicación utiliza QoS 1 (al menos una entrega) para garantizar
que el mensaje llegue al broker, aceptando posibles duplicados que
el firmware maneja siendo idempotente.
"""
import json
import paho.mqtt.publish as publish

# Prefijo para todos los topics MQTT de NodeAlert.
# El topic completo sigue el patrón: nodealert/{device_id}/commands
TOPIC_PREFIX = 'nodealert/'


def publish_command(device_id: str, cmd: str, params: dict = None) -> bool:
    """
    Publica un comando MQTT hacia un dispositivo específico.
    Construye el payload como JSON con el comando y un timestamp UNIX.
    Si se proporcionan parámetros adicionales, se incluyen en el campo
    'params' del payload.
    La conexión se realiza contra el broker Mosquitto en el puerto
    estándar 1883, autenticándose con el usuario 'mqtt_publisher'.

    Args:
        device_id: Identificador único del dispositivo destino.
        cmd: Comando a ejecutar (buzzer_on, reboot, etc.).
        params: Diccionario opcional con parámetros del comando.

    Returns:
        True si la publicación fue exitosa, False en caso de error.
    """
    import time as _time
    payload = {'cmd': cmd, 'timestamp': int(_time.time())}
    if params:
        payload['params'] = params

    host = 'mosquitto'
    port = 1883
    user = 'mqtt_publisher'
    password = 'test_password'

    try:
        publish.single(
            topic=f"{TOPIC_PREFIX}{device_id}/commands",
            payload=json.dumps(payload),
            qos=1,
            hostname=host,
            port=port,
            auth={'username': user, 'password': password},
        )
        return True
    except Exception as e:
        print(f"[MQTT PUBLISH ERROR] {e}")
        return False
