"""Tests for DRF API endpoints: devices, readings, events."""
import json
from datetime import datetime, timedelta
from django.test import TestCase
from django.urls import reverse
from django.contrib.auth import get_user_model
from rest_framework import status
from rest_framework.test import APIClient
from rest_framework.authtoken.models import Token

from core.models import Device, Reading, Event


def _auth_client():
    """Return an APIClient with a valid auth token."""
    user = get_user_model().objects.create_user(
        username='testuser', password='testpass123'
    )
    token, _ = Token.objects.get_or_create(user=user)
    client = APIClient()
    client.credentials(HTTP_AUTHORIZATION=f'Token {token.key}')
    return client


class DeviceAPITest(TestCase):
    """Test CRUD operations on /api/v1/devices/."""

    def setUp(self):
        self.client = _auth_client()
        self.device_data = {
            'device_id': 'esp32-test-01',
            'name': 'Living Room Sensor',
            'location': 'Office',
        }
        self.device = Device.objects.create(
            device_id='esp32-existing',
            name='Existing Device',
            location='Lab',
        )
        self.list_url = '/api/v1/devices/'
        self.detail_url = f'/api/v1/devices/{self.device.id}/'

    def test_list_devices_returns_200(self):
        """GET /api/v1/devices/ returns 200 and paginated results."""
        response = self.client.get(self.list_url)
        self.assertEqual(response.status_code, status.HTTP_200_OK)
        data = response.json()
        self.assertIn('count', data)
        self.assertIn('results', data)

    def test_list_devices_contains_existing(self):
        """GET /api/v1/devices/ includes the existing device."""
        response = self.client.get(self.list_url)
        data = response.json()
        device_ids = [d['device_id'] for d in data['results']]
        self.assertIn('esp32-existing', device_ids)

    def test_create_device_returns_201(self):
        """POST /api/v1/devices/ creates a device and returns 201."""
        response = self.client.post(
            self.list_url,
            data=json.dumps(self.device_data),
            content_type='application/json',
        )
        self.assertEqual(response.status_code, status.HTTP_201_CREATED)
        data = response.json()
        self.assertEqual(data['device_id'], 'esp32-test-01')

    def test_create_device_has_readonly_timestamps(self):
        """POST /api/v1/devices/ should not allow setting created_at."""
        data = dict(self.device_data)
        data['created_at'] = '2020-01-01T00:00:00Z'
        response = self.client.post(
            self.list_url,
            content_type='application/json',
            data=json.dumps(data),
        )
        self.assertEqual(response.status_code, status.HTTP_201_CREATED)
        created = response.json()['created_at']
        self.assertNotEqual(created[:4], '2020')

    def test_get_device_detail_returns_200(self):
        """GET /api/v1/devices/{id}/ returns a single device."""
        response = self.client.get(self.detail_url)
        self.assertEqual(response.status_code, status.HTTP_200_OK)
        data = response.json()
        self.assertEqual(data['device_id'], 'esp32-existing')

    def test_update_device_returns_200(self):
        """PUT /api/v1/devices/{id}/ updates a device."""
        response = self.client.put(
            self.detail_url,
            data=json.dumps({'device_id': 'esp32-existing', 'name': 'Updated', 'location': 'Updated Lab'}),
            content_type='application/json',
        )
        self.assertEqual(response.status_code, status.HTTP_200_OK)
        self.device.refresh_from_db()
        self.assertEqual(self.device.name, 'Updated')

    def test_delete_device_returns_204(self):
        """DELETE /api/v1/devices/{id}/ deletes a device."""
        response = self.client.delete(self.detail_url)
        self.assertEqual(response.status_code, status.HTTP_204_NO_CONTENT)
        self.assertFalse(Device.objects.filter(id=self.device.id).exists())

    def test_devices_under_api_v1_prefix(self):
        """Device endpoint should be under /api/v1/ prefix."""
        self.assertEqual(self.list_url, '/api/v1/devices/')

    def test_default_pagination_is_50(self):
        """Default page size should be 50."""
        for i in range(60):
            Device.objects.create(device_id=f'esp32-bulk-{i:03d}', name=f'Bulk {i}')
        response = self.client.get(self.list_url)
        data = response.json()
        self.assertEqual(len(data['results']), 50)


class ReadingAPITest(TestCase):
    """Test read-only operations on /api/v1/readings/."""

    def setUp(self):
        self.client = _auth_client()
        self.device = Device.objects.create(
            device_id='esp32-reader', name='Reader'
        )
        now = datetime.now()
        for i in range(5):
            Reading.objects.create(
                device=self.device,
                sensor_type='temperature' if i % 2 == 0 else 'humidity',
                value=25.0 + i,
                unit='°C' if i % 2 == 0 else '%',
                timestamp=now - timedelta(hours=i),
            )
        self.list_url = '/api/v1/readings/'

    def test_list_readings_returns_200(self):
        """GET /api/v1/readings/ returns 200."""
        response = self.client.get(self.list_url)
        self.assertEqual(response.status_code, status.HTTP_200_OK)

    def test_filter_readings_by_sensor_type(self):
        """GET /api/v1/readings/?sensor_type=temperature filters results."""
        response = self.client.get(f'{self.list_url}?sensor_type=temperature')
        data = response.json()
        for r in data['results']:
            self.assertEqual(r['sensor_type'], 'temperature')

    def test_filter_readings_by_device_id(self):
        """GET /api/v1/readings/?device_id=esp32-reader filters results."""
        response = self.client.get(f'{self.list_url}?device_id=esp32-reader')
        data = response.json()
        self.assertEqual(len(data['results']), 5)

    def test_readings_readonly_no_post(self):
        """POST /api/v1/readings/ should return 405 (read-only)."""
        response = self.client.post(
            self.list_url,
            data=json.dumps({'sensor_type': 'temperature', 'value': 30.0}),
            content_type='application/json',
        )
        self.assertIn(response.status_code, [status.HTTP_405_METHOD_NOT_ALLOWED])


class EventAPITest(TestCase):
    """Test CRUD operations on /api/v1/events/."""

    def setUp(self):
        self.client = _auth_client()
        self.device = Device.objects.create(
            device_id='esp32-eventer', name='Eventer'
        )
        now = datetime.now()
        self.event = Event.objects.create(
            device=self.device,
            event_type='high_temperature',
            severity='warning',
            message='Temperature exceeded 40°C',
            resolved=True,
            timestamp=now,
        )
        Event.objects.create(
            device=self.device,
            event_type='fire_detected',
            severity='critical',
            message='Flame detected!',
            resolved=False,
            timestamp=now - timedelta(minutes=5),
        )
        self.list_url = '/api/v1/events/'

    def test_list_events_returns_200(self):
        """GET /api/v1/events/ returns 200."""
        response = self.client.get(self.list_url)
        self.assertEqual(response.status_code, status.HTTP_200_OK)

    def test_filter_events_by_severity(self):
        """GET /api/v1/events/?severity=critical filters by severity."""
        response = self.client.get(f'{self.list_url}?severity=critical')
        data = response.json()
        for e in data['results']:
            self.assertEqual(e['severity'], 'critical')

    def test_filter_events_by_resolved(self):
        """GET /api/v1/events/?resolved=false filters unresolved."""
        response = self.client.get(f'{self.list_url}?resolved=false')
        data = response.json()
        self.assertEqual(len(data['results']), 1)

    def test_create_event_returns_201(self):
        """POST /api/v1/events/ creates an event."""
        response = self.client.post(
            self.list_url,
            data=json.dumps({
                'device': self.device.id,
                'event_type': 'gas_leak',
                'severity': 'critical',
                'message': 'Gas leak detected',
                'resolved': False,
            }),
            content_type='application/json',
        )
        self.assertEqual(response.status_code, status.HTTP_201_CREATED)
