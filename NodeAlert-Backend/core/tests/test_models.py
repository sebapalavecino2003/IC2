"""Tests for core models: Device, Reading, Event, User."""
from django.test import TestCase
from django.db import models
from django.contrib.auth.models import AbstractUser

from core.models import Device, Reading, Event, User


class DeviceModelTest(TestCase):
    """Verify Device model fields and behavior."""

    def test_device_has_expected_fields(self):
        """Device model should have device_id, name, location, is_active, created_at, updated_at."""
        field_names = {f.name: f for f in Device._meta.get_fields() if hasattr(f, 'column')}

        self.assertIn('device_id', field_names)
        self.assertIsInstance(field_names['device_id'], models.CharField)
        self.assertTrue(field_names['device_id'].unique)

        self.assertIn('name', field_names)
        self.assertIsInstance(field_names['name'], models.CharField)

        self.assertIn('location', field_names)
        self.assertIsInstance(field_names['location'], models.CharField)
        self.assertTrue(field_names['location'].blank)

        self.assertIn('is_active', field_names)
        self.assertIsInstance(field_names['is_active'], models.BooleanField)
        self.assertTrue(field_names['is_active'].default)

        self.assertIn('created_at', field_names)
        self.assertTrue(field_names['created_at'].auto_now_add)

        self.assertIn('updated_at', field_names)
        self.assertTrue(field_names['updated_at'].auto_now)

    def test_device_str_returns_name(self):
        """Device.__str__ returns name when set, otherwise device_id."""
        device = Device(device_id='esp32-test', name='Living Room')
        self.assertEqual(str(device), 'Living Room')

    def test_device_str_fallback_to_device_id(self):
        """Device.__str__ falls back to device_id when name is empty."""
        device = Device(device_id='esp32-01')
        self.assertEqual(str(device), 'esp32-01')


class ReadingModelTest(TestCase):
    """Verify Reading model fields and behavior."""

    def test_reading_has_expected_fields(self):
        """Reading model should have device FK, sensor_type, value, unit, timestamp."""
        field_names = {f.name: f for f in Reading._meta.get_fields() if hasattr(f, 'column')}

        self.assertIn('device', field_names)
        fk = field_names['device']
        self.assertIsInstance(fk, models.ForeignKey)
        self.assertEqual(fk.remote_field.model, Device)
        self.assertEqual(fk.remote_field.related_name, 'readings')

        self.assertIn('sensor_type', field_names)
        st = field_names['sensor_type']
        self.assertIsInstance(st, models.CharField)
        self.assertEqual(st.max_length, 20)
        choices = dict(st.choices)
        for expected in ['temperature', 'humidity', 'gas', 'flame']:
            self.assertIn(expected, choices)

        self.assertIn('value', field_names)
        self.assertIsInstance(field_names['value'], models.FloatField)

        self.assertIn('unit', field_names)
        self.assertIsInstance(field_names['unit'], models.CharField)

        self.assertIn('timestamp', field_names)
        self.assertIsInstance(field_names['timestamp'], models.DateTimeField)

    def test_reading_str_format(self):
        """Reading.__str__ returns device_id/sensor_type @ timestamp."""
        device = Device(device_id='esp32-test', name='Test')
        reading = Reading(
            device=device, sensor_type='temperature',
            value=25.5, unit='°C'
        )
        result = str(reading)
        self.assertIn('esp32-test', result)
        self.assertIn('temperature', result)


class EventModelTest(TestCase):
    """Verify Event model fields and behavior."""

    def test_event_has_expected_fields(self):
        """Event model should have device FK, event_type, severity, message, resolved, timestamp."""
        field_names = {f.name: f for f in Event._meta.get_fields() if hasattr(f, 'column')}

        self.assertIn('device', field_names)
        fk = field_names['device']
        self.assertIsInstance(fk, models.ForeignKey)
        self.assertEqual(fk.remote_field.model, Device)
        self.assertEqual(fk.remote_field.related_name, 'events')

        self.assertIn('event_type', field_names)
        self.assertIsInstance(field_names['event_type'], models.CharField)

        self.assertIn('severity', field_names)
        sev = field_names['severity']
        self.assertIsInstance(sev, models.CharField)
        choices = dict(sev.choices)
        for expected in ['info', 'warning', 'critical']:
            self.assertIn(expected, choices)

        self.assertIn('message', field_names)
        self.assertIsInstance(field_names['message'], models.TextField)

        self.assertIn('resolved', field_names)
        self.assertIsInstance(field_names['resolved'], models.BooleanField)
        self.assertFalse(field_names['resolved'].default)

        self.assertIn('timestamp', field_names)
        self.assertIsInstance(field_names['timestamp'], models.DateTimeField)

    def test_event_str_format(self):
        """Event.__str__ returns [severity] event_type @ device_id."""
        device = Device(device_id='esp32-test', name='Test')
        event = Event(
            device=device, event_type='fire_detected',
            severity='critical', message='Fire detected!'
        )
        result = str(event)
        self.assertIn('critical', result)
        self.assertIn('fire_detected', result)
        self.assertIn('esp32-test', result)


class UserModelTest(TestCase):
    """Verify User model extends AbstractUser."""

    def test_user_is_abstract_user_subclass(self):
        """User should extend AbstractUser."""
        self.assertTrue(issubclass(User, AbstractUser))

    def test_user_has_standard_auth_fields(self):
        """User should have standard Django auth fields."""
        field_names = {f.name for f in User._meta.get_fields()}
        for field in ['username', 'password', 'email', 'first_name', 'last_name']:
            self.assertIn(field, field_names)
