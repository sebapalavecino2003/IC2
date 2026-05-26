"""One-shot MQTT publisher for device commands (AUTO-04).

Uses paho.mqtt.publish.single() for connection-less publish.
This avoids threading issues with Django's auto-reloading dev server.
"""
import json
import os
import paho.mqtt.publish as publish

TOPIC_PREFIX = os.environ.get('MQTT_TOPIC_PREFIX', 'nodealert/')


def publish_command(device_id: str, cmd: str, params: dict = None) -> bool:
    """Publish a command to a device's command topic.

    Args:
        device_id: Device identifier (e.g., 'nodealert-01')
        cmd: Command name (buzzer_on, buzzer_off, return_to_auto,
             acknowledge_alarm, update_thresholds)
        params: Optional parameters dict for update_thresholds

    Returns:
        True if published successfully
    """
    import time as _time
    payload = {'cmd': cmd, 'timestamp': int(_time.time())}
    if params:
        payload['params'] = params

    host = os.environ.get('MQTT_BROKER_HOST', 'mosquitto')
    port = int(os.environ.get('MQTT_BROKER_PORT', '1883'))
    user = os.environ.get('MQTT_PUBLISHER_USER', 'mqtt_publisher')
    password = os.environ.get('MQTT_PUBLISHER_PASSWORD', '')

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
