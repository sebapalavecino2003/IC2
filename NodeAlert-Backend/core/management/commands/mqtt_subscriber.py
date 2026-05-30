"""
Comando de gestión Django para la suscripción MQTT e ingestión de
telemetría (API-03).

Este comando se ejecuta como un proceso separado (background) dentro
del contenedor Django. Se conecta al broker Mosquitto, se suscribe a
los topics de telemetría y eventos de todos los dispositivos, y
persiste los datos entrantes en la base de datos.

Maneja correctamente las señales SIGTERM y SIGINT para una
desconexión limpia del broker MQTT.
"""
import json
import signal
import sys
from datetime import datetime

from django.core.management.base import BaseCommand
from django.db import transaction
from django.utils import timezone
from django.utils.dateparse import parse_datetime

import paho.mqtt.client as mqtt

from core.models import Device, Reading, Event


class Command(BaseCommand):
    """
    Comando de gestión que implementa el subscriber MQTT persistente.

    Permanece en ejecución continua (loop_forever) procesando mensajes
    entrantes. La inicialización captura señales de terminación para
    garantizar una desconexión ordenada del broker.
    """
    help = 'Subscribe to MQTT topics and ingest sensor telemetry into the database'
    requires_system_checks = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._client = None
        # Registrar manejadores de señales para terminación ordenada.
        # SIGTERM lo envía Docker al detener el contenedor.
        # SIGINT corresponde a Ctrl+C en terminal.
        signal.signal(signal.SIGTERM, self._signal_handler)
        signal.signal(signal.SIGINT, self._signal_handler)

    def _signal_handler(self, signum, frame):
        """
        Manejador de señales de terminación.

        Desconecta el cliente MQTT antes de salir para que el broker
        no mantenga una sesión huérfana.
        """
        self.stdout.write('Received shutdown signal, disconnecting...')
        if self._client:
            self._client.disconnect()
        sys.exit(0)

    def handle(self, *args, **options):
        """
        Punto de entrada del comando.

        Configura el cliente MQTT con la API V2 de paho, establece
        credenciales, asigna callbacks y entra en el bucle infinito
        de procesamiento de mensajes.
        """
        broker = 'mosquitto'
        port = 1883
        keepalive = 60
        sub_user = 'mqtt_subscriber'
        sub_pass = 'test_password'

        self._client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self._client.username_pw_set(sub_user, sub_pass)
        self._client.on_connect = self.on_connect
        self._client.on_message = self.on_message

        self.stdout.write(f'Connecting to MQTT broker at {broker}:{port}...')
        self._client.connect(broker, port, keepalive)
        self._client.loop_forever()

    def on_connect(self, client, userdata, flags, rc, properties=None):
        """
        Callback ejecutado al conectar/ reconectar al broker MQTT.

        Si la conexión es exitosa (rc == 0), se suscribe a los topics
        de telemetría (QoS 0) y eventos (QoS 1). La diferencia de QoS
        refleja que los eventos son más críticos y deben entregarse
        al menos una vez, mientras que la telemetría puede perderse
        ocasionalmente sin impacto grave.
        """
        if rc == 0:
            self.stdout.write(self.style.SUCCESS('Connected to MQTT broker'))
            # nodealert/+/telemetry → QoS 0: telemetría de sensores.
            # El wildcard '+' captura cualquier device_id.
            client.subscribe('nodealert/+/telemetry', qos=0)
            # nodealert/+/events → QoS 1: eventos y alertas.
            # QoS 1 garantiza al menos una entrega.
            client.subscribe('nodealert/+/events', qos=1)
        else:
            self.stderr.write(f'Connection failed with result code {rc}')

    def on_message(self, client, userdata, msg):
        """
        Callback ejecutado al recibir un mensaje MQTT.

        Decodifica el payload JSON y lo enruta al manejador adecuado
        según el sufijo del topic (/telemetry o /events).
        """
        try:
            payload = json.loads(msg.payload.decode('utf-8'))
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            self.stderr.write(f'Invalid JSON on {msg.topic}: {e}')
            return

        if msg.topic.endswith('/telemetry'):
            self._handle_telemetry(msg.topic, payload)
        elif msg.topic.endswith('/events'):
            self._handle_event(msg.topic, payload)

    def _validate_device(self, device_id, mac_address=None):
        """
        Valida que el dispositivo exista y esté activo en la base de datos.

        Si el payload incluye una dirección MAC y el dispositivo tiene
        una registrada, verifica que coincidan. Esto previene que un
        nodo con la configuración incorrecta publique datos bajo un
        device_id que no le corresponde.

        Returns:
            La instancia de Device si es válida, None en caso contrario.
        """
        try:
            device = Device.objects.get(device_id=device_id, is_active=True)
            if mac_address and device.mac_address:
                if device.mac_address.lower() != mac_address.lower():
                    self.stderr.write(
                        f'MAC mismatch for {device_id}: payload={mac_address} '
                        f'expected={device.mac_address}')
                    return None
            return device
        except Device.DoesNotExist:
            self.stderr.write(f'Unknown or inactive device: {device_id}')
            return None

    def _handle_telemetry(self, topic, payload):
        """
        Procesa un mensaje de telemetría entrante.

        Extrae los valores de los sensores del payload JSON y crea
        registros Reading en la base de datos. Utiliza bulk_create
        dentro de una transacción atómica para eficiencia.

        El mapeo entre claves del JSON y tipos de sensor del modelo
        está definido en sensor_map. Solo se crean lecturas para los
        sensores cuyos valores no sean None.
        """
        device_id = payload.get('device_id', '')
        mac_address = payload.get('mac', '')
        device = self._validate_device(device_id, mac_address)
        if device is None:
            return

        # Procesar el timestamp: si el payload incluye uno, se usa;
        # si no, se asigna la hora actual del servidor. Esto permite
        # que dispositivos sin RTC confiable usen la hora del servidor.
        raw_ts = payload.get('timestamp')
        timestamp = timezone.now()
        if raw_ts is not None:
            try:
                parsed = parse_datetime(str(raw_ts))
                if parsed is not None:
                    timestamp = parsed
            except (ValueError, TypeError):
                pass

        # Mapeo de claves del JSON entrante a columnas del modelo Reading.
        # Formato: (json_key, (sensor_type_db, campo_valor, unidad))
        sensor_map = {
            'temperature':    ('temperature', payload.get('temperature'), '°C'),
            'humidity':       ('humidity',    payload.get('humidity'),    '%'),
            'gas_ppm':        ('gas',         payload.get('gas_ppm'),     'ADC'),
            'flame':          ('flame',       payload.get('flame'),          'ADC'),
        }

        readings = []
        for json_key, (db_type, value, unit) in sensor_map.items():
            if value is not None:
                try:
                    readings.append(Reading(
                        device=device,
                        sensor_type=db_type,
                        value=float(value),
                        unit=unit,
                        timestamp=timestamp,
                    ))
                except (TypeError, ValueError) as e:
                    self.stderr.write(f'Invalid {json_key} value {value}: {e}')

        if readings:
            with transaction.atomic():
                Reading.objects.bulk_create(readings)

        self.stdout.write(
            f'[TELEMETRY] {device_id}: {len(readings)} readings at '
            f'{timestamp.isoformat()}')

    def _handle_event(self, topic, payload):
        """
        Procesa un mensaje de evento/alert entrante.

        Crea un registro Event en la base de datos con la información
        proporcionada por el dispositivo. Los eventos pueden ser
        desde alarmas críticas hasta cambios de configuración.
        """
        device_id = payload.get('device_id', '')
        mac_address = payload.get('mac', '')
        device = self._validate_device(device_id, mac_address)
        if device is None:
            return

        raw_ts = payload.get('timestamp')
        timestamp = timezone.now()
        if raw_ts is not None:
            try:
                parsed = parse_datetime(str(raw_ts))
                if parsed is not None:
                    timestamp = parsed
            except (ValueError, TypeError):
                pass

        Event.objects.create(
            device=device,
            event_type=payload.get('event_type', 'unknown'),
            severity=payload.get('severity', 'info'),
            message=payload.get('message', ''),
            timestamp=timestamp,
        )

        self.stdout.write(
            f'[EVENT] {device_id}: {payload.get("event_type")} '
            f'({payload.get("severity")})')
