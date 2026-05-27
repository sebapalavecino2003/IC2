"""Management command for MQTT subscription and telemetry ingestion (API-03)."""
import json
import os
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
    help = 'Subscribe to MQTT topics and ingest sensor telemetry into the database'
    requires_system_checks = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._client = None
        signal.signal(signal.SIGTERM, self._signal_handler)
        signal.signal(signal.SIGINT, self._signal_handler)

    def _signal_handler(self, signum, frame):
        self.stdout.write('Received shutdown signal, disconnecting...')
        if self._client:
            self._client.disconnect()
        sys.exit(0)

    def handle(self, *args, **options):
        broker = os.environ.get('MQTT_BROKER_HOST', 'mosquitto')
        port = int(os.environ.get('MQTT_BROKER_PORT', '1883'))
        keepalive = int(os.environ.get('MQTT_KEEPALIVE', '60'))
        sub_user = os.environ.get('MQTT_SUBSCRIBER_USER', 'mqtt_subscriber')
        sub_pass = os.environ.get('MQTT_SUBSCRIBER_PASSWORD', '')

        self._client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self._client.username_pw_set(sub_user, sub_pass)
        self._client.on_connect = self.on_connect
        self._client.on_message = self.on_message

        self.stdout.write(f'Connecting to MQTT broker at {broker}:{port}...')
        self._client.connect(broker, port, keepalive)
        self._client.loop_forever()

    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.stdout.write(self.style.SUCCESS('Connected to MQTT broker'))
            client.subscribe('nodealert/+/telemetry', qos=0)
            client.subscribe('nodealert/+/events', qos=1)
        else:
            self.stderr.write(f'Connection failed with result code {rc}')

    def on_message(self, client, userdata, msg):
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

        sensor_map = {
            'temperature':    ('temperature', payload.get('temperature'), '°C'),
            'humidity':       ('humidity',    payload.get('humidity'),    '%'),
            'gas_ppm':        ('gas',         payload.get('gas_ppm'),     'ADC'),
            'flame_detected': ('flame',       payload.get('flame_detected'), 'ADC'),
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
