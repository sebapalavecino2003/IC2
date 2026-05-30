/**
 * Página de detalle de un dispositivo específico.
 *
 * Muestra información completa del nodo: métricas en tiempo real
 * (temperatura, humedad, gas, llama) mediante indicadores visuales,
 * gráficos históricos, alertas activas, historial de eventos,
 * acciones remotas y configuración de umbrales.
 *
 * Los datos se refrescan cada 5 segundos para mantener actualizadas
 * las lecturas y el estado del dispositivo.
 */
import { useState, useCallback } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import {
  Box, Typography, Grid, IconButton, Chip,
} from '@mui/material'
import ArrowBackIcon from '@mui/icons-material/ArrowBack'
import MemoryIcon from '@mui/icons-material/Memory'
import DeviceThermostatIcon from '@mui/icons-material/DeviceThermostat'
import WaterDropIcon from '@mui/icons-material/WaterDrop'
import WhatshotIcon from '@mui/icons-material/Whatshot'
import AirIcon from '@mui/icons-material/Air'
import usePolling from '../hooks/usePolling'
import { getDevice, getReadings, getEvents } from '../services/api'
import { deviceStatus, statusColor, formatTimestamp, getSensorValue, readingsForChart } from '../utils/formatters'
import SensorGauge from '../components/SensorGauge'
import SensorChart from '../components/SensorChart'
import ActiveAlerts from '../components/ActiveAlerts'
import EventHistory from '../components/EventHistory'
import DeviceActions from '../components/DeviceActions'
import DeviceConfig from '../components/DeviceConfig'
import StatusChip from '../components/StatusChip'

export default function DeviceDetailPage() {
  const { id } = useParams()
  const navigate = useNavigate()
  const [device, setDevice] = useState(null)
  const [readings, setReadings] = useState([])
  const [events, setEvents] = useState([])

  const fetchData = useCallback(async () => {
    try {
      const [devRes, readRes, evRes] = await Promise.all([
        getDevice(id),
        getReadings({ device_id: id, ordering: '-timestamp', page_size: 500 }),
        getEvents({ device_id: id, resolved: false, ordering: '-timestamp' }),
      ])
      setDevice(devRes.data || devRes)
      const readingList = Array.isArray(readRes.data) ? readRes.data : readRes.data?.results || []
      setReadings(readingList)
      const eventList = Array.isArray(evRes.data) ? evRes.data : evRes.data?.results || []
      setEvents(eventList)
    } catch {
      // Fallo silencioso: el polling reintentará en el próximo ciclo.
    }
  }, [id])

  usePolling(fetchData, 5000)

  const status = device ? deviceStatus(device.last_seen) : 'offline'
  const color = statusColor(status)

  const latest = {
    temperature: getSensorValue(readings, device?.id, 'temperature'),
    humidity: getSensorValue(readings, device?.id, 'humidity'),
    gas_ppm: getSensorValue(readings, device?.id, 'gas'),
    flame: getSensorValue(readings, device?.id, 'flame'),
  }
  const tempReadings = readings.filter((r) => r.sensor_type === 'temperature')
  const humReadings = readings.filter((r) => r.sensor_type === 'humidity')
  const avgTemp = tempReadings.length
    ? (tempReadings.reduce((s, r) => s + r.value, 0) / tempReadings.length).toFixed(1)
    : 0
  const avgHum = humReadings.length
    ? (humReadings.reduce((s, r) => s + r.value, 0) / humReadings.length).toFixed(1)
    : 0

  if (!device) {
    return (
      <Box sx={{ textAlign: 'center', py: 8 }}>
        <Typography variant="h6" color="text.secondary">Cargando dispositivo...</Typography>
      </Box>
    )
  }

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 3 }}>
        <IconButton onClick={() => navigate('/')}>
          <ArrowBackIcon />
        </IconButton>
        <Box sx={{ flex: 1 }}>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5, flexWrap: 'wrap' }}>
            <MemoryIcon sx={{ color: '#00e5ff', fontSize: 28 }} />
            <Typography variant="h5" fontWeight={700}>
              {device.name || device.device_id}
            </Typography>
            <StatusChip status={status} label={status === 'online' ? 'Online' : 'Offline'} />
          </Box>
          <Box sx={{ display: 'flex', gap: 2, mt: 0.5, flexWrap: 'wrap' }}>
            {device.last_seen && (
              <Typography variant="caption" color="text.secondary">
                Última conexión: {formatTimestamp(device.last_seen)}
              </Typography>
            )}
            <Typography variant="caption" color="text.secondary">
              IP: {device.ip_address || '—'}
            </Typography>
            <Typography variant="caption" color="text.secondary">
              MAC: {device.mac_address || '—'}
            </Typography>
            <Typography variant="caption" color="text.secondary">
              ID: {device.device_id}
            </Typography>
          </Box>
        </Box>
      </Box>

      <Grid container spacing={2} sx={{ mb: 3 }}>
        <Grid item xs={6} sm={3}>
          <SensorGauge
            label="Temperatura"
            value={latest.temperature}
            unit="°C"
            max={50}
            icon={DeviceThermostatIcon}
          />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge
            label="Humedad"
            value={latest.humidity}
            unit="%"
            max={100}
            icon={WaterDropIcon}
          />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge
            label="Gas"
            value={latest.gas_ppm}
            unit="ppm"
            max={2000}
            icon={WhatshotIcon}
          />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge
            label="Flama"
            value={latest.flame}
            unit="ADC"
            max={2000}
            icon={AirIcon}
          />
        </Grid>
      </Grid>

      <Box sx={{ display: 'flex', gap: 2, mb: 3, flexWrap: 'wrap' }}>
        <Box
          sx={{
            bgcolor: '#111827',
            border: '1px solid #1e293b',
            borderRadius: 2,
            px: 2,
            py: 1,
            textAlign: 'center',
          }}
        >
          <Typography variant="caption" color="text.secondary">Temp. Promedio</Typography>
          <Typography variant="h6" sx={{ fontFamily: '"Roboto Mono", monospace' }}>
            {avgTemp}°C
          </Typography>
        </Box>
        <Box
          sx={{
            bgcolor: '#111827',
            border: '1px solid #1e293b',
            borderRadius: 2,
            px: 2,
            py: 1,
            textAlign: 'center',
          }}
        >
          <Typography variant="caption" color="text.secondary">Hum. Promedio</Typography>
          <Typography variant="h6" sx={{ fontFamily: '"Roboto Mono", monospace' }}>
            {avgHum}%
          </Typography>
        </Box>
        <Box
          sx={{
            bgcolor: '#111827',
            border: '1px solid #1e293b',
            borderRadius: 2,
            px: 2,
            py: 1,
            textAlign: 'center',
          }}
        >
          <Typography variant="caption" color="text.secondary">Alertas Activas</Typography>
          <Typography variant="h6" sx={{ fontFamily: '"Roboto Mono", monospace', color: events.length ? '#ff1744' : '#00e676' }}>
            {events.filter((e) => !e.resolved).length}
          </Typography>
        </Box>
      </Box>

      <Grid container spacing={2} sx={{ mb: 3 }}>
        <Grid item xs={12} md={8}>
          <SensorChart
            title="Temperatura"
            dataKey="temperature"
            color="#ff6f00"
            data={readingsForChart(readings, 'temperature')}
            unit="°C"
          />
        </Grid>
        <Grid item xs={12} md={4}>
          <DeviceActions deviceId={id} />
        </Grid>
        <Grid item xs={12} md={6}>
          <SensorChart
            title="Humedad"
            dataKey="humidity"
            color="#00e5ff"
            data={readingsForChart(readings, 'humidity')}
            unit="%"
          />
        </Grid>
        <Grid item xs={12} md={6}>
          <SensorChart
            title="Gas"
            dataKey="gas_ppm"
            color="#ff1744"
            data={readingsForChart(readings, 'gas_ppm')}
            unit="ppm"
          />
        </Grid>
      </Grid>

      <Grid container spacing={2} sx={{ mb: 3 }}>
        <Grid item xs={12} md={6}>
          <ActiveAlerts deviceId={id} />
        </Grid>
        <Grid item xs={12} md={6}>
          <DeviceConfig deviceId={id} />
        </Grid>
      </Grid>

      <EventHistory deviceId={id} />
    </Box>
  )
}
